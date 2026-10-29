#pragma once
// Host adoption of non-host reported motion (NETCODE_ENHANCED_GOAL item 1,
// phase 2a). The host calls observe() once per tick after
// MultiplayerMatch::step, then adopt() for each remote view whose consumed
// input request carried a motion report (phase 2b decodes it). Decisions
// follow the original TU1.5 B41D0 mode 2 / B44C0 rules plus the port's
// anti-cheat bounds (adopt_nonhost_motion); writes go through native setters
// only (B4F00 + B9800 nudge; B4DA0 teleport once a native B95D0 placement
// exists). No TU1.5 call is made here.
#include "halo2/native_request_protocol.hpp"
#include "halo2/nonhost_movement_prediction.hpp"
#include "halo2/physics_impulse_journal.hpp"
#include "halo2/unit_damage.hpp"
#include "halo2/netcode_mode.hpp"
#include <array>
#include <functional>
#include <optional>
#include <span>

namespace halo2 {
struct MultiplayerState;
struct MultiplayerSourceMotion;
class MultiplayerObjectRuntime;

// Native world writes for one adopted report.
struct NonHostMotionWriter {
  // B44C0 decision 2 with the reported position and linear velocity (the
  // object's own forward/up/angular): its 0.06 dead band, B4F00 correction,
  // then B9800 velocities and wake. Required for any write.
  // Returns true when velocities were written.
  std::function<bool(std::uint32_t object, Vec3 position, Vec3 velocity)> nudge;
  // B44C0 decision 1: B4DA0 placement (B95D0) plus B9800. Absent until the
  // native placement owner exists; teleport decisions then nudge instead.
  std::function<bool(std::uint32_t object, Vec3 position, Vec3 velocity)> teleport;
};
// Production writer over the live TU1.5 bank: native B44C0 decision 2
// (B4BD0, BC200 dead bands, B4F00, B9800; native_tu15_object_motion). A
// deactivated island (unported activation listener) is skipped. No teleport.
NonHostMotionWriter native_tu15_motion_writer(MultiplayerObjectRuntime &);

// Why the host last moved a remote biped on its own.
enum class NonHostForcedMotion : std::uint8_t {
  none,
  respawn,              // a new current unit (spawn, respawn, ownership change)
  teleport,             // completed original teleporter (1A6830) serial
  damage,               // a damage event on the unit (damage force)
  impulse,              // a published point impulse on the unit
  unexplained_velocity, // a velocity change no input can produce (lift, man cannon, vehicle hit)
};

class NonHostMotionAdopter {
public:
  struct View {
    unsigned slot{};
    std::uint32_t consumed{}; // input sequence the step consumed for this view
  };
  // `bounds` supplies the authored speed/jump operands used for the speed
  // limit and the unexplained-velocity test (largest across representations).
  NonHostMotionAdopter(NonHostMotionDefinition bounds, float radius, NonHostAdoptionLimits = {},
                       NonHostMotionWriter = {});

  // After MultiplayerMatch::step and its state projection: record forced
  // motion for every remote view. A forced move stamps `consumed + 1`, the
  // first input whose authoritative motion includes it.
  void observe(const MultiplayerState &, MultiplayerObjectRuntime &, std::span<const View>);
  // Same, with the TU1.5 biped readback supplied (tests, other owners).
  using MotionReader = std::function<std::optional<MultiplayerSourceMotion>(std::uint32_t unit)>;
  void observe(const MultiplayerState &, const MotionReader &, std::span<const View>);
  // Host-loop form without a projected state: each view's current unit and
  // teleporter serial plus the tick's damage events and impulse journal.
  struct Observed {
    View view;
    std::uint32_t unit = 0xffffffffu, teleport_serial{};
  };
  void observe(std::span<const Observed>, std::span<const UnitDamageEffectEvent>,
               std::span<const PhysicsImpulseEvent>, const MotionReader &);

  struct Result {
    NonHostAdoption decision;
    bool written{}, placement_unavailable{};
  };
  // Adopt one view's reported motion (fed by phase 2b from the decoded
  // input request of `slot`). `host_steps` counts host ticks since this
  // view's previous report.
  Result adopt(MultiplayerObjectRuntime &, const MultiplayerState &, const CollisionWorld &,
               unsigned slot, const NonHostMotionReport &, unsigned host_steps);
  Result adopt(const MotionReader &, const MultiplayerState &, const CollisionWorld &, unsigned slot,
               const NonHostMotionReport &, unsigned host_steps);
  Result adopt(const MotionReader &, std::uint32_t unit, bool dead, const CollisionWorld &, unsigned slot,
               const NonHostMotionReport &, unsigned host_steps);

  // Snapshot field for the slot (phase 2b): newest forced-motion sequence.
  std::optional<std::uint32_t> forced_sequence(unsigned slot) const;
  NonHostForcedMotion last_forced(unsigned slot) const;
  const NonHostAdoptionLimits &limits() const noexcept { return limits_; }

  struct PeerState {
    bool seen{};
    std::uint32_t unit = 0xffffffffu, teleport_serial{};
    std::uint64_t damage_serial{};
    std::uint32_t impulse_serial{};
    Vec3 velocity;
    NonHostAdoptionState adoption;
    NonHostForcedMotion forced{};
  };
  struct Checkpoint {
    // Exact scalar bits of bounds, radius and limits, in checkpoint() order.
    // No writer, native object pointer or executor owner crosses this value.
    std::array<std::uint32_t,41> configuration{};
    std::array<PeerState,16> peers{};
  };
  Checkpoint checkpoint() const;
  static void validate_checkpoint(const Checkpoint &);
  // Requires this freshly constructed owner's exact configuration and keeps
  // its writer. A rejected checkpoint leaves all current peer state intact.
  void restore(const Checkpoint &);

private:
  std::array<std::uint32_t,41> checkpoint_configuration() const;
  NonHostMotionDefinition bounds_;
  float radius_{};
  NonHostAdoptionLimits limits_;
  NonHostMotionWriter writer_;
  std::array<PeerState, 16> peers_{};
};

// Wire form of a motion report (NativeMotionReport, 17 bytes): absolute
// 1/1024 wu position (signed 24-bit), 1/256 wu/s velocity saturated to i16,
// baseline age capped at 255. Absent when the position is out of range.
std::optional<NativeMotionReport> quantize_nonhost_motion(const NonHostMotionReport &);
// Host side: the report for input `sequence` (age 255 keeps the oldest
// representable baseline, which any later forced move makes stale).
NonHostMotionReport dequantize_nonhost_motion(const NativeMotionReport &, std::uint32_t sequence);
// Per-mode policy until a native B95D0 placement exists: every mode uses the
// TU1.5 B41D0/B44C0 decision with the anti-cheat bounds; Balanced/Full ask for
// exact adoption (teleport distance 0), which the nudge fallback serves.
// The client trusts its prediction within 0.05 wu (measured; see .cpp).
NonHostAdoptionLimits nonhost_adoption_limits(NetcodeMode);
NonHostMovementPredictor::Settings nonhost_predictor_settings(NetcodeMode);

// Unexplained-velocity test used by observe(): the largest velocity change
// one tick of player control can produce (ground acceleration, gravity, a
// jump kick) plus a tolerance. Exposed for tests.
bool nonhost_velocity_change_forced(Vec3 before, Vec3 after, const NonHostMotionDefinition &);
} // namespace halo2
