#include "halo2/multiplayer_outcome.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <iostream>

int main(int argc,char **argv) try {
  using namespace halo2;
  if(argc!=3 || !std::filesystem::is_regular_file(argv[1]) ||
      !std::filesystem::is_regular_file(argv[2]))return 77;
  MultiplayerOptions options;
  options.map=argv[1];
  options.ticks=2;
  options.mute=true;
  MultiplayerRuntimeOutcome outcome;
  if(run_multiplayer(options,outcome)!=0)
    throw FormatError("Native headless runtime did not finish its declared limit");
  // The runtime and its Cache/Match have been destroyed. Read the actual
  // retained result, rather than constructing an expected PCR/session bank.
  if(outcome.reason!=MultiplayerExitReason::headless_limit ||
      outcome.authority!=MultiplayerOutcomeAuthority::local ||
      !outcome.authoritative_banks || !outcome.state || outcome.state->tick!=options.ticks ||
      outcome.state->finished || outcome.state->results_finalized)
    throw FormatError("Real runtime exit, authority or unfinished state was lost");
  unsigned connected=0;
  for(const auto &player:outcome.state->players)if(player.connected) {
    ++connected;
    if(player.name!="Player 1" || player.source_player==null_datum)
      throw FormatError("Runtime handoff lost the actual native participant");
  }
  if(connected!=1)
    throw FormatError("Runtime handoff manufactured or discarded participants");
  std::cout<<"PASS actual native runtime handoff survives world destruction; "
              "unfinished local match remains unfinished; no PCR/session transition\n";
  // Run a normal authored time-limit completion. Do not call end_match or
  // mutate a state snapshot. The selected executable supplies the actual
  // game-over operands and result descriptors used by the runtime.
  const auto original=OriginalData::load_xbe(argv[2]);
  if(!original->game_over_constants || !original->result_stat_descriptors)
    throw FormatError("Completion handoff requires original game-over/results operands");
  options.gameplay_xbe=argv[2];
  options.variant.values.set(0xa,180); // Three-minute Time Limit, in source seconds.
  const auto rules=slayer_rules(options.variant);
  SourceGameOverState game_over;
  source_game_over_begin(game_over,30,*original->game_over_constants);
  options.ticks=rules.time_ticks+unsigned(game_over.countdown)+2;
  MultiplayerRuntimeOutcome completed;
  if(run_multiplayer(options,completed)!=0 || !completed.state ||
      !completed.authoritative_banks)
    throw FormatError("Completed runtime did not retain its actual state and banks");
  const auto &state=*completed.state;
  const auto &banks=*completed.authoritative_banks;
  if(completed.reason!=MultiplayerExitReason::headless_limit ||
      completed.authority!=MultiplayerOutcomeAuthority::local ||
      state.tick!=options.ticks || !state.finished || !state.results_finalized ||
      !state.game_over_complete() || !state.game_over_fade_started ||
      state.time_ticks!=rules.time_ticks || !banks.recorded_statistics ||
      banks.result_recording.enabled!=1 || banks.result_recording.frozen!=1 || !banks.recorded_events ||
      banks.recorded_events->index!=0 || banks.recorded_events->last==0 || banks.recorded_events->start==0)
    throw FormatError("Normal time-limit finalization or source recording was lost");
  connected=0;
  for(const auto &player:state.players)if(player.connected) {
    ++connected;
    if(player.name!="Player 1" || player.source_player==null_datum ||
        player.score!=0 || player.kills!=0 || player.deaths!=0)
      throw FormatError("Completed handoff changed actual idle participant results");
  }
  if(connected!=1 || banks.match_statistics.players[0].fields[7]!=1)
    throw FormatError("Normal completion lost the actual participant/win award");
  std::cout<<"PASS normal three-minute native Slayer expiry and source game-over "
              "retain finalized banks after world destruction; ticks="<<state.tick<<
              "; runtime termination remains its explicit headless limit\n";
  return 0;
} catch(const std::exception &error) {
  std::cerr<<error.what()<<'\n';return 1;
}
