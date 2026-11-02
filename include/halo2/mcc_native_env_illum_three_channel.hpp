#pragma once
#include "halo2/mcc_native_shaders.hpp"

namespace halo2 {
// A zero-sized tag alias must have a resident payload at the exact same
// address and group. The Cache provider retains that ownership decision.
using MccShaderAliasOwner=std::function<bool(const Tag&,std::size_t)>;
// Complete paired authored-data conversion. Shader programs and native pass
// implementations are unchanged; source clock curves and main values survive.
std::optional<MccOwnedShaderProjection> project_mcc_env_illum_three_channel(
    const Tag& shader,const Tag& source_mcc_stem,const Tag& native_stem,
    std::string_view native_payload_name,std::span<const std::byte> native_payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&,
    const MccShaderAliasOwner& = {});
}
