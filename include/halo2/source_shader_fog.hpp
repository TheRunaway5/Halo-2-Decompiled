#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
// Original 4B9F5C/80/84, camera forward at4B9DAC and pointer-backed
// default RGB at[4686D4]+4. The caller supplies the retained default;
// an unavailable fog interval does not establish a zero default.
struct SourceShaderFogContext {
    std::uint32_t datum;
    float near_distance,far_distance;
    std::array<float,3> camera_forward,default_color;
};
struct SourceShaderFogExterns {
    float scalar39;
    std::array<float,3> color41;
};
// Original3385D/33EF9: binary32 reciprocal, then product, then positive
// zero minus product. Requires finite inputs/results and nearest-even,
// gradual binary32 arithmetic. No view/fog/default producer is inferred.
SourceShaderFogExterns source_shader_fog_externs(const SourceShaderFogContext &);
}
