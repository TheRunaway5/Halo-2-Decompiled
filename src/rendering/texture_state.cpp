#include "halo2/cache.hpp"
#include <algorithm>

namespace halo2 {
ShaderTextureState decode_texture_state(std::span<const std::byte> bytes) {
  if(bytes.size()<24)throw FormatError("truncated texture state");
  auto byte = [&](unsigned at) { return std::to_integer<unsigned>(bytes[at]); };
  auto word = [&](unsigned at) { return byte(at) | byte(at+1)<<8 |
      byte(at+2)<<16 | byte(at+3)<<24; };
  // Exact state extraction in retail 000189A0, including MAG from byte 1
  // rather than the unused low nibble of byte 0.
  return {byte(4), byte(5), byte(6), byte(1)&15, byte(0)>>4,
          byte(1)>>4, word(20), byte(3)>>4, byte(3)&15,
          byte(2)>>4, word(16), byte(2)&15, word(8), word(12)};
}
} // namespace halo2

extern "C" int halo2_decode_texture_state(const std::uint8_t *input,
                                          std::uint32_t *output) {
  if (!input || !output) return 0;
  const auto state=halo2::decode_texture_state(
      {reinterpret_cast<const std::byte *>(input),24});
  const std::uint32_t values[]{state.address_u,state.address_v,state.address_w,
    state.mag_filter,state.min_filter,state.mip_filter,state.lod_bias,
    state.max_mip,state.anisotropy,state.color_key_op,state.color_sign,
    state.alpha_kill,state.border_color,state.key_color};
  std::copy(std::begin(values),std::end(values),output);
  return 1;
}
