#include "halo2/multiplayer_departure.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
struct Fixture {
  MultiplayerState state;
  std::vector<OnlineAdmission> roster;
  Fixture(bool teams) {
    state.tick=123;state.team_mode=teams;state.retained_team_mask=teams?0x81:0;
    state.source_round_lifecycle_available=true;state.round_phase=1;
    for(unsigned i=0;i<4;++i) {
      OnlineAdmission row;row.slot=i;row.peer=teams?(i<2?0:2):i;row.team=teams?(i<2?0:7):-1;
      roster.push_back(row);auto& p=state.players[i];p.connected=true;p.source_player=0x10000u+i;
      p.source_flags=1;p.source_membership=teams?row.team:int(i);p.source_unit=0x20000u+i;
      p.score=int(i)+3;p.total_score=10;p.wins=2;p.kills=3;p.deaths=4;p.assists=5;p.suicides=1;
    }
    state.team_scores[0]=11;state.team_statistics[0][0]=11;state.team_statistics[0][1]=20;
    state.team_statistics[0][7]=2;
  }
};
}
int main()try {
  for(bool teams:{false,true}) {
    Fixture f(teams);const std::uint16_t departed=teams?12:14;
    const auto result=multiplayer_departure_game_over(f.state,f.roster,0,departed);
    check(result && result->finished && !result->results_finalized && result->round_phase==2,
        "Last opponent did not produce local GAME OVER without authoritative finalization");
    check(result->game_over_countdown==210 && !result->game_over_complete(),"Departure skipped the original GAME OVER countdown");
    check(result->winner==(teams?-1:0) && result->winner_team==(teams?0:-1),"Departure winner differs from original participant query");
    check(result->players[0].total_score==13 && result->players[0].wins==3,"Known current score was not carried exactly once");
    check(result->players[1].wins==(teams?3:2),"Departed opponent or surviving teammate received the wrong award");
    check(!teams || (result->team_total_scores[0]==31 && result->team_wins[0]==3),"Team GAME OVER lost actual team counters");
    for(unsigned i=0;i<4;++i) {
      check(result->players[i].kills==3 && result->players[i].deaths==4 && result->players[i].assists==5 && result->players[i].suicides==1,
          "Departure fabricated combat statistics");
      check(result->players[i].connected && result->players[i].source_player==f.state.players[i].source_player,
          "Departure lost a retained scoreboard identity");
    }
    check(!f.state.finished && f.state.players[0].total_score==10 && f.state.players[0].wins==2 && f.state.players[2].source_flags==1,
        "Local presentation mutated its accepted source snapshot");
    check(!multiplayer_departure_game_over(f.state,f.roster,0,0),"Ordinary connection failure fabricated an opponent departure");
    check(!multiplayer_departure_game_over(f.state,f.roster,0,teams?4:6),"A remaining opponent did not keep the match active");
    check(!multiplayer_departure_game_over(f.state,f.roster,0,std::uint16_t(departed|1)),"Departing local player entered GAME OVER");
    check(!multiplayer_departure_game_over(f.state,f.roster,0,std::uint16_t(departed|0x8000)),"Unadmitted departure was accepted");
    f.state.players[3].connected=false;
    check(!multiplayer_departure_game_over(f.state,f.roster,0,departed),"Pre-world/missing participant fabricated completed gameplay");
  }
  Fixture couch(false);couch.roster[1].peer=0;
  check(!multiplayer_departure_game_over(couch.state,couch.roster,0,12),"Another local FFA player was ignored as an opponent");
  Fixture teams(true);teams.roster[2].team=teams.roster[3].team=0;
  teams.state.players[2].source_membership=teams.state.players[3].source_membership=0;
  check(!multiplayer_departure_game_over(teams.state,teams.roster,0,12),"A match that never had opposing teams was ended");
  Fixture complete(false);complete.state.finished=complete.state.results_finalized=true;
  const auto repeated=multiplayer_departure_game_over(complete.state,complete.roster,0,14);
  check(repeated && repeated->players[0].total_score==10 && repeated->players[0].wins==2,"Completed host results were awarded twice");
  Fixture extra(false);extra.state.players[15].connected=true;
  check(!multiplayer_departure_game_over(extra.state,extra.roster,0,14),"Unadmitted snapshot participant entered GAME OVER");
  Fixture campaign(false);campaign.state.source_game_type=1;
  check(!multiplayer_departure_game_over(campaign.state,campaign.roster,0,14),"Campaign entered multiplayer departure presentation");
  MultiplayerState full;std::vector<OnlineAdmission> full_roster;
  for(unsigned slot=0;slot<16;++slot) {
    OnlineAdmission row;row.slot=row.peer=slot;full_roster.push_back(row);
    auto& player=full.players[slot];player.connected=true;player.source_player=0x10000u+slot;
    player.source_flags=1;player.source_membership=std::int16_t(slot);player.source_unit=0x20000u+slot;
  }
  const auto high_slot=multiplayer_departure_game_over(full,full_roster,15,0x7fff);
  check(high_slot && high_slot->winner==15 && high_slot->players[15].wins==1,"Full sixteen-player roster lost its high-slot survivor");
  check(!multiplayer_departure_game_over(full,full_roster,15,0x7ffe),"Sixteen-player match ignored an active low-slot opponent");
  SourceGameOverState ending;SoundClassFades fades;const SourceGameOverConstants constants;
  source_game_over_begin(ending,30,constants);
  for(unsigned tick=0;tick<210;++tick) {
    check(!ending.complete(),"GAME OVER ended before the original seven-second countdown");
    source_game_over_tick(ending,tick_seconds,fades,true,constants);
  }
  check(ending.complete() && ending.fade_started,"Original GAME OVER countdown/fade failed to complete");
  std::cout<<"PASS FFA/team opponent departures, couch teams and local FFA opponents, active/missing/invalid peers, retained scores/identities, no combat-stat mutation or competitive finalization, original seven-second GAME OVER\n";
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
