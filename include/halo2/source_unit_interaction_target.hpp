#pragma once
#include "halo2/source_control_target.hpp"
#include <functional>
namespace halo2 {
struct SourceAllyTradeTarget {
  std::uint32_t unit=0xffffffffu,actor=0xffffffffu,weapon=0xffffffffu;
  Vec3 center,forward;
};
struct SourceAllyTradeOperands {
  float minimum_distance,maximum_distance,minimum_aim,normalization_epsilon;
};
// E5470 direct decisions. Centers are actual CAF60 marker results; player aim is
// CB7E0, ally forward118E80. Required CD7B0 services supply their real byte2.
// This does not construct an actor or replace the carry owner with success.
bool source_ally_trade_admitted(const SourceAllyTradeTarget &player,
    const SourceAllyTradeTarget &ally,const SourceAllyTradeOperands &,
    const std::function<bool(std::uint32_t unit,std::uint32_t weapon)> &can_replace_primary);
struct SourceVehicleActionQuery {
  std::uint32_t unit=0xffffffffu;
  std::uint8_t player_flags3{};
  Vec3 bound_center,root;
  float bound_radius{};
};
struct SourceVehicleActionCandidate {
  std::uint32_t object=0xffffffffu,occupant=0xffffffffu;
  std::uint8_t unit_10a{},vehicle_348{};
  Vec3 bound_center,root;
  float bound_radius{};
};
struct SourceVehicleSeatTarget {
  std::int16_t kind{},seat=-1; // Actual C8EF0 return:0/1/2/3.
  std::uint32_t object=0xffffffffu;
  Vec3 root; // Actual B9DD0 result of the selected object, possibly a child.
};
struct SourceVehicleActionServices {
  std::function<bool(std::uint32_t)> flipped; // Actual F5DC0 result.
  std::function<bool(std::uint32_t)> unit_busy; // Actual CC0C0 result.
  std::function<SourceVehicleSeatTarget(std::uint32_t,std::uint32_t)> seat; // C8EF0.
};
// Complete151430 direct branches and151320 selection for an enumerated
// vehicle. Mandatory reached services retain animation, occupancy and physics
// ownership. This adapter never infers a usable seat from a nearby marker.
bool source_vehicle_action_candidate(const SourceVehicleActionQuery &,
    const SourceVehicleActionCandidate &,const SourceVehicleActionServices &,
    float flip_radius_add,SourcePlayerActionTarget &current,Vec3 current_root);
}
