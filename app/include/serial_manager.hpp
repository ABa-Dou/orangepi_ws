#pragma once

#include "esaylog.hpp"
#include "wiringSerial.h"
#include <string>
#include <unordered_map>
#include <vector>

struct SerialPort {
    int fd = -1;
    std::string name;
    int baudrate = 0;
};

class SerialManager {
public:
    SerialManager() = default;
    ~SerialManager();

    int open(const std::string& tag, const char* path, int baudrate);
    void close(const std::string& tag);
    void close_all();

    const SerialPort* get(const std::string& tag) const;
    int fd(const std::string& tag) const;

    std::vector<std::string> list() const;

private:
    std::unordered_map<std::string, SerialPort> ports_;
};