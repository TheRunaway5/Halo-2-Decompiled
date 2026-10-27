#pragma once
// Non-host movement prediction (NETCODE_ENHANCED_GOAL item 1, phase 1).
//
// The 2004 distributed client (simulation mode 4) stepped its own biped at
// once and sent the host its resulting motion. The host classified it with
// retail A9F70 / TU1.5 B41D0 and applied it with retail AA260 / TU1.5 B44C0
// (teleport B4DA0, velocity nudge B4F00). In mode 4 the client ignored host
// corrections to its own unit below the B41D0 mode-1 distance and snapped
// above it; the 45%/35% blend (TU1.5 B3CA0/B3D70) smoothed other objects.
//
// The port's host runs the TU1.5 world and a non-host has no world, so this
// owner steps the local biped with the port's native player mover:
//   - predict(): every local input tick, before the input is sent, the native
//     mover advances the predicted biped by that input;
//   - reconcile(): a snapshot names the newest input the host consumed. When
//     the host's motion for that input differs from the prediction of the
//     same input by more than Settings::trust_distance, the predictor restarts
//     from the authoritative motion and replays every newer input;
//   - the shown difference between the old and the replayed present is not
//     cut: it becomes a presentation offset that decays by the original
//     45% position / 35% orientation blend per tick, unless it exceeds the
//     snap distance (TU1.5 own-unit biped snap, 2.5 wu) or the host cut.
// Simulation state is never blended: a replay always starts from the exact
// authoritative motion. Native code only; no TU1.5 world is required.
#include "halo2/biped_landing.hpp"
#include "halo2/collision.hpp"
#include "halo2/kernels.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace halo2 {
// One local input tick, the movement subset of Controls.
struct NonHostMotionInput {
  float forward{}, right{}, yaw{}, pitch{};
  bool jump{}, crouch{};
};
// Native mover state of the local biped. Body carries position, velocity,
// capsule height/radius and the native support contact.
struct NonHostBipedMotion {
  Body body;
  float crouch{}, yaw{}, pitch{};
  JumpState jump;
  SourceLandingState landing;
  // Unit forward (+70), the 152E80 fallback basis; source control only.
  Vec3 facing{1, 0, 0};
};
// Authored operands of the native player mover (BipedDefinition subset and
// the map's player MovementDefinition).
struct NonHostMotionDefinition {
  MovementDefinition movement;
  float height{}, crouch_height{}, crouch_velocity{}, jump_velocity{};
  std::uint32_t physics_flags{};
  SlopeDefinition slope;
  SourceLandingDefinition landing;
  // Original unit locomotion dead zone (bits of a float, 0x46A560); absent
  // disables the landing timers exactly as Simulation::move_unit does.
  std::optional<std::uint32_t> locomotion_deadzone_bits;
  // TU1.5 control chain (predictor): 152E80 basis and DF880 dead zone ahead
  // of the mover. Off reproduces Simulation::move_unit's direct Controls.
  bool source_control{};
  // Walk along the 152E80 facing instead of the aim yaw (source control).
  bool source_facing{};
  // Height of the authoritative biped origin above the native capsule feet.
  // TU1.5 bipeds rest 0.01 wu above their support (measured on Lockout:
  // every settled sample differs by exactly 0.0099 from the native capsule,
  // which resolves to 0.0001); authority positions are lowered by this when
  // they become mover state and raised again for reports/presentation.
  float origin_height{};
};
// The map's MovementDefinition plus the biped tag operands move_unit reads.
NonHostMotionDefinition native_motion_definition(const MovementDefinition &, const BipedDefinition &,
                                                 std::optional<std::uint32_t> deadzone_bits);
// The predictor's operands: native_motion_definition plus the TU1.5 control
// chain (152E80 basis and facing, DF880 dead zone) and the measured TU1.5
// origin height. Selected by the Lockout trajectory oracle.
NonHostMotionDefinition nonhost_predictor_definition(const MovementDefinition &, const BipedDefinition &,
                                                     std::optional<std::uint32_t> deadzone_bits);
inline constexpr float tu15_biped_origin_height = .0099f;
// Native mover position -> authoritative origin (reports, presentation).
inline Vec3 nonhost_authority_position(const NonHostMotionDefinition &d, Vec3 native) {
  return {native.x, native.y, native.z + d.origin_height};
}
// Simulation::move_unit's body (after its source-movement early return), the
// single native implementation: crouch, player intent/jump/walking or the
// steering branch, capsule move and landing timers. `facing` replaces the
// aim-yaw walking basis when supplied.
void step_native_unit_motion(NonHostBipedMotion &, const NonHostMotionDefinition &,
                             const CollisionWorld &, Vec3 desired, bool jump, bool crouch,
                             bool player_control, std::optional<Vec3> facing = std::nullopt);
// TU1.5 153070/152E80 control (native produce_source_player_control) for an
// on-foot biped, then DF880's throttle dead zone.
struct SourceMotionControl {
  Vec3 throttle, facing;
  bool dead{};
};
SourceMotionControl source_motion_control(const NonHostMotionInput &, Vec3 unit_forward,
                                          std::optional<std::uint32_t> deadzone_bits);
// One native player-control tick (multiplayer participant path): the same
// kernels, operands and order as Simulation::move_unit's player-control branch (retail-verified kernels:
// crouch DD7D0/DF5F0, intent 1E6120, jump E4680/E4770, walking/airborne
// 1F0870 = TU1.5 1F8710, landing E5960/E5A80) over the native capsule
// CollisionWorld. The host's TU1.5 chain (14FEC0 -> 153070 -> C92C0 ->
// DF880/DEAE0/E4A40 -> 1ED880 -> 1F8710, Havok step 31B1A0) also applies the
// DF880 throttle dead zone and 152E80 basis, and collides through Havok
// against BSP and other bodies; reconciliation hides those differences.
void step_native_player_motion(NonHostBipedMotion &, const NonHostMotionDefinition &,
                               const CollisionWorld &, const NonHostMotionInput &);

// Authoritative motion of the biped (decoded snapshot or TU1.5 readback).
struct NonHostAuthority {
  Vec3 position, velocity, forward{1, 0, 0};
  float crouch{}, yaw{}, pitch{};
  bool grounded{};
  SourceLandingState landing;
};
// Mover state for authoritative motion. The capsule height follows crouch
// (DF5F0); the native support contact is re-derived from collision. Jump
// timers are not replicated, so they carry over from `carried` (the client's
// own prediction of the same input) when supplied.
NonHostBipedMotion nonhost_motion_from_authority(const NonHostAuthority &, const NonHostMotionDefinition &,
                                                 float radius, float maximum_slope_cosine,
                                                 const CollisionWorld &,
                                                 const NonHostBipedMotion *carried = nullptr);

// Motion fields of one input request (phase 2 schema, see the design notes in
// nonhost_movement_prediction.cpp): the predicted result of input `sequence`,
// predicted from the authoritative motion of input `baseline`.
struct NonHostMotionReport {
  std::uint32_t sequence{}, baseline{};
  Vec3 position, velocity;
  bool grounded{}, crouching{};
};

// How a snapshot's motion of the local biped may be taken. Forced: the host
// moved it on its own (impulse, push) since the prediction's baseline, so it
// is replayed and blended even when trusted. Cut: teleport/respawn, snapped.
enum class NonHostCorrection : std::uint8_t { normal, forced, cut };

class NonHostMovementPredictor {
public:
  using Stepper = std::function<void(NonHostBipedMotion &, const NonHostMotionInput &)>;
  struct Settings {
    // TU1.5 B3CA0 / B3D70 (retail A9A70 / A9B40) fractions, once per tick.
    float position_blend = .45f, orientation_blend = .35f;
    // A replayed present further than this from the shown one is a cut, not
    // a correction: TU1.5 B41D0 mode-1 own-unit biped distance (2.5 wu from
    // 0x46A104 via 0x4DBD0C; retail 3.0). Vehicles use 7.5 (0x4DBD14).
    float snap_distance = 2.5f;
    // Authoritative motion within this distance of the prediction of the
    // same input is trusted as agreement and not replayed. 0 (phase 1, host
    // authoritative) always replays. With host adoption (phase 2), Original
    // uses the TU1.5 mode-1 rule: trust and snap distance both 2.5 wu.
    float trust_distance = 0;
    // Residual offsets below this are dropped (world units / radians). The
    // original's remote-object blend ends at 0.05 wu (0x46A12C); the local
    // first-person view uses a finer end.
    float settle_distance = 1.f / 4096.f;
    // Unacknowledged inputs retained for replay (about 2.1 s at 30 Hz).
    std::size_t capacity = 64;
  };
  // TU1.5 mode-4 own-unit policy, for use with host adoption.
  static Settings original_settings();
  explicit NonHostMovementPredictor(Stepper);
  NonHostMovementPredictor(Stepper, Settings);

  // Start (or restart after a cut) from authoritative motion that already
  // includes input `acknowledged`. Clears presentation offsets, then replays
  // `pending` (inputs already sent after it, ascending) as the new history,
  // so the prediction stays a full round trip ahead of the authority.
  struct PendingInput {
    std::uint32_t sequence{};
    NonHostMotionInput input;
  };
  void reset(const NonHostBipedMotion &authoritative, std::uint32_t acknowledged,
             std::span<const PendingInput> pending = {});
  bool active() const noexcept { return active_; }

  // Input tick: decay the presentation offset by one tick of the original
  // blend, then apply input `sequence` (strictly increasing, modulo 2^32) to
  // the predicted biped. Returns the predicted state after that input.
  const NonHostBipedMotion &predict(std::uint32_t sequence, const NonHostMotionInput &);

  struct Reconciliation {
    bool applied{}, cut{}, stale{}, trusted{};
    unsigned replayed{};      // inputs replayed after the acknowledged one
    float error{};            // |replayed present - previously shown present|
    float history_error{};    // |authoritative - prediction of the same input|
  };
  // Snapshot adoption: `authoritative` is the host's motion after it consumed
  // input `acknowledged`.
  Reconciliation reconcile(std::uint32_t acknowledged, const NonHostBipedMotion &authoritative,
                           NonHostCorrection = NonHostCorrection::normal);

  const NonHostBipedMotion &predicted() const noexcept { return state_; }
  Vec3 presented_position() const;
  float presented_yaw() const;
  Vec3 position_offset() const noexcept { return offset_; }
  std::optional<std::uint32_t> latest_sequence() const;
  std::optional<std::uint32_t> acknowledged_sequence() const;
  // Acknowledged input whose authoritative motion the prediction last
  // restarted from. A host-forced motion at or after it must be replayed.
  std::uint32_t baseline_sequence() const noexcept { return baseline_; }
  std::size_t pending() const noexcept { return history_.size(); }
  // The prediction of input `sequence` while it is pending or was the last
  // acknowledged one (source of the carried jump timers for a snapshot).
  const NonHostBipedMotion *prediction(std::uint32_t sequence) const;
  // Motion fields for the current input request (phase 2 upstream).
  NonHostMotionReport report() const;
  const Settings &settings() const noexcept { return settings_; }

private:
  struct Entry {
    std::uint32_t sequence{};
    NonHostMotionInput input;
    NonHostBipedMotion after;
  };
  Stepper step_;
  Settings settings_;
  bool active_{};
  std::uint32_t acknowledged_{}, baseline_{}, latest_{};
  NonHostBipedMotion state_;
  std::vector<Entry> history_; // ascending sequence, all newer than acknowledged_
  std::optional<Entry> acknowledged_entry_;
  Vec3 offset_;
  float yaw_offset_{};
};

// Host adoption decision for one reported motion (phase 2 kernel; the caller
// writes the result through the world's native position/velocity owners).
// Original rules, TU1.5 B41D0 mode 2 then B44C0:
//   parent, or distance > reject_distance          -> reject
//   distance > teleport_distance and the reported
//   capsule does not overlap collision             -> teleport (B4DA0: pose
//                                                     and both velocities)
//   otherwise                                      -> nudge (B4F00): reported
//     velocity when |dv|^2 > 0.06, else the host's, plus 0.9 * dpos capped to
//     0.05 wu/s; nothing is written when neither velocity was taken.
// Port additions, active unless `anti_cheat` is false: a report predicted
// from a baseline older than the host's last forced motion (teleport, spawn,
// impulse) is stale; horizontal speed beyond the authored bound is rejected;
// accepted corrections spend a refilling distance budget, less whole mover
// steps explained by held or skipped inputs.
struct NonHostAdoptionLimits {
  float reject_distance = 3.f;            // 0x4DBD10 (0x46A0E4), biped; 9.0 vehicle
  float teleport_distance = .2f;          // 0x46A16C; 1.5/0.8 for B4BD0 bipeds
  float velocity_deadband_squared = .06f; // 0x46A9FC
  float nudge_scale = .9f, nudge_limit = .05f;
  bool anti_cheat = true;
  float speed_tolerance = .25f;           // wu/s over the authored bound
  float budget_rate = .3f, budget_burst = .6f; // wu/s, wu
};
enum class NonHostAdoptionKind : std::uint8_t { reject, teleport, nudge, unchanged };
enum class NonHostRejection : std::uint8_t { none, invalid, parent, stale, distance, speed, budget };
struct NonHostAdoptionState {
  float budget = .6f;
  std::optional<std::uint32_t> forced_sequence; // newest host-forced motion
  std::optional<std::uint32_t> last_sequence;   // previous report considered
};
struct NonHostAdoptionInput {
  const NonHostMotionReport *report{};
  const NonHostBipedMotion *host{};
  bool parented{};
  float maximum_speed{};       // authored ground bound (largest of forward/backward/sideways)
  const CollisionWorld *collision{};
  float dt = 1.f / 30.f;
  unsigned host_steps = 1; // host ticks stepped since the previous report
};
struct NonHostAdoption {
  NonHostAdoptionKind kind{};
  NonHostRejection rejection{};
  // Teleport: the reported pose. Nudge: `velocity` is the native estimate of
  // the B4F00 result; `taken_velocity` is B44C0's linear operand (reported
  // when outside the dead band, else the host's), which the real B4F00
  // corrects from the object's own transform.
  Vec3 position, velocity, taken_velocity;
  float distance{};
};
NonHostAdoption adopt_nonhost_motion(NonHostAdoptionState &, const NonHostAdoptionLimits &,
                                     const NonHostAdoptionInput &);
} // namespace halo2
