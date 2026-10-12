#include "halo2/smg.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
void reset_barrel(SmgState &s) {
  s.barrel.counter = 0;
  s.barrel.mode = 0;
  s.barrel.shot_count = 0;
  s.barrel.flags &= ~1u;
}
void request_state(SmgState &s, std::uint32_t requested) {
  if (s.own_manager && (s.weapon_state == 0 ||
      (s.weapon_state > 0 && s.weapon_state < 3 && s.weapon_state <= requested)))
    s.weapon_state = requested;
}
}
void smg_initial_equip(SmgState &s, const SmgDefinition &d) {
  if (d.single_own_frames != 1)
    throw FormatError("Single-SMG own animation clock is outside the recovered path");
  s.own_manager = true; s.attached = true;
  s.own_flags = 2; s.own_events = 0;
  s.weapon_state = 9; s.blocking_ticks = 0;
  s.magazine_ticks = s.transfer_ticks = s.magazine_other_ticks = -1;
}
bool smg_reload_admitted(const SmgState &s) {
  return !s.trigger_state && !s.second_trigger_state && !s.weapon_state;
}
void smg_input(SmgState &s, bool held) {
  for (auto &timer : s.hand_timers)
    if (timer) --timer;
  s.input_flags = held ? std::uint16_t(2 | (!s.held ? 8 : 0)) : 0;
  s.held = held;
}
bool smg_reload(SmgState &s, const SmgDefinition &d,const SmgActivityCallback &activity) {
  if (!smg_reload_admitted(s) || (s.magazine_mode != 0 && s.magazine_mode != 5) || s.reserve <= 0 ||
      s.loaded >= d.capacity || (s.input_flags & 32)) return false;
  s.reload_empty = s.loaded == 0;
  s.magazine_mode = 1;
  s.magazine_ticks = s.transfer_ticks =
      s.reload_empty ? d.empty_reload_ticks : d.full_reload_ticks;
  s.magazine_other_ticks = s.magazine_ticks;
  request_state(s, 5);
  s.trigger_state = 0;
  s.trigger_ticks = 0;
  // 105640 tests the pre-reset mode when clearing the dry gate.
  if (s.barrel.mode != 1) s.barrel.flags &= ~64u;
  reset_barrel(s);
  if(activity)activity(s.reload_empty?0x0c000064:0x0b000065);
  return true;
}
bool smg_update(SmgState &s, const SmgDefinition &d, float dt,
    const SmgActivityCallback &activity,const SmgEffectCallback &effect) {
  auto &b = s.barrel;
  // Stock original own manager: the selected single-SMG record has one frame.
  // 10234A..10235D's non-bit0-event branch invokes1099B0 before magazine and
  // barrel updates; its force-idle predicate preserves states7,8,10.
  if (s.own_manager && !(s.own_events & 1u) &&
      ((s.own_flags & 2u) || d.single_own_frames == 1) &&
      (s.weapon_state < 7 || (s.weapon_state > 8 && s.weapon_state != 10)))
    s.weapon_state = 0;
  // 1021D0: the constructor's zero countdown arms the weapon on first update.
  s.weapon_flags |= 0x20;
  if (s.blocking_ticks > 0) --s.blocking_ticks;
  const bool blocked = (s.input_flags & 32) || s.blocking_ticks > 0 ||
                       (s.weapon_flags & 2);
  // 101E30 runs before trigger / barrel updates.
  if (s.magazine_ticks > 0) --s.magazine_ticks;
  if (s.transfer_ticks > 0) --s.transfer_ticks;
  if (s.magazine_mode == 1) {
    if (s.magazine_ticks < s.transfer_ticks)
      s.transfer_ticks = s.magazine_ticks;
    if (s.transfer_ticks == 0) {
      const int amount = std::min({int(s.reserve), int(d.capacity - s.loaded),
                                  int(d.reload_amount)});
      s.loaded += amount;
      s.reserve -= amount;
      s.transfer_ticks = -1;
      if(activity)activity(0x0400000c); // Original reload-transfer FP idle notification.
    }
    if (s.magazine_ticks == 0) {
      s.magazine_mode = 5;
      s.magazine_ticks = s.magazine_other_ticks = -1;
    }
  } else if (s.magazine_mode == 5) {
    request_state(s, 3); // 106E70 ->109A60 before109C30's mode6 reset.
    s.magazine_mode = 6;
    s.magazine_ticks = 0; // authored SMG chamber time is zero
  } else if (s.magazine_mode == 6 && s.magazine_ticks == 0) {
    s.magazine_mode = 0;
    s.magazine_ticks = -1;
  }
  // 1028C7 Spew. 107120 clears the dry gate only outside firing mode.
  const bool held = !blocked && (s.input_flags & (2 | 8));
  const bool eligible = b.mode != 2 && !(b.flags & 4) && !(s.weapon_flags & 2) &&
                        s.magazine_mode == 0 &&
                        !(s.loaded < 1 && (b.flags & 64));
  if (held && eligible) {
    s.trigger_flags |= 32;
    b.flags |= 1;
  } else s.trigger_flags &= ~32u;
  if ((!held || (s.input_flags & 8)) && b.mode != 1) b.flags &= ~64u;
  if (b.effect_b > 0) b.effect_b = std::max(0.f, b.effect_b - d.effect_decay * dt);
  if (b.ticks > 0) --b.ticks;
  bool auto_reload = false;
  s.marker = false;
  if (b.mode == 0) {
    if (blocked) b.flags &= ~1u;
    else if (b.flags & 1) b.mode = 1;
    else if (s.attached && s.loaded == 0 && s.reserve > 0) auto_reload = smg_reload(s, d,activity);
  }
  if (b.mode == 1) {
    if (blocked || (b.flags & 64) || !(b.flags & 1)) {
      b.mode = 2;
      b.ticks = 0; // authored recovery time is zero
    } else {
      const float ticks_per_shot = 30.f / d.rate;
      s.marker = true;
      if (b.shot_count && float(b.counter) + 1.f < ticks_per_shot) {
        b.counter = std::min(127, int(b.counter) + 1);
      } else {
        b.flags &= ~1u;
        // Authored one-entry zero-flag firing effects: no preparation RNG.
        b.effect_mask = 1;
        b.effect_index = b.effect_remaining = 0;
        const bool admitted=s.loaded>0;
        if (admitted) {
          request_state(s, 1); // Original1078A7, before projectile postpass.
          --s.loaded;
          ++b.shot_count;
          ++b.marker_count;
          if (s.hand_timer_registered)
            s.hand_timers[0] = 9; // EFCC0 after valid exact-weapon/view/player/current-unit lookup.
          if(activity)activity(0x06000006); // Original prepared-shot16A356, before postpass.
          if (d.preparation_effect) b.effect_b = 1;
          b.flags |= 2;
          s.weapon_flags |= 1;
        } else b.flags |= 0xc0;
        b.counter = 0;
        if(effect) b.effect_handle=effect(s,admitted?0u:2u,
            admitted?std::bit_cast<std::uint32_t>(b.rate):0x3f800000u,0);
      }
    }
  } else if (b.mode == 2 && b.ticks == 0) {
    // 108110 -> 107F80, without a persistent mode-3 row for this SMG.
    reset_barrel(s);
  }
  if (s.marker) b.rate = std::min(1.f, b.rate + d.rate_rise * dt);
  else if (b.rate > 0) b.rate = std::max(0.f, b.rate - d.rate_fall * dt);
  WeaponErrorState error{b.error, b.angle_change};
  weapon_error_step(error, d.error, dt, 0, s.marker);
  b.error = error.error;
  b.angle_change = error.recoil;
  return auto_reload;
}
std::optional<Vec3> smg_dispatch(SmgState &s, const SmgDefinition &d, Vec3 aim,
                               std::uint32_t &seed,
                               std::span<const Vec3, 1026> directions) {
  if (!(s.weapon_flags & 1)) return {};
  std::optional<Vec3> result;
  if (s.barrel.flags & 2) {
    auto &b = s.barrel;
    if (b.rate == 0 || b.dispatch_counter++ >= d.dispatch_interval)
      b.dispatch_counter = 0;
    const auto angles = weapon_error_angles(d.error, b.error, 0);
    result = weapon_random_deviation(aim, angles, seed, directions);
    b.flags &= ~2u;
  }
  s.weapon_flags &= ~1u;
  return result;
}
Vec3 smg_inherited_velocity(Vec3 aim, Vec3 forward, Vec3 owner) {
  const float x = aim.x * owner.x;
  const float z = aim.z * owner.z;
  const float xz = x + z;
  const float y = aim.y * owner.y;
  float scalar = xz + y;
  if (scalar < 0) scalar = 0;
  return {forward.x * scalar, forward.y * scalar, forward.z * scalar};
}
Vec3 smg_projectile_velocity(Vec3 forward, Vec3 inherited, float initial_speed,
                             float speed_scale) {
  const float speed = speed_scale * initial_speed;
  const float x = forward.x * speed;
  const float y = forward.y * speed;
  const float z = forward.z * speed;
  return {x + inherited.x, y + inherited.y, z + inherited.z};
}
} // namespace halo2

extern "C" {
void halo2_smg_projectile_velocity(const halo2::Vec3 *aim,
    const halo2::Vec3 *forward, const halo2::Vec3 *owner, float speed,
    float scale, halo2::Vec3 *inherited, halo2::Vec3 *velocity) {
  *inherited = halo2::smg_inherited_velocity(*aim, *forward, *owner);
  *velocity = halo2::smg_projectile_velocity(*forward, *inherited, speed, scale);
}
void halo2_smg_input(halo2::SmgState *s, int held) { halo2::smg_input(*s, held); }
int halo2_smg_reload(halo2::SmgState *s, const halo2::SmgDefinition *d) {
  return halo2::smg_reload(*s, *d);
}
int halo2_smg_update(halo2::SmgState *s, const halo2::SmgDefinition *d, float dt) {
  return halo2::smg_update(*s, *d, dt);
}
int halo2_smg_dispatch(halo2::SmgState *s, const halo2::SmgDefinition *d,
                      halo2::Vec3 *aim, std::uint32_t *seed,
                      const halo2::Vec3 *directions) {
  const auto result = halo2::smg_dispatch(*s, *d, *aim, *seed,
      std::span<const halo2::Vec3, 1026>(directions, 1026));
  if (!result) return 0;
  *aim = *result;
  return 1;
}
}
