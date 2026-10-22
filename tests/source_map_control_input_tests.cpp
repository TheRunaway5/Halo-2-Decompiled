#include "helpers/gameplay_fixture.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/math.hpp"
#include "halo2/source_shadow.hpp"
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
void map(const char *path,unsigned count) {
  Cache cache(path);const auto content=cache.load_content(0,true);
  SceneObjects scene(content,0);SourceDeviceWorld initial(content);initial.synchronize(scene);
  std::vector<std::size_t> controls;
  for(const auto &[handle,owner]:initial.owners())
    if(content.devices[owner.placement]->control)controls.push_back(owner.placement);
  require(controls.size()==count,"Authored control coverage changed");
  for(const auto placement:controls)for(unsigned players:{1u,4u}) {
    SlayerRules rules;rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
    MultiplayerMatch match(content,0,cache.spawns(),rules);match.configure_weapon_pickups();
    const auto slot=match.join("Control input");
    for(unsigned i=1;i<players;++i)match.join("Other local player");
    const auto &devices=match.simulation().devices();std::uint32_t handle=null_datum;
    for(const auto &[id,owner]:devices.owners())if(owner.placement==placement)handle=id;
    require(handle!=null_datum,"Match lost an authored control");
    const auto &owner=*devices.find(handle);const auto &definition=*content.devices[placement];
    const auto nodes=source_control_world_nodes(definition,owner);
    const auto &node=nodes.at(0);
    const auto computer=source_shadow_sphere(definition.control_bounds,node,owner.root.scale).center;
    const auto markers=pose_model_markers(definition.control_markers,nodes,definition.control_permutations,{},1);
    const auto front=markers.empty()?node.basis[0]:markers[0].world.basis[0];
    const auto &collision=match.simulation().collision();
    const auto &unit=match.unit(slot);
    std::array<Controls,multiplayer_capacity> input{};
    NativeActionPrompt prompt;unsigned attempts{};
    // Diagnostic placement searches collision-free positions and native target
    // admission. Containment's second switch is beside a sloped approach.
    const float front_yaw=std::atan2(front.y,front.x);
    for(float distance:{.6f,.5f,.4f,.3f,.2f,.7f,.8f,.9f}) {
      if(prompt.target.object==handle)break;
      for(float angle:{0.f,.785398163f,-.785398163f,1.570796327f,-1.570796327f,2.35619449f,-2.35619449f,3.141592654f}) {
        auto point=computer+Vec3{std::cos(front_yaw+angle)*distance,std::sin(front_yaw+angle)*distance,0};
        const auto floor=collision.raycast(point+Vec3{0,0,2},{0,0,-1},5);

        // CollisionWorld uses a vertical capsule whose lower sphere center
        // is feet+radius. Lift its feet by the actual floor-plane support
        // distance so sloped ground is not mistaken for a wall overlap.
        point.z=computer.z-unit.definition.bounding_offset.z;
        if(floor && floor->normal.z>0)point.z=std::max(point.z,
            floor->position.z+.01f+unit.definition.radius*(1.f/floor->normal.z-1.f));
        // This is diagnostic placement, including an airborne start when
        // needed to put the native unit bound near an elevated switch. It
        // does not establish a walking route through this map's terrain.
        point.z=std::max(point.z,computer.z-unit.definition.bounding_offset.z);
        if(collision.overlaps(point,unit.definition.height,unit.definition.radius))continue;
        const auto ray=computer-ordinary_unit_eye(point,unit.definition.camera_height,
            unit.definition.crouch_camera_height,0);
        const auto yaw=std::atan2(ray.y,ray.x),pitch=std::atan2(ray.z,std::hypot(ray.x,ray.y));
        Spawn at;at.position=point;at.bsp=0;at.facing=yaw;
        test::place_player(match,slot,at);
        input[slot].yaw=yaw;input[slot].pitch=pitch;match.step(input);++attempts;
        prompt=match.action_prompt(slot);
        if(prompt.target.object==handle)break;
      }
    }
    if(prompt.target.object!=handle || prompt.target.action!=3 || prompt.message!=definition.control->action_string)
      throw FormatError(std::string(path)+" control="+std::to_string(placement)+
          " placement attempts="+std::to_string(attempts)+" selected="+std::to_string(prompt.target.object));
    const auto group=unsigned(definition.placement.position_group);
    const auto before=devices.groups()[group];
    input[slot].pickup=true;input[slot].pickup_milliseconds=0;match.step(input);
    require(devices.groups()[group].value==before.value,"Unheld input changed a control group");
    input[slot].pickup_milliseconds=65535;match.step(input);
    require(devices.groups()[group].value==1 && devices.groups()[group].flags==(before.flags|2u),
        "Held native player input did not operate the authored shared group");
    std::vector<std::uint32_t> linked;
    for(const auto &[id,o]:devices.owners())if(content.devices[o.placement]->machine &&
        content.devices[o.placement]->placement.position_group==int(group))linked.push_back(id);
    require(!linked.empty(),"Control has no linked authored machine");
    input[slot].pickup=false;input[slot].pickup_milliseconds=0;
    unsigned partial{};
    for(unsigned i=0;i<300;++i) {
      match.step(input);
      for(const auto id:linked) {
        const auto &state=devices.find(id)->motion;
        partial+=state.position>0 && state.position<1;
      }
    }
    require(partial>0,"Linked machines did not advance gradually");
    for(const auto id:linked) {
      const auto &state=devices.find(id)->motion;
      require(state.position==1 && state.position_velocity==0,"Linked machine did not reach its authored endpoint");
    }
    input[slot].pickup=true;input[slot].pickup_milliseconds=65535;match.step(input);
    require(devices.groups()[group].value==1 && devices.groups()[group].flags==(before.flags|2u),
        "Repeated held input changed the completed group");
    require(match.simulation().scripts().diagnostics().empty(),"Control input fixture faulted an authored script");
    std::cout<<path<<" placement="<<placement<<" players="<<players
        <<" control="<<handle<<" message="<<prompt.message<<" linked="<<linked.size()
        <<" intermediate="<<partial<<" PASS native prompt/hold/motion/repeat\n";
  }
}
}
int main(int argc,char **argv)try {
  map("assets/disc/maps/zanzibar.map",1);
  map("assets/map-pack/maps/containment.map",2);
  map("assets/map-pack/maps/dune.map",1);
  require(argc==1,"No arguments expected");
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
