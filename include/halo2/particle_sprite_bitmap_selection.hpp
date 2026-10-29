#pragma once
#include "halo2/cache.hpp"
#include <functional>
#include <span>

namespace halo2 {
// The compiled first bitmap selector supplies the drawable sprite's bitmap
// even when the imported authoring bank omitted a default-valued parameter.
// A salted handle still needs its actual resident bitmap owner.
std::uint32_t particle_sprite_default_bitmap(std::span<const std::byte> selectors,
    std::span<const std::byte> bitmaps,const std::function<bool(std::uint32_t)>& resident_bitmap);
}
