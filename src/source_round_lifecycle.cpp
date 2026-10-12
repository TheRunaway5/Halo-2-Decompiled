#include "halo2/source_round_lifecycle.hpp"
#include "halo2/source_player_lives.hpp"
#include "halo2/source_netgame_weapon.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
template<class F>const F& required(const F& fn,const char* reason) {
  if(!fn)throw std::runtime_error(reason);
  return fn;
}
std::int16_t low_short(std::int32_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(std::uint32_t(value)));
}
}
bool source_round_apply_phase(SourceRoundLifecycle& state,const SourceRoundContext& context,
    SourceGameEndStats& stats,SourceResultRecording& recording,std::span<SourceRoundPlayer> players,
    std::span<NetgameWeaponState> rows,const SourceRoundServices& services) {
  if(state.status==state.phase)return false;
  if(!context.engine_present)throw std::runtime_error("Original phase dispatch requires a selected engine");
  if(state.status==1) {
    state.ranking.fill(0);state.clock_seconds=low_short(context.time_limit_seconds);
    if(state.index>0)required(services.reset_objects,"Original round object/map reset owner unavailable")();
    if(context.raw_mode!=4) {
      for(auto& player:players)if(!(player.flags&2)) {
        player.respawn_countdown=0;player.engine_respawn_flag=0;
        if(player.replication_entity!=0xffffffffu)
          required(services.reset_respawn_replication,"Original respawn replication session/tracking owner unavailable")(player.replication_entity);
      }
      source_netgame_weapon_reset(rows,false,context.variant_flags,state.index,
          [&](std::uint32_t datum){required(services.unlink_netgame_object,"Original netgame row object owner unavailable")(datum);});
      source_game_end_accumulate(stats,context.game_over);state.flags|=0x20;
      if(recording.enabled)recording.frozen=0;
    }
    for(auto& player:players)if(!(player.flags&2)) {player.player_1a7=0;player.player_1a8=255;}
    required(services.initialize_engine,"Original engine round initializer unavailable")();
    stats.header=(stats.header&0xffffff00u)|1;
    state.start_tick=context.tick;
    if(context.raw_mode!=4)for(auto& player:players)if(!(player.flags&2)) {
      source_initialize_player_lives(player.lives,context.life_limit,true,context.raw_mode);
      if(player.replication_entity!=0xffffffffu)
        required(services.dirty_entity,"Original player replication owner unavailable")(player.replication_entity,0x20);
      player.flags|=8;
    }
  } else if(state.status==2) {
    stats.header&=0xffffff00u;
    if(context.raw_mode!=4 && recording.enabled)recording.frozen=1;
    state.countdown=int(context.tick_rate)*5;
  } else if(state.status==3) {
    state.index=low_short(int(state.index)+1);
    if(context.match_replication_entity!=0xffffffffu)
      required(services.dirty_entity,"Original match replication owner unavailable")(context.match_replication_entity,8);
    state.countdown=context.tick_rate;
  }
  required(services.phase_changed,"Original engine phase callback unavailable")(state.status);
  state.phase=low_short(state.status);
  if(context.match_replication_entity!=0xffffffffu)
    required(services.dirty_entity,"Original match replication owner unavailable")(context.match_replication_entity,2);
  return true;
}
bool source_round_post_world(SourceRoundLifecycle& state,const SourceRoundContext& context,
    SourceGameEndStats& stats,SourceResultRecording& recording,std::span<SourceRoundPlayer> players,
    std::span<NetgameWeaponState> rows,const SourceRoundServices& services) {
  if(!context.engine_present || context.raw_mode==4 || context.game_over)return false;
  bool game_over=false;auto phase_context=context;
  if(state.phase==2 || state.phase==3) {
    state.countdown=std::bit_cast<std::int32_t>(std::uint32_t(state.countdown)-1);
    if(state.countdown<=0) {
      if(state.phase==2) {
        if(state.index<31)state.status=3;
        else {
          required(services.begin_game_over,"Original game-over begin owner unavailable")();
          game_over=true;phase_context.game_over=true;
        }
      } else {state.status=1;state.flags|=0x20;}
    }
  }
  source_round_apply_phase(state,phase_context,stats,recording,players,rows,services);
  return game_over;
}
} // namespace halo2
