#include "halo2/source_game_end.hpp"
#include "halo2/source_result_statistics.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
std::int16_t sum(std::int16_t a,std::int16_t b) {
  return std::bit_cast<std::int16_t>(std::uint16_t(int(a)+int(b)));
}
}
std::int16_t source_game_end_award(SourceGameEndStats& stats,
    std::span<const SourceGameEndParticipant> players,bool team,std::uint32_t winner,
    SourceResultPublication* publication) {
  if(winner==0xffffffffu)return 0;
  if(team) {
    if(winner>=8)throw std::invalid_argument("Source winning team outside stat bank");
    const auto wins=source_game_stat_team_add(stats,winner,7,1,6,publication);
    for(const auto& player:players)if(!(player.flags&2) && int(player.membership)==int(winner)) {
      const auto slot=player.datum&0xffffu;
      if(slot>=16)throw std::invalid_argument("Source player outside stat bank");
      source_game_stat_player_add(stats,slot,7,1,6,publication);
    }
    return wins;
  }
  const auto player=std::find_if(players.begin(),players.end(),
      [&](const auto& p){return p.datum==winner;});
  if(player==players.end())return 0;
  const auto slot=winner&0xffffu;
  if(slot>=16)throw std::invalid_argument("Source winner outside stat bank");
  return source_game_stat_player_add(stats,slot,7,1,6,publication);
}
bool source_round_finishes(std::int32_t kind,std::int16_t index,
    std::uint32_t winner,std::int16_t wins,bool force) {
  const bool selected=kind==0 ? index>=0 : kind==1 ? index>=1 :
      kind==2 ? index>=3 : kind==3 ? index>=5 :
      kind==4 ? winner!=0xffffffffu && wins>=2 :
      kind==5 ? winner!=0xffffffffu && wins>=3 :
      kind==6 ? winner!=0xffffffffu && wins>=4 : false;
  return force || selected || index>=31;
}
void source_game_end_accumulate(SourceGameEndStats& stats,bool over) {
  for(auto& player:stats.players) {
    player.fields[9]=player.fields[10]=0;player.last_kill_tick=0;
    if(player.fields[0]) {
      player.fields[1]=sum(player.fields[1],player.fields[0]);
      if(!over)player.fields[0]=0;
    }
  }
  for(auto& team:stats.teams)if(team.fields[0]) {
    team.fields[1]=sum(team.fields[1],team.fields[0]);
    if(!over)team.fields[0]=0;
  }
}
} // namespace halo2
