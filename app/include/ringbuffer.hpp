#pragma once

#include <array>
#include <cstddef>
#include <algorithm>
#include <mutex>
#include <condition_variable>

template<typename T, size_t N>
class RingBuffer {
public:
    size_t write(const T* data, size_t len) {
        std::lock_guard<std::mutex> lock(mutex_);
        size_t free_space = N - size_;
        size_t to_write = std::min(len, free_space);
        if (to_write == 0) return 0;

        size_t first = std::min(to_write, N - head_);
        std::copy(data, data + first, buffer_.data() + head_);

        if (first < to_write) {
            std::copy(data + first, data + to_write, buffer_.data());
        }

        head_ = (head_ + to_write) % N;
        size_ += to_write;
        not_empty_.notify_one();
        return to_write;
    }

    size_t read(T* data, size_t len) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return size_ > 0 || closed_; });
        if (size_ == 0) return 0;

        size_t to_read = std::min(len, size_);
        size_t first = std::min(to_read, N - tail_);
        std::copy(buffer_.data() + tail_, buffer_.data() + tail_ + first, data);

        if (first < to_read) {
            std::copy(buffer_.data(), buffer_.data() + (to_read - first), data + first);
        }

        tail_ = (tail_ + to_read) % N;
        size_ -= to_read;
        return to_read;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }

    size_t capacity() const { return N; }

    void close() {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
        not_empty_.notify_all();
    }

private:
    std::array<T, N> buffer_{};
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t size_ = 0;
    bool closed_ = false;

    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
};