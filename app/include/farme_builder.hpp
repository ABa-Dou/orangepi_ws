#pragma once

#include <vector>
#include <cstdint>
#include "protocol_parser.hpp"

class FrameBuilder {
public:
    static std::vector<uint8_t> build(const ParsedFrame& frame);

private:
    static uint16_t crc16_modbus(const uint8_t* data, size_t len);
};