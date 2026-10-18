#pragma once
#include "halo2/tags/types.hpp"
#include <array>
#include <cstddef>
#include <span>

namespace halo2 {
struct BitmapResource;
struct Texture;
struct MccLensBitmapLayout {
    TagHandle bitmap;
    unsigned frame{};
    std::array<std::byte,0x18> common;
};
// The caller owns the full typed bitmap directory/root/frame binding. This
// checks the complete MCC row against its actual decoded resource and image,
// retaining only the shared authored header. Original116-byte row holes and
// native aliases/header pointers/residency stamps are separate owners.
MccLensBitmapLayout project_mcc_lens_bitmap_layout(TagHandle bitmap,unsigned frame,
    std::span<const std::byte> row,const BitmapResource&,const Texture&);
}
