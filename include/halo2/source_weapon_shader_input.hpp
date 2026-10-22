#pragma once
#include "halo2/shader_animation.hpp"
#include <array>

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
// Whole1046C0 ->107140 primary-trigger selector over the actual published
// signed timer/mode and authored charging interval. No byte quantization.
FunctionValue source_weapon_primary_charge(std::uint8_t mode,std::int16_t ticks,
    float charging_seconds,float tick_seconds);
// Complete1046C0 primary/secondary_firing selector inputs. Counts come from
// this weapon's authored +2D0 block; values are its actual +1B4/+1E8 words.
// The source clock and shared weapon +248 last-shot tick retain 32-bit wrap.
struct SourceWeaponFiringMaterialState {
    std::int32_t barrel_count{};
    std::uint32_t current_tick{},last_fired_tick{};
    std::array<float,2> values{};
};
FunctionValue source_weapon_firing_material(const SourceWeaponFiringMaterialState &,unsigned barrel);
// Recovered turned_on, heat and age selectors. Other weapon inputs retain
// their own source producers, including charge and magazine digits.
std::optional<float> source_weapon_shader_input(std::uint32_t name,
                                               const SourceWeaponShaderState &);
} // namespace halo2
