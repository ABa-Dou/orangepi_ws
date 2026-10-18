#include "serial_manager.hpp"
#include <cstring>

SerialManager::~SerialManager()
{
    close_all();
}

int SerialManager::open(const std::string& tag, const char* path, int baudrate)
{
    int fd = serialOpen(path, baudrate);
    if (fd < 0) {
        LOGE("Failed to open serial port %s: %s", path, strerror(errno));
        return -1;
    }
    ports_[tag] = {fd, path, baudrate};
    LOGI("Serial port opened: [%s] %s @ %d, fd=%d", tag.c_str(), path, baudrate, fd);
    return fd;
}

void SerialManager::close(const std::string& tag)
{
    auto it = ports_.find(tag);
    if (it == ports_.end()) return;
    if (it->second.fd >= 0) {
        serialClose(it->second.fd);
        LOGI("Serial port closed: [%s] %s, fd=%d", tag.c_str(), it->second.name.c_str(), it->second.fd);
    }
    ports_.erase(it);
}

void SerialManager::close_all()
{
    for (auto& [tag, port] : ports_) {
        if (port.fd >= 0) {
            serialClose(port.fd);
            LOGI("Serial port closed: [%s] %s, fd=%d", tag.c_str(), port.name.c_str(), port.fd);
        }
    }
    ports_.clear();
}

const SerialPort* SerialManager::get(const std::string& tag) const
{
    auto it = ports_.find(tag);
    if (it != ports_.end()) return &it->second;
    return nullptr;
}

int SerialManager::fd(const std::string& tag) const
{
    auto* port = get(tag);
    return port ? port->fd : -1;
}

std::vector<std::string> SerialManager::list() const
{
    std::vector<std::string> tags;
    tags.reserve(ports_.size());
    for (const auto& [tag, _] : ports_) {
        tags.push_back(tag);
    }
    return tags;
}