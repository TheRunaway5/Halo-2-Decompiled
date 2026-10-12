#pragma once
#include <cstdint>

namespace halo2 {
// Literal bipd+1FC..20C values consumed by E5960.
struct SourceLandingDefinition {
  float soft_seconds{},hard_seconds{},soft_speed{},hard_speed{},maximum_speed{};
};
// DFaf0 cold initialization and E5960/E5A80's retained34C..34F fields.
struct SourceLandingState {
  std::int16_t kind=-1;
  std::int8_t elapsed{},duration{};
};
// E5960's timer writes. The physical/contact producer supplies impact_speed;
// this function does not infer a contact from an animation or grounded flag.
void begin_source_landing(SourceLandingState &,const SourceLandingDefinition &,
                          float impact_speed,int tick_rate,bool &force_hard);
// E5A80's timer/ordinary locomotion branch. Release-blend feedback and the
// separate climbing branch are outside this retained-state adapter.
void advance_source_landing(SourceLandingState &,bool moving);
std::uint32_t source_landing_activity(const SourceLandingState &,
                                      std::uint32_t ordinary_activity);
} // namespace halo2
