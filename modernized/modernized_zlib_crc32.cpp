// Modernized High-Performance C++20 Implementation of zlib CRC32 Checksum (Slice-by-8 Direct 64-bit Fetch)
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <array>

namespace ModernizedZlib {

constexpr uint32_t CRC32_POLY = 0xEDB88320U;

struct Slice8Table {
    uint32_t table[8][256];

    constexpr Slice8Table() : table{} {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? (CRC32_POLY ^ (c >> 1)) : (c >> 1);
            }
            table[0][i] = c;
        }

        for (uint32_t i = 0; i < 256; ++i) {
            for (int s = 1; s < 8; ++s) {
                table[s][i] = (table[s - 1][i] >> 8) ^ table[0][table[s - 1][i] & 0xFF];
            }
        }
    }
};

constexpr Slice8Table SLICE8_TABLE{};

/**
 * @brief High-performance Slice-by-8 32-bit IEEE 802.3 CRC32 checksum with direct 64-bit memory load.
 */
extern "C" uint32_t crc32_modernized(uint32_t crc, const uint8_t *buf, size_t len) {
    if (buf == nullptr) return 0U;

    crc = crc ^ 0xFFFFFFFFU;

    // Process 8-byte chunks using direct 64-bit memory load
    while (len >= 8) {
        uint64_t chunk;
        std::memcpy(&chunk, buf, sizeof(uint64_t));

        uint32_t low = static_cast<uint32_t>(chunk);
        uint32_t high = static_cast<uint32_t>(chunk >> 32);

        crc ^= low;

        crc = SLICE8_TABLE.table[7][crc & 0xFF] ^
              SLICE8_TABLE.table[6][(crc >> 8) & 0xFF] ^
              SLICE8_TABLE.table[5][(crc >> 16) & 0xFF] ^
              SLICE8_TABLE.table[4][(crc >> 24)] ^
              SLICE8_TABLE.table[3][high & 0xFF] ^
              SLICE8_TABLE.table[2][(high >> 8) & 0xFF] ^
              SLICE8_TABLE.table[1][(high >> 16) & 0xFF] ^
              SLICE8_TABLE.table[0][(high >> 24)];

        buf += 8;
        len -= 8;
    }

    // Tail bytes
    while (len > 0) {
        crc = SLICE8_TABLE.table[0][(crc ^ *buf) & 0xFF] ^ (crc >> 8);
        buf++;
        len--;
    }

    return crc ^ 0xFFFFFFFFU;
}

/**
 * @brief Combines two CRC-32 checksums for parallel stream processing.
 */
extern "C" uint32_t crc32_combine_modernized(uint32_t crc1, uint32_t crc2, size_t len2) {
    uint32_t row = CRC32_POLY;
    uint32_t combine = crc1;

    while (len2 > 0) {
        if (len2 & 1) {
            combine = (combine >> 1) ^ ((combine & 1) ? row : 0);
        }
        len2 >>= 1;
    }

    return combine ^ crc2;
}

} // namespace ModernizedZlib
