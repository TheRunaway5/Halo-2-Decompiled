#include "halo2/nonhost_motion_adoption.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_tu15_object_motion.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr float tick = 1.f / 30.f;
constexpr float gravity = 4.1712594f; // simulation.hpp retail_gravity (XBE 0x4455D8)
}

bool nonhost_velocity_change_forced(Vec3 before, Vec3 after, const NonHostMotionDefinition &d) {
  // Collisions and landings only remove speed. One tick of player control
  // adds at most its ground/air acceleration step plus the walking push,
  // gravity and a jump kick; anything faster came from another owner.
  const float control = std::max({d.movement.acceleration, d.movement.crouch_acceleration,
                                  d.movement.air_acceleration}) * tick + .234f;
  const float bound = control + gravity * tick + d.jump_velocity + .5f;
  return length(after) - length(before) > bound;
}

NonHostMotionAdopter::NonHostMotionAdopter(NonHostMotionDefinition bounds, float radius,
                                           NonHostAdoptionLimits limits, NonHostMotionWriter writer)
    : bounds_(std::move(bounds)), radius_(radius), limits_(limits), writer_(std::move(writer)) {
  if (!(radius_ > 0))
    throw std::invalid_argument("Non-host motion adoption requires the biped radius");
  for (auto &peer : peers_)
    peer.adoption.budget = limits_.budget_burst;
}

std::array<std::uint32_t,41> NonHostMotionAdopter::checkpoint_configuration() const {
  std::array<std::uint32_t,41> result{};
  std::size_t next{};
  const auto scalar=[&](float value){result[next++]=std::bit_cast<std::uint32_t>(value);};
  const auto &m=bounds_.movement;
  for(float value:{m.forward,m.backward,m.sideways,m.acceleration,m.crouch_forward,
      m.crouch_backward,m.crouch_sideways,m.crouch_acceleration,m.air_acceleration,m.limited_speed,
      bounds_.height,bounds_.crouch_height,bounds_.crouch_velocity,bounds_.jump_velocity})scalar(value);
  result[next++]=bounds_.physics_flags;
  const auto &s=bounds_.slope;
  const auto &l=bounds_.landing;
  for(float value:{s.downhill_scale,s.uphill_scale,s.downhill_full,s.downhill_start,s.uphill_start,s.uphill_full,
      l.soft_seconds,l.hard_seconds,l.soft_speed,l.hard_speed,l.maximum_speed})scalar(value);
  result[next++]=bounds_.locomotion_deadzone_bits.has_value();
  result[next++]=bounds_.locomotion_deadzone_bits.value_or(0);
  result[next++]=bounds_.source_control;result[next++]=bounds_.source_facing;
  scalar(bounds_.origin_height);scalar(radius_);
  for(float value:{limits_.reject_distance,limits_.teleport_distance,limits_.velocity_deadband_squared,
      limits_.nudge_scale,limits_.nudge_limit})scalar(value);
  result[next++]=limits_.anti_cheat;
  for(float value:{limits_.speed_tolerance,limits_.budget_rate,limits_.budget_burst})scalar(value);
  return result;
}
NonHostMotionAdopter::Checkpoint NonHostMotionAdopter::checkpoint() const {
  Checkpoint result{checkpoint_configuration(),peers_};
  validate_checkpoint(result);
  return result;
}
void NonHostMotionAdopter::validate_checkpoint(const Checkpoint &checkpoint) {
  const auto &configuration=checkpoint.configuration;
  for(unsigned n=0;n<configuration.size();++n) {
    if(n==14 || n==26 || n==27 || n==28 || n==29 || n==37)continue;
    if(!std::isfinite(std::bit_cast<float>(configuration[n])))
      throw std::invalid_argument("Non-host motion checkpoint has a nonfinite configuration");
  }
  for(unsigned n:{26u,28u,29u,37u})if(configuration[n]>1)
    throw std::invalid_argument("Non-host motion checkpoint has a noncanonical configuration flag");
  if((!configuration[26] && configuration[27]) ||
      (configuration[26] && !std::isfinite(std::bit_cast<float>(configuration[27]))) ||
      !(std::bit_cast<float>(configuration[31])>0))
    throw std::invalid_argument("Non-host motion checkpoint has invalid authored operands");
  for(unsigned n=32;n<configuration.size();++n)
    if(n!=37 && std::bit_cast<float>(configuration[n])<0)
      throw std::invalid_argument("Non-host motion checkpoint has invalid adoption limits");
  const float burst=std::bit_cast<float>(configuration[40]);
  for(const auto &peer:checkpoint.peers) {
    if(!std::isfinite(peer.velocity.x) || !std::isfinite(peer.velocity.y) || !std::isfinite(peer.velocity.z) ||
        !std::isfinite(peer.adoption.budget) || peer.adoption.budget<0 || peer.adoption.budget>burst ||
        unsigned(peer.forced)>unsigned(NonHostForcedMotion::unexplained_velocity) ||
        (peer.forced!=NonHostForcedMotion::none && !peer.adoption.forced_sequence))
      throw std::invalid_argument("Non-host motion checkpoint has invalid peer state");
  }
}
void NonHostMotionAdopter::restore(const Checkpoint &checkpoint) {
  validate_checkpoint(checkpoint);
  if(checkpoint.configuration!=checkpoint_configuration())
    throw std::invalid_argument("Non-host motion checkpoint configuration differs from its fresh owner");
  peers_=checkpoint.peers;
}

namespace {
NonHostMotionAdopter::MotionReader reader(MultiplayerObjectRuntime &runtime) {
  return [&runtime](std::uint32_t unit) -> std::optional<MultiplayerSourceMotion> {
    if (unit == null_datum || !runtime.owns_physics())
      return std::nullopt;
    return runtime.biped_motion(unit);
  };
}
} // namespace
void NonHostMotionAdopter::observe(const MultiplayerState &state, MultiplayerObjectRuntime &runtime,
                                   std::span<const View> views) {
  observe(state, reader(runtime), views);
}
NonHostMotionAdopter::Result NonHostMotionAdopter::adopt(MultiplayerObjectRuntime &runtime,
                                                         const MultiplayerState &state,
                                                         const CollisionWorld &collision, unsigned slot,
                                                         const NonHostMotionReport &report, unsigned host_steps) {
  return adopt(reader(runtime), state, collision, slot, report, host_steps);
}
void NonHostMotionAdopter::observe(const MultiplayerState &state, const MotionReader &read,
                                   std::span<const View> views) {
  std::vector<Observed> observed;
  for (const auto &view : views) {
    if (view.slot >= state.players.size())
      throw std::out_of_range("Non-host motion adoption slot");
    observed.push_back({view, state.players[view.slot].source_unit, state.players[view.slot].source_teleport_serial});
  }
  observe(observed, state.unit_damage_effects, state.physics_impulses, read);
}
void NonHostMotionAdopter::observe(std::span<const Observed> observed,
                                   std::span<const UnitDamageEffectEvent> damage,
                                   std::span<const PhysicsImpulseEvent> impulses, const MotionReader &read) {
  for (const auto &entry : observed) {
    const auto &view = entry.view;
    if (view.slot >= peers_.size())
      throw std::out_of_range("Non-host motion adoption slot");
    auto &peer = peers_[view.slot];
    const auto unit = entry.unit;
    auto forced = NonHostForcedMotion::none;
    if (unit != null_datum && (!peer.seen || unit != peer.unit))
      forced = NonHostForcedMotion::respawn;
    if (peer.seen && entry.teleport_serial != peer.teleport_serial)
      forced = NonHostForcedMotion::teleport;
    for (const auto &event : damage)
      if (event.serial > peer.damage_serial) {
        if (unit != null_datum && event.target == unit &&
            event.effect.kind != UnitDamageEffectKind::ShieldRecharging && forced == NonHostForcedMotion::none)
          forced = NonHostForcedMotion::damage;
      }
    for (const auto &event : damage)
      peer.damage_serial = std::max(peer.damage_serial, event.serial);
    // The impulse journal publishes loose physics objects; a biped event is
    // taken when present. HOOK: if biped damage/explosion impulses get their
    // own journal, read it here.
    for (const auto &event : impulses)
      if (event.serial > peer.impulse_serial) {
        if (unit != null_datum && event.object == unit && forced == NonHostForcedMotion::none)
          forced = NonHostForcedMotion::impulse;
        peer.impulse_serial = std::max(peer.impulse_serial, event.serial);
      }
    Vec3 velocity;
    if (const auto motion = unit != null_datum ? read(unit) : std::nullopt) {
      velocity = motion->velocity;
      if (forced == NonHostForcedMotion::none && peer.seen && unit == peer.unit && motion->parent == null_datum &&
          motion->physical_state == 1 && nonhost_velocity_change_forced(peer.velocity, velocity, bounds_))
        forced = NonHostForcedMotion::unexplained_velocity;
    }
    if (forced != NonHostForcedMotion::none) {
      peer.forced = forced;
      peer.adoption.forced_sequence = view.consumed + 1;
    }
    peer.seen = true;
    peer.unit = unit;
    peer.teleport_serial = entry.teleport_serial;
    peer.velocity = velocity;
  }
}

NonHostMotionAdopter::Result NonHostMotionAdopter::adopt(const MotionReader &read, const MultiplayerState &state,
                                                         const CollisionWorld &collision, unsigned slot,
                                                         const NonHostMotionReport &report, unsigned host_steps) {
  if (slot >= state.players.size())
    throw std::out_of_range("Non-host motion adoption slot");
  return adopt(read, state.players[slot].source_unit, state.players[slot].dead, collision, slot, report, host_steps);
}
NonHostMotionAdopter::Result NonHostMotionAdopter::adopt(const MotionReader &read, std::uint32_t unit, bool dead,
                                                         const CollisionWorld &collision, unsigned slot,
                                                         const NonHostMotionReport &report, unsigned host_steps) {
  if (slot >= peers_.size())
    throw std::out_of_range("Non-host motion adoption slot");
  auto &peer = peers_[slot];
  Result result;
  const auto reject = [&](NonHostRejection why) {
    result.decision.kind = NonHostAdoptionKind::reject;
    result.decision.rejection = why;
    return result;
  };
  const auto read_motion = unit == null_datum || dead ? std::nullopt : read(unit);
  if (!read_motion)
    return reject(NonHostRejection::invalid);
  const auto &motion = *read_motion;
  // 0x82980 admits a biped controlled by this player without a parent; the
  // port also requires the on-foot walking/airborne owner (state 1) alive.
  if ((motion.unit_flags & 4u) || motion.physical_state != 1)
    return reject(NonHostRejection::invalid);
  NonHostAuthority authority;
  authority.position = motion.position;
  authority.velocity = motion.velocity;
  authority.forward = motion.forward;
  authority.crouch = motion.crouch;
  authority.grounded = (motion.support_flags & 1u) == 0;
  const auto host = nonhost_motion_from_authority(authority, bounds_, radius_, .65f, collision);
  const float maximum_speed =
      std::max({bounds_.movement.forward, bounds_.movement.backward, bounds_.movement.sideways});
  NonHostAdoptionInput input{&report, &host, motion.parent != null_datum, maximum_speed, &collision, tick,
                             host_steps};
  result.decision = adopt_nonhost_motion(peer.adoption, limits_, input);
  switch (result.decision.kind) {
  case NonHostAdoptionKind::teleport:
    if (writer_.teleport) {
      result.written = writer_.teleport(unit, result.decision.position, result.decision.taken_velocity);
      break;
    }
    // No native B95D0 placement yet: B41D0's own fallback for a blocked
    // teleport is decision 2, so take B44C0's nudge branch.
    result.placement_unavailable = true;
    [[fallthrough]];
  case NonHostAdoptionKind::nudge:
  case NonHostAdoptionKind::unchanged:
    // B44C0 decision 2 owns its dead bands against BC200's current
    // velocities, so it receives the reported operands unfiltered.
    if (writer_.nudge)
      result.written = writer_.nudge(unit, report.position, report.velocity);
    break;
  case NonHostAdoptionKind::reject:
    break;
  }
  return result;
}

std::optional<NativeMotionReport> quantize_nonhost_motion(const NonHostMotionReport &r) {
  NativeMotionReport out;
  out.grounded = r.grounded;
  out.crouching = r.crouching;
  const auto age = r.sequence - r.baseline;
  out.baseline_age = std::uint8_t(std::min<std::uint32_t>(age, 255));
  const float p[3]{r.position.x, r.position.y, r.position.z}, v[3]{r.velocity.x, r.velocity.y, r.velocity.z};
  for (unsigned i = 0; i < 3; ++i) {
    if (!std::isfinite(p[i]) || !std::isfinite(v[i]))
      return std::nullopt;
    const auto scaled = std::nearbyint(double(p[i]) * 1024.0);
    if (std::abs(scaled) > native_motion_position_limit)
      return std::nullopt;
    out.position[i] = std::int32_t(scaled);
    out.velocity[i] = std::int16_t(std::clamp(std::nearbyint(double(v[i]) * 256.0), -32768.0, 32767.0));
  }
  return out;
}
NonHostMotionReport dequantize_nonhost_motion(const NativeMotionReport &m, std::uint32_t sequence) {
  NonHostMotionReport r;
  r.sequence = sequence;
  r.baseline = sequence - m.baseline_age;
  r.grounded = m.grounded;
  r.crouching = m.crouching;
  r.position = {float(m.position[0]) / 1024.f, float(m.position[1]) / 1024.f, float(m.position[2]) / 1024.f};
  r.velocity = {float(m.velocity[0]) / 256.f, float(m.velocity[1]) / 256.f, float(m.velocity[2]) / 256.f};
  return r;
}
NonHostAdoptionLimits nonhost_adoption_limits(NetcodeMode mode) {
  NonHostAdoptionLimits limits; // TU1.5 B41D0 mode 2 / B44C0, anti-cheat on
  if (mode != NetcodeMode::original)
    limits.teleport_distance = 0; // exact adoption (served by the nudge until native B95D0)
  return limits;
}
NonHostMovementPredictor::Settings nonhost_predictor_settings(NetcodeMode) {
  NonHostMovementPredictor::Settings settings;
  // Without a native B95D0 the host only follows through B44C0 nudges (at
  // most 0.05 wu/s of positional correction), so a wide trust band lets the
  // two drift apart. Measured over real UDP on Lockout (100 ms): trust 0.2
  // never corrects and leaves up to 0.29 wu after an impulse; trust 0 replays
  // every snapshot (largest shown ground correction 0.052 wu/tick); 0.05
  // replays ~3% of snapshots with at most 0.032 wu/tick and a 0.012 wu rest
  // offset. Original's 2.5 wu own-unit rule needs the native teleport.
  settings.trust_distance = .05f;
  return settings;
}

NonHostMotionWriter native_tu15_motion_writer(MultiplayerObjectRuntime &runtime) {
  NonHostMotionWriter writer;
  writer.nudge = [&runtime](std::uint32_t object, Vec3 position, Vec3 velocity) {
    auto &memory = runtime.memory();
    // A deactivated Havok island needs the unported island listener; the
    // host's own TU1.5 step wakes a biped its input moves, so skip.
    if (!native_tu15_object_velocities_unsupported(memory, object, &velocity, nullptr).empty())
      return false;
    const auto frame = runtime.tu15_world().prepare_source_call();
    if (!frame.floating_point)
      throw std::runtime_error("Native TU1.5 nudge requires the source floating-point environment");
    // Mode 2 (client motion); forward/up/angular keep the object's own.
    return native_tu15_nudge_object(memory, object, 2, &position, nullptr, nullptr, &velocity, nullptr,
                                    *frame.floating_point);
  };
  return writer;
}

std::optional<std::uint32_t> NonHostMotionAdopter::forced_sequence(unsigned slot) const {
  return slot < peers_.size() ? peers_[slot].adoption.forced_sequence : std::nullopt;
}
NonHostForcedMotion NonHostMotionAdopter::last_forced(unsigned slot) const {
  return slot < peers_.size() ? peers_[slot].forced : NonHostForcedMotion::none;
}
} // namespace halo2
