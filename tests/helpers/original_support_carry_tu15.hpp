#pragma once
#include "halo2/math.hpp"
#include "halo2/transform.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace halo2::test {
// Measures outputs of the actual TU1.5 controller. Owner/body are its +5C/+20
// fields; +24 is the rigid transform copied by original 1FA1A0/1D8B90. An
// authored PHMO node is not a stable identity across a real carrier handoff.
struct OriginalSupportObservationTu15 {
  std::uint32_t tick{}, owner{}, carrier{};
  std::int32_t body_index{-1};
  unsigned body_count{};
  bool resumed{}, grounded{};
  float power{};
  Vec3 player_position;
  Transform support_transform;
};
struct OriginalSupportCarrySegmentTu15 {
  std::uint32_t owner{}, first_tick{}, last_tick{};
  std::int32_t body_index{-1};
  unsigned moving_ticks{};
  float carried{}, expected_displacement{}, maximum_relative_error{};
};
class OriginalSupportCarryTu15 {
public:
  explicit OriginalSupportCarryTu15(float player_radius):radius_(player_radius) {}
  void observe(const OriginalSupportObservationTu15 &s) {
    const bool actual=s.owner==s.carrier && s.body_index>=0 &&
        unsigned(s.body_index)<s.body_count && s.grounded;
    if(!s.resumed) {
      paused_grounded_ticks+=s.grounded && s.power==0.f;
      paused_actual_carrier_ticks+=actual && s.power==0.f;
      anchor_.reset();return;
    }
    if(!actual || s.power!=1.f) {anchor_.reset();return;}
    const auto &t=s.support_transform;
    if(!(t.scale>0.f) || !std::isfinite(t.scale))throw FormatError("Invalid original support transform scale");
    for(const auto v:{t.position,t.basis[0],t.basis[1],t.basis[2]})
      if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))throw FormatError("Nonfinite original support transform");
    if(!anchor_ || s.owner!=segments.back().owner || s.body_index!=segments.back().body_index || s.tick!=segments.back().last_tick+1u) {
      const auto delta=s.player_position-t.position;
      anchor_=Anchor{s.player_position,{dot(delta,t.basis[0])/t.scale,dot(delta,t.basis[1])/t.scale,dot(delta,t.basis[2])/t.scale}};
      segments.push_back({s.owner,s.tick,s.tick,s.body_index});return;
    }
    auto &segment=segments.back();segment.last_tick=s.tick;
    const auto local=anchor_->local;
    const auto expected=t.position+(t.basis[0]*local.x+t.basis[1]*local.y+t.basis[2]*local.z)*t.scale;
    const auto error=length(s.player_position-expected);
    segment.maximum_relative_error=std::max(segment.maximum_relative_error,error);
    const auto travel=length(s.player_position-anchor_->position),expected_travel=length(expected-anchor_->position);
    // A moving biped or stale contact on a stationary body cannot satisfy this.
    // The tolerance is the actual authored biped radius, smaller than the old
    // half-height node check. Airborne frames never count as supported carry.
    if(error<radius_ && travel>.1f && expected_travel>.1f) {
      ++moving_ticks;++segment.moving_ticks;
      maximum_carried=std::max(maximum_carried,travel);
      segment.carried=std::max(segment.carried,travel);
      segment.expected_displacement=std::max(segment.expected_displacement,expected_travel);
    }
  }
  unsigned paused_grounded_ticks{},paused_actual_carrier_ticks{},moving_ticks{};
  float maximum_carried{};
  std::vector<OriginalSupportCarrySegmentTu15> segments;
private:
  struct Anchor {Vec3 position,local;};
  std::optional<Anchor> anchor_;
  float radius_{};
};
}
