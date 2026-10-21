#include"lcl/net/TcpClient.hpp"

#include<sys/socket.h>

#include<atomic>
#include<csignal>
#include<cstdlib>
#include<iostream>
#include<string>
#include<thread>

int main(int argc,char* argv[]){
    if(argc!=4){
        std::cerr<<"Usage: "<<argv[0]<<" <ip> <port> <username>\n";
        return 1;
    }
    std::signal(SIGPIPE,SIG_IGN);

    try{
        lcl::net::TcpClient client(argv[1],static_cast<std::uint16_t>(std::atoi(argv[2])));
        auto& sock=client.socket();

        //首行发送用户名完成注册
        std::string name=std::string(argv[3])+"\n";
        sock.send_all(name.data(),name.size(),MSG_NOSIGNAL);

        std::atomic<bool> quitting{false};

        //后台线程：接收并打印服务器广播
        std::thread reader([&sock,&quitting]{
            char buf[4096];
            try{
                while(true){
                    ssize_t n=sock.recv(buf,sizeof(buf),MSG_NOSIGNAL);
                    if(n<=0){
                        break;
                    }
                    std::cout.write(buf,static_cast<std::streamsize>(n));
                    std::cout.flush();
                }
            }catch(const std::exception&){
            }
            if(!quitting.load()){
                std::cout<<"[服务器已断开]\n";
                std::cout.flush();
                std::_Exit(0);  //main阻塞在stdin，直接退出进程
            }
        });

        std::string line;
        while(std::getline(std::cin,line)){
            if(line=="/quit"){
                break;
            }
            if(line.empty()){
                continue;
            }
            line+='\n';
            sock.send_all(line.data(),line.size(),MSG_NOSIGNAL);
        }

        //优雅退出：唤醒reader线程后回收
        quitting.store(true);
        ::shutdown(sock.get(),SHUT_RDWR);
        reader.join();
    }catch(const std::exception& e){
        std::cerr<<"Error: "<<e.what()<<"\n";
        return 1;
    }
    return 0;
}
