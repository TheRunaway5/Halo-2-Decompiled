#include "halo2/source_unit_interaction_target.hpp"
#include <iostream>
using namespace halo2;
int main()try {
  char kind;
  while(std::cin>>kind) {
    if(kind=='t') {
      SourceAllyTradeTarget player,ally;SourceAllyTradeOperands k;
      unsigned actor,pw,aw,ap,pp;
      std::cin>>actor>>pw>>aw>>ally.center.x>>ally.center.y>>ally.center.z
          >>player.forward.x>>player.forward.y>>player.forward.z
          >>ally.forward.x>>ally.forward.y>>ally.forward.z>>ap>>pp
          >>k.minimum_distance>>k.maximum_distance>>k.minimum_aim>>k.normalization_epsilon;
      player.unit=0x10001;ally.unit=0x10002;ally.actor=actor?0x10007:0xffffffffu;
      player.weapon=pw?0x10003:0xffffffffu;ally.weapon=aw?0x10004:0xffffffffu;
      unsigned calls{};
      const auto ok=source_ally_trade_admitted(player,ally,k,[&](auto unit,auto weapon){
        ++calls;if(unit==ally.unit && weapon==player.weapon)return bool(ap);
        if(unit==player.unit && weapon==ally.weapon)return bool(pp);
        throw FormatError("Wrong trade carry query identity/order");
      });
      std::cout<<ok<<' '<<calls<<'\n';
    } else if(kind=='v') {
      SourceVehicleActionQuery q;SourceVehicleActionCandidate c;SourcePlayerActionTarget target;
      unsigned dead,flipped,busy,flags,locked,occupied,unit,seat_kind;float distance;
      std::cin>>dead>>flipped>>busy>>flags>>locked>>occupied>>unit>>seat_kind>>distance;
      q.unit=unit?0x10001:0xffffffffu;q.player_flags3=flags;q.bound_radius=.3f;
      c.object=0x10002;c.unit_10a=dead?4:0;c.vehicle_348=locked?0x40:0;
      c.occupant=occupied?0x10003:0xffffffffu;c.bound_center.x=distance;c.bound_radius=.6f;c.root=c.bound_center;
      unsigned calls{};SourceVehicleActionServices s;
      s.flipped=[&](auto){calls=calls*10+1;return bool(flipped);};
      s.unit_busy=[&](auto){calls=calls*10+2;return bool(busy);};
      s.seat=[&](auto,auto){calls=calls*10+3;return SourceVehicleSeatTarget{std::int16_t(seat_kind),2,0x10002,c.root};};
      const auto ok=source_vehicle_action_candidate(q,c,s,.2f,target,{});
      std::cout<<ok<<' '<<target.action<<' '<<target.index<<' '<<target.object<<' '<<calls<<'\n';
    } else throw FormatError("Unknown oracle scenario");
  }
  // A reached unavailable owner must be explicit, while an earlier gate
  // must return without asking an unrelated subsystem to pretend success.
  SourceVehicleActionQuery q;SourceVehicleActionCandidate c;c.unit_10a=4;
  SourcePlayerActionTarget target;
  if(source_vehicle_action_candidate(q,c,{},.2f,target,{}))throw FormatError("Dead vehicle passed");
  c.unit_10a=0;bool failed{};
  try{source_vehicle_action_candidate(q,c,{},.2f,target,{});}catch(const FormatError &){failed=true;}
  if(!failed)throw FormatError("Vehicle prompt manufactured unavailable flip/seat owners");
  return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
