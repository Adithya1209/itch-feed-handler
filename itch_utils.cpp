#include "itch_utils.h"
#include <cstdio>
#include <iomanip>

// Trims trailing space padding from an ASCII ticker symbol
std::string clean_symbol(const char* symbol_bytes, size_t length) {
    std::string s(symbol_bytes, length);
    size_t end = s.find_last_not_of(' ');
    if (end != std::string::npos) {
        s.erase(end + 1);
    } else {
        s.clear();
    }
    return s;
}

// Formats nanoseconds since midnight into HH:MM:SS.nanoseconds
std::string format_timestamp(uint64_t ns) {
    uint64_t sec = ns / 1000000000ULL;
    uint64_t remaining_ns = ns % 1000000000ULL;
    uint64_t hour = sec / 3600;
    uint64_t min = (sec % 3600) / 60;
    sec = sec % 60;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%02u:%02u:%02u.%09llu",
                 static_cast<unsigned>(hour),
                 static_cast<unsigned>(min),
                 static_cast<unsigned>(sec),
                 static_cast<unsigned long long>(remaining_ns));
    return std::string(buf);
}
