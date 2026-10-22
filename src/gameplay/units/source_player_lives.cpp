#include "halo2/source_player_lives.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
void source_initialize_player_lives(std::int16_t& remaining,std::int32_t configured,
    bool engine_present,std::uint8_t mode) {
  if(engine_present && mode!=4)
    remaining=configured ? std::bit_cast<std::int16_t>(std::uint16_t(configured)) : -1;
}
void source_player_life_death(std::int16_t& remaining,std::int32_t game_type,
    std::uint8_t mode,bool engine_present,std::int16_t lifecycle,std::int32_t status) {
  if(engine_present && game_type==2 && mode!=4 && lifecycle==1 && status==1 && remaining!=-1)
    remaining=std::int16_t(std::max(int(remaining)-1,0));
}
bool source_player_lives_exhausted(std::int32_t configured,std::uint32_t unit,
    std::int16_t remaining) {
  return configured>0 && unit==0xffffffffu && remaining==0;
}
bool source_player_respawn_allowed(std::int32_t configured,std::uint16_t flags,
    std::int8_t membership,std::uint32_t unit,std::int16_t remaining) {
  if(!(flags&1) || (flags&2) || membership==-1)return false;
  if(unit!=0xffffffffu)return true;
  return !(flags&0x4000) && !source_player_lives_exhausted(configured,unit,remaining);
}
bool source_team_lives_exhausted(std::int32_t configured,bool engine_present,
    bool team_mode,std::int32_t team,std::span<const SourcePlayerLifeParticipant> players) {
  if(!engine_present || !team_mode || configured<=0)return false;
  for(const auto& player:players)
    if(!(player.flags&2) && player.membership==team &&
        (player.current_unit!=0xffffffffu || player.remaining!=0))return false;
  return true;
}
} // namespace halo2
