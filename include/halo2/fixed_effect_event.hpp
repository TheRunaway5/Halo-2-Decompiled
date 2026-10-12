#pragma once
#include <functional>
#include <span>
#include <cstdint>
#include <stdexcept>
namespace halo2 {
// Closed fixed, single, non-looping event branch of TU1.1 17AD20.
// Services own parts/particles/retirement. No allocation success is implied.
struct FixedEffectEventState {
  float elapsed{},target{};
  bool active{},finished{};
};
inline void advance_fixed_effect_event(FixedEffectEventState &s,float duration,float seconds,
    const std::function<void()> &begin,const std::function<void(float,float)> &advance) {
  for(unsigned iteration=0;seconds>=0 && !s.finished && iteration<8;++iteration) {
    const float remaining=s.target-s.elapsed;
    const float dt=remaining<=seconds?remaining:seconds;
    const float before=s.elapsed;
    const bool complete=!(seconds<remaining);
    if(complete){s.elapsed=s.target;seconds=seconds-remaining;}
    else{s.elapsed=s.elapsed+seconds;seconds=-1.f;}
    if(s.active) {
      advance(before,dt);
      if(complete)s.finished=true;
    }else if(complete) {
      s.active=true;s.elapsed=0;s.target=duration;begin();
    }
  }
}
// Complete deterministic non-looping sequence branch of17AD20/17AC00/179D10.
// The loader rejects random skips/delays/durations and looping definitions.
struct FixedEffectEventDefinition {float delay{},duration{};};
struct FixedEffectSequenceState {
  FixedEffectEventState clock;
  std::uint16_t event{};
};
inline void advance_fixed_effect_sequence(FixedEffectSequenceState &s,
    std::span<const FixedEffectEventDefinition> events,float seconds,
    const std::function<void(unsigned)> &begin,
    const std::function<void(unsigned,float,float)> &advance) {
  if(events.empty() || s.event>=events.size())throw std::runtime_error("Fixed effect sequence has no authored event");
  auto &c=s.clock;
  for(unsigned iteration=0;seconds>=0 && !c.finished && iteration<8;++iteration) {
    const float remaining=c.target-c.elapsed;
    const float dt=remaining<=seconds?remaining:seconds;
    const float before=c.elapsed;
    const bool complete=!(seconds<remaining);
    if(complete){c.elapsed=c.target;seconds=seconds-remaining;}
    else{c.elapsed=c.elapsed+seconds;seconds=-1.f;}
    if(c.active) {
      advance(s.event,before,dt);
      if(complete) {
        if(unsigned(s.event)+1>=events.size())c.finished=true;
        else {++s.event;c.active=false;c.elapsed=0;c.target=events[s.event].delay;}
      }
    }else if(complete) {
      c.active=true;c.elapsed=0;c.target=events[s.event].duration;begin(s.event);
    }
  }
}
// Exact dry/underwater and camera eligibility in17AA90. Camera0 is FP.
// A multi-user view admits both camera classes; placement remains per-view.
inline bool source_effect_system_admitted(std::uint16_t disposition,std::uint16_t camera,
    bool underwater,unsigned local_users,bool first_person) {
  if(disposition==(underwater?1u:2u))return false;
  if(local_users==1 && ((camera==2 && first_person) || (camera==1 && !first_person)))return false;
  return true;
}
} // namespace halo2