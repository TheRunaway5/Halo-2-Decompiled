#include "halo2/inventory.hpp"
#include <algorithm>
#include <bit>
namespace halo2 {
bool netgame_weapon_timer(NetgameWeaponState &state, unsigned row,
    std::int32_t tick, int rate, bool eligible, bool playback, Vec3 authored,
    Vec3 position, std::uint32_t flags, int delay, int option) {
  if (rate <= 0 || row >= 100)
    throw FormatError("Netgame weapon timer outside source row/rate bounds");
  if (playback || !eligible || int(row) % rate != tick % rate) return false;
  if (!state.timer) {
    bool start = state.object == 0xffffffffu || (flags & 3);
    if (!start) {
      const float dz = position.z - authored.z;
      const float dy = position.y - authored.y;
      const float dx = position.x - authored.x;
      const float zz = dz * dz, yy = dy * dy, xx = dx * dx;
      const float zy = zz + yy;
      start = zy + xx > 1.f;
    }
    if (start) {
      if (option == 1) delay = -1;
      else if (option == 2) delay >>= 1;
      else if (option == 3)
        delay = std::bit_cast<std::int32_t>(std::uint32_t(delay) << 1);
      state.timer = std::bit_cast<std::int16_t>(std::uint16_t(delay));
    }
  }
  if (state.timer <= 0) return false;
  --state.timer;
  return !state.timer;
}
void netgame_weapon_spawn_result(NetgameWeaponState &state, std::uint32_t object) {
  state = {object, 0, 0};
}
std::array<std::uint8_t, 2> starting_grenades(std::uint32_t flags,
    bool enabled, bool maximum_one, int weapon_set) {
  const int maximum = maximum_one ? 1 : 2;
  int frag = maximum, plasma = 0;
  if (flags & 1) frag = plasma = 0;
  if (flags & 2) { plasma += frag; frag = 0; }
  if (!enabled) frag = plasma = 0;
  if (weapon_set == 15 || weapon_set == 17) { plasma += frag; frag = 0; }
  if (weapon_set == 16) { frag += plasma; plasma = 0; }
  return {std::uint8_t(std::min(frag, maximum)),
          std::uint8_t(std::min(plasma, maximum))};
}
std::int8_t grenade_ready_type(std::int8_t requested,
    const std::array<std::uint8_t, 2> &counts) {
  const int first = requested == -1 ? 0 : requested;
  if (first < 0 || first > 1) return -1;
  if (counts[first]) return std::int8_t(first);
  const int other = first == 0 ? 1 : 0;
  return counts[other] ? std::int8_t(other) : std::int8_t(-1);
}
}
extern "C" void halo2_starting_grenades(std::uint32_t flags, int enabled,
    int maximum_one, int weapon_set, std::uint8_t *counts) {
  const auto result = halo2::starting_grenades(flags, enabled, maximum_one, weapon_set);
  counts[0] = result[0]; counts[1] = result[1];
}
extern "C" int halo2_netgame_weapon_timer(halo2::NetgameWeaponState *state,
    unsigned row, std::int32_t tick, int rate, int eligible, int playback,
    const halo2::Vec3 *authored, const halo2::Vec3 *position,
    std::uint32_t flags, int delay, int option) {
  return halo2::netgame_weapon_timer(*state, row, tick, rate, eligible, playback,
      *authored, *position, flags, delay, option);
}
extern "C" void halo2_netgame_weapon_spawn_result(halo2::NetgameWeaponState *state,
    std::uint32_t object) {
  halo2::netgame_weapon_spawn_result(*state, object);
}
