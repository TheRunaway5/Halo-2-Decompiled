#pragma once
#include "halo2/simulation.hpp"
namespace halo2 {
WeaponState *native_inventory_weapon(Unit &, int slot);
const WeaponState *native_inventory_weapon(const Unit &, int slot);
WeaponState *native_hand_weapon(Unit &, unsigned hand);
const WeaponState *native_hand_weapon(const Unit &, unsigned hand);
WeaponState native_weapon_state(const WeaponDefinition &, std::uint32_t datum);
bool native_select_weapon(Unit &, int slot, std::uint32_t tick);
//104AA0/104CF0 finite-ammo branch: current loaded plus reserve from every
// matching inventory owner. Loaded rounds in other weapons are excluded.
int native_weapon_ammunition_available(const Unit &,const WeaponState &);
void native_inventory_changed(Unit &);
}
