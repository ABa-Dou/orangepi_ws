#pragma once

#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace esaylog {

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL,
};

inline LogLevel& global_level() {
    static LogLevel level = LogLevel::DEBUG;
    return level;
}

inline void set_level(LogLevel level) {
    global_level() = level;
}

inline const char* level_name(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "D";
        case LogLevel::INFO:  return "I";
        case LogLevel::WARN:  return "W";
        case LogLevel::ERROR: return "E";
        case LogLevel::FATAL: return "F";
    }
    return "?";
}

inline const char* level_color(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "\033[36m";
        case LogLevel::INFO:  return "\033[32m";
        case LogLevel::WARN:  return "\033[33m";
        case LogLevel::ERROR: return "\033[31m";
        case LogLevel::FATAL: return "\033[35m";
    }
    return "\033[0m";
}

inline const char* basename(const char* path) {
    const char* p = strrchr(path, '/');
    return p ? p + 1 : path;
}

inline void log(LogLevel level, const char* file, int line, const char* fmt, ...) {
    if (level < global_level()) return;

    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_buf;
    localtime_r(&ts.tv_sec, &tm_buf);

    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &tm_buf);

    int ms = static_cast<int>(ts.tv_nsec / 1000000);

    fprintf(stderr, "%s[%s %s.%03d %s:%d] ",
            level_color(level), level_name(level),
            time_buf, ms, basename(file), line);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fprintf(stderr, "\033[0m\n");
}

} // namespace esaylog

#define LOGD(fmt, ...) esaylog::log(esaylog::LogLevel::DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) esaylog::log(esaylog::LogLevel::INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) esaylog::log(esaylog::LogLevel::WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) esaylog::log(esaylog::LogLevel::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOGF(fmt, ...) esaylog::log(esaylog::LogLevel::FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)