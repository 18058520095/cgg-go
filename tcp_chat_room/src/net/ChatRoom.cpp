#include"lcl/net/ChatRoom.hpp"

#include <sys/eventfd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

namespace{

    //信号处理函数只能调用异步信号安全函数，这里只往eventfd写一笔唤醒epoll
    std::atomic<int> g_wake_fd{-1};
    
    void on_signal(int){
        const std::uint64_t one=1;
        int fd=g_wake_fd.load();
        if(fd>=0){
            ssize_t n=::write(fd,&one,sizeof(one));
            (void)n;    //信号处理器内无法处理写失败，仅消除unused-result警告
        }
    }

    constexpr std::size_t kMaxInBuf=64*1024;    //单行超长保护
    constexpr std::size_t kMaxOutBuf=1024*1024; //慢消费者踢出阈值
    constexpr std::size_t kMaxRoomName=32;
    constexpr const char* kDefaultRoom="lobby"; //注册后自动进入的默认房间

    std::string trim(const std::string& s){
        const std::size_t b=s.find_first_not_of(" \t");
        if(b==std::string::npos){
            return {};
        }
        const std::size_t e=s.find_last_not_of(" \t");
        return s.substr(b,e-b+1);
    }
}

namespace lcl::net{

    ChatRoom::ChatRoom(const std::string& ip,std::uint16_t port)
    :ip_(ip),
    listen_socket_(Socket::create(SocketType::Stream))
    {
        listen_socket_.set_reuseaddr(true);
        listen_socket_.bind(SocketAddress(ip,port));
        listen_socket_.listen(128);
        listen_socket_.set_nonblocking(true);

        int wfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
        if(wfd<0){
            throw std::system_error(errno,std::system_category(),"eventfd");
        }
        wake_fd_.reset(wfd);

        //广播时对端可能已关闭，避免SIGPIPE终止进程
        std::signal(SIGPIPE,SIG_IGN);
    }

    std::uint16_t ChatRoom::port() const{
        sockaddr_in addr{};
        socklen_t len=sizeof(addr);
        if(::getsockname(listen_socket_.get(),reinterpret_cast<sockaddr*>(&addr),&len)<0){
            throw std::system_error(errno,std::system_category(),"getsockname");
        }
        return ::ntohs(addr.sin_port);
    }

    void ChatRoom::run(){
        g_wake_fd.store(wake_fd_.get());
        std::signal(SIGINT,on_signal);
        std::signal(SIGTERM,on_signal);

        running_.store(true);
        std::cout<<"ChatRoom(epoll Reactor) listening on "<<ip_<<":"<<port()<<std::endl;

        epoll_.add(listen_socket_.get(),EPOLLIN);
        epoll_.add(wake_fd_.get(),EPOLLIN);

        while(running_.load()){
            auto events=epoll_.wait(500);
            for(auto& ev:events){
                int fd=ev.data.fd;

                if(fd==wake_fd_.get()){
                    std::uint64_t v;
                    while(::read(fd,&v,sizeof(v))>0){}  //排空计数
                    running_.store(false);
                    break;
                }

                if(fd==listen_socket_.get()){
                    accept_new();
                    continue;
                }

                const std::uint32_t revents=ev.events;
                if(revents&(EPOLLHUP|EPOLLERR)){
                    close_connection(fd);
                    continue;
                }
                if(revents&EPOLLIN){
                    handle_readable(fd);    //可能关闭该连接
                }
                if(revents&EPOLLOUT&&conns_.count(fd)){
                    handle_writable(fd);
                }
            }
        }

        g_wake_fd.store(-1);
        conns_.clear(); //socket析构关闭fd，内核自动从epoll摘除
        rooms_.clear();
    }

    void ChatRoom::stop(){
        running_.store(false);
        if(wake_fd_.valid()){
            const std::uint64_t one=1;
            ssize_t n;
            do{
                n=::write(wake_fd_.get(),&one,sizeof(one));
            }while(n<0&&errno==EINTR);
        }
    }

    void ChatRoom::accept_new(){
        while(true){
            Socket conn;
            try{
                conn=listen_socket_.accept();
            }catch(const std::system_error& e){
                const int ec=e.code().value();
                if(ec==EINTR) continue;
                return; //EAGAIN：已取完所有待决连接
            }

            conn.set_nonblocking(true);
            const int fd=conn.get();
            epoll_.add(fd,EPOLLIN|EPOLLRDHUP);
            conns_.emplace(fd,Connection{std::move(conn),{},{},{},{}});
        }
    }

    void ChatRoom::handle_readable(int fd){
        auto it=conns_.find(fd);
        if(it==conns_.end()) return;

        char buf[4096];
        while(true){
            ssize_t n=::recv(fd,buf,sizeof(buf),0);
            if(n>0){
                it->second.in_buf.append(buf,static_cast<std::size_t>(n));
                if(it->second.in_buf.size()>kMaxInBuf){
                    close_connection(fd);   //超长未成行，疑似恶意流量
                    return;
                }
                if(static_cast<std::size_t>(n)<sizeof(buf)) break;  //水平触发，剩余数据会再次就绪
            }else if(n==0){
                close_connection(fd);       //对端关闭
                return;
            }else if(errno==EAGAIN||errno==EWOULDBLOCK){
                break;
            }else if(errno==EINTR){
                continue;
            }else{
                close_connection(fd);       //ECONNRESET等
                return;
            }
        }

        //按行切分
        std::size_t pos;
        while((pos=it->second.in_buf.find('\n'))!=std::string::npos){
            std::string line=it->second.in_buf.substr(0,pos);
            it->second.in_buf.erase(0,pos+1);
            if(!line.empty()&&line.back()=='\r'){
                line.pop_back();
            }
            process_line(fd,std::move(line));
            it=conns_.find(fd); //处理中连接可能被关闭
            if(it==conns_.end()) return;
        }
    }

    void ChatRoom::handle_writable(int fd){
        auto it=conns_.find(fd);
        if(it==conns_.end()) return;

        auto& out=it->second.out_buf;
        std::size_t sent=0;
        while(sent<out.size()){
            ssize_t n=::send(fd,out.data()+sent,out.size()-sent,MSG_NOSIGNAL);
            if(n>0){
                sent+=static_cast<std::size_t>(n);
                continue;
            }
            if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){
                break;
            }
            if(n<0&&errno==EINTR){
                continue;
            }
            close_connection(fd);
            return;
        }
        out.erase(0,sent);
        update_events(fd);  //写完则摘掉EPOLLOUT
    }

    void ChatRoom::process_line(int fd,std::string line){
        auto it=conns_.find(fd);
        if(it==conns_.end()) return;

        if(it->second.name.empty()){
            //首行：注册用户名，并自动加入默认房间
            if(line.empty()){
                line="user_"+std::to_string(fd);
            }
            if(line.size()>32){
                line.resize(32);
            }
            it->second.name=line;
            std::cout<<"[chat] "<<line<<" joined"<<std::endl;
            join_room(fd,kDefaultRoom);
            return;
        }

        if(line.empty()){
            return;
        }
        if(line[0]=='/'){
            handle_command(fd,line);
            return;
        }

        it=conns_.find(fd);     //命令处理可能关闭连接
        if(it==conns_.end()) return;
        broadcast_to_room(it->second.room,it->second.name+": "+line);
    }

    void ChatRoom::handle_command(int fd,const std::string& line){
        const std::size_t sp=line.find(' ');
        const std::string cmd=(sp==std::string::npos)?line:line.substr(0,sp);
        std::string arg=(sp==std::string::npos)?std::string():trim(line.substr(sp+1));

        auto it=conns_.find(fd);
        if(it==conns_.end()) return;

        if(cmd=="/join"){
            if(arg.empty()){
                send_to(fd,"[系统] 用法: /join <房间名>\n");
                return;
            }
            if(arg.size()>kMaxRoomName){
                arg.resize(kMaxRoomName);
            }
            if(arg==it->second.room){
                send_to(fd,"[系统] 你已经在房间 "+arg+" 中\n");
                return;
            }
            join_room(fd,arg);
            return;
        }

        if(cmd=="/rooms"){
            //先快照，发送过程中可能因连接异常而修改rooms_
            std::vector<std::pair<std::string,std::size_t>> list;
            list.reserve(rooms_.size());
            for(const auto& [name,members]:rooms_){
                list.emplace_back(name,members.size());
            }
            std::sort(list.begin(),list.end());
            send_to(fd,"[系统] 房间列表（共 "+std::to_string(list.size())+" 个）\n");
            for(const auto& [name,count]:list){
                send_to(fd,"[系统]   "+name+" ("+std::to_string(count)+" 人)\n");
            }
            return;
        }

        if(cmd=="/help"){
            send_to(fd,"[系统] 可用命令: /join <房间名> 切换房间 | /rooms 房间列表 | /help 帮助\n");
            return;
        }

        send_to(fd,"[系统] 未知命令: "+cmd+"，输入 /help 查看可用命令\n");
    }

    void ChatRoom::join_room(int fd,const std::string& room){
        auto it=conns_.find(fd);
        if(it==conns_.end()) return;

        if(!it->second.room.empty()){
            leave_room(fd,true);    //先退出旧房间并通知其成员
            it=conns_.find(fd);
            if(it==conns_.end()) return;
        }

        it->second.room=room;
        rooms_[room].insert(fd);
        std::cout<<"[chat] "<<it->second.name<<" joined room "<<room<<std::endl;
        broadcast_to_room(room,"[系统] "+it->second.name+" 加入了房间 "+room);
    }

    void ChatRoom::leave_room(int fd,bool notify){
        auto it=conns_.find(fd);
        if(it==conns_.end()) return;

        const std::string room=it->second.room;
        if(room.empty()) return;

        const std::string name=it->second.name;
        it->second.room.clear();    //先清空，broadcast可能触发其他连接的关闭流程

        auto rit=rooms_.find(room);
        if(rit==rooms_.end()) return;

        rit->second.erase(fd);
        if(rit->second.empty()){
            rooms_.erase(rit);      //空房间回收
            return;
        }
        if(notify&&!name.empty()){
            broadcast_to_room(room,"[系统] "+name+" 离开了房间 "+room);
        }
    }

    void ChatRoom::send_to(int fd,const std::string& data){
        auto it=conns_.find(fd);
        if(it==conns_.end()) return;
        auto& conn=it->second;

        if(!conn.out_buf.empty()){
            conn.out_buf+=data; //已有积压：直接排队，等EPOLLOUT刷出
        }else{
            std::size_t sent=0;
            while(sent<data.size()){
                ssize_t n=::send(fd,data.data()+sent,data.size()-sent,MSG_NOSIGNAL);
                if(n>0){
                    sent+=static_cast<std::size_t>(n);
                    continue;
                }
                if(n<0&&(errno==EAGAIN||errno==EWOULDBLOCK)){
                    break;
                }
                if(n<0&&errno==EINTR){
                    continue;
                }
                close_connection(fd);   //EPIPE等：对端已断
                return;
            }
            if(sent<data.size()){
                conn.out_buf=data.substr(sent); //内核缓冲区满：剩余进背压缓冲
            }
        }

        if(conn.out_buf.size()>kMaxOutBuf){
            close_connection(fd);   //慢消费者踢出
            return;
        }
        update_events(fd);
    }

    void ChatRoom::broadcast_to_room(const std::string& room,const std::string& msg,int except_fd){
        auto rit=rooms_.find(room);
        if(rit==rooms_.end()) return;

        std::string data=msg;
        if(data.empty()||data.back()!='\n'){
            data+='\n';
        }

        //先快照成员：send_to可能触发close_connection，进而修改rooms_
        std::vector<int> fds(rit->second.begin(),rit->second.end());
        for(int fd:fds){
            if(fd!=except_fd){
                send_to(fd,data);
            }
        }
    }

    void ChatRoom::close_connection(int fd,bool notify){
        auto it=conns_.find(fd);
        if(it==conns_.end()) return;

        const std::string name=it->second.name;
        leave_room(fd,notify);  //先退出房间并通知成员（不会关闭本fd）

        it=conns_.find(fd);
        if(it==conns_.end()) return;
        epoll_.remove(fd);
        conns_.erase(it);       //socket析构关闭fd

        if(notify&&!name.empty()){
            std::cout<<"[chat] "<<name<<" left"<<std::endl;
        }
    }

    void ChatRoom::update_events(int fd) const{
        auto it=conns_.find(fd);
        if(it==conns_.end()) return;

        std::uint32_t events=EPOLLIN|EPOLLRDHUP;
        if(!it->second.out_buf.empty()){
            events|=EPOLLOUT;
        }
        epoll_.modify(fd,events);
    }
}
