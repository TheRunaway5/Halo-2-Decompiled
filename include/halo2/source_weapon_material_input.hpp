#pragma once
#include "halo2/source_weapon_shader_input.hpp"
#include "halo2/source_weapon_magazine.hpp"
namespace halo2 {
// Resolve authored object functions from actual weapon producers. The handler
// path preserves inactive zero, enable gates, flags and curves; it is distinct
// from an interpolated function override. Unbound producers remain unresolved.
std::optional<FunctionValue> source_weapon_material_input(std::uint32_t name,
    const SourceWeaponShaderState &,std::span<const SourceWeaponMagazineDefinition>,
    std::span<const SourceWeaponMagazineState>,std::span<const ObjectFunction>,
    const ShaderFunctionTables &,float seconds,std::optional<FunctionValue> primary_trigger = {});
} // namespace halo2
