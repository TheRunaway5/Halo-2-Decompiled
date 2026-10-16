#include "halo2/weapon_hud.hpp"
#include <algorithm>
#include <bit>
#include <set>
namespace halo2 {
WeaponHudOwnership weapon_hud_ownership(const SourcePhysicsContactMemory &memory,
    std::uint32_t unit,SourcePhysicsContactImage image) {
  const auto read=[&](std::uint32_t p,unsigned size=4) {
    std::array<std::byte,4> bytes{};memory.read(p,std::span(bytes).first(size));
    std::uint32_t value{};
    for(unsigned n=0;n<size;++n)value|=std::to_integer<std::uint32_t>(bytes[n])<<(8*n);
    return value;
  };
  const bool later=image==SourcePhysicsContactImage::TitleUpdate15;
  if(!later && image!=SourcePhysicsContactImage::MapPack11)
    throw FormatError("Weapon HUD ownership requires its original image layout");
  const auto headers=read(read(later?0x4ecbd0u:0x4e78d0u)+0x44u);
  const auto tags=read(later?0x4f0414u:0x4eb114u);
  const auto body=[&](std::uint32_t object){return read(headers+(object&65535u)*12u+8u);};
  const auto definition=[&](std::uint32_t p){return read(tags+(read(p)&65535u)*16u+8u);};
  const auto equipped=[&](std::uint32_t p) {
    const auto slot=std::bit_cast<std::int8_t>(std::uint8_t(read(p+0x212u,1)));
    return slot==-1?0xffffffffu:read(p+0x218u+std::uint32_t(std::int32_t(slot)*4));
  };
  WeaponHudOwnership result;
  const auto player=body(unit),parent=read(player+0x14u);
  const auto seat=std::bit_cast<std::int16_t>(std::uint16_t(read(player+0x1fcu,2)));
  if(parent==0xffffffffu || seat==-1)return result;
  result.seated=true;
  const auto vehicle=body(parent);
  const auto seat_record=read(definition(vehicle)+0x1ccu)+std::uint32_t(std::int32_t(seat)*0xb0);
  const auto flags=read(seat_record);
  // Complete22E0F0: use campaign interface0 or multiplayer interface1,
  // bounded by the authored count and then tested as a signed short.
  const auto count=std::bit_cast<std::int32_t>(read(seat_record+0x7cu)-1u);
  const auto multiplayer=read(read(later?0x4f3218u:0x4edf18u)+8u)==2u;
  const auto selected=std::bit_cast<std::int16_t>(std::uint16_t(std::min(count,multiplayer?1:0)));
  if(selected>=0)result.parent_hud=read(read(seat_record+0x80u)+unsigned(selected)*8u+4u);
  result.handheld_visible=(flags&0x20u)!=0;
  if(!(flags&8u))return result;
  result.vehicle_weapon=equipped(vehicle);
  if(result.vehicle_weapon!=0xffffffffu)return result;
  std::set<std::uint32_t> visited;
  for(auto child=read(vehicle+0x10u);child!=0xffffffffu;) {
    if(!visited.insert(child).second)throw FormatError("Weapon HUD child hierarchy contains a cycle");
    const auto p=body(child),type=read(p+0xaau,1);
    if((read(p+4u)&0x4000000u) && ((1u<<(type&31u))&3u) &&
        (read(definition(p)+0xbcu)&0x4000000u)) {
      result.vehicle_weapon=equipped(p);
      if(result.vehicle_weapon!=0xffffffffu)break;
    }
    child=read(p+0xcu);
  }
  return result;
}
}
