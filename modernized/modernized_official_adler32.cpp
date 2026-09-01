// Modernized High-Performance C++20 Implementation of Official zlib adler32 Checksum
#include <cstdint>
#include <cstddef>

namespace ModernizedZlib {

constexpr uint32_t BASE = 65521U; // Largest prime smaller than 65536
constexpr size_t NMAX = 5552U;    // Max bytes before 32-bit uint accumulator overflow

/**
 * @brief Fast Division-Free Modulo 65521.
 * Exploits prime property 65521 = 2^16 - 15 to eliminate hardware division instructions.
 */
inline constexpr uint32_t mod65521(uint32_t a) {
    a = (a & 0xffffU) + (a >> 16) * 15U;
    a = (a & 0xffffU) + (a >> 16) * 15U;
    return (a >= BASE) ? (a - BASE) : a;
}

/**
 * @brief High-performance Adler-32 checksum with 16-byte block unrolling and division-free modulo.
 * Eliminates 93.75% of loop branch instruction overhead.
 */
extern "C" uint32_t adler32_modernized(uint32_t adler, const uint8_t *buf, size_t len) {
    if (buf == nullptr) return 1U;

    uint32_t s1 = adler & 0xffff;
    uint32_t s2 = (adler >> 16) & 0xffff;

    while (len > 0) {
        size_t k = (len < NMAX) ? len : NMAX;
        len -= k;

        // 16-byte block unrolled loop
        while (k >= 16) {
            s1 += buf[0];  s2 += s1;
            s1 += buf[1];  s2 += s1;
            s1 += buf[2];  s2 += s1;
            s1 += buf[3];  s2 += s1;
            s1 += buf[4];  s2 += s1;
            s1 += buf[5];  s2 += s1;
            s1 += buf[6];  s2 += s1;
            s1 += buf[7];  s2 += s1;
            s1 += buf[8];  s2 += s1;
            s1 += buf[9];  s2 += s1;
            s1 += buf[10]; s2 += s1;
            s1 += buf[11]; s2 += s1;
            s1 += buf[12]; s2 += s1;
            s1 += buf[13]; s2 += s1;
            s1 += buf[14]; s2 += s1;
            s1 += buf[15]; s2 += s1;
            buf += 16;
            k -= 16;
        }

        // Remaining tail bytes
        while (k > 0) {
            s1 += *buf++;
            s2 += s1;
            --k;
        }

        s1 = mod65521(s1);
        s2 = mod65521(s2);
    }

    return (s2 << 16) | s1;
}

/**
 * @brief Combines two Adler-32 checksums for parallel multi-threaded stream processing.
 */
extern "C" uint32_t adler32_combine_modernized(uint32_t adler1, uint32_t adler2, size_t len2) {
    uint32_t rem = mod65521(static_cast<uint32_t>(len2));
    uint32_t s1 = adler1 & 0xffff;
    uint32_t s2 = (adler1 >> 16) & 0xffff;

    s2 = mod65521(s2 + (s1 * rem));

    uint32_t s1_2 = adler2 & 0xffff;
    uint32_t s2_2 = (adler2 >> 16) & 0xffff;

    s1 = mod65521(s1 + s1_2 + BASE - 1);
    s2 = mod65521(s2 + s2_2 + BASE - rem);

    return (s2 << 16) | s1;
}

} // namespace ModernizedZlib
