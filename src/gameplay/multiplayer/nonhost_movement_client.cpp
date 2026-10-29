#include "halo2/nonhost_movement_client.hpp"
#include "halo2/math.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
bool newer(std::uint32_t a, std::uint32_t b) { return std::int32_t(a - b) > 0; }
Vec3 lerp(Vec3 a, Vec3 b, float t) { return a + (b - a) * t; }
} // namespace

NonHostMovementClient::NonHostMovementClient(NetcodeMode mode, MovementDefinition movement,
                                             std::optional<std::uint32_t> deadzone_bits)
    : mode_(mode), movement_(movement), deadzone_(deadzone_bits) {}

bool NonHostMovementClient::eligible(const MultiplayerState::Player &p) const {
  // Match host adoption's native walking/airborne owner. Special movement
  // (including Sword mode6) keeps its received trajectory; this mover cannot
  // continue the source special component from ordinary movement inputs.
  return p.connected && !p.dead && p.source_unit != null_datum && p.source_parent == null_datum &&
         p.source_seat == -1 && p.native_physical_state == 1;
}

NonHostAuthority NonHostMovementClient::authority(const MultiplayerState::Player &p) const {
  NonHostAuthority a;
  a.position = p.position;
  a.velocity = p.velocity;
  a.forward = direction(p.yaw);
  a.crouch = p.crouch;
  a.yaw = p.yaw;
  a.pitch = p.pitch;
  a.grounded = p.native_grounded;
  a.landing = p.landing;
  return a;
}

std::optional<NativeMotionReport> NonHostMovementClient::predict(unsigned view, std::uint32_t sequence,
                                                                 const Controls &controls, float yaw, float pitch,
                                                                 const MultiplayerState::Player &latest,
                                                                 const BipedDefinition &biped,
                                                                 const CollisionWorld &collision) {
  auto &v = views_.at(view);
  v.collision = &collision;
  if (biped.datum != v.biped || !v.definition) {
    v.definition = nonhost_predictor_definition(movement_, biped, deadzone_);
    v.biped = biped.datum;
    v.radius = biped.radius;
    v.slope = biped.maximum_slope_cosine;
    v.predictor.reset(); // reseeded by the next snapshot
    v.have_shown = false;
  }
  if (!eligible(latest)) {
    v.predictor.reset();
    v.eligible = false;
    v.have_shown = false;
    v.sent.clear();
    return std::nullopt;
  }
  NonHostMotionInput input{controls.forward, controls.right, yaw, pitch, controls.jump, controls.crouch};
  v.sent.push_back({sequence, input});
  while (v.sent.size() > 64)
    v.sent.pop_front();
  if (!v.predictor || !v.predictor->active())
    return std::nullopt; // no authoritative baseline yet; replayed when seeded
  v.predictor->predict(sequence, input);
  ++v.stats.predicted;
  const auto shown = nonhost_authority_position(*v.definition, v.predictor->presented_position());
  v.previous_shown = v.have_shown ? v.shown : shown;
  v.shown = shown;
  v.have_shown = true;
  auto report = v.predictor->report();
  report.position = nonhost_authority_position(*v.definition, report.position);
  return quantize_nonhost_motion(report);
}

void NonHostMovementClient::reconcile(unsigned view, const MultiplayerState::Player &player,
                                      const LanClient::InputReceipt &receipt, const CollisionWorld &collision) {
  auto &v = views_.at(view);
  v.collision = &collision;
  if (!eligible(player)) {
    v.predictor.reset();
    v.eligible = false;
    v.have_shown = false;
    v.sent.clear(); // No walking input may replay across a native motion owner.
    return;
  }
  if (!receipt.known || !v.definition)
    return;
  const auto *carried = v.predictor ? v.predictor->prediction(receipt.consumed) : nullptr;
  const auto motion = nonhost_motion_from_authority(authority(player), *v.definition, v.radius, v.slope, collision,
                                                    carried);
  const bool cut = !v.predictor || !v.predictor->active() || !v.eligible || player.source_unit != v.unit ||
                   player.source_teleport_serial != v.teleport_serial;
  if (cut) {
    if (!v.predictor) {
      View *self = &v;
      v.predictor = std::make_unique<NonHostMovementPredictor>(
          [self](NonHostBipedMotion &m, const NonHostMotionInput &in) {
            step_native_player_motion(m, *self->definition, *self->collision, in);
          },
          nonhost_predictor_settings(mode_));
    }
    // Inputs already sent after the consumed one stay predicted.
    std::vector<NonHostMovementPredictor::PendingInput> pending(v.sent.begin(), v.sent.end());
    v.predictor->reset(motion, receipt.consumed, pending);
    ++v.stats.cuts;
    v.shown = v.previous_shown = nonhost_authority_position(*v.definition, v.predictor->presented_position());
    v.have_shown = true;
  } else {
    auto kind = NonHostCorrection::normal;
    if (const auto forced = receipt.forced.at(view);
        forced && newer(*forced, v.predictor->baseline_sequence())) {
      kind = NonHostCorrection::forced;
      ++v.stats.forced;
    }
    const auto result = v.predictor->reconcile(receipt.consumed, motion, kind);
    if (result.cut) {
      // A snap or a history gap: restart from this authority with every
      // input already sent after it (the client's own log is complete).
      std::vector<NonHostMovementPredictor::PendingInput> pending(v.sent.begin(), v.sent.end());
      v.predictor->reset(motion, receipt.consumed, pending);
    }
    if (result.applied) {
      ++v.stats.reconciled;
      v.stats.trusted += result.trusted;
      v.stats.replayed += !result.trusted && !result.cut;
      v.stats.cuts += result.cut;
      v.stats.last_error = result.trusted ? result.history_error : result.error;
      v.stats.max_error = std::max(v.stats.max_error, v.stats.last_error);
      if (result.cut)
        v.shown = v.previous_shown = nonhost_authority_position(*v.definition, v.predictor->presented_position());
    }
  }
  v.unit = player.source_unit;
  v.teleport_serial = player.source_teleport_serial;
  v.eligible = true;
}

std::optional<Vec3> NonHostMovementClient::presented(unsigned view, float tick_fraction) const {
  const auto &v = views_.at(view);
  if (!v.predictor || !v.predictor->active() || !v.have_shown || !v.definition)
    return std::nullopt;
  return lerp(v.previous_shown, v.shown, std::clamp(tick_fraction, 0.f, 1.f));
}

void NonHostMovementClient::present(MultiplayerState &shown, std::span<const unsigned> slots,
                                    float tick_fraction) const {
  for (unsigned view = 0; view < slots.size() && view < views_.size(); ++view)
    // Buffered playout can still show a native special-motion endpoint after
    // the newest received endpoint has resumed ordinary prediction.
    if (const auto position = eligible(shown.players.at(slots[view])) ? presented(view, tick_fraction) : std::nullopt) {
      auto &player = shown.players.at(slots[view]);
      if(player.source_player_camera &&
          (player.position.x!=position->x || player.position.y!=position->y || player.position.z!=position->z)) {
        // Preserve245060/CD4A0/E18D0's measured eye-to-body displacement.
        // Prediction translates this same source body; leaving its completed
        // eye at the accepted host root makes the local legs chase the view.
        const auto delta=*position-player.position;
        player.source_player_camera->position=player.source_player_camera->position+delta;
      }
      player.position = *position;
      player.velocity = views_[view].predictor->predicted().body.velocity;
    }
}
} // namespace halo2
