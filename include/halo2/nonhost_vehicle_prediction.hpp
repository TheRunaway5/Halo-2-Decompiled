#pragma once
// Non-host vehicle movement prediction (NETCODE_ENHANCED_GOAL item 1,
// vehicles; phase 1 prototype, not wired into the runtime).
//
// The 2004 distributed client stepped its driven vehicle in its own world
// and sent the vehicle's motion (retail 828B0: position, forward, up, linear
// and angular velocity of the parent the unit drives); the host applied it
// through TU1.5 865B0 -> B41D0 mode 2 (vehicle reject 9.0 wu, teleport past
// 0.2 wu) -> B44C0 (decision 2: B4F00, B4C40 rider bookkeeping, B9800). The
// client's own vehicle took host corrections only past the B41D0 mode-1
// vehicle distance (7.5 wu, 0x4DBD14), then snapped.
//
// This owner is the vehicle counterpart of NonHostMovementPredictor: replay
// of unacknowledged inputs from the exact authoritative motion, a trust band,
// and a shown offset that decays by the 2004 object blend (TU1.5
// B3CA0/B3D70, retail A9A70/A9B40: 45% position, 35% orientation by
// normalized quaternion blend) unless it exceeds the snap distance. The
// stepper is supplied by the caller. The port has no native vehicle physics
// a client could run (see the phase-1 report); the only native stepper here
// is rigid-body extrapolation, which ignores input. Native code only.
#include "halo2/cache.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
// One local input tick, the driving subset of Controls.
struct NonHostVehicleInput {
  float forward{}, right{}, yaw{}, pitch{};
  std::uint16_t buttons{}; // stepper-defined (jump, crouch, boost ...)
};
// Rigid motion of the driven vehicle: BC190 position/forward/up and BC200
// linear/angular velocity of the root body (world space, per second).
struct NonHostVehicleMotion {
  Vec3 position, forward{1, 0, 0}, up{0, 0, 1}, linear, angular;
};
// Unit quaternion (x, y, z, w) helpers over forward/up bases. The basis is
// forward, left = up x forward, up, as the original 0x34 transform.
using NonHostQuaternion = std::array<float, 4>;
NonHostQuaternion nonhost_vehicle_orientation(Vec3 forward, Vec3 up);
void nonhost_vehicle_basis(const NonHostQuaternion &, Vec3 &forward, Vec3 &up);
// Angle in radians of the rotation from `a` to `b` (0..pi).
float nonhost_vehicle_angle(const NonHostQuaternion &a, const NonHostQuaternion &b);
// The 2004 orientation blend step (A9B40): normalize(current*w + saved*(1-w))
// with the sign of w following the quaternion dot product.
NonHostQuaternion nonhost_vehicle_blend(const NonHostQuaternion &saved, const NonHostQuaternion &current,
                                        float weight);

// Force-free rigid-body extrapolation over `dt`: position += linear * dt and
// the orientation rotated by angular * dt about its world axis; velocities
// are kept. Exact for a body under no force; it ignores input, contacts,
// suspension, gravity and damping.
void step_nonhost_vehicle_extrapolation(NonHostVehicleMotion &, float dt = 1.f / 30.f);

// Motion fields of one input request for a driven vehicle (see the phase-1
// schema design): the predicted result of input `sequence` from the
// authoritative motion of input `baseline`.
struct NonHostVehicleReport {
  std::uint32_t sequence{}, baseline{};
  NonHostVehicleMotion motion;
};

enum class NonHostVehicleCorrection : std::uint8_t { normal, forced, cut };

class NonHostVehiclePredictor {
public:
  using Stepper = std::function<void(NonHostVehicleMotion &, const NonHostVehicleInput &)>;
  struct Settings {
    // TU1.5 B3CA0 / B3D70 (retail A9A70 / A9B40) fractions, once per tick.
    float position_blend = .45f, orientation_blend = .35f;
    // A replayed present further than this from the shown one is a cut:
    // TU1.5 B41D0 mode-1 own-unit vehicle distance (0x4DBD14, 7.5 wu; retail
    // 8.0). B41D0 compares positions only, so orientation never cuts.
    float snap_distance = 7.5f;
    // Authoritative motion within both bands of the prediction of the same
    // input is trusted and not replayed. 0 always replays (no host adoption).
    float trust_distance = 0, trust_angle = 0;
    // Residual offsets below these are dropped (wu, radians).
    float settle_distance = 1.f / 4096.f, settle_angle = 1.f / 4096.f;
    std::size_t capacity = 64;
  };
  explicit NonHostVehiclePredictor(Stepper);
  NonHostVehiclePredictor(Stepper, Settings);

  struct PendingInput {
    std::uint32_t sequence{};
    NonHostVehicleInput input;
  };
  // Start (or restart) from authoritative motion that includes input
  // `acknowledged`, then replay `pending` (an unbroken ascending run).
  void reset(const NonHostVehicleMotion &authoritative, std::uint32_t acknowledged,
             std::span<const PendingInput> pending = {});
  bool active() const noexcept { return active_; }

  // Input tick: decay the shown offsets by one tick of the 2004 blend, then
  // step input `sequence` (strictly increasing modulo 2^32).
  const NonHostVehicleMotion &predict(std::uint32_t sequence, const NonHostVehicleInput &);

  struct Reconciliation {
    bool applied{}, cut{}, stale{}, trusted{};
    unsigned replayed{};
    float error{}, angle_error{};                 // replayed present vs shown present
    float history_error{}, history_angle_error{}; // authority vs prediction of the same input
  };
  Reconciliation reconcile(std::uint32_t acknowledged, const NonHostVehicleMotion &authoritative,
                           NonHostVehicleCorrection = NonHostVehicleCorrection::normal);

  const NonHostVehicleMotion &predicted() const noexcept { return state_; }
  // Presented pose: predicted pose plus the decaying offsets.
  NonHostVehicleMotion presented() const;
  Vec3 position_offset() const noexcept { return offset_; }
  float orientation_offset() const;
  std::optional<std::uint32_t> latest_sequence() const;
  std::uint32_t baseline_sequence() const noexcept { return baseline_; }
  std::size_t pending() const noexcept { return history_.size(); }
  const NonHostVehicleMotion *prediction(std::uint32_t sequence) const;
  NonHostVehicleReport report() const;
  const Settings &settings() const noexcept { return settings_; }

private:
  struct Entry {
    std::uint32_t sequence{};
    NonHostVehicleInput input;
    NonHostVehicleMotion after;
  };
  Stepper step_;
  Settings settings_;
  bool active_{};
  std::uint32_t acknowledged_{}, baseline_{}, latest_{};
  NonHostVehicleMotion state_;
  std::vector<Entry> history_;
  std::optional<Entry> acknowledged_entry_;
  Vec3 offset_;
  NonHostQuaternion rotation_offset_{0, 0, 0, 1}; // shown = offset * predicted
};
} // namespace halo2
