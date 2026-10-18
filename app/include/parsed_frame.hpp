#pragma once
#include <cstdint>
#include <vector>

struct ParsedFrame {
    uint8_t type = 0;
    uint8_t seq = 0;
    int64_t timestamp_ns = 0;
    std::vector<uint8_t> payload;
};