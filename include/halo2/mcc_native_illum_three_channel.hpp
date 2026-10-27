#pragma once
#include "halo2/mcc_native_shaders.hpp"

namespace halo2 {
// Exact nine-pair shared light setup. No arbitrary family parameter mapping,
// new animation evaluator or GPU program interpretation is introduced.
std::optional<MccOwnedShaderProjection> project_mcc_clock_illum_three_channel(
    const Tag& shader,const Tag& source_mcc_stem,const Tag& native_stem,
    std::string_view native_payload_name,std::span<const std::byte> native_payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&);
}
