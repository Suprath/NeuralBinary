// Modernized C++20 Implementation of Official zlib adler32 Checksum
#include <cstdint>
#include <cstddef>

namespace ModernizedZlib {

constexpr uint32_t BASE = 65521U; // Largest prime smaller than 65536

/**
 * @brief Computes 32-bit Adler checksum with 100% behavioral parity.
 */
extern "C" uint32_t adler32_modernized(uint32_t adler, const uint8_t *buf, size_t len) {
    uint32_t s1 = adler & 0xffff;
    uint32_t s2 = (adler >> 16) & 0xffff;

    if (buf == nullptr) return 1U;

    for (size_t i = 0; i < len; ++i) {
        s1 = (s1 + buf[i]) % BASE;
        s2 = (s2 + s1) % BASE;
    }

    return (s2 << 16) | s1;
}

} // namespace ModernizedZlib
