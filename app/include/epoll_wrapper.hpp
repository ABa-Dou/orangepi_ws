#pragma once

#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <cerrno>
#include <cstdint>

class EpollWrapper {
public:
    using EventHandler = std::function<void(int fd, uint32_t events)>;

    explicit EpollWrapper(int max_events = 16);
    ~EpollWrapper();

    EpollWrapper(const EpollWrapper&) = delete;
    EpollWrapper& operator=(const EpollWrapper&) = delete;
    EpollWrapper(EpollWrapper&&) = delete;
    EpollWrapper& operator=(EpollWrapper&&) = delete;

    bool add(int fd, uint32_t events, const EventHandler& handler);
    bool modify(int fd, uint32_t events);
    bool remove(int fd);

    int wait(int timeout = -1);

    void loop(int timeout = -1);
    void stop();

private:
    int epfd_;
    int max_events_;
    std::vector<struct epoll_event> events_;
    std::unordered_map<int, EventHandler> handlers_;
    bool running_ = false;
};