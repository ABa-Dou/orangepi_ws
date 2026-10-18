#pragma once
#include "parsed_frame.hpp"
#include "ringbuffer.hpp"
#include <cstdint>
class ProtocolParser {
    public:
        using FrameCallback = std::function<void(const ParsedFrame&&)>;
        explicit ProtocolParser(FrameCallback callback);

        void feed(const uint8_t* data, size_t len);
        void reset();

    private:
        static constexpr size_t MAX_PAYLOAD = 8192;
        FrameCallback callback_;
        std::vector<uint8_t> pending_;
        void parse();
        static uint16_t crc16_modbus(const uint8_t* data, size_t len);
        
};