#pragma once
#include "halo2/mcc_native_shaders.hpp"
#include <string>
namespace halo2 {
struct MccResidentMeterOwners {
    MccOwnedShaderProjection pass,stem;
};
struct MccResidentWeaponPassContract {
    std::string stem,pass,vertex;
};
std::span<const MccResidentWeaponPassContract> mcc_resident_weapon_pass_contracts();
// Exact MCC compiled meter metadata selects an already compiled Vulkan
// kernel and already resident vertex classes. No executable bank is added.
std::optional<MccResidentMeterOwners> project_mcc_resident_meter_owners(
    const Tag& source_stem,const Tag& source_pass,const Tag& resident_vertex,
    const MccShaderRead&,const MccShaderFind&);
std::optional<MccOwnedShaderProjection> project_mcc_resident_meter_material(
    const Tag& shader,const Tag& source_stem,const Tag& native_stem,
    const MccShaderRead&,const MccShaderFind&,const MccShaderBitmapFrame&,
    const std::function<std::string(std::uint32_t)>& string_id);
}
