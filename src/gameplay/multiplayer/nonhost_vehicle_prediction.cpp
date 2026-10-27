#include "halo2/nonhost_vehicle_prediction.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace halo2 {
namespace {
bool newer(std::uint32_t a, std::uint32_t b) { return std::int32_t(a - b) > 0; }
bool finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }
bool finite(const NonHostVehicleMotion &m) {
  return finite(m.position) && finite(m.forward) && finite(m.up) && finite(m.linear) && finite(m.angular);
}
using Q = NonHostQuaternion;
Q multiply(const Q &a, const Q &b) { // a then b applied first: (a * b) v = a (b v)
  return {a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
          a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
          a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
          a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]};
}
Q conjugate(const Q &q) { return {-q[0], -q[1], -q[2], q[3]}; }
Q normalize(Q q) {
  const float n = std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
  if (!(n > 0))
    return {0, 0, 0, 1};
  return {q[0] / n, q[1] / n, q[2] / n, q[3] / n};
}
Q axis_angle(Vec3 rotation) { // rotation vector (axis * angle)
  const float angle = length(rotation);
  if (!(angle > 0))
    return {0, 0, 0, 1};
  const float s = std::sin(angle * .5f) / angle;
  return {rotation.x * s, rotation.y * s, rotation.z * s, std::cos(angle * .5f)};
}
Vec3 rotate(const Q &q, Vec3 v) {
  const Vec3 u{q[0], q[1], q[2]};
  const Vec3 t = cross(u, v) * 2.f;
  return v + t * q[3] + cross(u, t);
}
} // namespace

NonHostQuaternion nonhost_vehicle_orientation(Vec3 forward, Vec3 up) {
  const Vec3 f = normalized(forward);
  const Vec3 l = normalized(cross(up, f));
  const Vec3 u = cross(f, l);
  // Columns forward, left, up (local x, y, z in world space).
  const float m00 = f.x, m01 = l.x, m02 = u.x, m10 = f.y, m11 = l.y, m12 = u.y, m20 = f.z, m21 = l.z, m22 = u.z;
  const float trace = m00 + m11 + m22;
  Q q;
  if (trace > 0) {
    const float s = std::sqrt(trace + 1.f) * 2.f;
    q = {(m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, .25f * s};
  } else if (m00 > m11 && m00 > m22) {
    const float s = std::sqrt(1.f + m00 - m11 - m22) * 2.f;
    q = {.25f * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s};
  } else if (m11 > m22) {
    const float s = std::sqrt(1.f + m11 - m00 - m22) * 2.f;
    q = {(m01 + m10) / s, .25f * s, (m12 + m21) / s, (m02 - m20) / s};
  } else {
    const float s = std::sqrt(1.f + m22 - m00 - m11) * 2.f;
    q = {(m02 + m20) / s, (m12 + m21) / s, .25f * s, (m10 - m01) / s};
  }
  return normalize(q);
}
void nonhost_vehicle_basis(const NonHostQuaternion &q, Vec3 &forward, Vec3 &up) {
  forward = rotate(q, {1, 0, 0});
  up = rotate(q, {0, 0, 1});
}
float nonhost_vehicle_angle(const NonHostQuaternion &a, const NonHostQuaternion &b) {
  const float d = std::abs(a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
  return 2.f * std::acos(std::min(1.f, d));
}
NonHostQuaternion nonhost_vehicle_blend(const NonHostQuaternion &saved, const NonHostQuaternion &current,
                                        float weight) {
  const float d = current[3] * saved[3] + current[2] * saved[2] + current[1] * saved[1] + current[0] * saved[0];
  const float w = d < 0 ? -weight : weight, keep = 1.f - weight;
  return normalize({current[0] * w + saved[0] * keep, current[1] * w + saved[1] * keep,
                    current[2] * w + saved[2] * keep, current[3] * w + saved[3] * keep});
}

void step_nonhost_vehicle_extrapolation(NonHostVehicleMotion &m, float dt) {
  m.position = m.position + m.linear * dt;
  const auto q = normalize(multiply(axis_angle(m.angular * dt), nonhost_vehicle_orientation(m.forward, m.up)));
  nonhost_vehicle_basis(q, m.forward, m.up);
}

NonHostVehiclePredictor::NonHostVehiclePredictor(Stepper step)
    : NonHostVehiclePredictor(std::move(step), Settings{}) {}
NonHostVehiclePredictor::NonHostVehiclePredictor(Stepper step, Settings settings)
    : step_(std::move(step)), settings_(settings) {
  if (!step_)
    throw std::invalid_argument("Non-host vehicle prediction requires a native stepper");
  if (!(settings_.position_blend > 0 && settings_.position_blend <= 1) ||
      !(settings_.orientation_blend > 0 && settings_.orientation_blend <= 1) || !(settings_.snap_distance > 0) ||
      !(settings_.trust_distance >= 0) || !(settings_.trust_angle >= 0) || settings_.capacity == 0)
    throw std::invalid_argument("Invalid non-host vehicle prediction settings");
  history_.reserve(settings_.capacity);
}

void NonHostVehiclePredictor::reset(const NonHostVehicleMotion &authoritative, std::uint32_t acknowledged,
                                    std::span<const PendingInput> pending) {
  if (!finite(authoritative))
    throw std::invalid_argument("Non-finite authoritative vehicle motion");
  state_ = authoritative;
  acknowledged_ = baseline_ = latest_ = acknowledged;
  history_.clear();
  acknowledged_entry_.reset();
  offset_ = {};
  rotation_offset_ = {0, 0, 0, 1};
  active_ = true;
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

const NonHostVehicleMotion &NonHostVehiclePredictor::predict(std::uint32_t sequence,
                                                             const NonHostVehicleInput &input) {
  if (!active_)
    throw std::logic_error("Non-host vehicle prediction has no authoritative start");
  if (!newer(sequence, latest_))
    throw std::invalid_argument("Non-host vehicle input sequence did not advance");
  latest_ = sequence;
  offset_ = offset_ * (1 - settings_.position_blend);
  if (length(offset_) < settings_.settle_distance)
    offset_ = {};
  rotation_offset_ = nonhost_vehicle_blend(rotation_offset_, {0, 0, 0, 1}, settings_.orientation_blend);
  if (nonhost_vehicle_angle(rotation_offset_, {0, 0, 0, 1}) < settings_.settle_angle)
    rotation_offset_ = {0, 0, 0, 1};
  step_(state_, input);
  if (history_.size() == settings_.capacity)
    history_.erase(history_.begin());
  history_.push_back({sequence, input, state_});
  return state_;
}

NonHostVehiclePredictor::Reconciliation
NonHostVehiclePredictor::reconcile(std::uint32_t acknowledged, const NonHostVehicleMotion &authoritative,
                                   NonHostVehicleCorrection kind) {
  Reconciliation result;
  if (!finite(authoritative))
    return result; // 865B0's finite-values check refused such motion; keep predicting
  if (!active_) {
    reset(authoritative, acknowledged);
    result.applied = result.cut = true;
    return result;
  }
  if (newer(acknowledged_, acknowledged)) {
    result.stale = true;
    return result;
  }
  const auto shown = presented();
  const auto shown_rotation = nonhost_vehicle_orientation(shown.forward, shown.up);
  const NonHostVehicleMotion *predicted_ack = nullptr;
  const auto previous = std::find_if(history_.begin(), history_.end(),
                                     [&](const Entry &e) { return e.sequence == acknowledged; });
  if (previous != history_.end())
    predicted_ack = &previous->after;
  else if (acknowledged_entry_ && acknowledged_entry_->sequence == acknowledged)
    predicted_ack = &acknowledged_entry_->after;
  const bool known = predicted_ack != nullptr;
  const auto authority_rotation = nonhost_vehicle_orientation(authoritative.forward, authoritative.up);
  if (known) {
    result.history_error = length(authoritative.position - predicted_ack->position);
    result.history_angle_error = nonhost_vehicle_angle(
        authority_rotation, nonhost_vehicle_orientation(predicted_ack->forward, predicted_ack->up));
  }
  if (previous != history_.end())
    acknowledged_entry_ = *previous;
  const auto first = std::find_if(history_.begin(), history_.end(),
                                  [&](const Entry &e) { return newer(e.sequence, acknowledged); });
  const bool agrees = kind == NonHostVehicleCorrection::normal && known && settings_.trust_distance > 0 &&
                      result.history_error <= settings_.trust_distance &&
                      result.history_angle_error <= settings_.trust_angle;
  history_.erase(history_.begin(), first);
  acknowledged_ = acknowledged;
  result.applied = true;
  if (agrees) {
    result.trusted = true;
    return result;
  }
  baseline_ = acknowledged;
  state_ = authoritative;
  if (acknowledged_entry_ && acknowledged_entry_->sequence == acknowledged)
    acknowledged_entry_->after = authoritative;
  const bool gap = !history_.empty() && history_.front().sequence != acknowledged + 1;
  if (gap)
    history_.clear();
  for (auto &entry : history_) {
    step_(state_, entry.input);
    entry.after = state_;
    ++result.replayed;
  }
  const auto error = shown.position - state_.position;
  const auto replayed_rotation = nonhost_vehicle_orientation(state_.forward, state_.up);
  result.error = length(error);
  result.angle_error = nonhost_vehicle_angle(shown_rotation, replayed_rotation);
  if (kind == NonHostVehicleCorrection::cut || gap || !(result.error <= settings_.snap_distance)) {
    offset_ = {};
    rotation_offset_ = {0, 0, 0, 1};
    result.cut = true;
    return result;
  }
  // Keep the shown pose continuous; predict() decays both offsets.
  offset_ = error;
  rotation_offset_ = normalize(multiply(shown_rotation, conjugate(replayed_rotation)));
  return result;
}

NonHostVehicleMotion NonHostVehiclePredictor::presented() const {
  auto shown = state_;
  shown.position = state_.position + offset_;
  if (rotation_offset_ != NonHostQuaternion{0, 0, 0, 1})
    nonhost_vehicle_basis(normalize(multiply(rotation_offset_, nonhost_vehicle_orientation(state_.forward, state_.up))),
                          shown.forward, shown.up);
  return shown;
}
float NonHostVehiclePredictor::orientation_offset() const {
  return nonhost_vehicle_angle(rotation_offset_, {0, 0, 0, 1});
}
std::optional<std::uint32_t> NonHostVehiclePredictor::latest_sequence() const {
  return active_ ? std::optional(latest_) : std::nullopt;
}
const NonHostVehicleMotion *NonHostVehiclePredictor::prediction(std::uint32_t sequence) const {
  for (const auto &entry : history_)
    if (entry.sequence == sequence)
      return &entry.after;
  if (acknowledged_entry_ && acknowledged_entry_->sequence == sequence)
    return &acknowledged_entry_->after;
  return nullptr;
}
NonHostVehicleReport NonHostVehiclePredictor::report() const {
  return {latest_sequence().value_or(0), baseline_, state_};
}
} // namespace halo2
