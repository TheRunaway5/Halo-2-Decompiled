#pragma once
#include "halo2/path_tracing/scene.hpp"

namespace halo2::path_tracing {
// Optional proposal metadata follows the existing emissive triangle index
// list. It changes sampling directions only, never authored emission energy.
inline constexpr std::uint32_t emitter_group_magic=0x45525447u;
inline constexpr std::uint32_t emitter_group_header_words=4u,emitter_group_words=12u;
// Header: magic,count,total luminance power(floatBits),reserved.
// Group: material,triangleCount,reserved,area(floatBits), center.xyz,radius,
// area-weighted approximate emission RGB,cumulative luminance power.
void append_emitter_groups(Scene&);
}
