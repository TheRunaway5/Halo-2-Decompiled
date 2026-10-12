#pragma once
#include "halo2/sound_classes.hpp"
#include <cstdint>

namespace halo2 {
// Original compiled operands, identical in the three verified images.
// OriginalData imports the selected executable's words for live matches.
struct SourceGameOverConstants {
  float seconds=7.f;
  float fade_threshold_seconds=2.f;
  float ambient_linear_gain=.2f;
  float fade_seconds=1.f;
};
struct SourceGameOverState {
  bool active{},fade_started{};
  std::int32_t countdown{};
  bool complete()const{return active && countdown==0;}
};
// Retail1389C0 / TU13A130 / U5 13B4A0. Repeated requests preserve state.
// The supported original world clock uses nearest-even FISTP rounding.
void source_game_over_begin(SourceGameOverState&,std::int16_t tick_rate,
    const SourceGameOverConstants&);
// Complete original18BAC0/188CD0 request owner, also usable by replicas
// receiving the authority's fade-start signal.
void source_game_over_fade(SoundClassFades&,bool engine_present,
    const SourceGameOverConstants&);
// Retail138EB0 / TU13A620 / U5 13B990. Runs after finalization and before
// the world clock increment. Audio envelopes advance on their separate clock.
void source_game_over_tick(SourceGameOverState&,float tick_seconds,
    SoundClassFades&,bool engine_present,const SourceGameOverConstants&);
} // namespace halo2
