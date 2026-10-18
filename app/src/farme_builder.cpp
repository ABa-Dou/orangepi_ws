#include "farme_builder.hpp"
#include <cstring>

constexpr uint8_t HEADER_1 = 0xAA;
constexpr uint8_t HEADER_2 = 0x55;
constexpr uint8_t FOOTER_1 = 0x0D;
constexpr uint8_t FOOTER_2 = 0x0A;

std::vector<uint8_t> FrameBuilder::build(const ParsedFrame& frame) {
    uint16_t payload_len = frame.payload.size();
    // Length = 1(Type) + 1(Seq) + 8(Timestamp_ns) + N(Payload) + 2(CRC) = 12 + N
    uint16_t length = 12 + payload_len; 
    
    // 总长 = 2(Header) + 2(Length) + 1(Type) + 1(Seq) + 8(Timestamp) + N(Payload) + 2(CRC) + 2(Footer) = 18 + N
    std::vector<uint8_t> buf(18 + payload_len);
    size_t idx = 0;

    // 1. 帧头
    buf[idx++] = HEADER_1;
    buf[idx++] = HEADER_2;
    
    // 2. Length (小端序)
    buf[idx++] = length & 0xFF;
    buf[idx++] = (length >> 8) & 0xFF;
    
    // 3. Type, Seq
    buf[idx++] = frame.type;
    buf[idx++] = frame.seq;
    
    // 4. Timestamp_ns (小端序, 8字节)
    buf[idx++] = frame.timestamp_ns & 0xFF;
    buf[idx++] = (frame.timestamp_ns >> 8) & 0xFF;
    buf[idx++] = (frame.timestamp_ns >> 16) & 0xFF;
    buf[idx++] = (frame.timestamp_ns >> 24) & 0xFF;
    buf[idx++] = (frame.timestamp_ns >> 32) & 0xFF;
    buf[idx++] = (frame.timestamp_ns >> 40) & 0xFF;
    buf[idx++] = (frame.timestamp_ns >> 48) & 0xFF;
    buf[idx++] = (frame.timestamp_ns >> 56) & 0xFF;
    
    // 5. Payload
    if (payload_len > 0) {
        std::memcpy(&buf[idx], frame.payload.data(), payload_len);
        idx += payload_len;
    }
    
    // 6. CRC (从 Length 字段开始到 Payload 末尾，共 length 字节)
    uint16_t crc = crc16_modbus(&buf[2], length);
    buf[idx++] = crc & 0xFF;
    buf[idx++] = (crc >> 8) & 0xFF;
    
    // 7. 帧尾
    buf[idx++] = FOOTER_1;
    buf[idx++] = FOOTER_2;
    
    return buf;
}

uint16_t FrameBuilder::crc16_modbus(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}