#pragma once
#include "halo2/path_tracing/scene.hpp"

namespace halo2::path_tracing::material_atlas {
inline constexpr std::uint32_t maximum_words=16'777'216;
using ImageKey=std::array<std::uint64_t,2>;
// Albedo, emission and normal-map records move with a root payload. Cubes
// are shared independently and rebased separately by the atlas owner.
void relocate(GpuMaterial&,std::uint32_t destination);
// Called before publishing owner rows, with the complete current root set.
// The caller may be replacing one root, or preparing a sorted live image set.
bool compact(Scene&,std::size_t required_words,std::uint32_t replaced=~0u,
             std::span<const ImageKey> live_images={},const ImageKey* pending_cube=nullptr);
}
