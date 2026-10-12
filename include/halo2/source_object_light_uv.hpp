#pragma once
#include "halo2/source_object_light_streams.hpp"
namespace halo2 {
//Complete D2DC0. Uses the second descriptor (zero-based1, geometry+3C+20),
//including its+8 byte offset, and the
//literal geometry index triple. Declaration24 is float2; all other source
//declarations take the signed SHORT midpoint branch. Output clamps to the
//original[-1000,1000] bounds. No GPU-normalized or expanded UV substitute.
Vec2 source_object_light_uv(const SourceObjectLightDataGeometry&,
    std::int32_t lightmap_triangle,float weight_b,float weight_c);
}
