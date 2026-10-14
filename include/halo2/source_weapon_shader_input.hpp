#pragma once
#include "halo2/shader_animation.hpp"

namespace halo2 {
// Actual bound weapon fields consumed by retail FF5F0 -> 10CF50. They are
// runtime fields, not an authored default or a material-name classification.
struct SourceWeaponShaderState {
    std::uint32_t item_flags{};   // object +12C
    std::uint32_t weapon_flags{}; // object +16C
    float power_fraction{};      // object +184
    float heat{};                // object +180, independent of spent battery
};
// Narrow original1040F0/1021D0 ready countdown. Item attach/holster/drop
// consumers retain their literal low-byte masks; full inventory is separate.
std::int16_t source_weapon_ready_begin(SourceWeaponShaderState &,float seconds,std::int16_t rate);
void source_weapon_ready_tick(SourceWeaponShaderState &,std::int16_t &countdown);
FunctionValue source_weapon_turned_on(const SourceWeaponShaderState &);
// Recovered turned_on, heat and age selectors. Other weapon inputs retain
// their own source producers, including charge and magazine digits.
std::optional<float> source_weapon_shader_input(std::uint32_t name,
                                               const SourceWeaponShaderState &);
} // namespace halo2
