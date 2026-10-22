#pragma once
#include <bit>
#include <cstdint>
#include <stdexcept>

namespace halo2 {
enum class SourceDepthStorageEncoding : std::uint32_t {
    native_d24=0,
    // Exact source integer N is stored as N*2^-24 in a D32 float aspect.
    // Every writer/clear/transfer must preserve this canonical representation.
    d32_integer_d24=1,
};
inline constexpr std::uint32_t source_d24_maximum=0x00ffffffu;
// Integer-only IEEE binary32 encoding. No host rounding mode, source x87/SSE
// backend, floating conversion or normalized D24 sampling is involved.
inline std::uint32_t source_d24_carrier_word(std::uint32_t value) {
    if(value>source_d24_maximum)throw std::invalid_argument("Source depth exceeds its 24-bit integer domain");
    if(!value)return 0;
    const auto top=31u-std::countl_zero(value);
    return ((103u+top)<<23u)|((value<<(23u-top))&0x007fffffu);
}
inline std::uint32_t source_d24_carrier_integer(std::uint32_t word) {
    if(!word)return 0;
    const auto exponent=word>>23u;
    if(exponent<103u||exponent>126u)
        throw std::invalid_argument("Depth word is outside canonical D24 float storage");
    const auto shift=126u-exponent;
    const auto mantissa=(word&0x007fffffu)|0x00800000u;
    if(mantissa&((1u<<shift)-1u))
        throw std::invalid_argument("Depth float contains precision outside its source integer domain");
    return mantissa>>shift;
}
}
