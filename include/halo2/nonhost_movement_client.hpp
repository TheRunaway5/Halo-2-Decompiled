#pragma once
// Non-host side of movement prediction (NETCODE_ENHANCED_GOAL item 1): one
// predictor per local view, fed on every input tick and reconciled on every
// received snapshot. Shared by the multiplayer runtime and its tests.
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/nonhost_motion_adoption.hpp"
#include <array>
#include <deque>
#include <memory>
#include <optional>

namespace halo2 {
class NonHostMovementClient {
public:
  NonHostMovementClient(NetcodeMode, MovementDefinition, std::optional<std::uint32_t> deadzone_bits);
  // Predictor steppers borrow their view's slot: the owner stays in place.
  NonHostMovementClient(const NonHostMovementClient &) = delete;
  NonHostMovementClient &operator=(const NonHostMovementClient &) = delete;

  // Input tick of `view`, right before LanClient::send: predicts input
  // `sequence` only for a living, unseated native physical-mode1 player,
  // and returns the
  // motion block to attach (LanClient::set_motion). `latest` is the newest
  // received state of that player; `biped` its representation's definition.
  // Borrow `collision` afresh every tick (composition may replace it).
  // `yaw`/`pitch` are the processed look the host will apply (the host's
  // angles plus the raw look it has not consumed yet, as the look
  // prediction draws them).
  std::optional<NativeMotionReport> predict(unsigned view, std::uint32_t sequence, const Controls &,
                                            float yaw, float pitch, const MultiplayerState::Player &latest,
                                            const BipedDefinition &biped, const CollisionWorld &collision);
  // One received snapshot, in delivery order, with its input receipt.
  void reconcile(unsigned view, const MultiplayerState::Player &, const LanClient::InputReceipt &,
                 const CollisionWorld &collision);
  // Authority-frame position of the predicted biped: the last two input
  // ticks blended by `tick_fraction` (the local fixed-step remainder), with
  // the decaying correction offset. Absent while not predicting.
  std::optional<Vec3> presented(unsigned view, float tick_fraction) const;
  // Replace the shown local players' positions/velocities with the
  // prediction (views map to `slots`).
  void present(MultiplayerState &shown, std::span<const unsigned> slots, float tick_fraction) const;

  struct Stats {
    std::uint64_t predicted{}, reconciled{}, trusted{}, replayed{}, cuts{}, forced{};
    float last_error{}, max_error{};
  };
  const Stats &stats(unsigned view) const { return views_.at(view).stats; }
  const NonHostMovementPredictor *predictor(unsigned view) const { return views_.at(view).predictor.get(); }
  const NonHostMotionDefinition *definition(unsigned view) const {
    return views_.at(view).definition ? &*views_.at(view).definition : nullptr;
  }

private:
  struct View {
    std::unique_ptr<NonHostMovementPredictor> predictor;
    std::optional<NonHostMotionDefinition> definition;
    std::uint32_t biped = 0xffffffffu;
    float radius{}, slope = .65f;
    std::uint32_t unit = 0xffffffffu, teleport_serial{};
    bool eligible{};
    // Every input sent while eligible (newest last), replayed on a seed or cut.
    std::deque<NonHostMovementPredictor::PendingInput> sent;
    const CollisionWorld *collision{};
    Vec3 previous_shown, shown;
    bool have_shown{};
    Stats stats;
  };
  bool eligible(const MultiplayerState::Player &) const;
  NonHostAuthority authority(const MultiplayerState::Player &) const;
  NetcodeMode mode_;
  MovementDefinition movement_;
  std::optional<std::uint32_t> deadzone_;
  std::array<View, 4> views_{};
};
} // namespace halo2
