#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>
#include <cstdint>
#include "esaylog.hpp"
#include "thread_pool.hpp"
class TcpClientManager {
    public:
        using ReceiveCallback = std::function<void(const std::string& server_addr,  const uint8_t* data, size_t len)>;
        TcpClientManager();
        ~TcpClientManager();

        void add_server(const std::string& server_addr, int port);

        void start();

        void stop();

        void broadcast(const uint8_t* data, size_t len);
        void broadcast(const std::vector<uint8_t>& data);

        bool send_to(const std::string& ip, int port, const uint8_t* data, size_t len);
        void set_receive_callback(ReceiveCallback cb);

    private:
        struct ServerInfo {
            std::string addr_str; // "ip:port"
            std::string ip;
            int port;
            int fd;
            bool connected;
        };

        void connect_thread_func();
        void receive_thread_func(const std::string& addr_str);

        bool connect_to_server(ServerInfo& info);

        std::vector<ServerInfo> servers_;
        std::atomic<bool> running_;
        std::thread connect_manager_thread_;
        std::mutex servers_mutex_; // 保护 servers_ 和 fd 操作
        ReceiveCallback rx_callback_;
};
