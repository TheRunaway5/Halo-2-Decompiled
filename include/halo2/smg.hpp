#pragma once
#include "halo2/weapon_error.hpp"
#include <cstddef>
#include <optional>
#include <functional>

namespace halo2 {
// Bounded single-SMG path recovered from update 1.1. Asset values are supplied
// by Cache; this interface does not generalize the trigger to other weapons.
struct SmgDefinition {
  WeaponErrorDefinition error;
  float rate{}, rate_rise{}, rate_fall{}, effect_decay{};
  bool preparation_effect{};
  std::int16_t dispatch_interval{}, capacity{}, reload_amount{};
  std::int16_t full_reload_ticks{}, empty_reload_ticks{};
  std::int16_t ready_ticks{};
  std::int16_t single_own_frames{};
};
struct SmgBarrelState {
  std::int8_t counter = 127;
  std::uint8_t mode{};
  std::int16_t ticks{};
  std::uint16_t flags{}, shot_count{}, effect_mask{};
  std::int16_t effect_index{}, effect_remaining{}, dispatch_counter{};
  float rate{}, effect_a{}, effect_b{}, error{}, angle_change{};
  float subshot{}, recovery{};
  std::int16_t subshot_count{};
  std::uint8_t marker_count{}, reserved{};
  std::uint32_t effect_handle = 0xffffffffu;
};
static_assert(sizeof(SmgBarrelState) == 0x34);
static_assert(offsetof(SmgBarrelState, error) == 0x1c);
static_assert(offsetof(SmgBarrelState, subshot) == 0x24);
static_assert(offsetof(SmgBarrelState, marker_count) == 0x2e);
struct SmgState {
  SmgBarrelState barrel;
  std::int16_t loaded{}, reserve{}, magazine_mode{}, magazine_ticks{};
  std::int16_t transfer_ticks{}, blocking_ticks{};
  std::uint16_t weapon_flags{}, input_flags{};
  std::uint32_t trigger_flags{};
  std::int16_t trigger_ticks{};
  std::uint8_t trigger_state{}, hand_timers[2]{};
  std::uint32_t weapon_state{};
  bool held{}, reload_empty{}, marker{};
  std::uint8_t second_trigger_state{}, own_flags{};
  std::uint16_t own_events{};
  std::int16_t magazine_other_ticks{};
  std::uint32_t trigger_effect_handle = 0xffffffffu;
  bool own_manager{}, attached = true;
  bool hand_timer_registered = true; // Caller-supplied167D32/EFCC0 view/player/current-unit boundary.
};
// Actual CE510 instant initial equip after 101BB0 constructor. Unit hand
// timers are caller-owned and are not cleared by this weapon initializer.
void smg_initial_equip(SmgState &, const SmgDefinition &);
bool smg_reload_admitted(const SmgState &);
// Unit timer decay and 1045E0 input, before the child-weapon update.
void smg_input(SmgState &, bool held);
// 105640 command, before update for an explicit reload, inside update for the
// original empty-magazine action. Returns true only when a reload starts.
using SmgActivityCallback=std::function<void(std::uint32_t)>;
// Original107160->107B50 at preparation, after its source stores and before
// projectile postpass. Scalar words retain107B50's stack order: rate then
// zero for the admitted stock normal path; one then zero for its empty path.
// The callback owns real effect creation/refresh and returns its actual datum.
// With no callback the unavailable effect owner leaves its handle unchanged.
using SmgEffectCallback=std::function<std::uint32_t(const SmgState &,
    unsigned selection,std::uint32_t a_bits,std::uint32_t b_bits)>;
struct SmgRuntimeServices {
  float opposite_hand_blend{};
  bool shared_hand_feedback{};
  std::function<std::int32_t()> available;
  std::function<void(std::int32_t)> spend;
  std::function<std::int16_t(std::uint32_t,unsigned)> timing;
  std::function<bool(SmgBarrelState &)> fire_gate;
  std::function<void(bool chamber)> magazine_effect;
};
// The absent-service defaults retain the explicitly bounded single-SMG
// fixture. The unit adapter supplies its actual shared ammo and dual owners.
bool smg_reload(SmgState &, const SmgDefinition &,const SmgActivityCallback & = {},
    const SmgRuntimeServices & = {});
// 1021D0's bounded magazine / Spew / barrel update. Return: auto reload began.
bool smg_update(SmgState &, const SmgDefinition &, float dt,
    const SmgActivityCallback & = {},const SmgEffectCallback & = {},
    const SmgRuntimeServices & = {});
// 1019F0 -> 101A60 -> 108300. Consume each prepared request exactly once.
// Successful projectile creation and additional world RNG remain external.
std::optional<Vec3> smg_dispatch(SmgState &, const SmgDefinition &, Vec3 aim,
                               std::uint32_t &shared_seed,
                               std::span<const Vec3, 1026> directions,float opposite_hand_blend = 0);
// 108856 projection / 10921B descriptor / FC55F constructor. Ordinary
// owner with no supporting platform and zero barrel speed override only.
Vec3 smg_inherited_velocity(Vec3 original_aim, Vec3 final_forward,
                            Vec3 owner_velocity);
Vec3 smg_projectile_velocity(Vec3 final_forward, Vec3 inherited_velocity,
                             float initial_speed, float speed_scale = 1.f);
} // namespace halo2
