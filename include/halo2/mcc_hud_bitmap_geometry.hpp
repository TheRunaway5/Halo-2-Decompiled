#pragma once
#include <array>
#include <optional>

namespace halo2 {
struct UiImage;
// A recognized MCC bank keeps its physical image/UV data. Only the authored
// logical size used by the original HUD geometry kernel is projected. Complete
// frame/sequence tuples guard vanilla banks; other authoring is left unchanged.
std::optional<std::array<unsigned, 2>>
mcc_hud_bitmap_logical_extent(const UiImage &, unsigned frame);
// Keep the original shield quads while sampling the recognized MCC fill at
// its authored placement relative to the outline. Other pages are unchanged.
std::optional<std::array<float, 2>>
mcc_hud_shield_fill_sample_offset(const UiImage &, unsigned frame);
} // namespace halo2
