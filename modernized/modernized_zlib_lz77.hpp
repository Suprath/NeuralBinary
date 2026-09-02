#ifndef MODERNIZED_ZLIB_LZ77_HPP
#define MODERNIZED_ZLIB_LZ77_HPP

// High-Performance 64-Bit LZ77 Sliding Window Match Engine for Modernized zlib
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>

namespace ModernizedZlib {

constexpr size_t MIN_MATCH = 3;
constexpr size_t MAX_MATCH = 258;
constexpr size_t WINDOW_SIZE = 32768; // 32KB LZ77 Sliding Window

/**
 * @brief Fast 64-bit Word Match Engine (longest_match_fast).
 * Scans 8 bytes of sliding window memory per clock cycle using Count Trailing Zeros (__builtin_ctzll).
 * Replaces traditional 1-byte scalar loops in zlib's deflate.c.
 */
inline size_t longest_match_fast(const uint8_t *s1, const uint8_t *s2, size_t max_len) {
    size_t len = 0;
    max_len = std::min(max_len, MAX_MATCH);

    // 1. Process 8 bytes per iteration using 64-bit word XOR comparison
    while (len + 8 <= max_len) {
        uint64_t w1, w2;
        std::memcpy(&w1, s1 + len, sizeof(uint64_t));
        std::memcpy(&w2, s2 + len, sizeof(uint64_t));

        uint64_t diff = w1 ^ w2;
        if (diff != 0) {
            // Count trailing matching zero bits to find exact byte offset (little endian)
#if defined(__GNUC__) || defined(__clang__)
            int zero_bits = __builtin_ctzll(diff);
#else
            int zero_bits = 0;
            while ((diff & 0xFF) == 0) {
                zero_bits += 8;
                diff >>= 8;
            }
#endif
            return len + (zero_bits >> 3);
        }
        len += 8;
    }

    // 2. Remaining tail bytes
    while (len < max_len && s1[len] == s2[len]) {
        len++;
    }

    return len;
}

} // namespace ModernizedZlib

#endif // MODERNIZED_ZLIB_LZ77_HPP
