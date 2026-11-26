#pragma once
#include "halo2/path_tracing/scene.hpp"

namespace halo2::path_tracing {
// Sampling metadata follows the emissive triangle indices and cumulative power.
inline constexpr std::uint32_t emitter_group_magic=0x45525447u;
inline constexpr std::uint32_t emitter_group_header_words=4u,emitter_group_words=12u;
inline constexpr std::uint32_t emitter_bounds_limit=8u,emitter_bounds_words=6u;
// Header: magic,groupCount,total luminance power(floatBits),unbakedEmitterCount.
// Group: material,triangleCount,reserved,area(floatBits), center.xyz,radius,
// area-weighted approximate emission RGB,cumulative luminance power.
// After the groups: up to eight bounds for the live triangle prefix, as
// minimum.xyz and maximum.xyz in WORLD coordinates.
void append_emitter_groups(Scene&, std::uint32_t unbaked_emitter_count);
}
