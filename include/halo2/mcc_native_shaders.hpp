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
}
