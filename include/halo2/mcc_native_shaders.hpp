#pragma once
#include "halo2/tags/types.hpp"
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace halo2 {
struct Texture;
struct MccShaderDefaultDetail {
    const Tag* tag{};
    const Texture* frame0{};
};
// All pointer words in this owned payload are relative to its beginning.
// Publication must relocate every listed field and retain this exact map.
struct MccOwnedShaderProjection {
    std::vector<std::byte> bytes;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> pointers;
};
using MccShaderRead = std::function<std::span<const std::byte>(std::uint32_t,std::size_t)>;
using MccShaderFind = std::function<const Tag*(std::uint32_t)>;
using MccShaderBitmapFrame = std::function<bool(std::uint32_t,std::uint32_t)>;

// Original default_vector_cube_map frame0 equals the authenticated MCC
// rasterizer default_cube_map frame3. Supply an owned single-frame metadata
// view only for that missing native-library dependency, retaining the actual
// MCC resource references. Changed metadata, pixels or mip owners are rejected.
std::optional<MccOwnedShaderProjection> project_mcc_native_default_vector_cube(
    const Tag& missing,const Tag& donor,const Texture& frame3,const MccShaderRead&);

// A bounded data conversion for the authenticated, unanimated tex_bump
// contract. Unsupported or changed authoring returns no projection.
std::optional<MccOwnedShaderProjection> project_mcc_unanimated_tex_bump(
    const Tag& shader,const Tag& native_stem,std::string_view native_payload_name,
    std::span<const std::byte> native_payload,const MccShaderRead&,
    const MccShaderFind&,const MccShaderBitmapFrame&);
std::optional<MccOwnedShaderProjection> project_mcc_unanimated_tex_bump(
    const Tag& shader,const Tag& native_stem,std::string_view native_payload_name,
    std::span<const std::byte> native_payload,const MccShaderRead&,
    const MccShaderFind&,const MccShaderBitmapFrame&,const MccShaderDefaultDetail&);
// Exact paired original/MCC transparent glass cooked tuple. The complete
// authored data and retained native template are authenticated; unknown or
// modified tuples stay unresolved. No material-name/suffix aliasing.
std::optional<MccOwnedShaderProjection> project_mcc_transparent_two_alpha_env_illum(
    const Tag& shader,const Tag& native_stem,std::string_view native_payload_name,
    std::span<const std::byte> native_payload,const MccShaderRead&,
    const MccShaderFind&,const MccShaderBitmapFrame&);
}
