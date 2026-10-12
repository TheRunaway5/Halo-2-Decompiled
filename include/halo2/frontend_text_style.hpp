#pragma once
#include "halo2/frontend.hpp"

namespace halo2 {
// Original 13EC70/13EB60 and 13F470 text state, before glyph submission.
UiColor frontend_text_color(UiColor);
std::uint32_t frontend_text_pixel(UiColor);
UiColor frontend_text_pixel_color(std::uint32_t);
bool frontend_text_texture_color(std::uint32_t codepoint);
// Definition flags are interpreted by different original setup callers.
bool frontend_text_shadow(unsigned definition_flags, bool button);
struct FrontendTextSetup {
  int font{};
  float scale{};
  bool shadow{};
};
// Original253765 uses font1 and actual wigl+4C for standard flag8 text.
// Original253CFF button setup retains the authored font and unit scale.
FrontendTextSetup frontend_text_setup(unsigned definition_flags,
                                      int authored_font, bool button,
                                      float shared_small_text_scale);
} // namespace halo2
