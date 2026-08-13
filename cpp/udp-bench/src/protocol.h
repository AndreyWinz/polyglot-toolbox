#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <chrono>

constexpr uint16_t DEFAULT_PORT = 9000;
constexpr size_t DEFAULT_PACKET_SIZE = 1400; // Optimal to avoid IP fragmentation
constexpr size_t HEADER_SIZE = 16;           // 8 bytes seq + 8 bytes timestamp

struct PacketHeader {
    uint64_t seq_num;
    uint64_t timestamp_ns;
};

inline void pack_u64(uint8_t* buf, uint64_t val) {
    buf[0] = static_cast<uint8_t>((val >> 56) & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 48) & 0xFF);
    buf[2] = static_cast<uint8_t>((val >> 40) & 0xFF);
    buf[3] = static_cast<uint8_t>((val >> 32) & 0xFF);
    buf[4] = static_cast<uint8_t>((val >> 24) & 0xFF);
    buf[5] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[6] = static_cast<uint8_t>((val >> 8)  & 0xFF);
    buf[7] = static_cast<uint8_t>(val & 0xFF);
}

inline uint64_t unpack_u64(const uint8_t* buf) {
    return (static_cast<uint64_t>(buf[0]) << 56) |
           (static_cast<uint64_t>(buf[1]) << 48) |
           (static_cast<uint64_t>(buf[2]) << 40) |
           (static_cast<uint64_t>(buf[3]) << 32) |
           (static_cast<uint64_t>(buf[4]) << 24) |
           (static_cast<uint64_t>(buf[5]) << 16) |
           (static_cast<uint64_t>(buf[6]) << 8)  |
           (static_cast<uint64_t>(buf[7]));
}

inline uint64_t get_timestamp_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
}

#endif // PROTOCOL_H
