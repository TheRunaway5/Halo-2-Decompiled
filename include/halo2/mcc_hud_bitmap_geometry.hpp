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
} // namespace halo2
