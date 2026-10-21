#include"lcl/net/ChatRoom.hpp"
#include"lcl/net/TcpClient.hpp"

#include <sys/socket.h>
#include <sys/time.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

//Release构建下assert会被NDEBUG禁用，测试需显式检查
#define CHECK(cond) do{ \
    if(!(cond)){ \
        std::cerr<<"CHECK failed: "<<#cond<<" ("<<__FILE__<<":"<<__LINE__<<")\n"; \
        std::exit(1); \
    } \
}while(0)

namespace{

    using lcl::net::Socket;
    using lcl::net::TcpClient;

    //逐字节读到换行，与服务器协议一致
    std::string read_line(Socket& sock){
        std::string line;
        char ch;
        while(true){
            ssize_t n=sock.recv(&ch,1,MSG_NOSIGNAL);
            if(n<=0){
                break;
            }
            if(ch=='\n'){
                break;
            }
            if(ch!='\r'){
                line.push_back(ch);
            }
        }
        return line;
    }

    //带重试的连接，等待服务器accept循环就绪
    std::unique_ptr<TcpClient> connect_with_retry(const std::string& ip,std::uint16_t port){
        for(int i=0;i<40;++i){
            try{
                return std::make_unique<TcpClient>(ip,port);
            }catch(const std::exception&){
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }
        throw std::runtime_error("connect failed after retry");
    }

    //读取若干行，检查其中出现期望内容
    bool expect_in_lines(Socket& sock,const std::string& needle,int max_lines){
        for(int i=0;i<max_lines;++i){
            if(read_line(sock).find(needle)!=std::string::npos){
                return true;
            }
        }
        return false;
    }

    //在超时窗口内确认未收到某内容（用于验证房间隔离）
    bool expect_absent(Socket& sock,const std::string& needle,int max_lines){
        timeval tv{0,300*1000};
        ::setsockopt(sock.get(),SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));

        bool absent=true;
        try{
            for(int i=0;i<max_lines;++i){
                if(read_line(sock).find(needle)!=std::string::npos){
                    absent=false;
                    break;
                }
            }
        }catch(const std::exception&){
            //接收超时：窗口内没有更多数据
        }

        timeval zero{0,0};
        ::setsockopt(sock.get(),SOL_SOCKET,SO_RCVTIMEO,&zero,sizeof(zero));
        return absent;
    }
}

int main(){
    //端口0由内核分配，避免测试端口冲突
    lcl::net::ChatRoom room("127.0.0.1",0);
    const std::uint16_t port=room.port();

    std::thread server_thread([&room]{ room.run(); });

    {
        auto alice=connect_with_retry("127.0.0.1",port);
        auto bob=connect_with_retry("127.0.0.1",port);

        //注册后自动进入默认房间 lobby；读取各自的加入通知，确保注册已被服务端处理
        alice->socket().send_all("alice\n",6,MSG_NOSIGNAL);
        CHECK(expect_in_lines(alice->socket(),"alice 加入了房间 lobby",20));
        bob->socket().send_all("bob\n",4,MSG_NOSIGNAL);
        CHECK(expect_in_lines(bob->socket(),"bob 加入了房间 lobby",20));

        //同房间：alice发言，bob与alice都能收到
        const std::string msg="hello bob\n";
        alice->socket().send_all(msg.data(),msg.size(),MSG_NOSIGNAL);
        CHECK(expect_in_lines(bob->socket(),"hello bob",20));
        CHECK(expect_in_lines(alice->socket(),"hello bob",20));

        //bob 切换到 tech 房间：lobby 收到离开通知，bob 收到加入通知
        bob->socket().send_all("/join tech\n",11,MSG_NOSIGNAL);
        CHECK(expect_in_lines(alice->socket(),"bob 离开了房间 lobby",20));
        CHECK(expect_in_lines(bob->socket(),"bob 加入了房间 tech",20));

        //不同房间的消息互不可见
        alice->socket().send_all("lobby only\n",11,MSG_NOSIGNAL);
        CHECK(expect_absent(bob->socket(),"lobby only",3));

        bob->socket().send_all("tech only\n",10,MSG_NOSIGNAL);
        CHECK(expect_absent(alice->socket(),"tech only",3));

        //房间列表包含两个房间
        bob->socket().send_all("/rooms\n",7,MSG_NOSIGNAL);
        CHECK(expect_in_lines(bob->socket(),"lobby",20));
        CHECK(expect_in_lines(bob->socket(),"tech",20));

        //第三个客户端加入 tech，bob 应收到加入通知
        auto carol=connect_with_retry("127.0.0.1",port);
        carol->socket().send_all("carol\n",6,MSG_NOSIGNAL);
        carol->socket().send_all("/join tech\n",11,MSG_NOSIGNAL);
        CHECK(expect_in_lines(bob->socket(),"carol 加入了房间 tech",20));

        //bob 退出，tech 房间的 carol 收到离开通知
        bob.reset();
        CHECK(expect_in_lines(carol->socket(),"bob 离开了房间 tech",20));
    }

    room.stop();
    server_thread.join();

    std::cout<<"test_chat: all tests passed\n";
    return 0;
}
