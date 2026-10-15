#include <iostream>
#include <string>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <cstring>

int main()
{
    std::cout << "Hello World!" << std::endl;
    int fd = serialOpen("/dev/ttyS5", 921600);
    if (fd < 0) {
        std::cerr << "Failed to open serial port: " << strerror(errno) << std::endl;
        return 1;
    }
    std::cout << "Serial port opened successfully." << std::endl;
    serialClose(fd);
    return 0;
}