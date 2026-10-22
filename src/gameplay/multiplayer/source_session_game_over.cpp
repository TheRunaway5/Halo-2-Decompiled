#include "halo2/source_session_game_over.hpp"
#include <stdexcept>

namespace halo2 {
void source_session_phase_set(SourceSessionPhaseState& state,std::int32_t phase,
    std::uint32_t milliseconds) {
  if(state.phase==phase)return;
  if(state.record_count>std::int32_t(state.records.size()))
    throw std::invalid_argument("Original session record count exceeds its16 records");
  bool occupied=false;
  for(std::int32_t i=0;i<state.record_count;++i)
    if(state.records[unsigned(i)].occupied) {
      state.records[unsigned(i)].changed=1;occupied=true;
    }
  state.previous_phase=state.phase;
  state.transition_pending=occupied?1:0;
  state.previous_phase_time=milliseconds;
  state.phase=phase;
  ++state.sequence;
  state.phase_time=milliseconds;
  ++state.generation;
}
bool source_session_game_over_update(const SourceSessionGameOverInput& input,
    SourceSessionPhaseState& state,SourceSessionRequest& request,
    SourceGameOverState& game_over,std::int16_t tick_rate,
    std::uint32_t milliseconds,const SourceGameOverConstants& constants) {
  if(input.preflight_changed)return true;
  if(request.pending || input.main_session_state<=2 || input.main_session_state>8)
    return false;
  if(state.phase!=4) {
    request.pending=1;request.kind=state.phase==5?4:1;
    request.length=0;request.first_payload_word=0;
    return false;
  }
  const bool phase_write=input.main_session_state>=5 && input.main_session_state<=8;
  const bool closing=input.world_active && input.world_closing;
  if(closing) {
    if(!phase_write)throw std::invalid_argument("Original closing session state3/4 service is unrecovered");
    source_session_phase_set(state,1,milliseconds);
    return true;
  }
  if(input.game_ready && input.same_game_identity)
    throw std::invalid_argument("Original identity-matched map transition is unrecovered");
  if(phase_write && !game_over.complete() && input.force_end && input.game_ready && input.game_kind==1)
    throw std::invalid_argument("Original campaign session end service is unrecovered");
  if(phase_write && game_over.complete()) {
    source_session_phase_set(state,5,milliseconds);
    return true;
  }
  if(phase_write && input.force_end && input.game_ready && input.game_kind==2)
    source_game_over_begin(game_over,tick_rate,constants);
  return false;
}
bool source_game_over_requests_menu(std::int8_t mode,const SourceGameOverState& state) {
  return mode!=4 && state.complete() && !(mode>1 && mode<6);
}
} // namespace halo2
