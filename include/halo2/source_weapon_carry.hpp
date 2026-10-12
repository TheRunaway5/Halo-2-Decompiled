#pragma once
#include "halo2/player_inventory_admission.hpp"
namespace halo2 {
// CD7B0's retained ordinary finite-magazine/energy inventory operands.
struct SourceWeaponCarryItem {
  std::uint32_t datum=0xffffffffu,definition=0xffffffffu,alternate=0xffffffffu,flags{};
  int reserve{},loaded{};
  bool finite_magazine{},busy{},energy{};
  float spent{}; // weapon+184: battery fraction consumed.
};
struct SourceWeaponCarryQuery {
  std::array<SourceWeaponCarryItem,4> slots;
  std::array<std::int8_t,2> hands{-1,-1};
  bool compatible{},actor{},player{};
  SourcePlayerCarryAdmission player_limits;
};
std::array<std::uint8_t,4> source_weapon_carry_permissions(
    const SourceWeaponCarryQuery &,const SourceWeaponCarryItem &);
}
