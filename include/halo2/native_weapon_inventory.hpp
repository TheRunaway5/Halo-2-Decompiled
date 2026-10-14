#pragma once
#include "halo2/simulation.hpp"
namespace halo2 {
WeaponState *native_inventory_weapon(Unit &, int slot);
const WeaponState *native_inventory_weapon(const Unit &, int slot);
WeaponState *native_hand_weapon(Unit &, unsigned hand);
const WeaponState *native_hand_weapon(const Unit &, unsigned hand);
WeaponState native_weapon_state(const WeaponDefinition &, std::uint32_t datum);
using NativeWeaponDetach=std::function<void(Unit &,WeaponState &)>;
bool native_select_weapon(Unit &, int slot, std::uint32_t tick,const NativeWeaponDetach & = {});
//104AA0/104CF0 finite branch: current loaded/reserve plus matching other
// full-datum inventory owners. Duplicate self slots and other loaded rounds
// are excluded; signed16 words retain their original interpretation.
int native_weapon_ammunition_available(const Unit &,const WeaponState &);
void native_inventory_changed(Unit &);
}
