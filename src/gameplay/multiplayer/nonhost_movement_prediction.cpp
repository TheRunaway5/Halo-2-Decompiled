#include "halo2/nonhost_movement_prediction.hpp"
#include "halo2/math.hpp"
#include "halo2/player_command.hpp"
#include <array>
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

// Phase 2 schema (lands with the input-request owner). Per local view, an
// optional motion block appended to NativeInputControl; the request's own
// input sequence names the input it describes:
//   u8     flags     bit0 valid (on foot, alive, predictor active),
//                    bit1 grounded, bit2 crouching
//   u8     baseline  sequence - acknowledged sequence the prediction started
//                    from (255: unknown; the host then treats it as stale)
//   3x i24 position  1/1024 wu, absolute (+-8192 wu)
//   3x i16 velocity  1/256 wu/s (+-128 wu/s; original wire cap 350)
// = 17 bytes per view. The original sent position, forward, up and both
// velocities (0xAB7F0, 73-142 bits); on foot forward/up follow the absolute
// yaw/pitch already in the controls, and bipeds have no angular velocity.
// Downstream, each snapshot adds the newest input sequence the host had
// consumed when it stepped (u32 per peer, next to the consumed raw look).

namespace halo2 {
namespace {
// The values of simulation.hpp's tick_seconds and retail_gravity (XBE
// 0x4455D8), restated so this owner does not pull in the SDL-facing header.
constexpr float tick_seconds_native = 1.f / 30.f;
constexpr float retail_gravity_native = 4.1712594f;
bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
bool newer(std::uint32_t a, std::uint32_t b) { return std::int32_t(a - b) > 0; }
float wrap_angle(float a) { return std::remainder(a, 6.283185307f); }
} // namespace

NonHostMotionDefinition native_motion_definition(const MovementDefinition &movement,
                                                 const BipedDefinition &biped,
                                                 std::optional<std::uint32_t> deadzone_bits) {
  NonHostMotionDefinition d;
  d.movement = movement;
  d.height = biped.height;
  d.crouch_height = biped.crouch_height;
  d.crouch_velocity = biped.crouch_velocity;
  d.jump_velocity = biped.jump_velocity;
  d.physics_flags = biped.physics_flags;
  d.slope = biped.slope;
  d.landing = biped.landing;
  d.locomotion_deadzone_bits = deadzone_bits;
  return d;
}

NonHostMotionDefinition nonhost_predictor_definition(const MovementDefinition &movement,
                                                     const BipedDefinition &biped,
                                                     std::optional<std::uint32_t> deadzone_bits) {
  auto d = native_motion_definition(movement, biped, deadzone_bits);
  d.source_control = d.source_facing = true;
  d.origin_height = tu15_biped_origin_height;
  return d;
}

void step_native_unit_motion(NonHostBipedMotion &u, const NonHostMotionDefinition &d,
                             const CollisionWorld &collision, Vec3 desired, bool jump, bool crouch,
                             bool player_control, std::optional<Vec3> facing) {
  // Simulation::move_unit, after its source-movement early return.
  const bool landing_enabled = d.locomotion_deadzone_bits.has_value();
  if (landing_enabled) {
    const float magnitude = desired.x * desired.x + desired.y * desired.y + desired.z * desired.z;
    advance_source_landing(u.landing, magnitude > std::bit_cast<float>(*d.locomotion_deadzone_bits));
  }
  const auto move_and_land = [&] {
    const bool supported = u.body.grounded;
    collision.move(u.body, tick_seconds_native);
    // Explicit native capsule-contact adaptation of DEFC0's incoming impact
    // operand. The retained timer and authored thresholds are E5960's rules.
    if (landing_enabled && !supported && u.body.grounded && u.body.landing_speed > 0) {
      bool force_hard = false;
      begin_source_landing(u.landing, d.landing, u.body.landing_speed, 30, force_hard);
    }
  };
  const float proposed =
      u.crouch + crouch_delta(u.crouch, tick_seconds_native, d.crouch_velocity, crouch, false);
  const float height = capsule_segment_height(d.height, d.crouch_height, u.body.radius, proposed,
                                              (d.physics_flags & 2) != 0) +
                       2 * u.body.radius;
  if (proposed >= u.crouch || !collision.overlaps(u.body.position, height, u.body.radius)) {
    u.crouch = proposed;
    u.body.height = std::max(2 * u.body.radius, height);
  }
  if (player_control && !(d.physics_flags & 8)) {
    const auto intent = movement_intent(d.movement, desired, u.crouch, false, !u.body.grounded);
    if (jump_gate(u.jump, jump, true, !u.body.grounded, u.landing.kind == 1, u.body.grounded,
                  tick_seconds_native, 30)) {
      u.body.velocity = jump_impulse(u.body.velocity, {0, 0, 1}, {}, d.jump_velocity);
      u.body.grounded = false;
    }
    WalkingInput input;
    input.input = intent.velocity;
    input.current = u.body.velocity;
    input.facing = input.basis = facing.value_or(direction(u.yaw));
    input.normal = u.body.support_normal;
    input.slope = d.slope;
    input.acceleration = intent.acceleration;
    input.air_acceleration = intent.air_acceleration;
    input.gravity = retail_gravity_native;
    input.dt = tick_seconds_native;
    input.flags = 8 | (u.body.grounded ? 0 : 4);
    u.body.velocity = walking_velocity(input).velocity;
    move_and_land();
    return;
  }
  const auto delta = desired - Vec3{u.body.velocity.x, u.body.velocity.y, 0};
  const auto acceleration = u.body.grounded
                                ? (u.crouch > .5f ? d.movement.crouch_acceleration : d.movement.acceleration)
                                : d.movement.air_acceleration;
  const auto change = normalized(delta) * std::min(length(delta), acceleration * tick_seconds_native);
  u.body.velocity.x += change.x;
  u.body.velocity.y += change.y;
  if (jump && u.body.grounded)
    u.body.velocity.z = d.jump_velocity;
  u.body.velocity.z -= retail_gravity_native * tick_seconds_native;
  move_and_land();
}

SourceMotionControl source_motion_control(const NonHostMotionInput &in, Vec3 unit_forward,
                                          std::optional<std::uint32_t> deadzone_bits) {
  // 153070's command fields: +4 yaw, +8 pitch, +0C forward, +10 -right.
  std::array<std::byte, 0x5c> command{};
  initialize_source_neutral_command(command);
  const auto put = [&](std::size_t at, float value) {
    const auto bits = std::bit_cast<std::uint32_t>(value);
    for (unsigned i = 0; i < 4; ++i)
      command[at + i] = std::byte((bits >> (8 * i)) & 255u);
  };
  put(4, in.yaw);
  put(8, in.pitch);
  put(0xc, in.forward);
  put(0x10, 0.f - in.right);
  // 152E00 admission for an on-foot, living (physical state 1) biped.
  const SourceCommandCurrentUnit current{0, 0, 0xffffffffu, 1, false,
                                         {unit_forward.x, unit_forward.y, unit_forward.z},
                                         0xffffffffu, -1, std::nullopt};
  const auto control = produce_source_player_control(command, current, 0);
  if (!control)
    throw std::logic_error("On-foot source control unexpectedly required a seat transform");
  const auto f = [&](std::size_t at) {
    std::uint32_t bits = 0;
    for (unsigned i = 0; i < 4; ++i)
      bits |= std::to_integer<std::uint32_t>(control->packet[at + i]) << (8 * i);
    return std::bit_cast<float>(bits);
  };
  SourceMotionControl out;
  out.throttle = {f(0x14), f(0x18), f(0x1c)};
  out.facing = {f(0x28), f(0x2c), f(0x30)};
  // DF880 at DFB15: COMISS threshold, |throttle|^2 then JBE keeps the vector
  // at/above the threshold (and unordered); below it the unit fields take the
  // source zero vector.
  if (deadzone_bits) {
    const auto &t = out.throttle;
    const float squared = (t.x * t.x + t.y * t.y) + t.z * t.z;
    if (std::bit_cast<float>(*deadzone_bits) > squared) {
      out.throttle = {};
      out.dead = true;
    }
  }
  return out;
}

void step_native_player_motion(NonHostBipedMotion &u, const NonHostMotionDefinition &d,
                               const CollisionWorld &collision, const NonHostMotionInput &in) {
  // The multiplayer participant path of Simulation::step_impl: adopt the
  // delivered absolute look, then move_unit({forward,-right,0}, ..., true).
  u.yaw = in.yaw;
  u.pitch = in.pitch;
  if (!d.source_control) {
    step_native_unit_motion(u, d, collision, {in.forward, -in.right, 0}, in.jump, in.crouch, true);
    return;
  }
  // TU1.5 control chain ahead of the physical step: 152E80 basis/facing
  // (153070 -> C92C0 copies the throttle to unit+1B0) and the DF880 dead zone.
  const auto control = source_motion_control(in, u.facing, d.locomotion_deadzone_bits);
  u.facing = control.facing;
  step_native_unit_motion(u, d, collision, control.throttle, in.jump, in.crouch, true,
                          d.source_facing ? std::optional(control.facing) : std::nullopt);
}

NonHostBipedMotion nonhost_motion_from_authority(const NonHostAuthority &a, const NonHostMotionDefinition &d,
                                                 float radius, float maximum_slope_cosine,
                                                 const CollisionWorld &collision,
                                                 const NonHostBipedMotion *carried) {
  NonHostBipedMotion m;
  if (carried)
    m.jump = carried->jump;
  m.body.position = {a.position.x, a.position.y, a.position.z - d.origin_height};
  m.body.velocity = a.velocity;
  m.body.radius = radius;
  m.body.maximum_slope_cosine = maximum_slope_cosine;
  m.body.height = std::max(2 * radius, capsule_segment_height(d.height, d.crouch_height, radius, a.crouch,
                                                              (d.physics_flags & 2) != 0) +
                                           2 * radius);
  m.body.grounded = a.grounded;
  m.crouch = a.crouch;
  m.yaw = a.yaw;
  m.pitch = a.pitch;
  m.landing = a.landing;
  m.facing = a.forward;
  if (a.grounded)
    if (const auto support = collision.raycast(m.body.position + Vec3{0, 0, radius}, {0, 0, -1}, radius + .05f);
        support && support->normal.z >= maximum_slope_cosine) {
      m.body.support_normal = support->normal;
      m.body.support_triangle = support->triangle;
    }
  return m;
}

NonHostMovementPredictor::Settings NonHostMovementPredictor::original_settings() {
  Settings s;
  s.trust_distance = s.snap_distance = 2.5f;
  return s;
}
NonHostMovementPredictor::NonHostMovementPredictor(Stepper step)
    : NonHostMovementPredictor(std::move(step), Settings{}) {}
NonHostMovementPredictor::NonHostMovementPredictor(Stepper step, Settings settings)
    : step_(std::move(step)), settings_(settings) {
  if (!step_)
    throw std::invalid_argument("Non-host movement prediction requires a native stepper");
  if (!(settings_.position_blend > 0 && settings_.position_blend <= 1) ||
      !(settings_.orientation_blend > 0 && settings_.orientation_blend <= 1) ||
      !(settings_.snap_distance > 0) || !(settings_.trust_distance >= 0) || settings_.capacity == 0)
    throw std::invalid_argument("Invalid non-host movement prediction settings");
  history_.reserve(settings_.capacity);
}

void NonHostMovementPredictor::reset(const NonHostBipedMotion &authoritative, std::uint32_t acknowledged,
                                     std::span<const PendingInput> pending) {
  if (!finite(authoritative.body.position) || !finite(authoritative.body.velocity))
    throw std::invalid_argument("Non-finite authoritative biped motion");
  state_ = authoritative;
  acknowledged_ = baseline_ = latest_ = acknowledged;
  history_.clear();
  acknowledged_entry_.reset();
  offset_ = {};
  yaw_offset_ = 0;
  active_ = true;
  // Replay only an unbroken run that starts right after the acknowledgement.
  for (const auto &entry : pending) {
    if (!newer(entry.sequence, latest_))
      continue;
    if (entry.sequence != latest_ + 1)
      break;
    step_(state_, entry.input);
    if (history_.size() == settings_.capacity)
      history_.erase(history_.begin());
    history_.push_back({entry.sequence, entry.input, state_});
    latest_ = entry.sequence;
  }
}

const NonHostBipedMotion &NonHostMovementPredictor::predict(std::uint32_t sequence,
                                                            const NonHostMotionInput &input) {
  if (!active_)
    throw std::logic_error("Non-host movement prediction has no authoritative start");
  if (!newer(sequence, latest_))
    throw std::invalid_argument("Non-host input sequence did not advance");
  latest_ = sequence;
  // One tick of the original correction blend on the shown offset.
  offset_ = offset_ * (1 - settings_.position_blend);
  if (length(offset_) < settings_.settle_distance)
    offset_ = {};
  yaw_offset_ *= 1 - settings_.orientation_blend;
  if (std::abs(yaw_offset_) < settings_.settle_distance)
    yaw_offset_ = 0;
  step_(state_, input);
  if (history_.size() == settings_.capacity)
    history_.erase(history_.begin()); // oldest unacknowledged input; replay restarts later
  history_.push_back({sequence, input, state_});
  return state_;
}

NonHostMovementPredictor::Reconciliation
NonHostMovementPredictor::reconcile(std::uint32_t acknowledged,
                                    const NonHostBipedMotion &authoritative, NonHostCorrection kind) {
  const bool cut = kind == NonHostCorrection::cut;
  Reconciliation result;
  if (!finite(authoritative.body.position) || !finite(authoritative.body.velocity))
    return result; // the original refused non-finite motion; keep predicting
  if (!active_) {
    reset(authoritative, acknowledged);
    result.applied = result.cut = true;
    return result;
  }
  if (newer(acknowledged_, acknowledged)) {
    result.stale = true; // an older snapshot than one already adopted
    return result;
  }
  const auto shown = presented_position();
  const auto shown_yaw = presented_yaw();
  // The prediction of the acknowledged input: still pending, or the last
  // acknowledged one (a host that held the same input for another tick).
  const NonHostBipedMotion *predicted_ack = nullptr;
  const auto previous = std::find_if(history_.begin(), history_.end(),
                                     [&](const Entry &e) { return e.sequence == acknowledged; });
  if (previous != history_.end())
    predicted_ack = &previous->after;
  else if (acknowledged_entry_ && acknowledged_entry_->sequence == acknowledged)
    predicted_ack = &acknowledged_entry_->after;
  const bool known = predicted_ack != nullptr;
  if (known)
    result.history_error = length(authoritative.body.position - predicted_ack->body.position);
  if (previous != history_.end())
    acknowledged_entry_ = *previous;
  // Inputs the host has not consumed yet, in order.
  const auto first = std::find_if(history_.begin(), history_.end(),
                                  [&](const Entry &e) { return newer(e.sequence, acknowledged); });
  const bool agrees = kind == NonHostCorrection::normal && known && settings_.trust_distance > 0 &&
                      result.history_error <= settings_.trust_distance;
  history_.erase(history_.begin(), first);
  acknowledged_ = acknowledged;
  result.applied = true;
  if (agrees) {
    // The host holds (or is adopting) this prediction: keep predicting.
    result.trusted = true;
    return result;
  }
  baseline_ = acknowledged;
  state_ = authoritative;
  if (acknowledged_entry_ && acknowledged_entry_->sequence == acknowledged)
    acknowledged_entry_->after = authoritative;
  // History overflowed past the acknowledged input (more than `capacity`
  // inputs in flight): the replay would skip inputs, so restart instead.
  const bool gap = !history_.empty() && history_.front().sequence != acknowledged + 1;
  if (gap)
    history_.clear();
  for (auto &entry : history_) {
    step_(state_, entry.input);
    entry.after = state_;
    ++result.replayed;
  }
  const auto error = shown - state_.body.position;
  result.error = length(error);
  if (cut || gap || !(result.error <= settings_.snap_distance)) {
    offset_ = {};
    yaw_offset_ = 0;
    result.cut = true;
    return result;
  }
  // Keep the shown position continuous; the offset decays by the original
  // per-tick blend in predict().
  offset_ = error;
  yaw_offset_ = wrap_angle(shown_yaw - state_.yaw);
  return result;
}

const NonHostBipedMotion *NonHostMovementPredictor::prediction(std::uint32_t sequence) const {
  for (const auto &entry : history_)
    if (entry.sequence == sequence)
      return &entry.after;
  if (acknowledged_entry_ && acknowledged_entry_->sequence == sequence)
    return &acknowledged_entry_->after;
  return nullptr;
}
Vec3 NonHostMovementPredictor::presented_position() const { return state_.body.position + offset_; }
float NonHostMovementPredictor::presented_yaw() const { return wrap_angle(state_.yaw + yaw_offset_); }
std::optional<std::uint32_t> NonHostMovementPredictor::latest_sequence() const {
  return active_ ? std::optional(latest_) : std::nullopt;
}
std::optional<std::uint32_t> NonHostMovementPredictor::acknowledged_sequence() const {
  return active_ ? std::optional(acknowledged_) : std::nullopt;
}
NonHostMotionReport NonHostMovementPredictor::report() const {
  NonHostMotionReport r;
  r.sequence = latest_sequence().value_or(0);
  r.baseline = baseline_;
  r.position = state_.body.position;
  r.velocity = state_.body.velocity;
  r.grounded = state_.body.grounded;
  r.crouching = state_.crouch > .5f;
  return r;
}

NonHostAdoption adopt_nonhost_motion(NonHostAdoptionState &state, const NonHostAdoptionLimits &limits,
                                     const NonHostAdoptionInput &in) {
  NonHostAdoption out;
  if (!in.report || !in.host)
    throw std::invalid_argument("Non-host motion adoption requires a report and the host motion");
  const auto &report = *in.report;
  const auto &host = in.host->body;
  out.position = host.position;
  out.velocity = out.taken_velocity = host.velocity;
  if (limits.anti_cheat)
    state.budget = std::min(limits.budget_burst, state.budget + limits.budget_rate * in.dt);
  const auto reject = [&](NonHostRejection why) {
    out.kind = NonHostAdoptionKind::reject;
    out.rejection = why;
    return out;
  };
  // 0xAB9F0's finite checks (forward/up are implied by the controls here).
  if (!finite(report.position) || !finite(report.velocity))
    return reject(NonHostRejection::invalid);
  // 0x82980: a parented (seated) unit does not take client motion.
  if (in.parented)
    return reject(NonHostRejection::parent);
  // A prediction that started before the host's newest forced motion would
  // undo it (teleport, spawn, impulse): wait for the client to rebase.
  if (limits.anti_cheat && state.forced_sequence && newer(*state.forced_sequence, report.baseline))
    return reject(NonHostRejection::stale);
  const auto delta = report.position - host.position;
  out.distance = length(delta);
  if (out.distance > limits.reject_distance)
    return reject(NonHostRejection::distance);
  // Held or skipped inputs (a host that stepped a different number of ticks
  // than the report advanced) legitimately move the two apart by whole steps.
  float allowance = 0;
  if (state.last_sequence && newer(report.sequence, *state.last_sequence)) {
    const auto inputs = report.sequence - *state.last_sequence;
    const auto steps = in.host_steps;
    const auto unmatched = inputs > steps ? inputs - steps : steps - inputs;
    allowance = float(unmatched) * in.maximum_speed * in.dt * 1.1f;
  }
  state.last_sequence = report.sequence;
  const float charge = std::max(0.f, out.distance - allowance);
  if (limits.anti_cheat) {
    const float speed = std::hypot(report.velocity.x, report.velocity.y);
    const float host_speed = std::hypot(host.velocity.x, host.velocity.y);
    if (speed > std::max(in.maximum_speed, host_speed) + limits.speed_tolerance)
      return reject(NonHostRejection::speed);
    if (charge > state.budget)
      return reject(NonHostRejection::budget);
  }
  const bool overlaps = in.collision && in.collision->overlaps(report.position, host.height, host.radius);
  if (out.distance > limits.teleport_distance && !overlaps) {
    if (limits.anti_cheat)
      state.budget -= charge;
    out.kind = NonHostAdoptionKind::teleport;
    out.position = report.position;
    out.velocity = out.taken_velocity = report.velocity;
    return out;
  }
  // B44C0 decision 2 with B4F00's correction toward the reported position.
  const auto dv = report.velocity - host.velocity;
  const bool take = dot(dv, dv) > limits.velocity_deadband_squared;
  if (!take) {
    // Neither velocity was taken: the original writes nothing.
    out.kind = NonHostAdoptionKind::unchanged;
    return out;
  }
  auto correction = delta * limits.nudge_scale;
  const float magnitude = length(correction);
  if (magnitude > limits.nudge_limit)
    correction = correction * (limits.nudge_limit / magnitude);
  if (limits.anti_cheat)
    state.budget -= std::min(charge, length(correction) * in.dt);
  out.kind = NonHostAdoptionKind::nudge;
  out.taken_velocity = report.velocity;
  out.velocity = report.velocity + correction;
  return out;
}
} // namespace halo2
