#pragma once
#include "halo2/texture.hpp"
#include <array>
#include <string_view>
#include <vector>

namespace halo2 {
// Exact TU1.5 renderer's DVDmatg rasterizer references+1C/+24/+2C/+3C/+44.
// Owns all source bitmap pages/mips; it supplies no loading clock, progress,
// shader, completion signal, synthesized mask or replacement loading audio.
struct OriginalLoadingBitmap {
  std::string_view name;
  std::uint32_t source_datum{};
  std::vector<Texture> frames;
};
const std::array<OriginalLoadingBitmap,5>& original_loading_bitmaps();
} // namespace halo2
