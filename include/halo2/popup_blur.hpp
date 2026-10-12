#pragma once
#include "halo2/render_program.hpp"

namespace halo2 {
struct UiBackdrop {
  std::size_t before_vertex{};
  float fade{};
  ShaderVector tint{}; // Authored wigl RGBA, before original byte packing.
  bool widescreen_background{}; // Native side-margin tint/blur; centered popup unchanged.
};

struct PopupBlur {
  PixelProgram program;
  std::array<float, 4> radii{};
  unsigned passes{};
};

// Retail 235756 -> 211A0 -> 20F30. Apply to the completed lower UI
// channels, after their authored tint, and before submitting the popup.
PopupBlur frontend_popup_blur(float fade);

// Retail 234F8C packs the authored wigl tint using the current fade.
// tint is RGBA; the result is the original packed ARGB vertex color.
std::uint32_t frontend_popup_tint(float fade, const ShaderVector &tint);
} // namespace halo2
