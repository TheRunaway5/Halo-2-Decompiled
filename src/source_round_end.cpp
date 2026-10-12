#include "halo2/source_round_end.hpp"
#include "halo2/source_player_lives.hpp"
#include <bit>

namespace halo2 {
SourceRoundEndDecision source_round_end_query(const SourceRoundEndInput& input,
    std::span<const SourceRoundEndParticipant> players,std::uint32_t initial_winner) {
  SourceRoundEndDecision result{false,false,initial_winner};
  if(!input.engine_present)return result;
  const auto entrants=input.team_mode ? std::popcount(unsigned(input.retained_team_mask&0xff)) : int(players.size());
  std::uint32_t eligible=0,present=0,last=0xffffffffu;
  for(const auto& player:players) {
    if((player.flags&2) || player.membership<0 || player.membership>=16)continue;
    const auto bit=std::uint32_t(1)<<player.membership;present|=bit;
    if(source_player_respawn_allowed(input.life_limit,player.flags,player.membership,player.current_unit,player.lives) ||
        !(player.flags&3)) {
      eligible|=bit;last=player.datum;
    }
  }
  const auto threshold=entrants<2 ? 1 : 2;
  result.end=std::popcount(eligible)<threshold;
  result.force=std::popcount(present)<threshold;
  if(result.end && std::popcount(eligible)==1 && last!=0xffffffffu) {
    result.winner=last;
    if(input.team_mode) {
      for(const auto& player:players)if(player.datum==last){result.winner=std::uint32_t(player.membership);break;}
      if(!result.force && (input.source_engine==1 || input.source_engine==9) &&
          input.variant_240==1 && input.engine_2f0==result.winner)result.winner=0xffffffffu;
    }
  }
  return result;
}
SourceRoundEndDecision source_round_end_check(const SourceRoundEndInput& input,
    std::span<const SourceRoundEndParticipant> players) {
  if(!input.engine_present || input.lifecycle!=1 || (input.raw_mode!=4 && input.status!=1))return {};
  return source_round_end_query(input,players);
}
} // namespace halo2
