#include "halo2/texture.hpp"
#include <algorithm>
namespace halo2 {
Texture prepare_cubemap(const Bytes &raw, std::uint32_t width,
                        std::uint16_t format, bool swizzled,
                        unsigned additional_mips) {
  // Xbox faces have separate mip chains, padded to 128-byte boundaries.
  // Cache resources include the tail/padding even when retail mip selection
  // stops at 4x4 for block-compressed formats.
  if (raw.size() % 6 || (raw.size() / 6) % 128)
    throw FormatError("invalid Xbox cubemap face stride");
  const auto stride = raw.size() / 6;
  Texture result{width, width, format, {}, {}, 6};
  for (unsigned face = 0; face < 6; ++face) {
    auto texture =
        prepare_texture(Bytes(raw.slice(face * stride, stride)), width, width,
                        format, swizzled, additional_mips);
    result.format = texture.format;
    for (auto mip : texture.mips) {
      mip.offset += result.pixels.size();
      result.mips.push_back(mip);
    }
    result.pixels.insert(result.pixels.end(), texture.pixels.begin(),
                         texture.pixels.end());
  }
  return result;
}
} // namespace halo2
