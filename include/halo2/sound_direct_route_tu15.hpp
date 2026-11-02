#pragma once
#include <cstdint>

namespace halo2 {
// TU1.5 18CE77..18CE87, before 223B20: a class may request the direct
// channel unless this sound explicitly opts out. Inputs are native-layout
// metadata; MCC route/default bits must have been projected by its owner.
constexpr std::uint16_t source_sound_direct_location_flags_tu15(
    std::uint16_t sound_flags,std::uint16_t class_flags) {
    return (class_flags&0x80u) && !(sound_flags&0x100u)?0x800u:0u;
}
// 12C7CA..12C7DA makes the direct channel available; 12C95A..12C9D8
// selects its zero distance attenuation and adds the authored class direct
// gain. The located channel's distance gain remains a separate value. This
// scalar selection does not claim the Xbox mix-bin/effects/DSP closure.
constexpr float source_sound_direct_gain_tu15(std::uint16_t location_flags,
    float located_gain_db,float class_direct_gain_db) {
    return location_flags&0x800u?class_direct_gain_db:located_gain_db;
}
}
