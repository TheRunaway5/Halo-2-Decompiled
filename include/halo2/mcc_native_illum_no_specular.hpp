#pragma once
#include "halo2/mcc_native_shaders.hpp"

namespace halo2 {
// Finite original Turf layout/clock-curve admission for the two District signs.
// Authored image/color/vector values remain source-owned; shader programs and
// the animation evaluator are unchanged.
std::optional<MccOwnedShaderProjection> project_mcc_clock_illum_no_specular(
    const Tag& shader,const Tag& source_mcc_stem,const Tag& native_stem,
    std::string_view native_payload_name,std::span<const std::byte> native_payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&);
}
