#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <cstdint>
namespace halo2 {
// TU15FFD0 + TU1A2A52. The runtime maximum flag and variant fields are
// inputs; this initializer does not select or throw a grenade.
std::array<std::uint8_t, 2> starting_grenades(std::uint32_t equipment_flags,
    bool enabled, bool maximum_one, int weapon_set);
// Complete TU1.5 CE3A0 count scan. Direction0 preserves the selected type;
// signed nonzero direction cycles. NONE begins at frag; counts are signed bytes.
std::int8_t grenade_ready_type(std::int8_t requested,
    const std::array<std::uint8_t, 2> &counts,std::int16_t direction=0);
struct NetgameWeaponState {
  std::uint32_t object = 0xffffffffu;
  std::int16_t timer = 1, empty_counter{};
};
static_assert(sizeof(NetgameWeaponState) == 8);
// Bounded TU15C4B0 weapon branch: classification0, timer-start0, type2.
// Eligibility, definition/variant choice, allocation and settling are supplied.
// True requests allocation; the caller must unbind the old object's row first.
bool netgame_weapon_timer(NetgameWeaponState &, unsigned row, std::int32_t tick,
    int tick_rate, bool eligible, bool playback_mode, Vec3 authored_position,
    Vec3 object_position, std::uint32_t item_flags, int delay_seconds,
    int timer_option);
void netgame_weapon_spawn_result(NetgameWeaponState &, std::uint32_t object);
}
