#pragma once
#include <array>
#include <cstddef>
#include <mutex>
#include <condition_variable>
template<typename T, size_t N>
class RingBuffer {
public:
    bool push(const T& data) {
        size_t next = (head + 1) % N;

        if (next == tail) {
            return false;  // 缓冲区已满
        }

        buffer[head] = data;
        head = next;
        size++;
        return true;
    }

    bool pop(T& data) {
        if (head == tail) {
            return false;  // 缓冲区为空
        }

        data = buffer[tail];
        tail = (tail + 1) % N;
        size--;
        return true;
    }

private:
    std::array<T, N> buffer{};
    size_t head = 0;
    size_t tail = 0;
    size_t size = 0;

    bool closed_ = false;

    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
};