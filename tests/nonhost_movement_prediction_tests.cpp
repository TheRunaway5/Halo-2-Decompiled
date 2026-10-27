// Deterministic latency harness for non-host movement prediction
// (NETCODE_ENHANCED_GOAL item 1, phase 1). A simulated host steps the
// non-host's biped from delivered inputs and returns snapshots through a
// delayed, optionally jittered and lossy channel; the client predicts with
// the native player mover and reconciles. No sockets, threads or clocks.
#include "halo2/nonhost_movement_prediction.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <memory>
#include <optional>
#include <cstdio>
#include <deque>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
constexpr double tick_ms = 1000.0 / 30.0;
void check(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
CollisionTriangle triangle(Vec3 a, Vec3 b, Vec3 c, std::uint16_t material = 1) {
  CollisionTriangle t{};
  t.a = a;
  t.b = b;
  t.c = c;
  t.material = material;
  t.source_normal = normalized(cross(b - a, c - a));
  return t;
}
void quad(std::vector<CollisionTriangle> &out, Vec3 a, Vec3 b, Vec3 c, Vec3 d) {
  out.push_back(triangle(a, b, c));
  out.push_back(triangle(a, c, d));
}
// Floor, a 20 degree ramp to a raised deck, and a wall to slide along.
std::vector<CollisionTriangle> arena() {
  std::vector<CollisionTriangle> out;
  quad(out, {-20, -20, 0}, {20, -20, 0}, {20, 20, 0}, {-20, 20, 0});
  const float rise = std::tan(20.f * 3.14159265f / 180.f) * 4.f;
  quad(out, {6, -4, 0}, {10, -4, rise}, {10, 4, rise}, {6, 4, 0});
  quad(out, {10, -4, rise}, {16, -4, rise}, {16, 4, rise}, {10, 4, rise});
  quad(out, {-20, 8, 0}, {20, 8, 0}, {20, 8, 3}, {-20, 8, 3});
  return out;
}
// A crate that exists only in the host's world (a player, vehicle or prop
// the client's static collision does not contain).
std::vector<CollisionTriangle> crate(Vec3 low, Vec3 high) {
  std::vector<CollisionTriangle> out;
  const Vec3 p[8]{{low.x, low.y, low.z},  {high.x, low.y, low.z},  {high.x, high.y, low.z},
                  {low.x, high.y, low.z}, {low.x, low.y, high.z},  {high.x, low.y, high.z},
                  {high.x, high.y, high.z}, {low.x, high.y, high.z}};
  quad(out, p[0], p[3], p[2], p[1]);
  quad(out, p[4], p[5], p[6], p[7]);
  quad(out, p[0], p[1], p[5], p[4]);
  quad(out, p[1], p[2], p[6], p[5]);
  quad(out, p[2], p[3], p[7], p[6]);
  quad(out, p[3], p[0], p[4], p[7]);
  return out;
}
// Illustrative multiplayer-like operands; the production predictor reads the
// map's player MovementDefinition and the representation's biped tag.
NonHostMotionDefinition definition() {
  NonHostMotionDefinition d;
  d.movement = {2.25f, 2.f, 2.f, 12.f, 1.f, .9f, .9f, 8.f, 3.f, 1.f};
  d.height = .7f;
  d.crouch_height = .5f;
  d.crouch_velocity = 4.f;
  d.jump_velocity = 1.2f;
  d.landing = {.2f, .5f, 1.2f, 2.f, 3.f};
  d.locomotion_deadzone_bits = std::bit_cast<std::uint32_t>(.01f);
  return d;
}
NonHostBipedMotion spawn(Vec3 at) {
  NonHostBipedMotion m;
  m.body.position = at;
  m.body.radius = .2f;
  m.body.height = .7f;
  m.body.grounded = true;
  return m;
}
// Scripted local input: run, strafe while turning, jump, climb the ramp,
// crouch-walk, slide along the wall, then stand still.
NonHostMotionInput script(unsigned t) {
  NonHostMotionInput in;
  in.yaw = 0;
  if (t < 10)
    return in;
  if (t < 40) {
    in.forward = 1;
  } else if (t < 70) {
    in.forward = .7f;
    in.right = .7f;
    in.yaw = float(t - 40) * .02f;
  } else if (t < 80) {
    in.forward = 1;
    in.jump = t == 72;
    in.yaw = .6f;
  } else if (t < 140) {
    in.forward = 1;
    in.yaw = 0;
  } else if (t < 170) {
    in.forward = -1;
    in.crouch = true;
    in.yaw = 0;
  } else if (t < 230) {
    in.forward = 1;
    in.right = -1;
    in.yaw = 1.2f;
  }
  return in;
}

enum class Host : std::uint8_t {
  authoritative, // phase 1 / today: the host world alone owns the biped
  adopt_original, // phase 2 Original: TU1.5 B41D0/B44C0 rules
  adopt_exact,    // phase 2 Balanced/Full: adopt every in-limit report's position
};
struct Scenario {
  const char *name;
  double up_ms{}, down_ms{}, jitter_ms{};
  unsigned loss_every{};          // drop every Nth snapshot (0: none)
  bool latest_wins{};             // current LanHost held-input policy
  float host_speed_scale = 1, host_acceleration_scale = 1; // TU1.5-vs-native model
  bool host_crate{};              // dynamic object only the host collides with
  unsigned impulse_tick{};        // host-only push (forced motion), 0: none
  unsigned teleport_tick{};       // host cut, 0: none
  Host host = Host::authoritative;
  bool anti_cheat = true;
  float client_speed_scale = 1;   // > 1: a speed-hacked client
  bool expect_exact{};
  float max_visible_correction{}; // normal-movement bound per tick (wu), 0: blend bound only
  bool source_control{};          // predictor operands: 152E80 basis/facing, DF880 dead zone
};
struct Packet {
  double arrive{};
  std::uint32_t sequence{};
  NonHostMotionInput input;
  NonHostMotionReport report;
};
struct Snapshot {
  double arrive{};
  std::uint32_t tick{}, acknowledged{};
  NonHostBipedMotion state;
  bool cut{};
  std::optional<std::uint32_t> forced; // first input whose motion includes a host-forced move
};
struct Result {
  unsigned predicted_first_move{}, snapshot_first_move{};
  unsigned reconciliations{}, trusted{}, cuts{}, max_replayed{};
  unsigned teleports{}, nudges{}, rejected{}, stale{}, speed{}, budget{};
  float max_error{}, max_history_error{}, max_visible_correction{}, max_presented_step{};
  float max_host_speed{}, max_host_report_gap{};
  float final_presented_error{}, final_predicted_error{};
  std::vector<Vec3> presented;
};
float horizontal(Vec3 v) { return std::hypot(v.x, v.y); }
// Deterministic bounded jitter.
double jitter(unsigned n, double amount) {
  if (amount == 0)
    return 0;
  const unsigned h = (n * 2654435761u) >> 16;
  return (double(h % 2001) / 1000.0 - 1.0) * amount;
}
bool newer(std::uint32_t a, std::uint32_t b) { return std::int32_t(a - b) > 0; }

NonHostMovementPredictor::Settings settings_for(Host host) {
  if (host == Host::adopt_original)
    return NonHostMovementPredictor::original_settings();
  NonHostMovementPredictor::Settings settings;
  // Exact adoption: trust differences of a few mover steps (a host that held
  // or skipped an input), replay anything larger.
  if (host == Host::adopt_exact)
    settings.trust_distance = .25f;
  return settings;
}
Result run(const Scenario &s) {
  auto d = definition();
  d.source_control = d.source_facing = s.source_control;
  const auto client_world = std::make_shared<const CollisionWorld>(arena());
  auto host_triangles = arena();
  if (s.host_crate) {
    const auto box = crate({3.6f, -.8f, 0}, {4.2f, .8f, .6f});
    host_triangles.insert(host_triangles.end(), box.begin(), box.end());
  }
  const CollisionWorld host_world(host_triangles);
  auto host_definition = d;
  host_definition.movement.forward *= s.host_speed_scale;
  host_definition.movement.backward *= s.host_speed_scale;
  host_definition.movement.sideways *= s.host_speed_scale;
  host_definition.movement.acceleration *= s.host_acceleration_scale;
  host_definition.movement.crouch_acceleration *= s.host_acceleration_scale;
  auto client_definition = d;
  client_definition.movement.forward *= s.client_speed_scale;
  client_definition.movement.backward *= s.client_speed_scale;
  client_definition.movement.sideways *= s.client_speed_scale;

  const bool adopting = s.host != Host::authoritative;
  NonHostMovementPredictor predictor(
      [&](NonHostBipedMotion &m, const NonHostMotionInput &in) {
        step_native_player_motion(m, client_definition, *client_world, in);
      },
      settings_for(s.host));
  NonHostAdoptionLimits limits;
  limits.anti_cheat = s.anti_cheat;
  if (s.host == Host::adopt_exact)
    limits.teleport_distance = 0;
  NonHostAdoptionState adoption;
  const float maximum_speed = std::max({d.movement.forward, d.movement.backward, d.movement.sideways});

  const auto start = spawn({0, 0, 0});
  NonHostBipedMotion host = start;
  predictor.reset(start, 0);
  std::deque<Packet> uplink;
  std::deque<Snapshot> downlink;
  std::vector<Packet> received; // host-side, ascending sequence
  std::uint32_t consumed = 0;
  NonHostMotionInput held;
  std::optional<NonHostMotionReport> held_report;
  std::optional<std::uint32_t> forced;
  unsigned host_steps = 0;
  NonHostBipedMotion snapshot_only = start; // what the client draws today
  Result r;
  // 230 scripted ticks, then 160 idle ticks for every snapshot to arrive
  // and the presentation offset to settle.
  constexpr unsigned ticks = 390;
  Vec3 previous_presented = predictor.presented_position();
  unsigned sent_snapshots = 0;
  for (unsigned t = 1; t <= ticks; ++t) {
    const double now = t * tick_ms;
    // Client: adopt every snapshot that has arrived, in arrival order.
    while (!downlink.empty() && downlink.front().arrive <= now) {
      const auto snap = downlink.front();
      downlink.pop_front();
      auto kind = NonHostCorrection::normal;
      if (snap.cut)
        kind = NonHostCorrection::cut;
      else if (snap.forced && newer(*snap.forced, predictor.baseline_sequence()))
        kind = NonHostCorrection::forced;
      const auto rec = predictor.reconcile(snap.acknowledged, snap.state, kind);
      if (rec.applied) {
        ++r.reconciliations;
        r.trusted += rec.trusted;
        r.cuts += rec.cut;
        r.max_replayed = std::max(r.max_replayed, rec.replayed);
        if (!rec.cut) {
          r.max_error = std::max(r.max_error, rec.error);
          r.max_history_error = std::max(r.max_history_error, rec.history_error);
        }
        snapshot_only = snap.state;
      }
      if (!r.snapshot_first_move && horizontal(snapshot_only.body.position - start.body.position) > 1e-3f)
        r.snapshot_first_move = t;
    }
    // Client input tick: predict at once, then send the input with its motion.
    const auto input = script(t);
    const auto before = predictor.position_offset();
    const auto presented_before = predictor.presented_position();
    if (t > 1)
      check(length(presented_before - previous_presented) < 1e-5f || r.cuts,
            std::string(s.name) + ": reconciliation moved the shown position outside a cut");
    predictor.predict(t, input);
    const auto after = predictor.position_offset();
    const auto presented = predictor.presented_position();
    r.max_visible_correction = std::max(r.max_visible_correction, length(after - before));
    if (!(s.teleport_tick && t >= s.teleport_tick))
      r.max_presented_step = std::max(r.max_presented_step, length(presented - previous_presented));
    if (!r.predicted_first_move && horizontal(presented - start.body.position) > 1e-3f)
      r.predicted_first_move = t;
    previous_presented = presented;
    r.presented.push_back(presented);
    uplink.push_back({now + s.up_ms + jitter(t, s.jitter_ms), t, input, predictor.report()});

    // Host tick, 10 ms after the client's in this lockstep clock.
    const double host_now = now + 10;
    for (auto at = uplink.begin(); at != uplink.end();) {
      if (at->arrive <= host_now) {
        received.push_back(*at);
        at = uplink.erase(at);
      } else
        ++at;
    }
    std::sort(received.begin(), received.end(),
              [](const Packet &a, const Packet &b) { return a.sequence < b.sequence; });
    held_report.reset();
    if (s.latest_wins) {
      // LanHost today: the newest delivered controls are held until replaced.
      if (!received.empty() && received.back().sequence > consumed) {
        held = received.back().input;
        held_report = received.back().report;
        consumed = received.back().sequence;
      }
      received.clear();
    } else {
      // In-order consumption: one input per tick when present.
      std::erase_if(received, [&](const Packet &p) { return p.sequence <= consumed; });
      if (!received.empty() && received.front().sequence == consumed + 1) {
        held = received.front().input;
        held_report = received.front().report;
        consumed = received.front().sequence;
        received.erase(received.begin());
      }
    }
    if (consumed)
      step_native_player_motion(host, host_definition, host_world, held);
    ++host_steps;
    if (adopting && held_report) {
      const NonHostAdoptionInput in{&*held_report, &host, false, maximum_speed, &host_world,
                                    1.f / 30.f, host_steps};
      host_steps = 0;
      const auto a = adopt_nonhost_motion(adoption, limits, in);
      switch (a.kind) {
      case NonHostAdoptionKind::teleport:
        ++r.teleports;
        host.body.position = a.position;
        host.body.velocity = a.velocity;
        host.body.grounded = held_report->grounded;
        break;
      case NonHostAdoptionKind::nudge:
        ++r.nudges;
        host.body.velocity = a.velocity;
        break;
      case NonHostAdoptionKind::unchanged:
        break;
      case NonHostAdoptionKind::reject:
        ++r.rejected;
        r.stale += a.rejection == NonHostRejection::stale;
        r.speed += a.rejection == NonHostRejection::speed;
        r.budget += a.rejection == NonHostRejection::budget;
        break;
      }
      r.max_host_report_gap = std::max(r.max_host_report_gap, length(host.body.position - held_report->position));
    }
    bool cut = false;
    if (s.impulse_tick && t == s.impulse_tick) {
      host.body.velocity = host.body.velocity + Vec3{0, 2.5f, 1.f};
      host.body.grounded = false;
      forced = consumed + 1;
    }
    if (s.teleport_tick && t == s.teleport_tick) {
      host.body.position = host.body.position + Vec3{-12, -6, 0};
      host.body.velocity = {};
      forced = consumed + 1;
      cut = true;
    }
    adoption.forced_sequence = forced;
    if (!s.impulse_tick || t < s.impulse_tick || t > s.impulse_tick + 30)
      r.max_host_speed = std::max(r.max_host_speed, horizontal(host.body.velocity));
    ++sent_snapshots;
    const bool lost = s.loss_every && sent_snapshots % s.loss_every == 0 && t < 300;
    if (!lost)
      downlink.push_back({host_now + s.down_ms + jitter(t + 7919, s.jitter_ms), t, consumed, host, cut, forced});
    std::stable_sort(downlink.begin(), downlink.end(),
                     [](const Snapshot &a, const Snapshot &b) { return a.arrive < b.arrive; });
  }
  r.final_presented_error = length(predictor.presented_position() - host.body.position);
  r.final_predicted_error = length(predictor.predicted().body.position - host.body.position);
  return r;
}

void report(const Scenario &s, const Result &r) {
  std::printf("%-30s rtt=%3.0fms move:+%u/+%u reconcile=%u trusted=%u cut=%u replay<=%u "
              "err=%.4f hist=%.4f vis/tick=%.4f step=%.4f final=%.1e",
              s.name, s.up_ms + s.down_ms, r.predicted_first_move - 10, r.snapshot_first_move - 10,
              r.reconciliations, r.trusted, r.cuts, r.max_replayed, r.max_error, r.max_history_error,
              r.max_visible_correction, r.max_presented_step, r.final_presented_error);
  if (s.host != Host::authoritative)
    std::printf(" | host teleport=%u nudge=%u reject=%u (stale=%u speed=%u budget=%u) gap=%.4f vmax=%.3f",
                r.teleports, r.nudges, r.rejected, r.stale, r.speed, r.budget, r.max_host_report_gap,
                r.max_host_speed);
  std::printf("\n");
}
} // namespace

// The pre-refactor Simulation::move_unit body (git HEAD), verbatim apart from
// its members becoming parameters, against the shared native mover.
struct LegacyUnit {
  Body body;
  float crouch{}, yaw{};
  JumpState jump;
  SourceLandingState landing;
  BipedDefinition definition;
};
void legacy_move_unit(LegacyUnit &u, const MovementDefinition &movement, const CollisionWorld &world,
                      std::optional<std::uint32_t> deadzone, Vec3 desired, bool jump, bool crouch,
                      bool player_control) {
  constexpr float tick_seconds = 1.f / 30.f, retail_gravity = 4.1712594f;
  const auto collision = [&]() -> const CollisionWorld & { return world; };
  const bool landing_enabled=deadzone.has_value();
  if(landing_enabled) {
    const float magnitude=desired.x*desired.x+desired.y*desired.y+desired.z*desired.z;
    advance_source_landing(u.landing,magnitude>std::bit_cast<float>(*deadzone));
  }
  const auto move_and_land=[&] {
    const bool supported=u.body.grounded;
    collision().move(u.body,tick_seconds);
    if(landing_enabled && !supported && u.body.grounded && u.body.landing_speed>0) {
      bool force_hard=false;
      begin_source_landing(u.landing,u.definition.landing,u.body.landing_speed,30,force_hard);
    }
  };
  const float proposed =
      u.crouch + crouch_delta(u.crouch, tick_seconds,
                              u.definition.crouch_velocity, crouch, false);
  const float height =
      capsule_segment_height(u.definition.height, u.definition.crouch_height,
                             u.body.radius, proposed,
                             (u.definition.physics_flags & 2) != 0) +
      2 * u.body.radius;
  if (proposed >= u.crouch ||
      !collision().overlaps(u.body.position, height, u.body.radius)) {
    u.crouch = proposed;
    u.body.height = std::max(2 * u.body.radius, height);
  }
  if (player_control && !(u.definition.physics_flags & 8)) {
    const auto intent = movement_intent(movement, desired, u.crouch,
                                        false, !u.body.grounded);
    if (jump_gate(u.jump, jump, true, !u.body.grounded, u.landing.kind==1, u.body.grounded,
                  tick_seconds, 30)) {
      u.body.velocity = jump_impulse(u.body.velocity, {0, 0, 1}, {},
                                     u.definition.jump_velocity);
      u.body.grounded = false;
    }
    WalkingInput input;
    input.input = intent.velocity;
    input.current = u.body.velocity;
    input.facing = input.basis = direction(u.yaw);
    input.normal = u.body.support_normal;
    input.slope = u.definition.slope;
    input.acceleration = intent.acceleration;
    input.air_acceleration = intent.air_acceleration;
    input.gravity = retail_gravity;
    input.dt = tick_seconds;
    input.flags = 8 | (u.body.grounded ? 0 : 4);
    u.body.velocity = walking_velocity(input).velocity;
    move_and_land();
    return;
  }
  const auto delta = desired - Vec3{u.body.velocity.x, u.body.velocity.y, 0};
  const auto acceleration =
      u.body.grounded ? (u.crouch > .5f ? movement.crouch_acceleration
                                        : movement.acceleration)
                      : movement.air_acceleration;
  const auto change =
      normalized(delta) * std::min(length(delta), acceleration * tick_seconds);
  u.body.velocity.x += change.x;
  u.body.velocity.y += change.y;
  if (jump && u.body.grounded)
    u.body.velocity.z = u.definition.jump_velocity;
  u.body.velocity.z -= retail_gravity * tick_seconds;
  move_and_land();
}
bool same_bits(float a, float b) { return std::bit_cast<std::uint32_t>(a) == std::bit_cast<std::uint32_t>(b); }
bool same_bits(Vec3 a, Vec3 b) { return same_bits(a.x, b.x) && same_bits(a.y, b.y) && same_bits(a.z, b.z); }
// Every tick of a long random control sequence (player and steering
// branches, both physics-flag cases, crouch, jump, ramp/wall contacts) must
// leave bit-identical state through both implementations.
void move_unit_refactor_checks() {
  const auto world = std::make_shared<const CollisionWorld>(arena());
  const auto d = definition();
  for (const std::uint32_t flags : {0u, 2u, 8u}) {
    BipedDefinition biped;
    biped.height = d.height;
    biped.crouch_height = d.crouch_height;
    biped.crouch_velocity = d.crouch_velocity;
    biped.jump_velocity = d.jump_velocity;
    biped.physics_flags = flags;
    biped.landing = d.landing;
    biped.slope = {1.5f, .5f, -.707106769f, -.2f, .3f, .766044438f};
    const auto shared = native_motion_definition(d.movement, biped, d.locomotion_deadzone_bits);
    LegacyUnit legacy;
    legacy.body = spawn({0, 0, 0}).body;
    legacy.definition = biped;
    NonHostBipedMotion motion;
    motion.body = legacy.body;
    std::uint32_t random = 0x2468ace1u ^ flags;
    const auto next = [&] {
      random = random * 1664525u + 1013904223u;
      return float(random >> 8) / float(1u << 24);
    };
    for (unsigned t = 0; t < 3000; ++t) {
      const Vec3 desired{next() * 2.4f - 1.2f, next() * 2.4f - 1.2f, next() < .1f ? next() - .5f : 0.f};
      const bool jump = next() < .05f, crouch = next() < .2f, player = next() < .8f;
      legacy.yaw = motion.yaw = next() * 6.2831853f;
      legacy_move_unit(legacy, d.movement, *world, d.locomotion_deadzone_bits, desired, jump, crouch, player);
      step_native_unit_motion(motion, shared, *world, desired, jump, crouch, player);
      check(same_bits(legacy.body.position, motion.body.position) &&
                same_bits(legacy.body.velocity, motion.body.velocity) &&
                same_bits(legacy.body.height, motion.body.height) && legacy.body.grounded == motion.body.grounded &&
                same_bits(legacy.body.support_normal, motion.body.support_normal) &&
                legacy.body.support_triangle == motion.body.support_triangle &&
                same_bits(legacy.body.landing_speed, motion.body.landing_speed) &&
                same_bits(legacy.crouch, motion.crouch) && legacy.jump.since_jump == motion.jump.since_jump &&
                legacy.jump.held_ticks == motion.jump.held_ticks && legacy.landing.kind == motion.landing.kind &&
                legacy.landing.elapsed == motion.landing.elapsed &&
                legacy.landing.duration == motion.landing.duration,
            "move_unit refactor changed native mover state at tick " + std::to_string(t));
      if (length(motion.body.position) > 15) // stay inside the arena
        legacy.body.position = motion.body.position = {0, 0, .2f};
    }
  }
  std::puts("move_unit refactor checks passed (3 x 3000 ticks bit-identical)");
}

// Direct checks of the edge rules and of the adoption kernel's original
// thresholds (TU1.5 B41D0 mode 2: 3.0 reject, 0.2 teleport; B44C0: 0.06 dead
// band; B4F00: 0.9 * dpos capped at 0.05).
void edge_checks() {
  const auto world = std::make_shared<const CollisionWorld>(arena());
  const auto d = definition();
  const auto stepper = [&](NonHostBipedMotion &m, const NonHostMotionInput &in) {
    step_native_player_motion(m, d, *world, in);
  };
  NonHostMovementPredictor::Settings small;
  small.capacity = 4;
  NonHostMovementPredictor p(stepper, small);
  const auto start = spawn({0, 0, 0});
  p.reset(start, 0);
  NonHostMotionInput run;
  run.forward = 1;
  for (std::uint32_t t = 1; t <= 6; ++t)
    p.predict(t, run);
  check(p.pending() == 4, "edge: history capacity not enforced");
  auto bad = start;
  bad.body.position.x = std::nanf("");
  check(!p.reconcile(1, bad).applied, "edge: non-finite motion adopted");
  // Inputs 1 and 2 fell out of the bounded history: restart, do not replay a gap.
  const auto gap = p.reconcile(1, start);
  check(gap.applied && gap.cut && p.pending() == 0, "edge: replay across a history gap");
  check(p.reconcile(0, start).stale, "edge: older snapshot adopted");
  bool threw = false;
  try {
    p.predict(6, run);
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  check(threw, "edge: repeated input sequence accepted");

  NonHostAdoptionLimits limits;
  NonHostAdoptionState state;
  NonHostBipedMotion host = spawn({0, 0, 0});
  NonHostMotionReport report;
  report.sequence = 10;
  report.baseline = 8;
  const auto adopt = [&](Vec3 position, Vec3 velocity, bool parented = false) {
    report.position = position;
    report.velocity = velocity;
    ++report.sequence;
    state.budget = limits.budget_burst;
    return adopt_nonhost_motion(state, limits, {&report, &host, parented, 2.25f, world.get()});
  };
  check(adopt({.1f, 0, 0}, {}, true).rejection == NonHostRejection::parent, "adopt: seated unit took motion");
  check(adopt({3.01f, 0, 0}, {}).rejection == NonHostRejection::distance, "adopt: 3.0 wu limit");
  limits.anti_cheat = false;
  const auto far = adopt({2.9f, 0, 0}, {1, 0, 0});
  check(far.kind == NonHostAdoptionKind::teleport && far.position.x == 2.9f && far.velocity.x == 1,
        "adopt: in-limit move did not teleport to the reported pose");
  check(adopt({.15f, 0, 0}, {.2f, 0, 0}).kind == NonHostAdoptionKind::unchanged,
        "adopt: velocity inside the 0.06 dead band was written");
  const auto nudge = adopt({.15f, 0, 0}, {1, 0, 0});
  check(nudge.kind == NonHostAdoptionKind::nudge && std::abs(nudge.velocity.x - 1.05f) < 1e-6f &&
            nudge.position.x == 0,
        "adopt: nudge did not add the capped 0.9 * dpos correction");
  // A reported capsule inside the wall is nudged, never teleported (B41D0's
  // capsule overlap test).
  host.body.position = {0, 7.5f, 0};
  check(adopt({0, 7.9f, 0}, {0, 1, 0}).kind == NonHostAdoptionKind::nudge, "adopt: teleported into a wall");
  host.body.position = {};
  limits.anti_cheat = true;
  state.forced_sequence = 9;
  check(adopt({.5f, 0, 0}, {}).rejection == NonHostRejection::stale, "adopt: pre-impulse prediction adopted");
  report.baseline = 9;
  check(adopt({.5f, 0, 0}, {}).kind == NonHostAdoptionKind::teleport, "adopt: rebased prediction refused");
  check(adopt({.5f, 0, 0}, {3.5f, 0, 0}).rejection == NonHostRejection::speed, "adopt: speed bound");
  std::puts("edge checks passed");
}

int main() try {
  move_unit_refactor_checks();
  edge_checks();
  using H = Host;
  // Input begins at tick 10; "move:+a/+b" is the first tick the shown own
  // position moved (prediction) and the first tick a snapshot showed it
  // moving (today's client), relative to the input tick.
  std::vector<Scenario> scenarios;
  const auto add = [&](Scenario s) { scenarios.push_back(s); };
  // Phase 1: the host world alone owns the biped (today's host).
  add({.name = "parity_100ms", .up_ms = 50, .down_ms = 50, .expect_exact = true});
  add({.name = "parity_150ms", .up_ms = 75, .down_ms = 75, .expect_exact = true});
  add({.name = "parity_150ms_latest", .up_ms = 75, .down_ms = 75, .latest_wins = true, .expect_exact = true});
  add({.name = "parity_150ms_source_control", .up_ms = 75, .down_ms = 75, .expect_exact = true,
       .source_control = true});
  add({.name = "divergent_100ms", .up_ms = 50, .down_ms = 50, .host_speed_scale = 1.04f,
       .host_acceleration_scale = 1.15f, .max_visible_correction = .01f});
  add({.name = "divergent_150ms", .up_ms = 75, .down_ms = 75, .host_speed_scale = 1.04f,
       .host_acceleration_scale = 1.15f, .max_visible_correction = .01f});
  add({.name = "divergent_150ms_jitter", .up_ms = 75, .down_ms = 75, .jitter_ms = 12,
       .host_speed_scale = 1.04f, .host_acceleration_scale = 1.15f, .max_visible_correction = .01f});
  add({.name = "divergent_150ms_jitter_latest", .up_ms = 75, .down_ms = 75, .jitter_ms = 12,
       .latest_wins = true, .host_speed_scale = 1.04f, .host_acceleration_scale = 1.15f});
  add({.name = "divergent_150ms_loss_latest", .up_ms = 75, .down_ms = 75, .jitter_ms = 12,
       .loss_every = 7, .latest_wins = true, .host_speed_scale = 1.04f, .host_acceleration_scale = 1.15f});
  add({.name = "host_crate_impulse_150ms", .up_ms = 75, .down_ms = 75, .host_crate = true,
       .impulse_tick = 120});
  add({.name = "host_teleport_150ms", .up_ms = 75, .down_ms = 75, .teleport_tick = 200});
  // Phase 2 preview: the host adopts reported motion (decision kernel only).
  add({.name = "adopt_original_jitter_latest", .up_ms = 75, .down_ms = 75, .jitter_ms = 12,
       .latest_wins = true, .host_speed_scale = 1.04f, .host_acceleration_scale = 1.15f,
       .host = H::adopt_original});
  add({.name = "adopt_exact_jitter_latest", .up_ms = 75, .down_ms = 75, .jitter_ms = 12,
       .latest_wins = true, .host_speed_scale = 1.04f, .host_acceleration_scale = 1.15f,
       .host = H::adopt_exact});
  add({.name = "adopt_original_impulse", .up_ms = 75, .down_ms = 75, .impulse_tick = 120,
       .host = H::adopt_original});
  add({.name = "adopt_original_teleport", .up_ms = 75, .down_ms = 75, .teleport_tick = 200,
       .host = H::adopt_original});
  add({.name = "adopt_speed_hack_unchecked", .up_ms = 75, .down_ms = 75, .host = H::adopt_original,
       .anti_cheat = false, .client_speed_scale = 1.5f});
  add({.name = "adopt_speed_hack_checked", .up_ms = 75, .down_ms = 75, .host = H::adopt_original,
       .client_speed_scale = 1.5f});
  for (const auto &s : scenarios) {
    const auto r = run(s);
    report(s, r);
    const auto again = run(s);
    check(again.presented.size() == r.presented.size(), std::string(s.name) + ": nondeterministic length");
    for (std::size_t i = 0; i < r.presented.size(); ++i)
      check(std::bit_cast<std::uint32_t>(again.presented[i].x) == std::bit_cast<std::uint32_t>(r.presented[i].x) &&
                std::bit_cast<std::uint32_t>(again.presented[i].y) == std::bit_cast<std::uint32_t>(r.presented[i].y) &&
                std::bit_cast<std::uint32_t>(again.presented[i].z) == std::bit_cast<std::uint32_t>(r.presented[i].z),
            std::string(s.name) + ": nondeterministic presentation");
    const std::string name = s.name;
    // The local biped answers on the input tick, a round trip before the
    // first snapshot that contains the input.
    check(r.predicted_first_move == 10, name + ": prediction did not respond on the input tick");
    check(r.snapshot_first_move >= r.predicted_first_move + 3, name + ": snapshot path unexpectedly fast");
    // Every shown correction is spread by the original 45% blend.
    check(r.max_visible_correction <= .45f * r.max_error + 1e-5f, name + ": correction exceeded the original blend");
    const bool host_event = s.host_crate || s.impulse_tick || s.teleport_tick;
    if (s.client_speed_scale > 1) {
      if (s.anti_cheat) {
        check(r.speed > 0 && r.max_host_speed <= 2.25f * 1.01f, name + ": host adopted a speed hack");
        check(r.cuts > 0, name + ": hacked client was not pulled back");
      } else
        check(r.max_host_speed > 3.f, name + ": original rules should follow the reported motion");
      continue;
    }
    if (s.host == Host::adopt_original) {
      // TU1.5 B41D0/B44C0: a gap at or below 0.2 wu is left to the velocity
      // nudge, so the host may rest up to 0.2 wu from the client's own view.
      check(r.final_presented_error <= .2f + 1e-4f, name + ": did not stay within the original adoption band");
      check(r.max_host_report_gap <= .2f + 1e-4f || host_event, name + ": host left the adoption band");
    } else
      check(r.final_presented_error < 1e-4f && r.final_predicted_error < 1e-4f,
            name + ": did not converge to the host state");
    if (s.host == Host::adopt_exact)
      check(r.max_host_report_gap < 1e-4f && r.max_error < 1e-4f, name + ": exact adoption left a gap");
    // No snaps: in normal movement a shown step never exceeds the fastest
    // scripted mover step (about 0.08 wu/tick) plus the visible correction.
    if (!host_event)
      check(r.max_presented_step <= .0825f + (s.max_visible_correction > 0 ? s.max_visible_correction
                                                                            : .45f * r.max_error),
            name + ": shown position snapped");
    if (s.expect_exact)
      check(r.max_error == 0 && r.max_history_error == 0 && r.max_visible_correction == 0,
            name + ": parity mover produced a correction");
    if (s.max_visible_correction > 0)
      check(r.max_visible_correction <= s.max_visible_correction, name + ": visible correction per tick too large");
    if (s.teleport_tick)
      check(r.cuts >= 1, name + ": teleport was blended, not cut");
    else
      check(r.cuts == 0, name + ": unexpected cut");
    if (s.impulse_tick && s.host != Host::authoritative)
      check(r.stale > 0 && r.cuts == 0, name + ": stale reports were not refused after a forced move");
  }
  std::puts("nonhost movement prediction: all scenarios passed");
  return 0;
} catch (const std::exception &error) {
  std::fprintf(stderr, "nonhost movement prediction failed: %s\n", error.what());
  return 1;
}
