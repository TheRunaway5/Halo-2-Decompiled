#include "halo2/source_campaign_player_queries_tu15.hpp"
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint32_t at(std::uint32_t owner,std::uint32_t offset,unsigned width) {
  if(!owner || std::uint64_t(owner)+offset+width>0x100000000ull)
    throw std::invalid_argument("Campaign player query has no complete source owner");
  return owner+offset;
}
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
  std::array<std::byte,4> bytes{};memory.read(address,std::span(bytes).first(width));
  std::uint32_t result{};
  for(unsigned i=0;i<width;++i)result|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
  return result;
}
}
bool source_campaign_is_cooperative_tu15(const SourcePhysicsContactMemory &memory) {
  const auto options=read(memory,0x4f3218u);
  if(read(memory,at(options,8,4))!=1u)return false;
  return read(memory,at(options,0x134u,1),1)!=0;
}
std::uint32_t source_campaign_script_players_tu15(const SourcePhysicsContactMemory &memory,
    const SourceHsObjectListServices &lists) {
  if(!lists.allocate || !lists.append)
    throw std::invalid_argument("Campaign players query requires its HS list owners");
  const auto list=lists.allocate();
  const auto pool=read(memory,0x4f54f4u);
  const auto capacity=read(memory,at(pool,0x20u,4));
  const auto stride=read(memory,at(pool,0x24u,4));
  const auto high_water=read(memory,at(pool,0x38u,4));
  if(!capacity || capacity>16 || stride!=0x21cu || high_water>capacity)
    throw std::invalid_argument("Campaign players query requires the original player-pool layout");
  if(!high_water)return list;
  const auto rows=read(memory,at(pool,0x44u,4));
  const auto bitmap=read(memory,at(pool,0x48u,4));
  const auto occupied=read(memory,at(bitmap,0,4));
  for(std::uint32_t index=0;index<high_water;++index) {
    if(!(occupied&(1u<<index)))continue;
    const auto unit=read(memory,at(rows,index*stride+0x2cu,4));
    if(unit!=0xffffffffu)lists.append(list,unit);
  }
  return list;
}
}
