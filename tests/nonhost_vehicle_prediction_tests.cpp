// Non-host vehicle prediction (phase 1 prototype): deterministic checks of
// the orientation helpers, force-free extrapolation, replay under injected
// latency, the 2004 45%/35% correction blend, the 7.5 wu vehicle snap, the
// trust band and the history rules. Self-contained; no assets.
#include "halo2/math.hpp"
#include "halo2/nonhost_vehicle_prediction.hpp"
#include <cmath>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace halo2;
namespace {
void check(bool value, const std::string &message) {
  if (!value)
    throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance) { return std::abs(a - b) <= tolerance; }
bool near(Vec3 a, Vec3 b, float tolerance) { return length(a - b) <= tolerance; }
constexpr float dt = 1.f / 30.f;

// A deterministic input-driven toy vehicle (both the "host" and the stepper
// in the agreement cases): throttle accelerates along the heading, `right`
// sets a yaw rate, and the velocity is damped.
void toy(NonHostVehicleMotion &m, const NonHostVehicleInput &in) {
  const Vec3 heading = normalized(Vec3{m.forward.x, m.forward.y, 0});
  m.linear = m.linear * .98f + heading * (in.forward * 9.f * dt);
  m.angular = {0, 0, -in.right * 1.5f};
  step_nonhost_vehicle_extrapolation(m, dt);
}
NonHostVehicleInput input_at(std::uint32_t tick) {
  NonHostVehicleInput in;
  in.forward = tick % 90 < 60 ? 1.f : -.5f;
  in.right = tick % 40 < 15 ? .8f : tick % 40 < 25 ? 0.f : -.6f;
  return in;
}
NonHostVehicleMotion start() {
  NonHostVehicleMotion m;
  m.position = {10, -4, 2};
  m.forward = normalized(Vec3{1, .3f, 0});
  m.up = {0, 0, 1};
  return m;
}

void orientation_helpers() {
  const Vec3 f = normalized(Vec3{.3f, .9f, .1f});
  const Vec3 u = normalized(cross(f, cross(Vec3{0, 0, 1}, f)));
  const auto q = nonhost_vehicle_orientation(f, u);
  Vec3 f2, u2;
  nonhost_vehicle_basis(q, f2, u2);
  check(near(f, f2, 1e-5f) && near(u, u2, 1e-5f), "Quaternion round trip lost the basis");
  check(near(nonhost_vehicle_angle(q, q), 0.f, 2e-3f), "Identical orientations differ");
  const auto yaw90 = nonhost_vehicle_orientation({0, 1, 0}, {0, 0, 1});
  check(near(nonhost_vehicle_angle(nonhost_vehicle_orientation({1, 0, 0}, {0, 0, 1}), yaw90), 1.5707963f, 1e-4f),
        "A quarter turn did not measure pi/2");
  // A9B40: weight follows the dot sign, so q and -q blend alike.
  const NonHostQuaternion identity{0, 0, 0, 1}, flipped{-yaw90[0], -yaw90[1], -yaw90[2], -yaw90[3]};
  const auto a = nonhost_vehicle_blend(yaw90, identity, .35f), b = nonhost_vehicle_blend(flipped, identity, .35f);
  check(near(nonhost_vehicle_angle(a, b), 0.f, 2e-3f), "The 2004 blend depends on quaternion sign");
  // normalize(0.65*q + 0.35*1) for a quarter turn about z.
  const float s = std::sqrt(.5f), x = .65f * s, w = .65f * s + .35f, n = std::sqrt(x * x + w * w);
  const float expected = 2.f * std::atan2(x / n, w / n);
  check(near(nonhost_vehicle_angle(a, identity), expected, 1e-4f), "The 2004 orientation blend weight changed");
}

void extrapolation() {
  NonHostVehicleMotion m = start();
  m.linear = {6, -2, .5f};
  m.angular = {0, 0, .9f};
  const auto p0 = m.position;
  const auto q0 = nonhost_vehicle_orientation(m.forward, m.up);
  for (unsigned i = 0; i < 30; ++i)
    step_nonhost_vehicle_extrapolation(m, dt);
  check(near(m.position, p0 + Vec3{6, -2, .5f}, 1e-4f), "Extrapolated position is not p + v t");
  check(near(nonhost_vehicle_angle(q0, nonhost_vehicle_orientation(m.forward, m.up)), .9f, 1e-3f),
        "Extrapolated yaw is not w t");
  check(near(m.linear, Vec3{6, -2, .5f}, 0) && near(m.angular, Vec3{0, 0, .9f}, 0), "Extrapolation changed velocity");
  check(near(length(m.forward), 1, 1e-5f) && near(length(m.up), 1, 1e-5f) && near(dot(m.forward, m.up), 0, 1e-5f),
        "Extrapolated basis is not orthonormal");
}

// Host truth and client prediction under a fixed round trip: each snapshot
// names the input it consumed `latency` ticks ago.
struct Run {
  NonHostVehiclePredictor predictor;
  std::deque<std::pair<std::uint32_t, NonHostVehicleMotion>> snapshots;
  NonHostVehicleMotion host;
  float largest_error{}, largest_shown_step{};
  unsigned replays{}, cuts{}, trusted{};
  explicit Run(NonHostVehiclePredictor::Settings settings) : predictor(toy, settings), host(start()) {
    predictor.reset(host, 0);
  }
  // Host consumes input `tick` (with an optional disturbance), the client
  // predicts it, and the snapshot of input `tick - latency` arrives.
  void tick(std::uint32_t tick, unsigned latency, Vec3 push = {}, float twist = 0) {
    const auto before = predictor.presented().position;
    const auto in = input_at(tick);
    predictor.predict(tick, in);
    toy(host, in);
    host.position = host.position + push;
    if (twist != 0) {
      const auto q = nonhost_vehicle_orientation(host.forward, host.up);
      const NonHostQuaternion r{0, 0, std::sin(twist * .5f), std::cos(twist * .5f)};
      const NonHostQuaternion out{r[3] * q[0] + r[0] * q[3] + r[1] * q[2] - r[2] * q[1],
                                  r[3] * q[1] - r[0] * q[2] + r[1] * q[3] + r[2] * q[0],
                                  r[3] * q[2] + r[0] * q[1] - r[1] * q[0] + r[2] * q[3],
                                  r[3] * q[3] - r[0] * q[0] - r[1] * q[1] - r[2] * q[2]};
      nonhost_vehicle_basis(out, host.forward, host.up);
    }
    snapshots.emplace_back(tick, host);
    if (snapshots.size() > latency) {
      const auto [ack, motion] = snapshots.front();
      snapshots.pop_front();
      const auto result = predictor.reconcile(ack, motion);
      replays += result.replayed > 0;
      cuts += result.cut;
      trusted += result.trusted;
    }
    largest_shown_step = std::max(largest_shown_step, length(predictor.presented().position - before));
    largest_error = std::max(largest_error, length(predictor.predicted().position - host.position));
  }
};

void agreement_under_latency() {
  Run run({});
  for (std::uint32_t t = 1; t <= 300; ++t)
    run.tick(t, 5);
  // Same stepper as the host: the present is exact on the input tick and
  // every replay reproduces the history.
  check(run.largest_error < 1e-4f, "Agreeing stepper did not predict the host present");
  check(length(run.predictor.position_offset()) == 0 && run.predictor.orientation_offset() == 0,
        "Agreeing replay left a shown offset");
  check(run.cuts == 0 && run.replays > 250, "Agreeing run cut or skipped replays");
  check(run.predictor.pending() == 5, "Prediction is not one round trip ahead");
  const auto report = run.predictor.report();
  check(report.sequence == 300 && report.baseline == 295, "Report names the wrong sequence or baseline");
}

void correction_blend() {
  Run run({});
  for (std::uint32_t t = 1; t <= 60; ++t)
    run.tick(t, 4);
  // A 1.2 wu push and a 0.4 rad twist on the host at input 61.
  run.tick(61, 4, {1.2f, 0, 0}, .4f);
  for (std::uint32_t t = 62; t <= 65; ++t)
    run.tick(t, 4);
  // Snapshot of input 61 arrives on tick 65: offset = shown - replayed.
  const float offset = length(run.predictor.position_offset());
  const float twist = run.predictor.orientation_offset();
  check(near(offset, 1.2f, .02f), "Shown position was not kept continuous across the correction");
  check(near(twist, .4f, .01f), "Shown orientation was not kept continuous across the correction");
  run.tick(66, 4);
  check(near(length(run.predictor.position_offset()), offset * .55f, 1e-4f), "Position offset did not decay 45%");
  // One A9B40 step toward identity.
  const NonHostQuaternion q{0, 0, std::sin(twist * .5f), std::cos(twist * .5f)};
  const auto once = nonhost_vehicle_angle(nonhost_vehicle_blend(q, {0, 0, 0, 1}, .35f), {0, 0, 0, 1});
  check(near(run.predictor.orientation_offset(), once, 1e-3f), "Orientation offset did not take the 35% blend");
  for (std::uint32_t t = 67; t <= 110; ++t)
    run.tick(t, 4);
  check(length(run.predictor.position_offset()) == 0 && run.predictor.orientation_offset() == 0,
        "Offsets did not settle");
  check(run.largest_shown_step < 1.f, "A blended correction jumped");
  check(near(run.predictor.predicted().position, run.host.position, 1e-4f), "Replay did not adopt the host");
}

void snap_and_trust() {
  {
    Run run({});
    for (std::uint32_t t = 1; t <= 40; ++t)
      run.tick(t, 3);
    run.tick(41, 3, {8.f, 0, 0}); // beyond 7.5 wu: TU1.5 B41D0 mode-1 vehicle cut
    for (std::uint32_t t = 42; t <= 44; ++t)
      run.tick(t, 3);
    check(run.cuts == 1 && length(run.predictor.position_offset()) == 0, "A 8 wu correction was not cut");
  }
  {
    NonHostVehiclePredictor::Settings settings;
    settings.trust_distance = .1f;
    settings.trust_angle = .02f;
    Run run(settings);
    for (std::uint32_t t = 1; t <= 40; ++t)
      run.tick(t, 3);
    const auto replays = run.replays;
    run.tick(41, 3, {.05f, 0, 0}); // within the trust band
    for (std::uint32_t t = 42; t <= 50; ++t)
      run.tick(t, 3);
    check(run.replays == replays && run.trusted > 0, "A trusted report was replayed");
    run.tick(51, 3, {.3f, 0, 0}); // outside it
    for (std::uint32_t t = 52; t <= 55; ++t)
      run.tick(t, 3);
    check(run.replays > replays, "An untrusted report was not replayed");
  }
}

void history_rules() {
  NonHostVehiclePredictor predictor(toy);
  auto m = start();
  predictor.reset(m, 10);
  for (std::uint32_t t = 11; t <= 15; ++t)
    predictor.predict(t, input_at(t));
  bool threw = false;
  try {
    predictor.predict(15, {});
  } catch (const std::invalid_argument &) {
    threw = true;
  }
  check(threw, "A repeated sequence was predicted");
  check(predictor.reconcile(13, m).applied && predictor.pending() == 2, "Acknowledged inputs were not dropped");
  check(predictor.reconcile(12, m).stale, "An older snapshot was adopted");
  auto bad = m;
  bad.linear.x = std::nanf("");
  check(!predictor.reconcile(14, bad).applied, "Non-finite authority was adopted");
  // Pending replay on reset skips a broken run.
  std::vector<NonHostVehiclePredictor::PendingInput> pending{{21, {}}, {22, {}}, {24, {}}};
  predictor.reset(m, 20, pending);
  check(predictor.pending() == 2 && predictor.latest_sequence() == 22u, "Reset replayed across a gap");
  // Capacity overflow: the acknowledged input left the history, so cut.
  NonHostVehiclePredictor::Settings small;
  small.capacity = 3;
  NonHostVehiclePredictor tiny(toy, small);
  tiny.reset(m, 0);
  for (std::uint32_t t = 1; t <= 6; ++t)
    tiny.predict(t, input_at(t));
  check(tiny.reconcile(1, m).cut, "A replay across overflowed history did not cut");
  bool rejected = false;
  try {
    NonHostVehiclePredictor::Settings wrong;
    wrong.snap_distance = 0;
    NonHostVehiclePredictor invalid(toy, wrong);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected, "Invalid settings were accepted");
}
} // namespace

int main() try {
  orientation_helpers();
  extrapolation();
  agreement_under_latency();
  correction_blend();
  snap_and_trust();
  history_rules();
  std::cout << "native non-host vehicle prediction passed\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << "native non-host vehicle prediction failed: " << error.what() << '\n';
  return 1;
}
