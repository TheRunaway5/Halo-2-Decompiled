#include "halo2/native_unit_interaction.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/source_weapon_carry.hpp"
#include "halo2/source_unit_interaction_target.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cmath>
#include "halo2/source_shadow.hpp"
#include "halo2/first_person_ik.hpp"
namespace halo2 {
namespace {
SourceWeaponCarryItem item(const WeaponState &w,const LevelContent &c) {
  const auto &d=c.weapons.at(w.definition);
  return {w.source_datum,w.definition,null_datum,d.inventory_flags,w.reserve,w.loaded,d.capacity>0,
      w.reload>0,d.capacity==0,w.shader.power_fraction};
}
bool replace(const Unit &u,const WeaponState &w,const LevelContent &c,bool actor) {
  if(!u.exists || u.vitality.dead || u.source_parent!=null_datum || (u.source_action_word&0x400000u))return false;
  SourceWeaponCarryQuery q;q.compatible=actor || !(c.weapons.at(w.definition).inventory_flags&(1u<<29));q.actor=actor;q.player=!actor;
  q.hands=u.source_inventory.current;q.player_limits.parent=u.source_parent;
  q.player_limits.target_definition_flags=c.weapons.at(w.definition).inventory_flags;
  for(int s=0;s<4;++s)if(const auto *weapon=native_inventory_weapon(u,s)) {
    q.slots[s]=item(*weapon,c);q.player_limits.slots[s]={weapon->source_datum,c.weapons.at(weapon->definition).inventory_flags};
  }
  for(unsigned h=0;h<2;++h){const auto s=u.source_inventory.current[h],r=u.source_inventory.requested[h];
    q.player_limits.current[h]=s<0?null_datum:u.source_inventory.slots[unsigned(s)];
    q.player_limits.requested[h]=r<0?null_datum:u.source_inventory.slots[unsigned(r)];}
  return source_weapon_carry_permissions(q,item(w,c))[2];
}
Vec3 trade_center(const Unit &u,const NativeUnitInteractionDefinition &d,std::uint64_t tick) {
  Transform root;root.position=u.body.position;
  const auto nodes=native_unit_world_nodes(u,d,tick);
  const auto markers=pose_model_markers(d.markers.at(0x04000095u),nodes,d.permutations,{},1);
  // B8D30 count-zero output is the object's resolved root.
  return markers.empty()?root.position:markers.front().world.position;
}
}
std::vector<Transform> native_unit_world_nodes(const Unit &u,const NativeUnitInteractionDefinition &d,std::uint64_t tick) {
  Transform root;root.position=u.body.position;const auto b=authored_basis({u.yaw,0,0});root.basis={b.forward,b.left,b.up};
  const auto *clip=u.custom_animation?u.custom_animation.get():d.idle.get();
  auto nodes=pose_model_nodes_world(d.hierarchy,root);
  if(clip) {
    // Marker consumers only require bones present in the current graph. Other
    // model bones (for example optional marine equipment) retain their own bind.
    auto animated=d.hierarchy;animated.bones.clear();std::vector<std::size_t> mapping;
    for(std::size_t i=0;i<d.hierarchy.bones.size();++i)if(std::find(clip->nodes.begin(),clip->nodes.end(),d.hierarchy.bones[i].name)!=clip->nodes.end()) {
      animated.bones.push_back(d.hierarchy.bones[i]);mapping.push_back(i);
    }
    const auto seconds=float(u.custom_animation?tick-u.custom_animation_tick:tick)/30.f;
    const auto posed=pose_model_nodes_world(animated,root,clip,seconds);
    for(std::size_t i=0;i<mapping.size();++i)nodes[mapping[i]]=posed[i];
  }
  return nodes;
}
ShadowCasterSphere native_unit_query_sphere(const Unit &u,const NativeUnitInteractionDefinition &d,std::uint64_t tick) {
  const auto nodes=native_unit_world_nodes(u,d,tick);
  if(nodes.empty())throw FormatError("Unit query sphere lacks its current model node0");
  ShadowCasterDefinition bounds;bounds.bounding_offset=d.bound_offset;bounds.bounding_radius=d.bound_radius;
  return source_shadow_sphere(bounds,nodes.front(),1);
}
bool native_can_trade_weapon(const Unit &player,const Unit &ally,const LevelContent &content,bool allied,std::uint64_t tick) {
  if(!content.unit_interactions || !player.exists || player.vitality.dead || player.source_parent!=null_datum ||
      !ally.exists || ally.vitality.dead || ally.bsp_hidden || !ally.visible || ally.controlled ||
      ally.source_parent!=null_datum || !allied)return false;
  const auto p=content.unit_interactions->units.find(player.definition.datum),a=content.unit_interactions->units.find(ally.definition.datum);
  if(p==content.unit_interactions->units.end() || a==content.unit_interactions->units.end())return false;
  SourceAllyTradeTarget pt{player.source_datum,null_datum,player.weapon.source_datum,trade_center(player,p->second,tick),direction(player.yaw,player.pitch)};
  SourceAllyTradeTarget at{ally.source_datum,ally.source_datum,ally.weapon.source_datum,trade_center(ally,a->second,tick),direction(ally.yaw)};
  return source_ally_trade_admitted(pt,at,{0,1,.8f,.0001f},[&](auto unit,auto weapon){
    if(unit==ally.source_datum && weapon==player.weapon.source_datum)return replace(ally,player.weapon,content,true);
    if(unit==player.source_datum && weapon==ally.weapon.source_datum)return replace(player,ally.weapon,content,false);
    throw FormatError("Trade carry check lost its actual inventory owner");});
}
NativeActionPrompt native_ally_action_prompt(const Unit &player,std::span<const Unit> allies,const LevelContent &content,
    NativeActionPrompt current,Vec3 current_root,const std::function<bool(int,int)> &allied,std::uint64_t tick) {
  for(const auto &ally:allies)if(native_can_trade_weapon(player,ally,content,allied(player.team,ally.team),tick)) {
    SourcePlayerActionTarget target{2,0,ally.source_datum};
    if(source_player_action_target_replace(current.target,target,player.body.position,current_root,ally.body.position)) {
      current.definition=ally.weapon.definition;current.message=0;current_root=ally.body.position;
    }
  }
  return current;
}
bool native_trade_weapons(Unit &player,Unit &ally,const LevelContent &content,std::uint32_t tick,bool allied) {
  if(!native_can_trade_weapon(player,ally,content,allied,tick))return false;
  const auto ps=player.source_inventory.current[0],as=ally.source_inventory.current[0];
  if(ps<0 || as<0)return false;
  std::swap(player.weapon,ally.weapon);
  for(auto *u:{&player,&ally}) {
    const auto slot=unsigned(u->source_inventory.current[0]);u->source_inventory.slots[slot]=u->weapon.source_datum;
    u->source_inventory.ready_ticks[slot]=tick;
    u->weapon.shader.item_flags=(u->weapon.shader.item_flags&~10u)|1u|(u==&player?8u:0u);
    u->weapon.burst_left=0;u->weapon.smg.input_flags=0;u->weapon.smg.hand_timer_registered=false;
    source_zoom_reset(u->source_zoom);++u->zoom_reset_serial;native_inventory_changed(*u);
  }
  player.source_player_actions.flags|=0x10u;return true;
}
}
