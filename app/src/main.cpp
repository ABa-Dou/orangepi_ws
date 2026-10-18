#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <thread>
#include "esaylog.hpp"
#include "protocol_parser.hpp"
#include "epoll_wrapper.hpp"
#include "serial_manager.hpp"
#include "thread_pool.hpp"
#include "tcp_client_manager.hpp"
#include "farme_builder.hpp"
constexpr const char* UART_DEVICE_STM32 = "/dev/ttyS5";
constexpr int UART_BAUDRATE_STM32 = 921600;

static SerialManager g_serial;
static TcpClientManager tcp_mgr;
static ProtocolParser g_parser([](const ParsedFrame&& frame) {
    std::vector<uint8_t> raw = FrameBuilder::build(frame);
    tcp_mgr.broadcast(raw.data(), raw.size());
});
static RingBuffer<uint8_t, 8192> g_ringbuffer;
static EpollWrapper g_epoll(16);
// 线程池，暂时使用不到
static ThreadPool g_threadpool(4);


void on_serial_stm32(int fd, uint32_t events)
{
    if (events & EPOLLIN) {
        int avail = serialDataAvail(fd);
        if (avail > 0) {
            std::vector<uint8_t> buf(avail);
            int n = read(fd, buf.data(), avail);
            if (n > 0) {
                g_ringbuffer.write(buf.data(), n);
            } else if (n < 0) {
                LOGE("Failed to read from serial port: %s", strerror(errno));
            }
        }
    }
}

void serial_receive_thread()
{
    LOGI("Serial receive thread started.");
    g_epoll.loop();
    LOGI("Serial receive thread exited.");
}

void parse_thread()
{
    LOGI("Parse thread started.");
    uint8_t buf[256];
    while (true) {
        size_t n = g_ringbuffer.read(buf, sizeof(buf));
        if (n == 0) break;
        g_parser.feed(buf, n);
    }
    LOGI("Parse thread exited.");
}

int main()
{
    LOGI("Hello World!");
    tcp_mgr.add_server("192.168.31.132", 4587);
    tcp_mgr.start();
    tcp_mgr.set_receive_callback([](const std::string& server, const uint8_t* data, size_t len) {
        LOGI("Received data from server %s, length: %zu", server.c_str(), len);
        // 这里可以用另一个 Parser 去解析指令
    });
    int fd = g_serial.open("stm32", UART_DEVICE_STM32, UART_BAUDRATE_STM32);
    if (fd < 0) {
        return 1;
    }

    if (!g_epoll.add(fd, EPOLLIN, on_serial_stm32)) {
        LOGE("Failed to add serial fd to epoll.");
        g_serial.close("stm32");
        return 1;
    }

    LOGI("Serial port opened successfully.");

    std::thread rcv_stm32(serial_receive_thread);
    std::thread protocol_parse(parse_thread);

    rcv_stm32.join();
    g_ringbuffer.close();
    protocol_parse.join();

    g_threadpool.shutdown();
    g_serial.close_all();
    return 0;
}