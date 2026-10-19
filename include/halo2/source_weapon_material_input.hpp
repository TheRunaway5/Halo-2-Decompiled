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
    static constexpr std::array<std::uint32_t,9> original_{
        0x030005a6u,0x040005a7u,0x09000568u,0x04000567u,0x03000577u,
        0x1200056au,0x1700056bu,0x1700056cu,0x0f000574u};
    std::array<std::uint32_t,9> authored_=original_;
};
// Resolve authored object functions from actual weapon producers. The handler
// path preserves inactive zero, enable gates, flags and curves; it is distinct
// from an interpolated function override. Unbound producers remain unresolved.
std::optional<FunctionValue> source_weapon_material_input(std::uint32_t name,
    const SourceWeaponShaderState &,std::span<const SourceWeaponMagazineDefinition>,
    std::span<const SourceWeaponMagazineState>,std::span<const ObjectFunction>,
    const ShaderFunctionTables &,float seconds,std::optional<FunctionValue> primary_trigger = {},
    const SourceWeaponMaterialNames &names = {});
} // namespace halo2
