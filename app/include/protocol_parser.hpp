#pragma once

#include "ringbuffer.hpp"
#include <cstdint>
class ProtocolParser {
    public:
        ProtocolParser();
    private:
        RingBuffer<uint8_t, 8192> input_buffer;
};