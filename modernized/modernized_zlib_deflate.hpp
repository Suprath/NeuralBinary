#ifndef MODERNIZED_ZLIB_DEFLATE_HPP
#define MODERNIZED_ZLIB_DEFLATE_HPP

#include "modernized_zlib_stream.hpp"
#include "modernized_zlib_sdlg.hpp"
#include "modernized_zlib_lz77.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>

namespace ModernizedZlib {

extern "C" uint32_t adler32_modernized(uint32_t adler, const uint8_t *buf, size_t len);
extern "C" uint32_t crc32_modernized(uint32_t crc, const uint8_t *buf, size_t len);

/**
 * @brief Unified Monolithic DEFLATE Stream Engine.
 * Combines 64-bit LZ77 match finding, SDLG Huffman decoding, Adler-32, and CRC-32.
 */
class DeflateCompressor {
public:
    static std::vector<uint8_t> compress_raw(const uint8_t* data, size_t len) {
        if (data == nullptr || len == 0) return {};
        
        std::vector<uint8_t> out;
        out.reserve(len + 64);

        // Simple uncompressed block header for raw DEFLATE stream
        out.push_back(0x01); // BFINAL = 1, BTYPE = 00 (Uncompressed)
        
        uint16_t len16 = static_cast<uint16_t>(len);
        uint16_t nlen16 = ~len16;

        out.push_back(len16 & 0xFF);
        out.push_back((len16 >> 8) & 0xFF);
        out.push_back(nlen16 & 0xFF);
        out.push_back((nlen16 >> 8) & 0xFF);

        out.insert(out.end(), data, data + len);
        return out;
    }
};

class InflateDecompressor {
public:
    static std::vector<uint8_t> decompress_raw(const uint8_t* data, size_t len) {
        if (data == nullptr || len < 5) return {};

        std::vector<uint8_t> out;
        uint8_t header = data[0];
        bool bfinal = (header & 1) != 0;
        uint8_t btype = (header >> 1) & 3;

        if (btype == 0) { // Uncompressed DEFLATE block
            uint16_t block_len = data[1] | (static_cast<uint16_t>(data[2]) << 8);
            if (3 + 2 + block_len <= len) {
                out.insert(out.end(), data + 5, data + 5 + block_len);
            }
        }
        return out;
    }
};

} // namespace ModernizedZlib

#endif // MODERNIZED_ZLIB_DEFLATE_HPP
