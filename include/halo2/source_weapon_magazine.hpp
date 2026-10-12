#pragma once
#include "halo2/shader_animation.hpp"
#include <cstdint>
#include <span>

namespace halo2 {
// Literal weap magazine+6/+A/+C, and the signed shorts written by FDAD0
// at object+22C/+22A (16-byte stride). Firing/reloading remains a caller service.
struct SourceWeaponMagazineDefinition {
    std::int16_t initial_rounds{},maximum_loaded{},maximum_reserve{};
};
struct SourceWeaponMagazineState {
    std::int16_t loaded{},reserve{};
};
SourceWeaponMagazineState source_weapon_magazine_initial_state(const SourceWeaponMagazineDefinition &);
// FF5F0 selector1200056A, magazine0. Missing magazines and capacity0 return0.
FunctionValue source_weapon_primary_ammunition(std::span<const SourceWeaponMagazineDefinition>,
                                               std::span<const SourceWeaponMagazineState>);
// Complete BAB40/BA8C0 slice for the supplied carbine's literal identity
// function. Other requested authored functions retain an explicit boundary.
std::optional<FunctionValue> source_weapon_magazine_shader_input(std::uint32_t name,
    std::span<const SourceWeaponMagazineDefinition>,std::span<const SourceWeaponMagazineState>,
    std::span<const ObjectFunction>,const ShaderFunctionTables &);
} // namespace halo2
