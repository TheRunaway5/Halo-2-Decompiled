#include "halo2/source_unit_interaction_target.hpp"
#include "halo2/math.hpp"
#include <cmath>
namespace halo2 {
bool source_ally_trade_admitted(const SourceAllyTradeTarget &player,
    const SourceAllyTradeTarget &ally,const SourceAllyTradeOperands &k,
    const std::function<bool(std::uint32_t,std::uint32_t)> &replace) {
  constexpr auto none=0xffffffffu;
  if(ally.actor==none || ally.weapon==none || player.weapon==none)return false;
  auto delta=ally.center-player.center;
  const long double x=delta.x,y=delta.y,z=delta.z;
  const float distance=float(std::sqrt((x*x+y*y)+z*z));
  //30BF0 returns XMM0=zero below its x87 epsilon; it leaves the vector.
  if(!(k.normalization_epsilon<=std::abs(distance)))return false;
  delta=delta*(1.f/distance);
  if(!(k.minimum_distance<distance && distance<k.maximum_distance))return false;
  const float aim=(delta.z*player.forward.z+delta.y*player.forward.y)+delta.x*player.forward.x;
  if(!(k.minimum_aim<aim))return false;
  const float facing=(delta.z*ally.forward.z+delta.y*ally.forward.y)+delta.x*ally.forward.x;
  if(!(facing<0.f))return false;
  if(!replace)throw FormatError("Ally trade requires actual CD7B0 carry admission");
  if(!replace(ally.unit,player.weapon))return false;
  return replace(player.unit,ally.weapon);
}
bool source_vehicle_action_candidate(const SourceVehicleActionQuery &q,
    const SourceVehicleActionCandidate &c,const SourceVehicleActionServices &s,
    float flip_radius_add,SourcePlayerActionTarget &current,Vec3 current_root) {
  if(c.unit_10a&4u)return false;
  if(!s.flipped)throw FormatError("Vehicle target requires actual F5DC0 owner");
  SourcePlayerActionTarget candidate;
  Vec3 root=c.root;
  if(!s.flipped(c.object)) {
    if(!s.unit_busy)throw FormatError("Vehicle target requires actual CC0C0 owner");
    if(s.unit_busy(q.unit) || (q.player_flags3&8u))return false;
    if(!s.seat)throw FormatError("Vehicle target requires actual C8EF0 seat owner");
    const auto seat=s.seat(q.unit,c.object);
    if(!seat.kind)return false;
    if(seat.kind<1 || seat.kind>3 || seat.object==0xffffffffu || seat.seat<0)
      throw FormatError("Invalid actual vehicle seat target");
    candidate={std::int16_t(seat.kind+3),seat.seat,seat.object};root=seat.root;
  } else {
    if(q.unit==0xffffffffu || (c.vehicle_348&0x40u) || c.occupant!=0xffffffffu)return false;
    const auto delta=c.bound_center-q.bound_center;
    const float squared=(delta.z*delta.z+delta.x*delta.x)+delta.y*delta.y;
    const float radius=(q.bound_radius+c.bound_radius)+flip_radius_add;
    if(radius*radius<=squared)return false;
    candidate={8,0,c.object};
  }
  return source_player_action_target_replace(current,candidate,q.root,current_root,root);
}
}
