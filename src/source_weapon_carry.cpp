#include "halo2/source_weapon_carry.hpp"
namespace halo2 {
std::array<std::uint8_t,4> source_weapon_carry_permissions(
    const SourceWeaponCarryQuery &q,const SourceWeaponCarryItem &target) {
  constexpr auto none=0xffffffffu;
  std::array<std::uint8_t,4> p{};
  if(!q.compatible || target.datum==none)return p;
  const bool empty=target.finite_magazine && target.loaded==0 && target.reserve==0;
  const bool dual=target.flags&((1u<<22)|(1u<<23));
  bool matching=false;int total_reserve=0;
  const auto matches=[&](const auto &w){return w.definition==target.definition || w.definition==target.alternate;};
  for(const auto &w:q.slots)if(w.datum!=none && matches(w)){matching=true;total_reserve+=w.reserve;}
  std::array<bool,2> other{};
  std::array<const SourceWeaponCarryItem*,2> held{};
  for(unsigned h=0;h<2;++h) {
    const auto slot=q.hands[h];if(slot>=0 && slot<4 && q.slots[unsigned(slot)].datum!=none)held[h]=&q.slots[unsigned(slot)];
    p[h]=!held[h] || (!(held[h]->flags&8u) && !held[h]->busy);
    p[h+2]=held[h]!=nullptr;
  }
  if(dual) {
    if(!held[0] || !(held[0]->flags&((1u<<22)|(1u<<23))))p[1]=p[3]=0;
    if(target.flags&(1u<<23))p[0]=p[2]=0;
    other={held[1]!=nullptr,held[0]!=nullptr};
  } else p[1]=p[3]=0;
  if(matching)for(unsigned h=0;h<2;++h) {
    if(!other[h])p[h]=0;
    if(p[h+2] && (( !matches(*held[h]) || held[h]->spent<=target.spent) &&
        (matches(*held[h]) || !other[h])))p[h]=p[h+2]=0;
  }
  if(empty)for(unsigned h=0;h<2;++h) {
    if(!total_reserve || (other[h] && held[h] && held[h]->definition==target.definition && held[h]->reserve==total_reserve))other[h]=false;
    if(!other[h])p[h]=p[h+2]=0;
  }
  if(q.actor && ((target.energy && target.spent==1.f) || empty))p={};
  if(q.player)source_restrict_player_inventory(p,q.player_limits);
  return p;
}
}
