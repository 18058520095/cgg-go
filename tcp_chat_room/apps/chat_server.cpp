#include"lcl/net/ChatRoom.hpp"

#include<iostream>
#include<cstdlib>

int main(int argc,char* argv[]){
    if(argc!=3){
        std::cerr<<"Usage: "<<argv[0]<<" <ip> <port>\n";
        return 1;
    }

    try{
        lcl::net::ChatRoom room(argv[1],static_cast<std::uint16_t>(std::atoi(argv[2])));
        room.run();
    }catch(const std::exception& e){
        std::cerr<<"Error: "<<e.what()<<"\n";
        return 1;
    }
    return 0;
}
