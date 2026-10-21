#include"lcl/net/Epoll.hpp"

#include <unistd.h>

#include <cerrno>
#include <system_error>

namespace lcl::net{

    Epoll::Epoll(){
        int fd=::epoll_create1(0);
        if(fd<0){
            throw std::system_error(errno,std::system_category(),"epoll_create1");
        }
        fd_.reset(fd);
    }

    void Epoll::add(int fd,std::uint32_t events) const{
        epoll_event ee{};
        ee.events=events;
        ee.data.fd=fd;
        if(::epoll_ctl(fd_.get(),EPOLL_CTL_ADD,fd,&ee)<0){
            throw std::system_error(errno,std::system_category(),"epoll_ctl ADD");
        }
    }

    void Epoll::modify(int fd,std::uint32_t events) const{
        epoll_event ee{};
        ee.events=events;
        ee.data.fd=fd;
        if(::epoll_ctl(fd_.get(),EPOLL_CTL_MOD,fd,&ee)<0){
            throw std::system_error(errno,std::system_category(),"epoll_ctl MOD");
        }
    }

    void Epoll::remove(int fd) const{
        ::epoll_ctl(fd_.get(),EPOLL_CTL_DEL,fd,nullptr);
    }

    std::vector<epoll_event> Epoll::wait(int timeout_ms) const{
        epoll_event evs[64];
        int n=::epoll_wait(fd_.get(),evs,64,timeout_ms);
        if(n<0){
            if(errno==EINTR){
                return {};
            }
            throw std::system_error(errno,std::system_category(),"epoll_wait");
        }
        return std::vector<epoll_event>(evs,evs+n);
    }
}
