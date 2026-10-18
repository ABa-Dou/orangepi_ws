#include "protocol_parser.hpp"
#include <algorithm>
#include <utility>
#include <time.h>

namespace {
int64_t now_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
}
}

namespace {
constexpr uint8_t HEADER_1 = 0xAA;
constexpr uint8_t HEADER_2 = 0x55;
constexpr uint8_t FOOTER_1 = 0x0D;
constexpr uint8_t FOOTER_2 = 0x0A;

constexpr size_t HEADER_SIZE  = 2;
constexpr size_t LENGTH_SIZE  = 2;
constexpr size_t TYPE_SIZE    = 1;
constexpr size_t SEQ_SIZE     = 1;
constexpr size_t TIMESTAMP_SIZE = 4;
constexpr size_t CRC_SIZE     = 2;
constexpr size_t FOOTER_SIZE  = 2;

constexpr size_t LENGTH_OFFSET = HEADER_SIZE;
constexpr size_t TYPE_OFFSET   = HEADER_SIZE + LENGTH_SIZE;
constexpr size_t SEQ_OFFSET    = TYPE_OFFSET + TYPE_SIZE;
constexpr size_t TIMESTAMP_OFFSET = SEQ_OFFSET + SEQ_SIZE;
constexpr size_t PAYLOAD_OFFSET   = TIMESTAMP_OFFSET + TIMESTAMP_SIZE;

constexpr size_t FIXED_OVERHEAD = HEADER_SIZE + LENGTH_SIZE + TYPE_SIZE
                                + SEQ_SIZE + TIMESTAMP_SIZE + CRC_SIZE + FOOTER_SIZE;

constexpr size_t LENGTH_CONTENT = TYPE_SIZE + SEQ_SIZE + TIMESTAMP_SIZE + CRC_SIZE;
}

ProtocolParser::ProtocolParser(FrameCallback callback) : callback_(std::move(callback)) {
    pending_.reserve(MAX_PAYLOAD);
}

void ProtocolParser::feed(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0) return;
    pending_.insert(pending_.end(), data, data + len);
    parse();
}

void ProtocolParser::reset() {
    pending_.clear();
}

uint16_t ProtocolParser::crc16_modbus(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void ProtocolParser::parse() {
    while (true) {
        //  粘包处理：找帧头，跳过帧头前的垃圾数据
        size_t pos = 0;
        while (pos + 1 < pending_.size()) {
            if (pending_[pos] == HEADER_1 && pending_[pos + 1] == HEADER_2) {
                break;
            }
            ++pos;
        }
        if (pos > 0) {
            pending_.erase(pending_.begin(), pending_.begin() + pos);
        }

        //  断包处理：帧头不完整，等更多数据
        if (pending_.size() < HEADER_SIZE) {
            if (!pending_.empty() && pending_[0] != HEADER_1) {
                pending_.clear();
            }
            return;
        }

        //  断包处理：Length 字段不完整，等更多数据
        if (pending_.size() < HEADER_SIZE + LENGTH_SIZE) {
            return;
        }

        uint16_t length = static_cast<uint16_t>(pending_[LENGTH_OFFSET])
                        | (static_cast<uint16_t>(pending_[LENGTH_OFFSET + 1]) << 8);

        //  错包处理：Length 值异常（小于最小内容长度）
        if (length < LENGTH_CONTENT) {
            pending_.erase(pending_.begin());
            continue;
        }

        uint16_t payload_len = length - LENGTH_CONTENT;

        //  错包处理：Payload 超过最大长度
        if (payload_len > MAX_PAYLOAD) {
            pending_.erase(pending_.begin());
            continue;
        }

        size_t total_len = FIXED_OVERHEAD + payload_len;

        //  断包处理：整帧数据不完整，等更多数据
        if (pending_.size() < total_len) {
            return;
        }

        //  错包处理：帧尾不匹配
        if (pending_[total_len - 2] != FOOTER_1 || pending_[total_len - 1] != FOOTER_2) {
            pending_.erase(pending_.begin());
            continue;
        }

        //  错包处理：CRC 校验失败
        size_t crc_pos = PAYLOAD_OFFSET + payload_len;
        uint16_t received_crc =
            static_cast<uint16_t>(pending_[crc_pos])
            | (static_cast<uint16_t>(pending_[crc_pos + 1]) << 8);

        size_t crc_data_len = LENGTH_SIZE + length - CRC_SIZE;
        uint16_t calculated_crc = crc16_modbus(
            pending_.data() + LENGTH_OFFSET,
            crc_data_len);

        if (received_crc != calculated_crc) {
            pending_.erase(pending_.begin());
            continue;
        }

        ParsedFrame frame;
        frame.type = pending_[TYPE_OFFSET];
        frame.seq = pending_[SEQ_OFFSET];
        // frame.timestamp_ms =
        //     static_cast<uint32_t>(pending_[TIMESTAMP_OFFSET])
        //     | (static_cast<uint32_t>(pending_[TIMESTAMP_OFFSET + 1]) << 8)
        //     | (static_cast<uint32_t>(pending_[TIMESTAMP_OFFSET + 2]) << 16)
        //     | (static_cast<uint32_t>(pending_[TIMESTAMP_OFFSET + 3]) << 24);
        //这里不使用上报上来的毫秒时间，暂时使用本系统的纳秒时间。
        frame.timestamp_ns = now_ns();

        frame.payload.assign(
            pending_.begin() + PAYLOAD_OFFSET,
            pending_.begin() + PAYLOAD_OFFSET + payload_len);

        //  粘包处理：从缓冲区移除已解析的完整帧，继续解析下一帧
        pending_.erase(
            pending_.begin(),
            pending_.begin() + total_len);

        if (callback_) {
            callback_(std::move(frame));
        }
    }
}