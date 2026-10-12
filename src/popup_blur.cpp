#include "halo2/popup_blur.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
PopupBlur frontend_popup_blur(float fade) {
  PopupBlur out;
  auto &p = out.program.words;
  p[0] = 0x18201920; p[1] = 0x1a201b20; p[2] = 0x1c201d20;
  p[8] = 0x040c0d00; p[9] = 0x1c00;
  p[11] = p[14] = p[17] = 0x00404040;
  p[26] = 0x30c00; p[27] = 0x30d00; p[28] = 0x30c00;
  p[34] = 0x08200940; p[35] = 0xcc012020; p[36] = 0x0c082c09;
  p[37] = 0x0a200b40; p[38] = 0xcd012020; p[39] = 0x0d0a2d0b;
  p[40] = 0x0c200d40; p[41] = 0xc4012020;
  p[45] = 0x20c00; p[46] = 0x30c00; p[47] = 0xc00;
  p[48] = 0x20d00; p[49] = 0x30d00; p[50] = 0xd00;
  p[51] = 0x20400; p[52] = 0x30400; p[53] = 0x11008; p[54] = 0x8421;
  const float amount = std::clamp(fade, 0.f, 1.f) * 3.75f;
  float radius = 1.f;
  for (unsigned i = 0; i < out.radii.size(); ++i) {
    const float fraction = std::clamp(amount - float(i), 0.f, 1.f);
    if (fraction > 0.f) out.radii[out.passes++] = fraction * radius;
    radius *= 2.f;
  }
  return out;
}

std::uint32_t frontend_popup_tint(float fade, const ShaderVector &tint) {
  fade = std::clamp(fade, 0.f, 1.f);
  const auto component = [](float value) {
    return std::uint32_t(std::nearbyint(value * 255.f));
  };
  return component(tint[3] * fade) << 24 | component(tint[0]) << 16 |
         component(tint[1]) << 8 | component(tint[2]);
}
} // namespace halo2

extern "C" unsigned halo2_frontend_popup_blur(float fade, std::uint32_t *words,
                                               float *radii) {
  const auto result = halo2::frontend_popup_blur(fade);
  std::copy(result.program.words.begin(), result.program.words.end(), words);
  std::copy(result.radii.begin(), result.radii.end(), radii);
  return result.passes;
}

extern "C" std::uint32_t halo2_frontend_popup_tint(float fade, const float *rgba) {
  return halo2::frontend_popup_tint(fade, {rgba[0], rgba[1], rgba[2], rgba[3]});
}
