#include "halo2/native_unit_interaction.hpp"
#include "halo2/simulation.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/source_unit_interaction_target.hpp"
#include "halo2/math.hpp"
#include "halo2/source_netgame_weapon.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/first_person_ik.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
namespace {
constexpr float native_vehicle_empty_distance_squared=100.f; // XBE 445420; validated in the operand audit.
Transform root(const SceneObjects::Object &o) {
 if(o.solved_transform)return *o.solved_transform;
 Transform r;r.position=o.position;r.scale=o.scale;const auto b=authored_basis(o.rotation);r.basis={b.forward,b.left,b.up};return r;
}
std::vector<Transform> nodes(const NativeUnitInteractionDefinition &d,const SceneObjects::Object &o) {
 return o.solved_nodes?*o.solved_nodes:pose_model_nodes_world(d.hierarchy,root(o));
}
std::vector<PosedModelMarker> marker(const NativeUnitInteractionDefinition &d,const SceneObjects::Object &o,std::uint32_t name,unsigned max=1) {
 const auto found=d.markers.find(name);if(found==d.markers.end())return {};
 return pose_model_markers(found->second,nodes(d,o),d.permutations,{},max);
}
float angle(Vec3 a,Vec3 b){return std::acos(std::clamp(dot(a,b),-1.f,1.f));}
struct SeatChoice {std::int16_t rank{},seat=-1;float distance{};const NativeUnitSeatAnimations::Entry *entry{};};
SeatChoice choose(const Unit &u,const NativeUnitInteractionAssets &assets,const SceneObjects::Object &o,
    const NativeUnitInteractionDefinition &d,std::span<const Unit *const> units,const CollisionWorld &collision,const LevelContent &content,bool multiplayer,const std::function<bool(const Unit&,const Unit&)> &allied) {
 SeatChoice best;const auto actor=assets.units.find(u.definition.datum);if(actor==assets.units.end())return best;
 if(u.weapon.reload>0 || (native_hand_weapon(u,1) && native_hand_weapon(u,1)->reload>0))return best;
 const auto eye=ordinary_unit_eye(u.body.position,u.definition.camera_height,u.definition.crouch_camera_height,u.crouch);
 bool driver=false;
 for(unsigned index=0;index<d.seats.size();++index) {
  const auto &s=d.seats[index];if((u.controlled?s.flags&(1u<<16):false))continue;
  // The explicit local player uses the player path even when no raw player datum exists.
  if(s.flags&(1u<<16))continue;
  const auto entry_markers=marker(d,o,s.entry_marker,4);bool reachable=false;
  for(const auto &m:entry_markers) {
   auto delta=m.world.position-eye;const auto distance=length(delta);if(distance>=s.radius)continue;
   if(distance>=.0001f)delta=delta*(1.f/distance);
   if(angle(delta,direction(u.yaw,u.pitch))>s.aim || 3.1415927f-angle(delta,m.world.basis[0])>s.facing)continue;
   if(length(u.body.velocity)>s.maximum_velocity)continue;
   reachable=true;break;
  }
  if(!reachable)continue;
  const auto sm=marker(d,o,s.marker);if(sm.empty())continue;
  const auto animations=actor->second.seat_animations.find(s.mode);
  if(animations==actor->second.seat_animations.end() || animations->second.entries.empty())continue;
  const NativeUnitSeatAnimations::Entry *entry=nullptr;float nearest=INFINITY;
  for(const auto &e:animations->second.entries){const auto p=compose_transform(sm.front().world,e.initial).position;const auto dd=dot(p-u.body.position,p-u.body.position);if(dd<nearest){nearest=dd;entry=&e;}}
  const auto distance=std::min(std::sqrt(nearest),length(sm.front().world.position-u.body.position));
  const Unit *occupant=nullptr,*partner=nullptr;bool enemy=false,partner_boarded=false;
  for(const auto *r:units)if(r->exists && !r->vitality.dead && r->source_parent==o.handle && r->source_seat>=0) {
   if(r->source_seat==int(index))occupant=r;
   else if((s.flags&(1u<<11)) && r->source_seat==s.boarding_seat)partner=r;
   else if(!allied(*r,u))enemy=true;
   if(unsigned(r->source_seat)<d.seats.size()){const auto &rs=d.seats[unsigned(r->source_seat)];
    if((rs.flags&(1u<<11)) && rs.boarding_seat==s.boarding_seat)partner_boarded=true;}
  }
  if((s.flags&(1u<<9)) && std::none_of(units.begin(),units.end(),[&](const Unit *r){return r->source_parent==o.handle;}))continue;
  bool fits=true;
  for(unsigned hand=0;hand<2;++hand)if(const auto *w=native_hand_weapon(u,hand)) {
   //101380/1013E0 inspect weapon flags20/21. Their multiplayer engine permission is separate.
   const auto flags=content.weapons.at(w->definition).inventory_flags;
   if(((s.flags&4u) && (flags&(1u<<20))) || ((s.flags&8u) && (flags&(1u<<21))))fits=false;
  }
  if(!fits)continue;
  const bool grouped=s.flags&(1u<<11);
  bool admitted=!occupant;
  if(!grouped)admitted&=!enemy || multiplayer;
  else if(s.boarding_seat<0)admitted&=!(s.flags&(1u<<14)) || enemy;
  else admitted&=partner && !allied(*partner,u) && !partner_boarded;
  std::int16_t rank=admitted?(grouped?3:2):0;
  if(!rank && occupant && allied(*occupant,u) && !occupant->controlled)rank=1;
  if(!rank)continue;
  const auto hit=collision.raycast(eye,root(o).position-eye,length(root(o).position-eye));
  if(hit && hit->distance+.001f<length(root(o).position-eye))continue;
  const float scale=driver && !(s.flags&4u)?1.5f:1.f;
  if(best.seat<0 || rank>best.rank || scale*distance<best.distance){best={rank,std::int16_t(index),distance,entry};driver=s.flags&4u;}
 }
 return best;
}
}
NativeActionPrompt Simulation::unit_action_prompt(const Unit &u,NativeActionPrompt current,Vec3 current_root)const {
 if(!content_.unit_interactions || !u.exists || u.vitality.dead)return current;
 const auto &assets=*content_.unit_interactions;
 if(u.source_parent!=null_datum) {
  for(const auto &o:scene_.objects())if(o.handle==u.source_parent) {
   const auto found=assets.units.find(content_.objects[o.placement].definition);
   if(found==assets.units.end() || u.source_seat<0 || unsigned(u.source_seat)>=found->second.seats.size())return {};
   const auto &s=found->second.seats[unsigned(u.source_seat)];if(!(s.flags&(1u<<11)))return {};
   //19188C's priority-zero seated hints retain their own authored message.
   auto message=s.melee_message;
   if(u.grenade_current>=0 && u.grenade_current<2 && u.grenades[unsigned(u.grenade_current)]>0 && s.grenade_marker &&
       marker(found->second,o,s.grenade_marker,1).size()==1 && s.grenade_message)message=s.grenade_message;
   return {{0,u.source_seat,o.handle},content_.objects[o.placement].definition,message};
  }
  return {};
 }
 if((source_physics_movement_?u.grenade_action.active_actions():u.source_action_word)&0x400000u)return {};
 std::vector<const Unit*> units{&player_};for(const auto &a:actors_)units.push_back(&a);
 const auto basis=authored_basis({u.yaw,0,0});
 const auto bound=u.body.position+basis.forward*u.definition.bounding_offset.x+basis.left*u.definition.bounding_offset.y+basis.up*u.definition.bounding_offset.z;
 // Retain the chosen root when weapon/control/trade admission ran first.
 for(const auto &o:scene_.objects())if(o.handle==current.target.object)current_root=root(o).position;
 for(const auto &a:actors_)if(a.source_datum==current.target.object)current_root=a.body.position;
 for(const auto &o:scene_.objects()) {
  if(o.handle==null_datum || !o.visible || o.bsp_hidden || content_.objects[o.placement].type!=1)continue;
  const auto definition=content_.objects[o.placement].definition;
  const auto found=assets.units.find(definition);if(found==assets.units.end())continue;
  const auto &d=found->second;const auto r=root(o);
  const auto center=r.position+(r.basis[0]*d.bound_offset.x+r.basis[1]*d.bound_offset.y+r.basis[2]*d.bound_offset.z)*r.scale;
  const bool occupied=std::any_of(units.begin(),units.end(),[&](auto p){return p->source_parent==o.handle;});
  const bool flipping=std::any_of(vehicle_flips_.begin(),vehicle_flips_.end(),[&](const auto &f){return f.object==o.handle;});
  SourceVehicleActionQuery q{u.source_datum,0,bound,u.body.position,u.definition.bounding_radius};
  SourceVehicleActionCandidate candidate{o.handle,occupied?u.source_datum:null_datum,0,std::uint8_t(flipping?0x40:0),center,r.position,d.bound_radius*r.scale};
  SourceVehicleActionServices services;
  services.flipped=[&](auto){return r.basis[2].z<assets.flipped_up_threshold;};
  services.unit_busy=[&](auto){return u.weapon.reload>0 || (native_hand_weapon(u,1) && native_hand_weapon(u,1)->reload>0);};
  services.seat=[&](auto,auto){
   if(source_vehicle_seat_query_)return source_vehicle_seat_query_(u.source_datum,o.handle);
   const auto s=choose(u,assets,o,d,units,collision(),content_,bool(content_.multiplayer_spawn),[&](const Unit &a,const Unit &b){return vehicle_allied_?vehicle_allied_(a,b):allied(a.team,b.team);});return SourceVehicleSeatTarget{s.rank,s.seat,s.rank?o.handle:null_datum,r.position};};
  if(source_vehicle_action_candidate(q,candidate,services,.2f,current.target,current_root)){current.definition=definition;current.message=0;current_root=r.position;}
 }
 return current;
}
bool Simulation::take_unit_action(Unit &u,const NativeActionPrompt &prompt) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
 if(content_.unit_interactions && u.source_parent!=null_datum && prompt.target.action==0 && prompt.target.object==u.source_parent && prompt.target.index==u.source_seat) {
  if(u.seat_action.phase!=Unit::SeatAction::Seated)return false;
  const auto &o=scene_.get(u.source_parent);const auto &d=content_.unit_interactions->units.at(content_.objects[o.placement].definition);
  const auto &s=d.seats.at(unsigned(u.source_seat));const auto &body=content_.unit_interactions->units.at(u.definition.datum);
  const auto animations=body.seat_animations.find(s.mode);
  u.seat_action.phase=Unit::SeatAction::Exiting;u.seat_action.started=ticks_;u.custom_animation.reset();
  if(animations!=body.seat_animations.end())u.custom_animation=animations->second.exit;
  u.custom_animation_tick=ticks_;u.custom_animation_loop=false;u.source_player_actions.flags|=0x10u;return true;
 }
 if(!content_.unit_interactions || prompt.target.action<4 || prompt.target.action>8 || prompt.target.action==7)return false;
 const auto admitted=unit_action_prompt(u);if(admitted.target.object!=prompt.target.object || admitted.target.action!=prompt.target.action || admitted.target.index!=prompt.target.index)return false;
 auto &o=scene_.get(prompt.target.object);const auto &assets=*content_.unit_interactions;
 const auto &d=assets.units.at(content_.objects[o.placement].definition);
 if(prompt.target.action==8) {
  const auto r=root(o);const auto side=cross(r.basis[2],r.basis[0]);const auto delta=r.position-u.body.position;
  std::uint8_t direction=std::abs(r.basis[0].z)<=.70710677f?(dot(r.basis[0],cross(Vec3{0,0,1},delta))<=0?1:2):(r.basis[0].z>=0?3:4);
  vehicle_flips_.push_back({o.handle,direction,0,{}});u.source_player_actions.flags|=0x10u;return true;
 }
 const auto seat=unsigned(prompt.target.index);if(seat>=d.seats.size())return false;
 if(prompt.target.action==4) {
  for(auto &ally:actors_)if(ally.source_parent==o.handle && ally.source_seat==int(seat) && (vehicle_allied_?vehicle_allied_(ally,u):allied(ally.team,u.team))) {
   if(ally.seat_action.phase==Unit::SeatAction::Seated)take_unit_action(ally,{{0,ally.source_seat,o.handle},content_.objects[o.placement].definition,0});
   u.source_player_actions.flags|=0x10u;return true;
  }
  return false;
 }
 std::vector<const Unit*> units{&player_};for(const auto &a:actors_)units.push_back(&a);
 const auto choice=choose(u,assets,o,d,units,collision(),content_,bool(content_.multiplayer_spawn),[&](const Unit &a,const Unit &b){return vehicle_allied_?vehicle_allied_(a,b):allied(a.team,b.team);});
 if(choice.seat!=int(seat) || !choice.entry)return false;
 u.source_parent=o.handle;u.source_seat=std::int16_t(seat);
 u.seat_action={Unit::SeatAction::Entering,ticks_,u.body.position};u.custom_animation=choice.entry->clip;u.custom_animation_tick=ticks_;u.custom_animation_loop=false;
 u.source_player_actions.flags|=0x10u;source_zoom_reset(u.source_zoom);++u.zoom_reset_serial;return true;
}
void Simulation::step_unit_interactions(const Controls &input) {
  NativeAuthorityBoundary::Call active(authority_calls_);++authority_calls_.mutations;
 if(!content_.unit_interactions)return;
 const auto &assets=*content_.unit_interactions;
 //15AE70: each row updates once per source second, retaining its salted object.
 for(const auto &[row,placement]:assets.netgame_placements) {
  if(row%30!=ticks_%30)continue;
  const auto &definition=content_.netgame_equipment[row];
  if(source_netgame_equipment_selector(definition,true,vehicle_engine_)<0)continue;
  auto &state=vehicle_rows_.at(row);
  auto found=std::find_if(scene_.objects().begin(),scene_.objects().end(),[&](const auto &o){return o.handle==state.object;});
  if(found==scene_.objects().end())state.object=null_datum;
  const bool occupied=state.object!=null_datum && (player_.source_parent==state.object || std::any_of(actors_.begin(),actors_.end(),[&](const auto &u){return u.exists && u.source_parent==state.object;}));
  const bool busy=std::any_of(vehicle_flips_.begin(),vehicle_flips_.end(),[&](const auto &f){return f.object==state.object;});
  if(state.object!=null_datum && definition.empty_seconds && vehicle_timer_option_!=1) {
   const auto delta=root(*found).position-definition.position;
   if(occupied || dot(delta,delta)<=native_vehicle_empty_distance_squared || busy)state.empty_counter=0;
   else ++state.empty_counter;
   if(definition.empty_seconds<state.empty_counter)state.timer=1;
  }
  if(!state.timer) {
   bool start=state.object==null_datum;
   if(!start && definition.timer_start==0){const auto delta=root(*found).position-definition.position;start=dot(delta,delta)>1.f;}
   if(start){int delay=definition.respawn_seconds?definition.respawn_seconds:definition.collection_delay?definition.collection_delay:30;
    if(vehicle_timer_option_==1)delay=-1;else if(vehicle_timer_option_==2)delay>>=1;else if(vehicle_timer_option_==3)delay<<=1;
    state.timer=std::int16_t(delay);}
  }
  if(state.timer>0 && --state.timer==0) {
   if(state.object!=null_datum && (definition.flags&2u)) {
    if(occupied || busy){state.timer=1;continue;}
    scene_.destroy(state.object);
   }
   state.object=scene_.create_placement(placement,ticks_);state.timer=state.empty_counter=0;
  }
 }
 // Child vehicle generations share the exact authored model attachment frame.
 for(std::size_t placement=0;placement<content_.objects.size();++placement)if(const auto &attachment=content_.objects[placement].model_attachment) {
  std::vector<std::uint32_t> parents;
  for(const auto &o:scene_.objects())if(o.handle!=null_datum && o.placement==attachment->parent)parents.push_back(o.handle);
  for(const auto handle:parents) {
   auto child=std::find_if(scene_.objects().begin(),scene_.objects().end(),[&](const auto &o){return o.handle!=null_datum && o.placement==placement && o.attachment_parent==handle;});
   const auto child_handle=child==scene_.objects().end()?scene_.create_attached_placement(placement,handle,ticks_):child->handle;
   auto &co=scene_.get(child_handle);co.attachment_parent=handle;
   // The original source body/WORLD bank owns the attached frame. Native
   // marker composition must not replace it or clear its completed nodes.
   if(source_physics_movement_)continue;
   const auto &parent=scene_.get(handle);const auto &pd=assets.units.at(content_.objects[parent.placement].definition);
   const auto &cd=assets.units.at(content_.objects[placement].definition);
   const auto pm=marker(pd,parent,attachment->parent_marker);auto cm=marker(cd,co,attachment->child_marker);
   if(pm.empty())continue;
   // FD560 uses the child marker relative to its root, or identity when unnamed.
   Transform local; if(attachment->child_marker && !cm.empty())local=compose_transform(inverse_transform(root(co)),cm.front().world);
   co.solved_transform=compose_transform(pm.front().world,inverse_transform(local));co.position=co.solved_transform->position;co.solved_nodes.reset();
  }
 }
 std::vector<std::uint32_t> orphans;
 for(const auto &o:scene_.objects())if(o.handle!=null_datum && o.attachment_parent!=null_datum &&
     std::none_of(scene_.objects().begin(),scene_.objects().end(),[&](const auto &p){return p.handle==o.attachment_parent;}))orphans.push_back(o.handle);
 for(const auto h:orphans)scene_.destroy(h);
 if(source_physics_movement_)return;
 const auto update=[&](Unit &u) {
  if(u.source_parent==null_datum)return;
  const auto object=std::find_if(scene_.objects().begin(),scene_.objects().end(),[&](const auto &o){return o.handle==u.source_parent;});
  if(object==scene_.objects().end() || !u.exists || u.vitality.dead){u.source_parent=null_datum;u.source_seat=-1;u.seat_action={};u.custom_animation.reset();return;}
  const auto &d=assets.units.at(content_.objects[object->placement].definition);
  if(u.source_seat<0 || unsigned(u.source_seat)>=d.seats.size())throw FormatError("Lost actual vehicle seat owner");
  const auto &s=d.seats[unsigned(u.source_seat)];const auto sm=marker(d,*object,s.marker);if(sm.empty())throw FormatError("Lost actual posed seat marker");
  const auto elapsed=ticks_-u.seat_action.started;
  const auto body=assets.units.find(u.definition.datum);
  const NativeUnitSeatAnimations *animations=nullptr;
  if(body!=assets.units.end())if(const auto a=body->second.seat_animations.find(s.mode);a!=body->second.seat_animations.end())animations=&a->second;
  const bool complete=!u.custom_animation || elapsed>=u.custom_animation->frame_count;
  const auto seated=[&]{u.seat_action.phase=Unit::SeatAction::Seated;u.custom_animation=animations?animations->idle:nullptr;u.custom_animation_tick=ticks_;u.custom_animation_loop=true;};
  if(u.seat_action.phase==Unit::SeatAction::Exiting && complete) {
   auto exit=sm.front().world;
   if(u.custom_animation && !u.custom_animation->nodes.empty()) {const auto pose=sample_animation(*u.custom_animation,0,float(u.custom_animation->frame_count-1));
    auto offset=source_marker_transform(pose.translation,pose.rotation);offset.scale=pose.scale;exit=compose_transform(exit,offset);}
   u.body.position=exit.position;u.body.velocity={};u.source_parent=null_datum;u.source_seat=-1;u.seat_action={};u.custom_animation.reset();return;
  }
  if(u.seat_action.phase==Unit::SeatAction::Entering && complete) {
   seated();
   if((s.flags&(1u<<11)) && s.boarding_seat>=0 && (s.flags&(1u<<13)) && animations && animations->board) {
    u.seat_action.phase=Unit::SeatAction::Boarding;u.seat_action.started=ticks_;u.custom_animation=animations->board;u.custom_animation_tick=ticks_;u.custom_animation_loop=false;
   }
  } else if(u.seat_action.phase==Unit::SeatAction::Boarding && complete) {
   const auto eject=[&](Unit &r){if(&r!=&u && r.source_parent==u.source_parent && r.source_seat==s.boarding_seat){r.source_parent=null_datum;r.source_seat=-1;r.seat_action={};r.custom_animation.reset();}};
   eject(player_);for(auto &r:actors_)eject(r);
   u.source_seat=s.boarding_seat;seated();return;
  }
  u.body.position=sm.front().world.position;u.body.velocity={};u.body.grounded=false;
 };
 update(player_);for(auto &u:actors_)update(u);
 for(auto at=vehicle_flips_.begin();at!=vehicle_flips_.end();) {
  const auto object=std::find_if(scene_.objects().begin(),scene_.objects().end(),[&](const auto &o){return o.handle==at->object;});
  if(object==scene_.objects().end()){at=vehicle_flips_.erase(at);continue;}
  auto &o=scene_.get(at->object);auto r=root(o);const auto &d=assets.units.at(content_.objects[o.placement].definition);
  if(at->ticks>=60 || r.basis[2].z>.9f){at=vehicle_flips_.erase(at);continue;}
  // EF870's original requested angular velocity, consumed by the native scene root.
  const auto side=cross(r.basis[2],r.basis[0]);auto axis=(at->direction==3 || at->direction==4)?side:r.basis[0];
  if(at->direction==1 || at->direction==2)axis=axis+side*(-r.basis[0].z);
  const auto rate=std::clamp(r.basis[2].z/tick_seconds*-2.f,d.flip_minimum,d.flip_maximum)*(at->direction==2 || at->direction==4?.3f:-.3f);
  const float magnitude=length(axis);if(magnitude>.0001f){axis=axis*(1.f/magnitude);const float angle=rate*magnitude*tick_seconds;const auto cs=std::cos(angle),sn=std::sin(angle);
   for(auto &v:r.basis)v=v*cs+cross(axis,v)*sn+axis*(dot(axis,v)*(1-cs));}
  o.solved_transform=r;o.solved_nodes.reset();++at->ticks;++at;
 }
 (void)input;
}
}
