#include "halo2/source_result_retention.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
bool live(const SourceResultLivePlayers& players,unsigned slot,std::int32_t high_water) {
  return std::int32_t(slot)<high_water && players[slot].salt!=0;
}
void validate(const SourceResultLivePlayers& players,std::int32_t high_water) {
  for(unsigned i=0;i<16;++i)if(live(players,i,high_water) &&
      (players[i].machine < -1 || players[i].machine>=16))
    throw std::invalid_argument("Retained source machine index escapes the recovered roster");
}
int find_machine(const SourceRetainedResults& results,const SourceResultMachineIdentity& identity) {
  for(unsigned i=0;i<16;++i)if(results.machines[i].occupied && results.machines[i].identity==identity)
    return int(i);
  return -1;
}
int add_machine(SourceRetainedResults& results,const SourceResultMachineIdentity& identity) {
  for(unsigned i=0;i<16;++i)if(!results.machines[i].occupied) {
    results.machines[i]={};results.machines[i].identity=identity;results.machines[i].occupied=1;
    return int(i);
  }
  return -1;
}
}
void source_result_bind_machines(SourceRetainedResults& r,const SourceResultLivePlayers& players,
    std::int32_t high_water,const SourceResultMachineRoster& roster,SourceResultMachinePolicy policy) {
  validate(players,high_water);
  std::uint32_t used=0;
  for(unsigned i=0;i<16;++i) {
    if(live(players,i,high_water) && players[i].machine!=-1)
      r.players[i].machine=std::int8_t(find_machine(r,roster[unsigned(players[i].machine)]));
    if(r.players[i].occupied && r.players[i].machine!=-1)
      used|=1u<<(std::uint8_t(r.players[i].machine)&31u);
  }
  if(policy==SourceResultMachinePolicy::PruneUnreferenced)
    for(unsigned i=0;i<16;++i)if(!(used&(1u<<i)))r.machines[i]={};
  for(unsigned i=0;i<16;++i)if(live(players,i,high_water) &&
      players[i].machine!=-1 && r.players[i].machine==-1) {
    const auto& identity=roster[unsigned(players[i].machine)];
    auto index=find_machine(r,identity);if(index==-1)index=add_machine(r,identity);
    r.players[i].machine=std::int8_t(index);
  }
}
void source_result_remap(SourceRetainedResults& r,SourceResultRecording recording,
    const SourceResultLivePlayers& players,std::int32_t high_water,
    const SourceResultMachineRoster& roster,SourceResultMachinePolicy policy) {
  if(!recording.enabled || r.header[1])return;
  validate(players,high_water);
  std::array<int,16> previous,following;previous.fill(-1);following.fill(-1);
  for(unsigned slot=0;slot<16;++slot)if(live(players,slot,high_water))
    for(unsigned old=0;old<16;++old)if(r.players[old].occupied && r.players[old].identity==players[slot].identity) {
      // Original stops at the first identity match even if already claimed.
      // A later duplicate live identity therefore takes the unmatched path.
      if(following[old]==-1){previous[slot]=int(old);following[old]=int(slot);}break;
    }
  const auto old_properties=r.players;
  for(unsigned slot=0;slot<16;++slot)if(live(players,slot,high_water)) {
    auto& row=r.players[slot];
    if(previous[slot]==-1) {
      row.occupied=1;row.identity=players[slot].identity;row.configuration=players[slot].configuration;
      row.rank=-1;row.machine=-1;
    } else row=old_properties[unsigned(previous[slot])];
  }
  const auto old_players=r.statistics.players;
  for(unsigned slot=0;slot<16;++slot)
    r.statistics.players[slot]=previous[slot]==-1 ? decltype(old_players)::value_type{} : old_players[unsigned(previous[slot])];
  const auto old_pvp=r.statistics.pvp;
  for(unsigned row=0;row<16;++row)for(unsigned column=0;column<16;++column)
    r.statistics.pvp[row][column]=previous[row]==-1 || previous[column]==-1 ?
        std::array<std::uint16_t,2>{} : old_pvp[unsigned(previous[row])][unsigned(previous[column])];
  for(auto& event:r.events)for(unsigned field:{1u,2u}) {
    const auto old=std::bit_cast<std::int8_t>(event[field]);
    event[field]=old<0 || old>=16 ? 0xffu : std::uint8_t(following[unsigned(old)]);
  }
  source_result_bind_machines(r,players,high_water,roster,policy);
}
} // namespace halo2
