#pragma once
#include "halo2/mcc_native_shaders.hpp"

namespace halo2 {
struct MccNoAlphaNativeOwners {
    MccOwnedShaderProjection stem,pass;
};
using MccShaderNamedFind=std::function<const Tag*(std::string_view,std::string_view)>;
using MccShaderString=std::function<std::string(std::uint32_t)>;
// Fully qualified PC authoring selects the existing TU shader programs.  Only
// its literal impl0 RGB write mask becomes the existing TU state67 data word.
// Returned owners retain the actual NoAlpha full handles/names on publication.
std::optional<MccNoAlphaNativeOwners> project_mcc_no_alpha_native_owners(
    const Tag& source_stem,const Tag& source_pass,
    const Tag& normal_stem,const Tag& normal_pass,
    std::span<const std::byte> normal_stem_payload,
    std::span<const std::byte> normal_pass_payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderNamedFind&);
std::optional<MccOwnedShaderProjection> project_mcc_no_alpha_material(
    const Tag& shader,const Tag& source_stem,const Tag& source_pass,
    const Tag& normal_stem,std::span<const std::byte> normal_stem_payload,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&,
    const MccShaderString&);
}
