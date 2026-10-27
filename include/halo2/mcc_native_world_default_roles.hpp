#pragma once
#include "halo2/mcc_native_shaders.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
struct MccWorldDefaultBitmap {Tag tag;Texture texture;};
using MccShaderBitmapByName=std::function<std::optional<MccWorldDefaultBitmap>(std::string_view)>;
// The original template's additional default bitmap/UV roles are admitted only
// with their actual resident typed source owner and complete paired remap graph.
std::optional<MccOwnedShaderProjection> project_mcc_world_default_roles(
    const Tag& shader,const Tag& source_mcc_stem,const Tag& native_stem,
    std::string_view native_payload_name,std::span<const std::byte> native_payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&,
    const MccShaderBitmapByName&);
}
