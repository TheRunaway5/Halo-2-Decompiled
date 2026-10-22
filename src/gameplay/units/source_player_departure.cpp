#include "halo2/source_player_departure.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
template<class T>const T& required(const T& service,const char* message) {
  if(!service)throw std::runtime_error(message);
  return service;
}
std::int16_t decrement(std::int16_t value) {
  return std::bit_cast<std::int16_t>(std::uint16_t(std::uint16_t(value)-1u));
}
}
bool source_simulation_player_depart(std::span<SourceDeparturePlayer> players,
    std::int32_t slot,const SourcePlayerIdentity& identity,SourceDepartureCensus& census,
    const SourceDepartureContext& context,const SourceDepartureServices& services) {
  if(players.size()>16)throw std::invalid_argument("Departure pool exceeds original player capacity");
  if(slot<0 || std::size_t(slot)>=players.size())return false;
  auto& player=players[slot];
  if(!player.salt || player.identity!=identity || (player.flags&2))return false;
  const auto datum=(std::uint32_t(player.salt)<<16)|std::uint32_t(slot);
  if(player.current_unit!=0xffffffffu)
    required(services.mark_current_unit,"Original departure unit mark owner unavailable")(player.current_unit);
  if(context.raw_mode==4)
    required(services.mode4_clear_unit_backlinks,"Original mode4 backlink owner unavailable")(datum);
  required(services.unlink_current_unit,"Original player-unit unlink owner unavailable")(datum);
  player.previous_unit=0xffffffffu;
  if(context.world_present && !context.world_flag)
    required(services.simulation_leave,"Original simulation player leave owner unavailable")(datum);
  player.machine_index=-1;player.machine={};player.machine_user=-1;player.machine_controller=-1;
  if(player.local_index!=-1) {
    const auto local=player.local_index;
    if(local<0 || local>=4)throw std::invalid_argument("Departure local user outside original census");
    required(services.control_set_unit,"Original departure local control owner unavailable")(local,0xffffffffu);
    required(services.first_person_set_unit,"Original departure local view owner unavailable")(0xffffffffu,local,-1);
    census.users[local]=0xffffffffu;census.user_count=decrement(census.user_count);
    player.local_index=-1;
  }
  if(player.controller_index!=-1) {
    const auto controller=player.controller_index;
    if(controller<0 || controller>=4)throw std::invalid_argument("Departure controller outside original census");
    census.controllers[controller]=0xffffffffu;census.controller_count=decrement(census.controller_count);
    player.controller_index=-1;
  }
  player.flags|=2;player.leave_tick=context.tick;player.flags&=std::uint16_t(~1u);
  census.count=std::bit_cast<std::int32_t>(std::uint32_t(census.count)-1u);
  if(context.engine_present) {
    if(context.raw_mode!=4 && context.engine_team_mode)
      required(services.refresh_team_masks,"Original departure team mask owner unavailable")();
    required(services.engine_leave,"Original engine leave callback unavailable")(datum);
    required(services.publish_leave_event,"Original departure event owner unavailable")(
        datum,std::bit_cast<std::int8_t>(player.configuration[0x7c]));
    required(services.remap_results,"Original departure result remap owner unavailable")();
    required(services.dirty_player,"Original departure replication owner unavailable")(
        std::int16_t(slot),0x200);
  }
  return true;
}
}
