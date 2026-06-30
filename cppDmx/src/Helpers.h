#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>

// --- 16-bit field I/O ---------------------------------------------------
// Art-Net stores opcodes little-endian but most other fields big-endian,
// so spelling out the byte order at each call site keeps it honest.

inline std::uint16_t readLittleEndian16(const std::uint8_t* src)
{
    return static_cast<std::uint16_t> (src[0] | (src[1] << 8));
}

inline std::uint16_t readBigEndian16(const std::uint8_t* src)
{
    return static_cast<std::uint16_t> ((src[0] << 8) | src[1]);
}

inline void writeLittleEndian16(std::uint8_t* dest, std::uint16_t value)
{
    dest[0] = static_cast<std::uint8_t> (value & 0xFF);
    dest[1] = static_cast<std::uint8_t> (value >> 8);
}

inline void writeBigEndian16(std::uint8_t* dest, std::uint16_t value)
{
    dest[0] = static_cast<std::uint8_t> (value >> 8);
    dest[1] = static_cast<std::uint8_t> (value & 0xFF);
}

// --- Text helpers -------------------------------------------------------

inline std::string trim(const std::string& text)
{
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };

    const auto first = std::find_if(text.begin(), text.end(), notSpace);
    const auto last = std::find_if(text.rbegin(), text.rend(), notSpace).base();

    return first < last ? std::string(first, last) : std::string();
}
