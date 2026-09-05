#ifndef MODERNIZED_ZLIB_MMAP_HPP
#define MODERNIZED_ZLIB_MMAP_HPP

#include "modernized_zlib_deflate.hpp"
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <stdexcept>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ModernizedZlib {

/**
 * @brief Zero-Copy OS Memory-Mapped Compression Streamer (MmapStreamer).
 * Bypasses heap allocation and user-space buffer copying by operating directly on OS page cache pointers.
 */
class MmapStreamer {
public:
    static std::vector<uint8_t> compress_mmap_file(const std::string& filepath) {
#if defined(_WIN32) || defined(_WIN64)
        HANDLE hFile = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return {};

        DWORD fileSize = GetFileSize(hFile, NULL);
        if (fileSize == 0) { CloseHandle(hFile); return {}; }

        HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (!hMap) { CloseHandle(hFile); return {}; }

        const uint8_t* ptr = (const uint8_t*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        if (!ptr) { CloseHandle(hMap); CloseHandle(hFile); return {}; }

        auto result = ParallelDeflateCompressor::compress_parallel(ptr, fileSize);

        UnmapViewOfFile(ptr);
        CloseHandle(hMap);
        CloseHandle(hFile);
        return result;
#else
        int fd = open(filepath.c_str(), O_RDONLY);
        if (fd < 0) return {};

        struct stat st;
        if (fstat(fd, &st) < 0 || st.st_size == 0) {
            close(fd);
            return {};
        }

        size_t file_size = static_cast<size_t>(st.st_size);
        void* mapped = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
        if (mapped == MAP_FAILED) {
            close(fd);
            return {};
        }

        const uint8_t* ptr = static_cast<const uint8_t*>(mapped);
        auto result = ParallelDeflateCompressor::compress_parallel(ptr, file_size);

        munmap(mapped, file_size);
        close(fd);
        return result;
#endif
    }
};

} // namespace ModernizedZlib

#endif // MODERNIZED_ZLIB_MMAP_HPP
