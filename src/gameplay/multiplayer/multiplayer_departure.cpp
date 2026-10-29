#include "halo2/multiplayer_departure.hpp"
#include "halo2/source_game_end.hpp"
#include "halo2/source_round_end.hpp"
#include <bit>

namespace halo2 {
std::optional<MultiplayerState> multiplayer_departure_game_over(
    const MultiplayerState& snapshot,std::span<const OnlineAdmission> roster,
    unsigned self,std::uint16_t departed,const SourceGameOverConstants& constants) {
  if(roster.size()<2 || roster.size()>16 || self>=16 || snapshot.source_game_type!=2 || !departed)return {};
  std::uint16_t admitted{},active{};bool local=false,had_opponent=false;
  const OnlineAdmission *local_player=nullptr;
  for(const auto& p:roster)if(p.slot==self && online_admission_peer(p)==self)local_player=&p;
  if(!local_player)return {};
  for(const auto& p:roster) {
    if(p.slot>=16 || (admitted&(1u<<p.slot)) || online_admission_peer(p)>=16)return {};
    const auto& row=snapshot.players[p.slot];
    if(!row.connected || row.source_player==null_datum || (row.source_player&0xffffu)!=p.slot)return {};
    admitted|=std::uint16_t(1u<<p.slot);
    if(snapshot.team_mode && (p.team<0 || p.team>=8 || row.source_membership!=p.team))return {};
    if(online_admission_peer(p)==self) {
      if((departed&(1u<<p.slot)) || (row.source_flags&2u))return {};
      local=true;
    }
    had_opponent|=snapshot.team_mode?p.team!=local_player->team:p.slot!=self;
    if(!(departed&(1u<<p.slot)) && !(row.source_flags&2u))active|=std::uint16_t(1u<<p.slot);
  }
  if(!local || !had_opponent || (departed&~admitted) || !(active&(1u<<self)))return {};
  for(unsigned slot=0;slot<snapshot.players.size();++slot)
    if(snapshot.players[slot].connected && !(admitted&(1u<<slot)))return {};
  for(const auto& p:roster) {
    const auto owner=online_admission_peer(p);
    if(!(admitted&(1u<<owner)) || bool(departed&(1u<<p.slot))!=bool(departed&(1u<<owner)))return {};
    if((active&(1u<<p.slot)) && (snapshot.team_mode?p.team!=local_player->team:p.slot!=self))return {};
  }
  if(snapshot.finished)return snapshot; // Never award/carry a completed snapshot twice.
  auto display=snapshot;
  SourceGameEndStats scores;
  std::array<SourceRoundEndParticipant,16> participants{};
  std::array<SourceGameEndParticipant,16> awards{};unsigned count{};
  for(const auto& p:roster) {
    auto& row=display.players[p.slot];
    if(departed&(1u<<p.slot))row.source_flags=std::uint16_t((row.source_flags|2u)&~1u);
    participants[count]={row.source_player,row.source_flags,std::int8_t(row.source_membership),row.source_unit,row.source_lives_remaining};
    awards[count++]={row.source_player,std::int8_t(row.source_membership),row.source_flags};
    auto& stats=scores.players[p.slot].fields;
    stats[0]=std::bit_cast<std::int16_t>(std::uint16_t(row.score));stats[1]=row.total_score;stats[7]=row.wins;
    row.source_status.reset(); // An active pre-object gate cannot describe this local ending.
  }
  for(unsigned team=0;team<8;++team)scores.teams[team].fields=display.team_statistics[team];
  SourceRoundEndInput input;input.team_mode=display.team_mode;input.life_limit=display.life_limit;
  input.retained_team_mask=display.retained_team_mask;input.source_engine=display.source_engine;
  const auto decision=source_round_end_query(input,std::span(participants.data(),count));
  if(!decision.end || !decision.force)return {};
  source_game_end_award(scores,std::span(awards.data(),count),display.team_mode,decision.winner);
  source_game_end_accumulate(scores,true);
  for(const auto& p:roster) {
    auto& row=display.players[p.slot];row.total_score=scores.players[p.slot].fields[1];row.wins=scores.players[p.slot].fields[7];
  }
  for(unsigned team=0;team<8;++team) {
    display.team_statistics[team]=scores.teams[team].fields;
    display.team_total_scores[team]=scores.teams[team].fields[1];display.team_wins[team]=scores.teams[team].fields[7];
  }
  display.finished=true;display.source_round_lifecycle_available=true;display.round_phase=2;
  SourceGameOverState ending;source_game_over_begin(ending,30,constants);
  display.game_over_countdown=ending.countdown;display.game_over_fade_started=ending.fade_started;
  display.winner=display.team_mode || decision.winner==null_datum?-1:int(decision.winner&0xffffu);
  display.winner_team=!display.team_mode || decision.winner==null_datum?-1:int(decision.winner);
  // results_finalized stays false: this is a local display of known public
  // counters. Full numeric/medal banks and result consent did not arrive.
  return display;
}
} // namespace halo2
