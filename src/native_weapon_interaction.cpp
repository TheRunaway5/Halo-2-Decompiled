#include "halo2/native_weapon_interaction.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/math.hpp"
#include "halo2/pickup_visibility.hpp"
#include "halo2/player_inventory_admission.hpp"
#include <algorithm>
#include <limits>
namespace halo2 {
NativeWeaponPromptChoice native_weapon_action_prompt(const Unit &u,const LevelContent &content,
    const CollisionWorld &collision,std::span<const NativeWeaponPickup> items,std::uint32_t tick) {
  NativeActionPrompt result;const auto &inventory=u.source_inventory;
  if(!u.exists || u.vitality.dead || u.source_parent!=null_datum || (u.source_action_word&0x400000u))return {};
  const auto basis=authored_basis({u.yaw,0,0});
  const auto bound=u.body.position+basis.forward*u.definition.bounding_offset.x+
      basis.left*u.definition.bounding_offset.y+basis.up*u.definition.bounding_offset.z;
  const auto capsule=pickup_capsule(u.body.position,{u.definition.physics_flags,u.definition.height,
      u.definition.crouch_height,u.definition.radius},u.crouch);
  const NativeWeaponPickup *target=nullptr;float nearest=std::numeric_limits<float>::max();int priority=0;
  for(const auto &p:items) {
    if(!p.visible || !p.weapon || tick<p.unlock_tick)continue;
    const auto &w=content.weapons.at(p.definition);
    if(w.capacity>0 && p.weapon->loaded+p.weapon->reserve==0)continue;
    const auto center=p.position+p.forward*w.bounding_offset.x+
        cross(p.up,p.forward)*w.bounding_offset.y+p.up*w.bounding_offset.z;
    const float radius=u.definition.bounding_radius+(w.pickup_priority?.1f:0.f)+.3f;
    const auto delta=center-bound;
    if(dot(delta,delta)>=radius*radius)continue;
    const int rank=w.pickup_priority?7:1;const auto root=p.position-u.body.position;
    const float distance=dot(root,root);
    if(rank<priority || (rank==priority && distance>=nearest))continue;
    const auto visibility=pickup_visibility(capsule,p.position,u.source_datum,p.handle,0,2,
        [&](const PickupVisibilityCast &cast) {
          const auto hit=collision.raycast(cast.start,cast.delta,length(cast.delta));
          return PickupVisibilityHit{bool(hit),hit?hit->position:Vec3{}};
        });
    if(!visibility.visible)continue;
    const auto &definition=w;
    SourcePlayerCarryAdmission carry;carry.parent=u.source_parent;
    carry.target_definition_flags=definition.inventory_flags;
    for(unsigned slot=0;slot<4;++slot) {
      carry.slots[slot].datum=inventory.slots[slot];
      if(const auto *w=native_inventory_weapon(u,int(slot)))
        carry.slots[slot].definition_flags=content.weapons.at(w->definition).inventory_flags;
      else if(inventory.slots[slot]!=null_datum)throw FormatError("Prompt has no retained inventory owner");
    }
    const auto datum=[&](std::int8_t slot){
      if(slot==-1)return null_datum;
      if(slot<0 || slot>=4)throw FormatError("Prompt has invalid hand selector");
      return inventory.slots[unsigned(slot)];
    };
    for(unsigned hand=0;hand<2;++hand){carry.current[hand]=datum(inventory.current[hand]);carry.requested[hand]=datum(inventory.requested[hand]);}
    std::array<std::uint8_t,4> permissions{1,1,1,1};source_restrict_player_inventory(permissions,carry);
    // Keep these native capability checks identical to the actual transfer.
    if(inventory.current[1]>=0 || !definition.dual_wieldable ||
        u.weapon.definition==null_datum || !content.weapons.at(u.weapon.definition).dual_wieldable)
      permissions[1]=permissions[3]=0;
    permissions[3]=0; // Native offhand replacement has no live transfer owner.
    for(int slot=0;slot<4;++slot)if(const auto *w=native_inventory_weapon(u,slot);
        w && w->definition==p.definition &&
        std::min(std::max(0,definition.reserve-w->reserve),p.weapon->loaded+p.weapon->reserve)==0)
      permissions[0]=permissions[2]=0;
    const auto flags=std::int16_t((permissions[0]?1:0)|(permissions[2]?2:0)|
        (permissions[1]?4:0)|(permissions[3]?8:0));
    // CD7B0 admission precedes151320 priority selection. An unusable nearby
    // item must not hide another item whose displayed action can be taken.
    if(!flags)continue;
    target=&p;nearest=distance;priority=rank;
    result={{std::int16_t(priority),flags,p.handle},p.definition,0};
  }
  return {result,target?target->position:Vec3{}};
}
void native_drop_weapon(Unit &u,int slot,std::vector<NativeWeaponPickup> &items,std::uint32_t tick) {
  auto *selected=native_inventory_weapon(u,slot);if(!selected)return;
  const auto handle=selected->source_datum,definition=selected->definition;
  auto item=std::find_if(items.begin(),items.end(),
      [&](const auto &p){return p.handle==handle;});
  if(item==items.end()) {
    items.push_back({});item=std::prev(items.end());
  }
  item->handle=handle;item->definition=definition;item->weapon=std::move(*selected);
  // Stop held inputs/pending bursts without replenishing either magazine.
  item->weapon->burst_left=0;item->weapon->smg.input_flags=0;
  item->weapon->smg.hand_timer_registered=false;
  item->weapon->shader.item_flags&=~9u; // FB120 detach clears active/player-owned bits.
  item->position=u.body.position;
  item->forward=direction(u.yaw);item->up={0,0,1};
  item->unlock_tick=tick;item->visible=true;item->bsp=u.bsp;
  auto &i=u.source_inventory;i.slots[slot]=null_datum;i.inactive_weapons[slot].reset();
  for(unsigned h=0;h<2;++h)if(i.current[h]==slot)i.current[h]=i.requested[h]=-1;
  if(u.weapon.source_datum==handle)u.weapon={};
  native_inventory_changed(u);
}
void native_drop_secondary_weapon(Unit &u,std::vector<NativeWeaponPickup> &items,std::uint32_t tick) {
  if(u.source_inventory.current[1]>=0)native_drop_weapon(u,u.source_inventory.current[1],items,tick);
}
NativeWeaponTransfer native_take_weapon(Unit &u,const LevelContent &content,
    std::vector<NativeWeaponPickup> &items,NativeActionPrompt prompt,bool dual_wield,std::uint32_t tick) {
  NativeWeaponTransfer result{prompt.target.object};
  if(prompt.target.action!=1 && prompt.target.action!=7)return result;
  auto &inventory=u.source_inventory;
    auto target=std::find_if(items.begin(),items.end(),
        [&](const auto &p){return p.handle==prompt.target.object && p.weapon.has_value();});
    if(target==items.end())return result;
    const auto &definition=content.weapons.at(target->definition);
    const auto flags=std::uint16_t(prompt.target.index);
    const std::array<std::uint8_t,4> carry_permissions{
        std::uint8_t(bool(flags&1)),std::uint8_t(bool(flags&4)),
        std::uint8_t(bool(flags&2)),std::uint8_t(bool(flags&8))};
    if(!(carry_permissions[dual_wield?1:0] || carry_permissions[dual_wield?3:2]))return result;
    if(dual_wield) {
      if(!carry_permissions[1])return result;
      if(inventory.current[1]>=0 || !definition.dual_wieldable ||
          !content.weapons.at(u.weapon.definition).dual_wieldable)return result;
    } else {
      // Same-definition pickup transfers reserve rounds, preserving both real items.
      WeaponState *same=nullptr;
      for(int slot=0;slot<4;++slot)
        if(auto *w=native_inventory_weapon(u,slot);w && w->definition==target->definition){same=w;break;}
      if(same) {
        const auto transfer=std::min(std::max(0,definition.reserve-same->reserve),
            target->weapon->loaded+target->weapon->reserve);
        if(!transfer)return result;
        const auto reserve=std::min(transfer,target->weapon->reserve);
        target->weapon->reserve-=reserve;target->weapon->loaded-=transfer-reserve;
        same->reserve+=transfer;same->smg.reserve=same->reserve;
        u.source_player_actions.flags|=0x10u;result.consumed=true;
        target->weapon->smg.reserve=target->weapon->reserve;target->weapon->smg.loaded=target->weapon->loaded;
        if(!target->weapon->loaded && !target->weapon->reserve) {
          target->weapon.reset();
          result.depleted=true;
        }
        return result;
      }
      const auto target_handle=target->handle;
      native_drop_secondary_weapon(u,items,tick);
      // A drop can reallocate the vector. Reacquire the target by full datum.
      const auto handle=target_handle;
      target=std::find_if(items.begin(),items.end(),[&](const auto &p){return p.handle==handle;});
    }
    const auto handle=target->handle;
    int slot=-1;
    for(int n=0;n<4;++n)
      if(slot<0 && inventory.slots[n]==null_datum)slot=n;
    if(!dual_wield && !carry_permissions[0]) {
      if(!carry_permissions[2] || inventory.current[0]<0)return result;
      slot=inventory.current[0];native_drop_weapon(u,slot,items,tick);
      target=std::find_if(items.begin(),items.end(),[&](const auto &p){return p.handle==handle;});
    }
    if(slot<0)return result;

    // FB120 player attachment: retain upper bytes, apply the literal low-byte
    // masks, then FB360 derives bit3 from the actual source player binding.
    auto &item_flags=target->weapon->shader.item_flags;
    item_flags=(item_flags&~0xffu)|(((item_flags|5u)&0xfdu)&0x1fu);
    if(u.source_player!=null_datum)item_flags|=8u;else item_flags&=~8u;
    inventory.slots[slot]=handle;inventory.inactive_weapons[slot]=std::move(*target->weapon);target->weapon.reset();
    inventory.ready_ticks[slot]=0;
    if(dual_wield)inventory.current[1]=inventory.requested[1]=std::int8_t(slot);
    else native_select_weapon(u,slot,tick);
    u.source_player_actions.flags|=dual_wield?0x20u:0x10u;
    result.consumed=result.attached=true;
    native_inventory_changed(u);
    source_zoom_reset(u.source_zoom);++u.zoom_reset_serial;
  return result;
}
}
