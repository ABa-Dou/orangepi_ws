#include <iostream>
#include <string>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <cstring>
#include "esaylog.hpp"
#include "protocol_parser.hpp"
int main()
{
    LOGI("Hello World!");
    int fd = serialOpen("/dev/ttyS5", 921600);
    if (fd < 0) {
        LOGE("Failed to open serial port: %s", strerror(errno));
        return 1;
    }
    ProtocolParser parser;
    LOGI("Serial port opened successfully.");
    while (true) {
        if (serialDataAvail(fd) > 0) {
            char c = serialGetchar(fd);
            
        }
    }
    serialClose(fd);
    return 0;
}