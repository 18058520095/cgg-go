#pragma once

#include"lcl/file/FileDescriptor.hpp"

#include <sys/epoll.h>

#include <cstdint>
#include <vector>

namespace lcl::net{

    //epoll实例的RAII封装（水平触发，data.fd记录就绪描述符）
    class Epoll{
        public:
        Epoll();

        void add(int fd,std::uint32_t events) const;
        void modify(int fd,std::uint32_t events) const;

        //fd关闭时内核会自动移除监听，此处的DEL失败将静默忽略
        void remove(int fd) const;

        //等待就绪事件；EINTR返回空列表
        std::vector<epoll_event> wait(int timeout_ms) const;

        [[nodiscard]] int get() const noexcept{ return fd_.get(); }

        private:
        file::FileDescriptor fd_;
    };
}
