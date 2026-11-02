#pragma once
#include "halo2/mcc_native_shaders.hpp"

namespace halo2 {
// Finite source-authenticated data admission for the three clock-only world
// profiles and independently paired sky layouts. Curves, periods and native
// destination roles remain literal; sky environment pixel slots follow the
// paired compiler ordering. Shader programs and the evaluator are unchanged.
std::optional<MccOwnedShaderProjection> project_mcc_animated_clock_world(
    const Tag& shader,const Tag& source_mcc_stem,const Tag& native_stem,
    std::string_view native_payload_name,std::span<const std::byte> native_payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&);
}
