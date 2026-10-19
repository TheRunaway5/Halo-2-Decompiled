#pragma once
#include <cstdint>

namespace halo2 {
// TU1.5 17D210 passes mode0 to 17DCE0. The iterator admits WORLD and
// unbound FFFF nodes, excluding encoded first-person nodes. Do not collapse
// equal marker names: separate admitted WORLD locations remain separate.
constexpr bool source_effect_world_location_admitted_tu15(std::uint16_t node) noexcept {
    return node==0xffffu || !(node&0x8000u);
}
}
