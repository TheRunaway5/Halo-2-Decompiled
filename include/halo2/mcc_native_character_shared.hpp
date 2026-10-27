#pragma once
#include "halo2/mcc_native_shaders.hpp"

namespace halo2 {
// Two complete literal shared Chief/emblem data setups proved against the
// original Coagulation owners. Preserves source images, input IDs and curves;
// emits only owned metadata for the separately authenticated native programs.
std::optional<MccOwnedShaderProjection> project_mcc_character_shared(
    const Tag&,const Tag& source_stem,const Tag& native_stem,
    std::string_view payload_name,std::span<const std::byte> payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&,
    const std::function<std::string(std::uint32_t)>& string_id);
}
