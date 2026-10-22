#include "halo2/native_unit_interaction.hpp"
#include "halo2/native_action_prompt.hpp"
#include "halo2/simulation.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include "halo2/native_weapon_interaction.hpp"
namespace halo2 {
NativeActionPrompt native_control_action_prompt(const Unit &u,const LevelContent &content,
    const SourceDeviceWorld &devices,NativeActionPrompt result,Vec3 current_root) {
  if(!u.exists || u.vitality.dead || u.source_parent!=null_datum ||
      (u.source_action_word&0x400000u))return {};
  const auto basis=authored_basis({u.yaw,0,0});
  SourceControlTargetQuery query;
  query.unit_parent=u.source_parent;query.unit_root=u.body.position;
  query.unit_bound_center=u.body.position+basis.forward*u.definition.bounding_offset.x+
      basis.left*u.definition.bounding_offset.y+basis.up*u.definition.bounding_offset.z;
  query.unit_bound_radius=u.definition.bounding_radius;
  query.unit_center=ordinary_unit_eye(u.body.position,u.definition.camera_height,
      u.definition.crouch_camera_height,u.crouch);
  query.unit_forward=direction(u.yaw,u.pitch);
  for(const auto &[handle,owner]:devices.owners()) {
    if(owner.bsp_hidden)continue;
    const auto &d=*content.devices.at(owner.placement);
    if(!d.control || !d.control->action_string || d.control_nodes.empty())continue;
    const auto nodes=source_control_world_nodes(d,owner);
    const auto sphere=source_shadow_sphere(d.control_bounds,nodes[0],owner.root.scale);
    SourceControlTargetCandidate candidate;candidate.object=handle;candidate.root=owner.root.position;
    candidate.bound_center=sphere.center;candidate.bound_radius=sphere.radius;
    const auto markers=pose_model_markers(d.control_markers,nodes,d.control_permutations,{},1);
    // B8D30 supplies node zero on a missing named marker, but returns zero.
    // 1078F0 applies its side gate only when the named query returns one.
    candidate.marker_count=std::uint16_t(markers.size());
    candidate.marker_forward=markers.empty()?nodes[0].basis[0]:markers[0].world.basis[0];
    if(devices.consider_control_target(query,candidate,result.target,current_root)) {
      result.definition=d.control->datum;result.message=d.control->action_string;current_root=candidate.root;
    }
  }
  return result;
}
NativeActionPrompt Simulation::action_prompt()const {
  if(!player_input_enabled_ || !presentation_.player_camera_enabled || presentation_.cinematic)return {};
  const auto selected=native_weapon_action_prompt(player_,content_,collision(),campaign_weapon_pickups_,std::uint32_t(ticks_));
  auto prompt=native_control_action_prompt(player_,content_,devices_,selected.prompt,selected.root);
  Vec3 root=selected.root;
  if(prompt.target.action==3)root=scene_.get(prompt.target.object).position;
  prompt=native_ally_action_prompt(player_,actors_,content_,prompt,root,[&](int a,int b){return allied(a,b);},ticks_);
  return unit_action_prompt(player_,prompt,root);
}
NativeActionPrompt Simulation::control_action_prompt()const {
  if(!player_input_enabled_ || !presentation_.player_camera_enabled || presentation_.cinematic)return {};
  return native_control_action_prompt(player_,content_,devices_);
}
}
