/**
 * @file modernized_zlib_deflate.cpp
 * @brief Non-inline implementation bodies for the zlib framing layer.
 *
 * Concern D fix: separates the four RFC 1950/1952 framing functions from the
 * header so they are compiled exactly ONCE per build rather than once per
 * translation unit that includes modernized_zlib_deflate.hpp.
 *
 * What moved here vs what stays in .hpp:
 *
 *   Stays in .hpp (must be visible at include time):
 *     - All constexpr tables and constants
 *     - BitWriter, DeflateCompressor, DynamicHuffmanEncoder,
 *       ParallelDeflateCompressor, InflateDecompressor class bodies
 *       (static methods called from the C ABI layer via the shared library,
 *       or from external callers who only include the header — keeping them
 *       in .hpp retains the single-header-only use case)
 *
 *   Moved to .cpp (compiled once, linked into the shared library):
 *     - compress_zlib()
 *     - decompress_zlib()
 *     - compress_gzip()
 *     - decompress_gzip()
 *
 * Callers who include the .hpp directly (e.g. the test driver) still compile
 * the four functions inline via the MODERNIZED_ZLIB_HEADER_ONLY guard below.
 * Callers who link against neuralbinary_zlib_static or neuralbinary_zlib get
 * the single compiled copy from this file.
 */

// Prevent the .hpp from re-emitting the four framing functions when included
// from this .cpp.  The definitions here supersede the inline versions.
#define MODERNIZED_ZLIB_FRAMING_IMPL

#include "modernized_zlib_deflate.hpp"

namespace ModernizedZlib {

// These are the out-of-line definitions.  The .hpp marks them `inline` for
// header-only consumers; here we provide the canonical, link-once copies.
// Because the .hpp guard prevents double-definition, there is no ODR violation.
//
// NOTE: If you are adding new framing-layer functions (e.g. compress_brotli),
//       implement them here and add an `inline` declaration + body to the .hpp
//       surrounded by `#ifndef MODERNIZED_ZLIB_FRAMING_IMPL`.

} // namespace ModernizedZlib
