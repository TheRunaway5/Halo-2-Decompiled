#pragma once
#include "halo2/source_weapon_shader_input.hpp"
#include "halo2/source_weapon_magazine.hpp"
#include <array>
namespace halo2 {
class Cache;
// Authored IDs stay in their owning cache namespace, including function
// aliases. Resolve only named builtins to their original producer identities.
class SourceWeaponMaterialNames {
public:
    SourceWeaponMaterialNames()=default;
    static SourceWeaponMaterialNames from_cache(const Cache &);
    std::optional<std::uint32_t> builtin(std::uint32_t authored) const;
private:
    static constexpr std::array<std::uint32_t,13> original_{
        0x030005a6u,0x040005a7u,0x09000568u,0x04000567u,0x03000577u,
        0x1200056au,0x1700056bu,0x1700056cu,0x0f000574u,0x0f0005aau,0x0500055bu,
        0x0e000572u,0x10000573u};
    std::array<std::uint32_t,13> authored_=original_;
};
// Resolve authored object functions from actual weapon producers. The handler
// path preserves inactive zero, enable gates, flags and curves; it is distinct
// from an interpolated function override. Unbound producers remain unresolved.
// Alive requires both the full object identity and its actual +10A damage flags.
// Callers must read flags from that owner; parent/player life is independent.
std::optional<FunctionValue> source_weapon_material_input(std::uint32_t name,
    const SourceWeaponShaderState &,std::span<const SourceWeaponMagazineDefinition>,
    std::span<const SourceWeaponMagazineState>,std::span<const ObjectFunction>,
    const ShaderFunctionTables &,float seconds,std::optional<FunctionValue> primary_trigger = {},
    const SourceWeaponMaterialNames &names = {},std::optional<std::uint32_t> object = {},
    std::optional<std::uint16_t> object_damage_flags = {},
    std::optional<SourceWeaponFiringMaterialState> firing = {});
} // namespace halo2
