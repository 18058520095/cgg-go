#pragma once

#include"lcl/net/Epoll.hpp"
#include"lcl/net/Socket.hpp"

#include <atomic>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace lcl::net{

    //基于epoll Reactor的TCP聊天室（支持多房间）
    //单线程事件循环 + 非阻塞IO（水平触发），无锁；写不complete时缓冲进out_buf并挂EPOLLOUT（写背压）
    //协议：行文本（'\n'结尾）；首行为用户名，之后每行是消息或命令
    //命令：/join <房间名>、/rooms、/help；消息只在所属房间内广播
    class ChatRoom{
        public:
        ChatRoom(const std::string& ip,std::uint16_t port);

        //阻塞运行事件循环，收到SIGINT/SIGTERM后退出
        void run();

        //线程安全：通过eventfd唤醒epoll，可从任意线程调用
        void stop();

        //监听的实际端口（构造时port传0由内核分配，用于测试）
        [[nodiscard]] std::uint16_t port() const;

        private:
        //单连接的收发状态
        struct Connection{
            Socket socket;
            std::string name;       //空表示尚未注册用户名
            std::string room;       //当前房间，空表示尚未加入
            std::string in_buf;     //半包缓冲
            std::string out_buf;    //待发送缓冲（背压）
        };

        void accept_new();
        void handle_readable(int fd);
        void handle_writable(int fd);
        void process_line(int fd,std::string line);
        void handle_command(int fd,const std::string& line);
        void join_room(int fd,const std::string& room);
        void leave_room(int fd,bool notify);
        void send_to(int fd,const std::string& data);
        void broadcast_to_room(const std::string& room,const std::string& msg,int except_fd=-1);
        void close_connection(int fd,bool notify=true);
        void update_events(int fd) const;

        std::string ip_;
        Socket listen_socket_;
        file::FileDescriptor wake_fd_;  //eventfd：信号/外部stop统一从这里唤醒
        Epoll epoll_;
        std::unordered_map<int,Connection> conns_;
        std::unordered_map<std::string,std::unordered_set<int>> rooms_;  //房间名 -> 成员fd
        std::atomic<bool> running_{false};
    };
}
