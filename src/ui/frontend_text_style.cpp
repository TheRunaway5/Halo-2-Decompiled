#include "halo2/frontend_text_style.hpp"
#include <algorithm>

namespace halo2 {
UiColor frontend_text_color(UiColor color) {
  constexpr float maximum = .68f;
  if ((color.red > maximum) & (color.green > maximum) &
      (color.blue > maximum)) {
    float minimum = color.green > color.blue ? color.blue : color.green;
    minimum = color.red > minimum ? minimum : color.red;
    if (maximum > minimum)
      minimum = maximum;
    else if (minimum > 1.f)
      minimum = 1.f;
    const float scale = maximum / minimum;
    color.red *= scale;
    color.green *= scale;
    color.blue *= scale;
  }
  color.alpha = std::clamp(color.alpha, 0.f, 1.f);
  color.red = std::clamp(color.red, 0.f, 1.f);
  color.green = std::clamp(color.green, 0.f, 1.f);
  color.blue = std::clamp(color.blue, 0.f, 1.f);
  return color;
}
std::uint32_t frontend_text_pixel(UiColor color) {
  std::uint32_t pixel = std::uint32_t(color.alpha * 255.f);
  pixel = (pixel << 8) | std::uint32_t(color.red * 255.f);
  pixel = (pixel << 8) | std::uint32_t(color.green * 255.f);
  return (pixel << 8) | std::uint32_t(color.blue * 255.f);
}
UiColor frontend_text_pixel_color(std::uint32_t pixel) {
  return {float(pixel >> 24) / 255.f, float((pixel >> 16) & 255) / 255.f,
          float((pixel >> 8) & 255) / 255.f, float(pixel & 255) / 255.f};
}
bool frontend_text_texture_color(std::uint32_t codepoint) {
  if (codepoint < 0xe000 || codepoint > 0xe3ff ||
      (codepoint >= 0xe106 && codepoint <= 0xe131))
    return false;
  return codepoint != 0xe000 && codepoint != 0xe001 &&
         codepoint != 0xe002 && codepoint != 0xe004 && codepoint != 0xe008;
}
bool frontend_text_shadow(unsigned definition_flags, bool button) {
  // 253765 maps standard text flag8 to buffer flag4. 253CFF does not.
  return button || !(definition_flags & 8);
}
FrontendTextSetup frontend_text_setup(unsigned definition_flags,
                                      int authored_font, bool button,
                                      float shared_small_text_scale) {
  const bool small = !button && (definition_flags & 8);
  return {small ? 1 : authored_font, small ? shared_small_text_scale : 1.f,
          frontend_text_shadow(definition_flags, button)};
}
} // namespace halo2
