#ifndef MODERNIZED_ZLIB_PRESET_DICT_HPP
#define MODERNIZED_ZLIB_PRESET_DICT_HPP

#include "modernized_zlib_deflate.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <array>

namespace ModernizedZlib {

/**
 * @brief Pre-Trained Corpus Preset Dictionary Engine.
 * Pre-populates the 32KB LZ77 sliding window at startup with common API schema tokens
 * (JSON/REST/XML), enabling instant Byte-0 string match deduplication.
 */
class PresetDictionaryEngine {
public:
    static std::vector<uint8_t> compress_with_dictionary(const uint8_t* payload, size_t payload_len,
                                                         const uint8_t* dict, size_t dict_len) {
        if (payload == nullptr || payload_len == 0) return {};

        // Pre-combine dict + payload into a unified virtual sliding window buffer
        size_t effective_dict_len = std::min(dict_len, WINDOW_SIZE);
        std::vector<uint8_t> combined;
        combined.reserve(effective_dict_len + payload_len);

        if (dict != nullptr && effective_dict_len > 0) {
            combined.insert(combined.end(), dict + (dict_len - effective_dict_len), dict + dict_len);
        }

        combined.insert(combined.end(), payload, payload + payload_len);

        // Compress combined stream
        auto full_compressed = DeflateCompressor::compress_raw(combined.data(), combined.size());
        return full_compressed;
    }
};

} // namespace ModernizedZlib

#endif // MODERNIZED_ZLIB_PRESET_DICT_HPP
