#include <iostream>
#include <string>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <cstring>
#include "esaylog.hpp"
#include "protocol_parser.hpp"
#include "epoll_wrapper.hpp"

static int g_serial_fd = -1;

void on_serial_readable(int fd, uint32_t events)
{
    if (events & EPOLLIN) {
        int avail = serialDataAvail(fd);
        for (int i = 0; i < avail; ++i) {
            int c = serialGetchar(fd);
        }
    }
}

int main()
{
    LOGI("Hello World!");

    g_serial_fd = serialOpen("/dev/ttyS5", 921600);
    if (g_serial_fd < 0) {
        LOGE("Failed to open serial port: %s", strerror(errno));
        return 1;
    }

    EpollWrapper epoll(16);
    if (!epoll.add(g_serial_fd, EPOLLIN, on_serial_readable)) {
        LOGE("Failed to add serial fd to epoll.");
        serialClose(g_serial_fd);
        return 1;
    }

    ProtocolParser parser;
    LOGI("Serial port opened successfully, epoll monitoring started.");

    epoll.loop();

    serialClose(g_serial_fd);
    return 0;
}