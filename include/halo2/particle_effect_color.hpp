#pragma once
#include <cmath>
#include <cstdint>

namespace halo2 {
// TU1.5 178920, with the actual22AD40/22AD50 flag getters.176E70
// initializes system+50 to FF808080.1346C0 stores normalized float channels;
// 1349E0 rounds their255 product and packs RGB without an alpha byte.
inline std::uint32_t source_particle_effect_color(std::uint32_t previous,
    std::uint32_t color_a,std::uint32_t color_b,std::uint32_t particle_flags) {
  const bool multiplied=particle_flags&0x80u,tinted=particle_flags&0x100u;
  if(!tinted)return multiplied?color_a:previous;
  std::uint32_t result{};
  for(unsigned shift=0;shift<24;shift+=8) {
    const auto unpack=[&](std::uint32_t color) {
      return float(double((color>>shift)&255u)*double(1.f/255.f));
    };
    const float b=unpack(color_b);
    const float channel=multiplied?unpack(color_a)*b:b*.5f;
    const auto packed=std::uint32_t(std::nearbyint(double(channel)*255.))&255u;
    result|=packed<<shift;
  }
  return result;
}
} // namespace halo2
