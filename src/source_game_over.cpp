#include "halo2/source_game_over.hpp"
#include <cmath>

namespace halo2 {
void source_game_over_begin(SourceGameOverState& state,std::int16_t rate,
    const SourceGameOverConstants& constants) {
  if(state.active)return;
  state.active=true;
  state.countdown=std::int32_t(std::nearbyint(float(rate)*constants.seconds));
  state.fade_started=false;
}
void source_game_over_tick(SourceGameOverState& state,float seconds,
    SoundClassFades& classes,bool engine_present,const SourceGameOverConstants& constants) {
  if(!state.active || state.countdown<=0)return;
  --state.countdown;
  if(!state.fade_started && float(state.countdown)*seconds<=constants.fade_threshold_seconds) {
    // Original18BAC0/188CD0: empty substring selects every nonempty class;
    // the three following selectors overwrite only their matching targets.
    source_game_over_fade(classes,engine_present,constants);
    state.fade_started=true;
  }
}
void source_game_over_fade(SoundClassFades& classes,bool engine_present,
    const SourceGameOverConstants& constants) {
  if(!engine_present)return;
  classes.set("",0,constants.fade_seconds);
  classes.set("ambient_nature",constants.ambient_linear_gain,constants.fade_seconds);
  classes.set("ambient_machinery",constants.ambient_linear_gain,constants.fade_seconds);
  classes.set("ambient_computers",constants.ambient_linear_gain,constants.fade_seconds);
}
} // namespace halo2
