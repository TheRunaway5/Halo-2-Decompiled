#pragma once
#include <cstdint>
#include <functional>
#include <optional>

namespace halo2 {
// Retained fields of the actual weapon object; heat (+180), spent (+184)
// and the cooling inhibitor (+188) have separate owners.
struct WeaponEnergyState {
  std::uint32_t definition{},item_flags{},weapon_flags{};
  float heat{},spent{},cooling_inhibitor{};
  std::uint32_t overheat_handle=0xffffffffu;
  std::int16_t blocking_ticks{};
};
struct WeaponHeatDefinition {
  float cooled_threshold{},maximum{},loss{},overheated_loss{},spent_loss_scale{};
  std::int16_t weapon_type{};
  std::uint32_t overheat_effect=0xffffffffu;
};
struct WeaponHeatServices {
  bool has_unit{};
  std::function<void()> reset_zoom;
  std::function<void(unsigned)> action; // Actual weapon action 14/15.
  std::function<std::int16_t()> vent_timing_ticks; // Actual16B44A(08000071,1), event4 selector.
  std::function<std::uint32_t(std::uint32_t)> create_overheat;
  std::function<void(std::uint32_t,bool)> stop_overheat; // true: holstered destroy.
};
// TU1.5 103436..10367E, the complete heat/effect/blocking phase of1030F0.
// Services retain their genuine owning callers; no clip duration is invented.
void weapon_heat_step(WeaponEnergyState &,const WeaponHeatDefinition &,
                      float tick_seconds,float one,const WeaponHeatServices &);
struct WeaponAgeDefinition {
  std::uint32_t exhausted_effect=0xffffffffu,aged_definition=0xffffffffu;
};
// Complete numeric/store contract of TU1.5 106E80. Infinite ammunition is
// the actual owning player's15C7F0 result; absent player means false.
bool weapon_age(WeaponEnergyState &,float amount,float one,bool infinite_ammo,
                const WeaponAgeDefinition &,
                const std::function<void(std::uint32_t)> &exhausted_effect);
// Actual16B44A section1: first event4 when it precedes the end, otherwise
// the full clip. The original caller uses zero when no clip resolves.
std::int16_t weapon_vent_timing(std::int16_t frames,
    std::optional<std::int16_t> event4,std::int16_t tick_rate,float animation_seconds);
// Complete16A06A/16B44A consumer after actual clip resolution. Section0 is
// the clip duration; section1 uses event4, section2/3 use the first event0.
std::int16_t weapon_animation_timing(std::int16_t frames,
    std::optional<std::int16_t> event4,std::optional<std::int16_t> event0,
    std::int16_t section,std::int16_t tick_rate,float animation_seconds);
struct FirstPersonBindingDefinition;
//169F3B/16B31B selects the graph by representation and dual mode before
//16A06A/16B44A consumes its literal activity/event record. A render model
//and a registered local view are not timing admission requirements.
std::int16_t weapon_animation_activity_timing(const FirstPersonBindingDefinition &,
    bool dual,std::uint32_t activity,std::int16_t section,
    std::int16_t tick_rate,float animation_seconds);
bool weapon_battery_misfire(float spent,float threshold,float chance,float one,
                           float random_scale,std::uint32_t &seed);
bool weapon_heat_detonation(float heat,float threshold,float chance,
                           float random_scale,std::uint32_t &seed);
} // namespace halo2
