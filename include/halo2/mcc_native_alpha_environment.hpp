#pragma once
#include "halo2/mcc_native_env_illum_three_channel.hpp"

namespace halo2 {
// Complete paired metadata conversion; original native programs stay unchanged.
// Static named UV lane conversion; authored animation needs its own mapping.
std::optional<MccOwnedShaderProjection> project_mcc_alpha_environment(
    const Tag& shader,const Tag& source_mcc_stem,const Tag& native_stem,
    std::string_view native_payload_name,std::span<const std::byte> native_payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&,
    const MccShaderAliasOwner& = {});
}
