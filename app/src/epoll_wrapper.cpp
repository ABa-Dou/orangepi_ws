#include "epoll_wrapper.hpp"
#include "esaylog.hpp"
#include <cstring>

EpollWrapper::EpollWrapper(int max_events)
    : epfd_(epoll_create1(0)), max_events_(max_events), events_(max_events)
{
    if (epfd_ < 0) {
        LOGE("Failed to create epoll instance: %s", strerror(errno));
    }
}

EpollWrapper::~EpollWrapper()
{
    if (epfd_ >= 0) {
        close(epfd_);
    }
}

bool EpollWrapper::add(int fd, uint32_t events, const EventHandler& handler)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) < 0) {
        LOGE("epoll_ctl ADD failed for fd=%d: %s", fd, strerror(errno));
        return false;
    }
    handlers_[fd] = handler;
    return true;
}

bool EpollWrapper::modify(int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) < 0) {
        LOGE("epoll_ctl MOD failed for fd=%d: %s", fd, strerror(errno));
        return false;
    }
    return true;
}

bool EpollWrapper::remove(int fd)
{
    if (epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) < 0) {
        LOGE("epoll_ctl DEL failed for fd=%d: %s", fd, strerror(errno));
        return false;
    }
    handlers_.erase(fd);
    return true;
}

int EpollWrapper::wait(int timeout)
{
    int nfds = epoll_wait(epfd_, events_.data(), max_events_, timeout);
    if (nfds < 0 && errno != EINTR) {
        LOGE("epoll_wait error: %s", strerror(errno));
    }
    return nfds;
}

void EpollWrapper::loop(int timeout)
{
    running_ = true;
    while (running_) {
        int nfds = wait(timeout);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < nfds; ++i) {
            int event_fd = events_[i].data.fd;
            uint32_t revents = events_[i].events;
            auto it = handlers_.find(event_fd);
            if (it != handlers_.end()) {
                it->second(event_fd, revents);
            }
        }
    }
}

void EpollWrapper::stop()
{
    running_ = false;
}