#pragma once

#include "ringbuffer.hpp"
#include <cstdint>
class ProtocolParser {
    public:
        ProtocolParser();
    public:
        RingBuffer<uint8_t, 8192> buffer_;
        
};