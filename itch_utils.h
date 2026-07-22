#pragma once

#include <cstdint>
#include <string>

// INLINE UTILITIES (Declared inline in the header for compiler optimizations)

#if defined(_MSC_VER)
#include <stdlib.h>
inline uint16_t bswap16(uint16_t val) { return _byteswap_ushort(val); }
inline uint32_t bswap32(uint32_t val) { return _byteswap_ulong(val); }
inline uint64_t bswap64(uint64_t val) { return _byteswap_uint64(val); }
#elif defined(__GNUC__) || defined(__clang__)
inline uint16_t bswap16(uint16_t val) { return __builtin_bswap16(val); }
inline uint32_t bswap32(uint32_t val) { return __builtin_bswap32(val); }
inline uint64_t bswap64(uint64_t val) { return __builtin_bswap64(val); }
#else
inline uint16_t bswap16(uint16_t val) {
    return (val >> 8) | (val << 8);
}
inline uint32_t bswap32(uint32_t val) {
    return ((val >> 24) & 0xff) | ((val >> 8) & 0xff00) |
           ((val << 8) & 0xff0000) | ((val << 24) & 0xff000000);
}
inline uint64_t bswap64(uint64_t val) {
    return (static_cast<uint64_t>(bswap32(val & 0xffffffff)) << 32) |
           bswap32(val >> 32);
}
#endif

// Reconstructs a 6-byte Big-Endian timestamp into a standard 64-bit integer
inline uint64_t parse_timestamp48(const uint8_t* bytes) {
    return (static_cast<uint64_t>(bytes[0]) << 40) |
           (static_cast<uint64_t>(bytes[1]) << 32) |
           (static_cast<uint64_t>(bytes[2]) << 24) |
           (static_cast<uint64_t>(bytes[3]) << 16) |
           (static_cast<uint64_t>(bytes[4]) << 8)  |
           (static_cast<uint64_t>(bytes[5]));
}

// Converts a 64-bit timestamp into a 6-byte Big-Endian representation
inline void pack_timestamp48(uint64_t ns, uint8_t* dest) {
    dest[0] = static_cast<uint8_t>((ns >> 40) & 0xFF);
    dest[1] = static_cast<uint8_t>((ns >> 32) & 0xFF);
    dest[2] = static_cast<uint8_t>((ns >> 24) & 0xFF);
    dest[3] = static_cast<uint8_t>((ns >> 16) & 0xFF);
    dest[4] = static_cast<uint8_t>((ns >> 8)  & 0xFF);
    dest[5] = static_cast<uint8_t>(ns & 0xFF);
}

// FUNCTION DECLARATIONS (Implementations in itch_utils.cpp)

std::string clean_symbol(const char* symbol_bytes, size_t length = 8);
std::string format_timestamp(uint64_t ns);
