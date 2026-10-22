#include "halo2/native_weapon_inventory.hpp"
#include <algorithm>
#include <bit>
namespace halo2 {
int native_weapon_ammunition_available(const Unit &u,const WeaponState &weapon) {
  const auto word=[](int value){return int(std::bit_cast<std::int16_t>(std::uint16_t(value)));};
  int result=word(weapon.loaded)+word(weapon.reserve);
  if(u.source_datum!=null_datum)for(int slot=0;slot<4;++slot)
    if(const auto *other=native_inventory_weapon(u,slot);other &&
        other->source_datum!=weapon.source_datum && other->definition==weapon.definition)
      result+=word(other->reserve);
  return result;
}

WeaponState *native_inventory_weapon(Unit &u, int slot) {
  auto &i=u.source_inventory;
  if(slot<0 || slot>=int(i.slots.size()) || i.slots[slot]==null_datum)return nullptr;
  if(i.current[0]==slot)return u.weapon.source_datum==i.slots[slot]?&u.weapon:nullptr;
  auto &w=i.inactive_weapons[slot];
  return w && w->source_datum==i.slots[slot]?&*w:nullptr;
}
const WeaponState *native_inventory_weapon(const Unit &u,int slot) {
  return native_inventory_weapon(const_cast<Unit &>(u),slot);
}
WeaponState *native_hand_weapon(Unit &u,unsigned hand) {
  return hand<2?native_inventory_weapon(u,u.source_inventory.current[hand]):nullptr;
}
const WeaponState *native_hand_weapon(const Unit &u,unsigned hand) {
  return native_hand_weapon(const_cast<Unit &>(u),hand);
}
WeaponState native_weapon_state(const WeaponDefinition &d,std::uint32_t datum) {
  WeaponState w;w.definition=d.datum;w.source_datum=datum;
  w.smg.hand_timer_registered=false; // Fresh world items have no167D32 local view.
  const auto total=std::max(0,d.initial_rounds<0?d.capacity:d.initial_rounds);
  w.loaded=std::min(total,std::max(0,d.capacity));
  w.reserve=std::min(std::max(0,total-w.loaded),std::max(0,d.reserve));
  w.smg.loaded=w.loaded;w.smg.reserve=w.reserve;
  if(d.smg)smg_initial_equip(w.smg,*d.smg);
  return w;
}
void native_inventory_changed(Unit &u) {
  auto &i=u.source_inventory;
  i.inventory_sequence=std::uint16_t((i.inventory_sequence+1)&15);
  i.replicated_sequence=i.inventory_sequence;
}
bool native_select_weapon(Unit &u,int slot,std::uint32_t tick,const NativeWeaponDetach &detach) {
  auto &i=u.source_inventory;
  if(slot==i.current[0] || slot==i.current[1] || !native_inventory_weapon(u,slot))return false;
  const auto previous=i.current[0];
  if(previous>=0 && detach)detach(u,u.weapon);
  auto next=std::move(*i.inactive_weapons[slot]);i.inactive_weapons[slot].reset();
  if(previous>=0) {
    u.weapon.smg.attached=false;u.weapon.smg.hand_timer_registered=false;
    u.weapon.shader.item_flags|=2u; // FB3F0 holstered item.
    i.inactive_weapons[previous]=std::move(u.weapon);
  }
  next.smg.attached=true;next.smg.hand_timer_registered=false; // Actual view rebuild owns re-registration.
  next.shader.item_flags&=~2u; // FB120 reattachment clears the holster bit.
  u.weapon=std::move(next);i.current[0]=i.requested[0]=std::int8_t(slot);i.ready_ticks[slot]=tick;
  source_zoom_reset(u.source_zoom);++u.zoom_reset_serial;
  u.source_action_word&=~1u;
  native_inventory_changed(u);return true;
}
}
