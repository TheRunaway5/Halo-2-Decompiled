#include "halo2/kernels.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cfenv>
#include <cmath>
#include <stdexcept>
namespace halo2 {
Vec3 ordinary_unit_eye(Vec3 position, float standing, float crouching,
                       float crouch, float scale) {
  const float complement = 1.f - crouch;
  const float standing_part = complement * standing;
  const float crouching_part = crouching * crouch;
  const float height = standing_part + crouching_part;
  const float scaled = height * scale;
  return {position.x, position.y, scaled + position.z};
}
namespace {
double precise_length(Vec3 v) {
  const double x = v.x, y = v.y, z = v.z;
  return std::sqrt((x * x + y * y) + z * z);
}
float retail_normalize(Vec3 &v, bool two_dimensions = false) {
  const auto magnitude = precise_length({v.x, v.y, two_dimensions ? 0.f : v.z});
  if (std::abs(magnitude) < double(.0001f))
    return 0;
  const float rounded = static_cast<float>(magnitude);
  const float inverse = 1.f / rounded;
  v.x *= inverse;
  v.y *= inverse;
  if (!two_dimensions)
    v.z *= inverse;
  return rounded;
}
} // namespace
float original_vector_normalize(Vec3 &value) { return retail_normalize(value); }
MovementIntent movement_intent(const MovementDefinition &m, Vec3 control,
                               float crouch, bool limited, bool ignore_crouch) {
  const float squared =
      (control.z * control.z + control.y * control.y) + control.x * control.x;
  if (squared > 1.f) {
    // Retail keeps the reciprocal square root in x87 until each product is
    // stored, rather than rounding an intermediate scale to float.
    const double inverse = 1. / std::sqrt(double(squared));
    control.x = static_cast<float>(control.x * inverse);
    control.y = static_cast<float>(control.y * inverse);
  }
  const float blend = ignore_crouch ? 0.f : crouch, remaining = 1.f - blend;
  const float standing = control.x > 0 ? m.forward : m.backward,
              crouched = control.x > 0 ? m.crouch_forward : m.crouch_backward;
  MovementIntent result{{standing * remaining + crouched * blend,
                         m.sideways * remaining + m.crouch_sideways * blend, 0},
                        m.acceleration * remaining +
                            m.crouch_acceleration * blend,
                        m.air_acceleration};
  if (limited) {
    result.velocity.x = control.x > 0 ? m.limited_speed : 0;
    result.velocity.y = 0;
  }
  result.velocity.x *= control.x;
  result.velocity.y *= control.y;
  return result;
}
WalkingResult walking_velocity(const WalkingInput &s) {
  bool gravity = (s.flags & 8) != 0;
  if (s.flags & 4) {
    Vec3 delta{s.scale * (s.facing.x * s.input.x - s.facing.y * s.input.y) -
                   s.current.x,
               s.scale * (s.facing.y * s.input.x + s.facing.x * s.input.y) -
                   s.current.y,
               0};
    auto limited = delta;
    const float step = s.air_acceleration * s.dt;
    if (retail_normalize(limited, true) > step)
      limited = limited * step;
    else
      limited = delta;
    return {{s.current.x + limited.x, s.current.y + limited.y,
             s.current.z - s.gravity * s.dt},
            gravity};
  }
  float magnitude = static_cast<float>(precise_length(s.input));
  Vec3 desired;
  if (s.normal.z > .001f) {
    desired.x = s.facing.x * s.input.x - s.facing.y * s.input.y;
    desired.y = s.facing.y * s.input.x + s.facing.x * s.input.y;
    desired.z = s.input.z -
                (s.normal.y * desired.y + s.normal.x * desired.x) / s.normal.z;
  } else {
    auto left = cross({0, 0, 1}, s.basis);
    retail_normalize(left);
    const float projection = (s.normal.z * s.basis.z + s.normal.y * s.basis.y) +
                             s.normal.x * s.basis.x;
    const auto forward = s.basis + s.normal * (0.f - projection);
    const float side_projection =
        (s.normal.z * left.z + s.normal.y * left.y) + s.normal.x * left.x;
    left = left + s.normal * (0.f - side_projection);
    desired = left * s.input.y + forward * s.input.x;
    desired.z = (desired.z + s.input.z) * 5.f;
  }
  retail_normalize(desired);
  if (s.slope.downhill_full >= desired.z)
    magnitude *= s.slope.downhill_scale;
  else if (s.slope.downhill_full > desired.z)
    // This apparently unreachable comparison is present in the original XBE.
    magnitude *= 1.f + (s.slope.downhill_scale - 1.f) *
                           (desired.z - s.slope.downhill_full) /
                           (s.slope.downhill_start - s.slope.downhill_full);
  else if (desired.z >= s.slope.uphill_full)
    magnitude *= s.slope.uphill_scale;
  else if (desired.z > s.slope.uphill_start)
    magnitude *= 1.f + (desired.z - s.slope.uphill_start) *
                           (s.slope.uphill_scale - 1.f) /
                           (s.slope.uphill_full - s.slope.uphill_start);
  const auto delta = desired * (s.scale * magnitude) - s.current;
  auto limited = delta;
  const float step = s.acceleration * s.dt;
  if (retail_normalize(limited) > step)
    limited = limited * step;
  else {
    limited = delta;
    gravity = false;
  }
  if (!s.suppress_ground_push)
    limited = limited - s.normal * .234f;
  if (s.flags & 16) {
    const float remaining = 1.f - s.damping;
    limited.x *= remaining;
    limited.y *= remaining;
  }
  if (gravity)
    limited.z -= s.gravity * s.dt;
  return {s.current + limited, gravity};
}
float projectile_acceleration(float scale, float gravity) {
  return 0.f - scale * gravity;
}
bool jump_gate(JumpState &s, bool held, bool player, bool unsupported,
               bool hard_landing, bool kick_accepted, float dt, int tick_rate) {
  if (s.since_jump < 127)
    ++s.since_jump;
  if (!held) {
    s.held_ticks = 0;
    return false;
  }
  const int delay = int(std::nearbyint(float(tick_rate) * .3f));
  // Original E46FA jumps to the finish when a player's hold reaches the
  // delay. The recovered C's inverse comparison is incorrect.
  const bool jumped = (!player || s.held_ticks < delay) &&
                      float(s.since_jump) * dt > .16f && !unsupported &&
                      !hard_landing && kick_accepted;
  if (jumped) {
    s.since_jump = 0; // Supplied E4770 kick also resets this timer.
    s.held_ticks = static_cast<std::int8_t>(delay);
  }
  if (s.held_ticks < 127)
    ++s.held_ticks;
  return jumped;
}
Vec3 jump_impulse(Vec3 velocity, Vec3 up, Vec3 support, float speed) {
  const auto relative = velocity - support;
  const float along =
      (up.z * relative.z + up.y * relative.y) + up.x * relative.x;
  if (speed > along) {
    const float kick = speed - along;
    velocity.x += up.x * kick;
    velocity.y += up.y * kick;
    velocity.z += up.z * kick;
  }
  return velocity;
}
ProjectileFlight projectile_flight(Vec3 position, Vec3 velocity, float gravity,
                                   float dt, float traveled,
                                   float maximum_range) {
  const float speed = static_cast<float>(precise_length(velocity));
  const float fall = gravity * dt;
  auto average = velocity;
  velocity.z -= fall;
  average.z -= fall * .5f;
  const bool exhausted =
      maximum_range != 0 && traveled + speed * dt > maximum_range;
  const float fraction =
      exhausted ? (speed != 0 ? (maximum_range - traveled) / (speed * dt) : 0.f)
                : 1.f;
  const float step = fraction * dt;
  const auto end = position + average * step;
  const auto displacement = end - position;
  return {velocity, end, displacement,
          static_cast<float>(double(traveled) + precise_length(displacement)),
          exhausted};
}
float crouch_delta(float crouch, float dt, float velocity, bool held,
                   bool forced) {
  float delta = dt * velocity;
  if (held) {
    const float available = 1.f - crouch;
    if (available < delta)
      delta = available;
  } else {
    if (crouch < delta)
      delta = crouch;
    delta = 0.f - delta;
  }
  if (forced) {
    delta = 1.f - crouch;
    if (!(1.f > delta))
      delta = 1.f;
  }
  return delta;
}
float capsule_segment_height(float standing, float crouching, float radius,
                             float crouch, bool sphere) {
  return sphere ? 0.f
                : (crouching - standing) * crouch + standing - 2.f * radius;
}
std::uint32_t scalar_cast_bits(std::uint32_t bits, unsigned source,
                               unsigned target) {
  if (source == target || source == 3)
    return bits;
  if (target == 4)
    return 0;
  const auto short_value = static_cast<std::int16_t>(bits);
  if (target == 5 && (source == 6 || source == 8))
    return (bits & 0xffffff00u) | (bits == 0);
  if (target == 5 && source == 7)
    return (bits & 0xffffff00u) | (short_value == 0);
  if (target == 6 && source == 7)
    return std::bit_cast<std::uint32_t>(float(short_value));
  if (target == 6 && source == 8)
    return std::bit_cast<std::uint32_t>(float(static_cast<std::int32_t>(bits)));
  if (target == 6 && source >= 44 && source <= 49)
    return std::bit_cast<std::uint32_t>(float(short_value + 1));
  if (source == 6 && (target == 7 || target == 8)) {
    const float value = std::bit_cast<float>(bits);
    const auto integer = !std::isfinite(value) ||
                                 double(value) >= 2147483648. ||
                                 double(value) < -2147483648.
                             ? 0x80000000u
                             : std::uint32_t(static_cast<std::int32_t>(value));
    return target == 8 ? integer : (bits & 0xffff0000u) | (integer & 0xffffu);
  }
  if (target == 8 && source == 7)
    return std::uint32_t(std::int32_t(short_value));
  if (target == 7 && source == 8)
    return bits;
  throw std::invalid_argument("unsupported scalar script cast");
}
float fade_curve_fraction(float x, std::span<const std::uint8_t, 1024> curve) {
  if (!std::isfinite(x))
    throw std::invalid_argument("nonfinite fade fraction");
  if (std::fegetround() != FE_TONEAREST)
    throw std::invalid_argument(
        "retail fade kernel requires round-to-nearest mode");
  x = std::clamp(x, 0.f, 1.f);
  const float scaled = x * 1023.f;
  const float fraction = std::fmod(scaled, 1.f);
  const auto i = int(std::nearbyint(scaled - .1f));
  const float a = curve[i] * (1.f / 255.f);
  if (i == 1023)
    return a;
  const float b = curve[i + 1] * (1.f / 255.f);
  return std::clamp(a * (1.f - fraction) + b * fraction, 0.f, 1.f);
}
bool recharge(RechargeState &s, float dt, float body_rate, float shield_rate,
              float maximum_body_fraction, float team_scale, bool body_enabled,
              bool shield_enabled) {
  if (s.dead)
    return false;
  bool changed = false;
  if (shield_enabled) {
    if (s.shield < 1) {
      if (!s.shield_stun) {
        s.shield_depleted = false;
        s.shield_recharging = true;
        s.shield += team_scale * (dt * shield_rate);
        if (s.shield > 1) {
          s.shield = 1;
          s.shield_recharging = false;
          s.shield_damaged = false;
        } else
          changed = true;
      } else {
        if (!s.paused && s.shield_stun != 0x7fff) {
          s.shield_stun = std::int16_t(s.shield_stun - 1);
          changed = true;
        }
        s.shield_recharging = false;
      }
    } else
      s.shield_recharging = false;
  }
  if (body_enabled && s.body < maximum_body_fraction && body_rate > 0) {
    if (!s.body_stun) {
      s.body_recharging = true;
      s.body += team_scale * (dt * body_rate);
      if (s.body > maximum_body_fraction) {
        s.body = maximum_body_fraction;
        s.body_recharging = false;
      } else
        changed = true;
    } else if (!s.paused) {
      s.body_stun = std::int16_t(s.body_stun - 1);
      changed = true;
    }
  }
  return changed;
}
namespace {
template <class T>
const T *damage_lookup(std::span<const T> entries, std::uint32_t key) {
  std::size_t start = 0, count = entries.size();
  while (count) {
    const auto middle = start + (count >> 1);
    const auto difference = std::bit_cast<std::int32_t>(key - entries[middle].name);
    if (!difference)
      return &entries[middle];
    if (difference > 0) {
      start = middle + 1;
      --count;
    }
    count >>= 1;
  }
  return nullptr;
}
} // namespace
float damage_multiplier(std::span<const DamageGroup> table,
                        std::uint32_t group_a, std::uint32_t group_b,
                        std::uint32_t armor_a, std::uint32_t armor_b) {
  float result = 1;
  for (const auto key : {group_a, group_b})
    if (const auto *group = damage_lookup(table, key))
      for (const auto armor : {armor_a, armor_b})
        if (const auto *modifier = damage_lookup<ArmorModifier>(group->modifiers, armor))
          result = modifier->multiplier * result;
  return result;
}
ShieldDamageResult shield_damage(ShieldDamageState &s,
                                 const ShieldDamageInput &in) {
  ShieldDamageResult out{in.damage};
  float amount = in.damage > 0 ? in.damage : 0.f;
  s.shield = s.shield > 0 ? s.shield : 0.f;
  if (s.shield == 0)
    amount = 0;
  if (s.flags & 16) {
    out.body_damage = 0;
    return out;
  }
  if (s.flags & 128)
    return out;
  const float inverse = in.maximum_shield > 0 ? 1.f / in.maximum_shield : 0.f;
  if (!in.radius)
    out.body_damage -= amount;
  const float multiplier = amount > 0 ? in.multiplier : 1.f;
  amount *= multiplier;
  out.shield_damage = inverse * amount;
  s.shield -= out.shield_damage;
  if (s.shield > 0 && in.side_effect == 3 && in.radius)
    s.shield = 0;
  if (s.shield <= 0) {
    if (s.shield < 0) {
      const float overflow = (0.f - s.shield) * in.maximum_shield;
      if (overflow > 0 && multiplier > .0001f)
        out.body_damage = (1.f / multiplier) * overflow + out.body_damage;
      s.shield = 0;
    }
    if (!(s.flags & 8)) {
      s.flags |= 8;
      out.flags |= 8;
      out.depleted_effect = true;
    }
  }
  if (!(s.flags & 2) && in.damaged_threshold > s.shield) {
    s.flags |= 2;
    out.damaged_effect = true;
  }
  if (amount > in.minimum_stun_damage ||
      (s.shield == 0 && out.body_damage > .0001f) ||
      (in.side_effect == 3 && in.radius)) {
    if (s.stun != 32767)
      s.stun = std::int16_t(int(std::nearbyint(float(in.tick_rate) * in.stun_seconds)));
  }
  return out;
}
BodyDamageResult body_damage(BodyDamageState &s, const BodyDamageInput &in) {
  BodyDamageResult out{in.damage, 0, 0, in.accumulator_flags, in.reporting_type};
  float amount = in.damage;
  if ((s.flags & 128) || ((in.info_flags & 32) && !(in.definition_flags & 32)))
    amount = 0;
  amount = in.multiplier * amount;
  if (amount > 0) {
    const float inverse = in.maximum_body > 0 ? 1.f / in.maximum_body : 0.f;
    if (in.region_valid && in.hit_region && (in.region_flags & 128) &&
        (in.definition_flags & 2) && in.headshot_allowed) {
      s.body = 0;
      out.flags |= 0x1c0;
      out.reporting_type = (out.reporting_type & 0x3f) | 0x40;
    }
    if (in.region_valid) {
      if (in.region_vitality > 0) {
        out.region_damage = (amount * inverse) / in.region_vitality;
        out.region_update = true;
      }
      if (in.region_flags & 1)
        amount = 0;
    }
    out.normalized_damage = inverse * amount;
    s.body -= out.normalized_damage;
    if ((in.info_flags & 128) || (s.flags & 0x4000))
      if (.0001f > s.body)
        s.body = .0001f;
  }
  if (in.death_maximum_body * s.body <= 0) {
    if (in.riders)
      s.flags |= 0x1000;
    else if (!(s.flags & 4)) {
      // D6800 is represented by a depletion request and its body-depleted
      // state bit; region, child, shield and effect callbacks belong to host.
      s.flags |= 4;
      out.flags |= 1;
      out.depleted = true;
    }
  }
  if (amount > in.minimum_stun_damage)
    s.stun = std::int16_t(int(std::nearbyint(float(in.tick_rate) * in.stun_seconds)));
  if ((in.event_flags & 1) && in.impact_effect &&
      ((!(s.flags & 4) && amount > 10.f) || (out.flags & 0x41)))
    out.impact_effect = true;
  out.remaining -= amount;
  return out;
}
} // namespace halo2
extern "C" void halo2_ordinary_unit_eye(const halo2::Vec3 *position,
    float standing, float crouching, float crouch, float scale, halo2::Vec3 *out) {
  *out = halo2::ordinary_unit_eye(*position, standing, crouching, crouch, scale);
}
extern "C" float halo2_crouch_delta(float c, float dt, float v, int held,
                                    int forced) {
  return halo2::crouch_delta(c, dt, v, held != 0, forced != 0);
}
extern "C" std::uint32_t halo2_scalar_cast(std::uint32_t bits, unsigned source,
                                           unsigned target) {
  return halo2::scalar_cast_bits(bits, source, target);
}
extern "C" float halo2_fade_curve(float x, const std::uint8_t *curve) {
  return halo2::fade_curve_fraction(x, std::span<const std::uint8_t,1024>(curve,1024));
}
extern "C" void halo2_movement_intent(const halo2::MovementDefinition *m,
                                      const halo2::Vec3 *control, float crouch,
                                      int limited, int ignore_crouch,
                                      halo2::MovementIntent *result) {
  *result = halo2::movement_intent(*m, *control, crouch, limited != 0,
                                   ignore_crouch != 0);
}
extern "C" void halo2_walking_velocity(const halo2::WalkingInput *input,
                                       halo2::WalkingResult *result) {
  *result = halo2::walking_velocity(*input);
}
extern "C" float halo2_capsule_height(float standing, float crouching,
                                      float radius, float crouch, int sphere) {
  return halo2::capsule_segment_height(standing, crouching, radius, crouch,
                                       sphere != 0);
}
extern "C" float halo2_projectile_acceleration(float scale, float gravity) {
  return halo2::projectile_acceleration(scale, gravity);
}
extern "C" int halo2_jump_gate(halo2::JumpState *state, int held, int player,
                               int unsupported, int hard_landing, int accepted,
                               float dt, int tick_rate) {
  return halo2::jump_gate(*state, held, player, unsupported, hard_landing,
                          accepted, dt, tick_rate);
}
extern "C" int halo2_recharge(halo2::RechargeState *state, float dt,
                              float body_rate, float shield_rate, float maximum,
                              float team_scale, int body_enabled,
                              int shield_enabled) {
  return halo2::recharge(*state, dt, body_rate, shield_rate, maximum,
                         team_scale, body_enabled, shield_enabled);
}
extern "C" float halo2_original_vector_normalize(halo2::Vec3 *value) {
  return halo2::original_vector_normalize(*value);
}
extern "C" void halo2_projectile_flight(const halo2::Vec3 *position,
                                        const halo2::Vec3 *velocity,
                                        float gravity, float dt, float traveled,
                                        float maximum_range,
                                        halo2::ProjectileFlight *result) {
  *result = halo2::projectile_flight(*position, *velocity, gravity, dt,
                                     traveled, maximum_range);
}
struct FlatDamageGroup {
  std::uint32_t name, offset, count;
};
extern "C" float halo2_damage_multiplier(const FlatDamageGroup *groups,
                                          unsigned count,
                                          const halo2::ArmorModifier *modifiers,
                                          std::uint32_t ga, std::uint32_t gb,
                                          std::uint32_t aa, std::uint32_t ab) {
  std::vector<halo2::DamageGroup> table;
  table.reserve(count);
  for (unsigned i = 0; i < count; ++i)
    table.push_back({groups[i].name,
                     {modifiers + groups[i].offset,
                      modifiers + groups[i].offset + groups[i].count}});
  return halo2::damage_multiplier(table, ga, gb, aa, ab);
}
extern "C" void halo2_shield_damage(halo2::ShieldDamageState *state,
                                     const halo2::ShieldDamageInput *input,
                                     halo2::ShieldDamageResult *result) {
  *result = halo2::shield_damage(*state, *input);
}
extern "C" void halo2_body_damage(halo2::BodyDamageState *state,
                                   const halo2::BodyDamageInput *input,
                                   halo2::BodyDamageResult *result) {
  *result = halo2::body_damage(*state, *input);
}
