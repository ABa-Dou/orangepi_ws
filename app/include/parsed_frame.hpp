#pragma once
#include <cstdint>
#include <vector>

struct ParsedFrame {
    uint8_t type = 0;
    uint8_t seq = 0;
    uint32_t timestamp_ms = 0;
    std::vector<uint8_t> payload;
};