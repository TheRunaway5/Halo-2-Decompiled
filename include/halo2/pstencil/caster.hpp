#pragma once
#include "halo2/cache.hpp"

namespace halo2::pstencil {
// May2003 geometry_part_type_is_shadow_casting001CB000. Tool0044DDD0
// produces1=shadow-only,2=visible caster,3=visible noncaster,4=transparent.
// Current0/5 and unknown/generated geometry are not 2003 authored part types.
constexpr bool shadow_casting_part(std::uint16_t type){return type==1 || type==2;}
// Original A7E00 nearest-even float32(opacity*16), for an8-bit opacity
// expanded by3B808081. No byte sample lands on a half-integer boundary.
constexpr unsigned shadow_stipple_level(std::uint8_t opacity){return (unsigned(opacity)*16u+127u)/255u;}

// Expanded source geometry retains its literal part metadata through posing.
// Only geometry without that provenance uses the older shader-layer fallback.
inline bool shadow_casting_triangle(std::span<const Vertex,3> triangle,
    std::span<const std::uint8_t> fallback_materials) {
    const auto& first=triangle[0];
    for(const auto& vertex:triangle)
        if(vertex.source_part_type!=first.source_part_type || vertex.source_part_flags!=first.source_part_flags ||
           vertex.source_section!=first.source_section || vertex.source_part!=first.source_part || vertex.material!=first.material)
            throw FormatError("pStencil caster triangle mixes source part metadata");
    if(first.source_part_type!=0xffffu)return shadow_casting_part(first.source_part_type);
    if(first.material>=fallback_materials.size())throw FormatError("pStencil caster material escapes resident bank");
    return fallback_materials[first.material]!=0;
}
}
