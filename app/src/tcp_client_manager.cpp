#include "tcp_client_manager.hpp"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
TcpClientManager::TcpClientManager() : running_(false) {}


TcpClientManager::~TcpClientManager() {
    stop();
}

void TcpClientManager::add_server(const std::string& ip, int port) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    std::string addr_str = ip + ":" + std::to_string(port);

    for (const auto& s : servers_) {
        if (s.addr_str == addr_str) return;
    }

    ServerInfo info;
    info.addr_str = addr_str;
    info.ip = ip;
    info.port = port;
    info.fd = -1;
    info.connected = false;
    servers_.push_back(info);
}


void TcpClientManager::start() {
    if (running_) return;
    running_ = true;
    connect_manager_thread_ = std::thread(&TcpClientManager::connect_thread_func, this);
    LOGI("TcpClientManager started.");
}

void TcpClientManager::stop() {
    if (!running_) return;
    running_ = false;
    {
        std::lock_guard<std::mutex> lock(servers_mutex_);
        for (auto& s : servers_) {
            if (s.fd >= 0) {
                shutdown(s.fd, SHUT_RDWR);
                close(s.fd);
                s.fd = -1;
            }
        }
    }

    if (connect_manager_thread_.joinable()) connect_manager_thread_.join();

    LOGI("TcpClientManager stopped.");
}


void TcpClientManager::set_receive_callback(ReceiveCallback cb) {
    rx_callback_ = cb;
}


void TcpClientManager::connect_thread_func() {
    while (running_) {
        {
            std::lock_guard<std::mutex> lock(servers_mutex_);
            for (auto& s : servers_) {
                if (!s.connected) {
                    if (connect_to_server(s)) {
                        s.connected = true;
                        LOGI("Connected to server: %s", s.addr_str.c_str());
                        std::string addr_str = s.addr_str;
                        int fd = s.fd;
                        std::thread([this, addr_str, fd]() {
                            uint8_t buf[1024];
                            while (running_) {
                                ssize_t n = recv(fd, buf, sizeof(buf), 0);
                                if (n > 0) {
                                    if (rx_callback_) rx_callback_(addr_str, buf, n);
                                } else {
                                    // 断线
                                    LOGW("Disconnected from server: %s", addr_str.c_str());
                                    std::lock_guard<std::mutex> lock(servers_mutex_);
                                    for (auto& srv : servers_) {
                                        if (srv.addr_str == addr_str && srv.fd == fd) {
                                            srv.connected = false;
                                            close(srv.fd);
                                            srv.fd = -1;
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }).detach();
                    }
                }
                
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }  
    }          
}

bool TcpClientManager::connect_to_server(ServerInfo& info) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return false;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(info.port);
    if (inet_pton(AF_INET, info.ip.c_str(), &serv_addr.sin_addr) <= 0) {
        close(fd);
        return false;
    }

    if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(fd);
        return false; // 连不上，返回失败，下次重试
    }

    info.fd = fd;
    return true;
}


void TcpClientManager::broadcast(const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(servers_mutex_);
    for (auto& s : servers_) {
        if (s.connected && s.fd >= 0) {
            size_t total = 0;
            while (total < len) {
                ssize_t n = send(s.fd, data + total, len - total, MSG_NOSIGNAL);
                if (n <= 0) {
                    // 发送失败说明断开了，交给重连线程处理
                    s.connected = false;
                    break;
                }
                total += n;
            }
        }
    }
}


void TcpClientManager::broadcast(const std::vector<uint8_t>& data) {
    broadcast(data.data(), data.size());
}

bool TcpClientManager::send_to(const std::string& ip, int port, const uint8_t* data, size_t len) {
    std::string target = ip + ":" + std::to_string(port);
    std::lock_guard<std::mutex> lock(servers_mutex_);
    for (auto& s : servers_) {
        if (s.addr_str == target && s.connected && s.fd >= 0) {
            size_t total = 0;
            while (total < len) {
                ssize_t n = send(s.fd, data + total, len - total, MSG_NOSIGNAL);
                if (n <= 0) {
                    s.connected = false;
                    return false;
                }
                total += n;
            }
            return true;
        }
    }
    return false; // 没找到该目标或未连接
}