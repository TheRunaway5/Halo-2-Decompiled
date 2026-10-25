#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <string>

namespace halo2 {
struct NativeGrenadeWireState {
    std::array<std::uint8_t,2> counts{};
    std::int8_t selected=-1;
};
// The original matg maximum is4 per type, not the starting inventory of2.
// Authority: original-grenade-count-authority-v1.json (three original maps).
// Two3-bit counts and the2-bit selection still occupy one native wire byte.
inline std::uint8_t encode_native_grenades(std::array<std::uint8_t,2> counts,std::int8_t selected) {
    if(counts[0]>4 || counts[1]>4)
        throw FormatError("Invalid native grenade counts: frag="+std::to_string(counts[0])+" plasma="+std::to_string(counts[1]));
    if(selected< -1 || selected>1)throw FormatError("Invalid native grenade selection");
    return std::uint8_t(counts[0]|(unsigned(counts[1])<<3)|(unsigned(selected+1)<<6));
}
inline NativeGrenadeWireState decode_native_grenades(std::uint8_t value) {
    NativeGrenadeWireState out{{std::uint8_t(value&7u),std::uint8_t((value>>3)&7u)},
        std::int8_t(int(value>>6)-1)};
    if(out.counts[0]>4 || out.counts[1]>4 || out.selected>1)
        throw FormatError("Invalid LAN grenade counts or selection");
    return out;
}
}
