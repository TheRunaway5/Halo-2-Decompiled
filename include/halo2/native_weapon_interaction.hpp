#pragma once
#include "halo2/native_weapon_inventory.hpp"
namespace halo2 {
struct NativeWeaponPromptChoice {NativeActionPrompt prompt;Vec3 root;};
NativeWeaponPromptChoice native_weapon_action_prompt(const Unit &,const LevelContent &,
    const CollisionWorld &,std::span<const NativeWeaponPickup>,std::uint32_t tick);
void native_drop_weapon(Unit &,int slot,std::vector<NativeWeaponPickup> &,std::uint32_t tick,const NativeWeaponDetach & = {});
void native_drop_secondary_weapon(Unit &,std::vector<NativeWeaponPickup> &,std::uint32_t tick,const NativeWeaponDetach & = {});
struct NativeWeaponTransfer {
  std::uint32_t object=null_datum;
  bool consumed{},attached{},depleted{};
};
// The caller must pass its freshly selected authoritative prompt. Scene/item
// lifetime and respawn policies belong to the owning campaign or match.
NativeWeaponTransfer native_take_weapon(Unit &,const LevelContent &,
    std::vector<NativeWeaponPickup> &,NativeActionPrompt,bool dual_wield,std::uint32_t tick,const NativeWeaponDetach & = {});
}
