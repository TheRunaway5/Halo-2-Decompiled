#include "halo2/native_action_prompt_hud.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/native_weapon_interaction.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
void approach(Simulation &simulation,const LevelContent &content,const NativeWeaponPickup &item) {
  auto &u=const_cast<Unit &>(simulation.player());
  const auto &w=content.weapons.at(item.definition);
  const auto center=item.position+item.forward*w.bounding_offset.x+cross(item.up,item.forward)*w.bounding_offset.y+item.up*w.bounding_offset.z;
  u.body.position=center-u.definition.bounding_offset;u.body.velocity={};u.yaw=u.pitch=0;
}
}
int main(int argc,char **argv)try {
  check(argc==3,"Supply campaign maps and Vista-backed HUD assets");
  const auto maps=std::filesystem::path(argv[1]);if(!std::filesystem::exists(maps/"00a_introduction.map"))return 77;
  unsigned map_count{},placements{},pickups{},duals{},restores{},script_births{},ammo_transfers{},empty_items{},bsp_transfers{};
  std::set<std::string> definitions;
  for(const auto &entry:std::filesystem::directory_iterator(maps)) {
    if(entry.path().extension()!=".map")continue;
    const auto name=entry.path().stem().string();
    if(name!="tutorial" && (name.empty() || name[0]<'0' || name[0]>'9'))continue;
    Cache cache(entry.path());const auto original=cache.load_content(0);++map_count;
    auto imported=cache.load_hud(original.player.datum,null_datum);imported.widgets.clear();
    Mesh mesh;NativeActionPromptHud hud(cache,original.player.datum,maps/"fonts",mesh,argv[2]);
    for(const auto &placement:original.objects) {
      const auto definition=original.weapons.find(placement.definition);
      if(placement.type!=2 || definition==original.weapons.end() ||
          definition->second.first_person_model==null_datum || definition->second.first_person_animation==null_datum)continue;
      auto content=original;content.collision.clear();content.squads.clear();content.scripts={};
      content.objects={placement};content.objects[0].flags&=~65u;content.objects[0].source=-1;
      content.objects[0].origin_bsp=-1;content.objects[0].bsp_policy=0;content.objects[0].name=-1;
      content.objects[0].manual_bsps=0;content.devices.clear();content.prop_sandbox.clear();content.static_scenery_collision.clear();
      content.object_names.clear();content.device_groups.clear();content.trigger_volumes.clear();content.bsp_transitions.clear();
      content.starting_weapon=null_datum;
      Simulation simulation(content,0,{});check(simulation.weapon_pickups().size()==1,"Campaign placed weapon has no item owner");
      const auto item=simulation.weapon_pickups().front();++placements;approach(simulation,content,item);
      const auto prompt=simulation.action_prompt();
      if(definition->second.capacity>0 && item.weapon->loaded+item.weapon->reserve==0) {
        check(prompt.target.object==null_datum,"Empty campaign gun displayed an unavailable pickup action");++empty_items;continue;
      }
      check(prompt.target.object==item.handle,"Actual campaign weapon was not selected");
      definitions.insert(definition->second.name);
      const auto keyboard=hud.frame(prompt,{}, {},false,1280,800);
      const auto pad=hud.frame(prompt,{}, {},true,1280,800);
      check(!keyboard.text.empty() && !keyboard.vertices.empty() && !pad.text.empty() && !pad.vertices.empty(),"Campaign pickup lost original authored message/buttons");
      auto &unit=const_cast<Unit &>(simulation.player());
      unit.vitality.dead=true;check(simulation.action_prompt().target.object==null_datum,"Dead campaign player retained item prompt");unit.vitality.dead=false;
      unit.source_parent=0x10001;check(simulation.action_prompt().target.object==null_datum,"Parented campaign player retained item prompt");unit.source_parent=null_datum;
      unit.source_action_word|=0x400000u;check(simulation.action_prompt().target.object==null_datum,"Blocked campaign action retained prompt");unit.source_action_word&=~0x400000u;
      simulation.request_checkpoint();simulation.step({});
      const auto threshold=source_pickup_hold_threshold(content.pickup_hold_seconds);
      Controls input;input.pickup=true;input.pickup_milliseconds=threshold-1;
      simulation.step(input);check(simulation.player().weapon.source_datum!=item.handle,"Campaign pickup ignored hold threshold");
      input.pickup_milliseconds=threshold;simulation.step(input);
      check(simulation.player().weapon.source_datum==item.handle,"Completed campaign hold did not transfer actual item");
      check(simulation.player().weapon.loaded==item.weapon->loaded && simulation.player().weapon.reserve==item.weapon->reserve,"Campaign attachment changed actual ammunition");
      check(simulation.action_prompt().target.object!=item.handle,"Attached campaign weapon retained world prompt");++pickups;
      simulation.request_revert();simulation.step({});
      check(simulation.player().weapon.source_datum!=item.handle && simulation.action_prompt().target.object==item.handle,"Checkpoint did not restore campaign pickup owner");++restores;
      // Same-definition reserve pickup consumes the real world magazine and
      // owner, rather than creating a second copy of the held weapon.
      simulation.equip(item.definition);approach(simulation,content,item);
      auto &held=const_cast<Unit &>(simulation.player()).weapon;held.reserve=held.smg.reserve=0;
      if(definition->second.reserve>0 && item.weapon->loaded+item.weapon->reserve>0) {
        simulation.step(input);check(simulation.player().weapon.source_datum!=item.handle && simulation.player().weapon.reserve>0,"Campaign ammo-only pickup replaced its held weapon");++ammo_transfers;
      }
      // Exercise the campaign secondary input with the actual compatibility
      // bits, starting with a single compatible native inventory owner.
      if(definition->second.dual_wieldable) {
        simulation.request_revert();simulation.step({});simulation.equip(item.definition);approach(simulation,content,item);
        const auto primary=simulation.player().weapon.source_datum;
        Controls dual;dual.dual_wield=true;dual.dual_wield_milliseconds=threshold;simulation.step(dual);
        const auto *offhand=native_hand_weapon(simulation.player(),1);
        check(offhand && offhand->source_datum==item.handle && simulation.player().weapon.source_datum==primary,"Campaign dual pickup replaced primary or lost secondary owner");++duals;
      }
      // Inventory membership must keep the actual item generation through a
      // BSP change even when its original placement is exclusive to BSP0.
      {
        auto crossing=content;crossing.bsp_count=2;crossing.objects[0].source=1;
        crossing.objects[0].origin_bsp=0;crossing.objects[0].bsp_policy=1;
        Simulation routed(crossing,0,{}, {},[](std::size_t){return BspContent{};});
        check(routed.weapon_pickups().size()==1,"BSP fixture item was not born");
        const auto retained=routed.weapon_pickups().front();approach(routed,crossing,retained);
        routed.step(input);check(routed.player().weapon.source_datum==retained.handle,"BSP fixture pickup failed");
        routed.request_bsp(1);routed.step({});
        check(routed.bsp()==1 && routed.player().weapon.source_datum==retained.handle,
            "Campaign BSP switch destroyed an attached item generation");++bsp_transfers;
      }
      // A manual placement must be discovered after an actual script birth,
      // including the next salted generation after destruction/recreation.
      content.objects[0].flags|=1u;content.objects[0].name=0;
      content.object_names={{"prompt_fixture",2,content.objects[0].placement}};
      Simulation scripted(content,0,{});check(scripted.weapon_pickups().empty(),"Manual item spawned before its script owner");
      auto &scene=const_cast<SceneObjects &>(scripted.scene());
      const auto first=scene.create(0);scripted.step({});check(scripted.weapon_pickups().size()==1 && scripted.weapon_pickups().front().handle==first,"Script-created weapon did not join campaign item bank");
      scene.destroy(first);const auto second=scene.create(0);scripted.step({});
      check(second!=first && scripted.weapon_pickups().size()==1 && scripted.weapon_pickups().front().handle==second,"Campaign item retained a stale full object generation");++script_births;
    }
  }
  check(placements>0 && pickups+empty_items==placements && restores==pickups && script_births==pickups,"Campaign catalog was not fully exercised");
  std::cout<<"PASS "<<map_count<<" campaign maps; "<<placements<<" placed weapons; "<<definitions.size()<<" definitions; "<<empty_items<<" empty items suppressed; "<<pickups<<" held pickups; "<<duals<<" dual pickups; "<<ammo_transfers<<" ammo transfers; "<<restores<<" checkpoint restores; "<<script_births<<" script birth/rebirth routes; "<<bsp_transfers<<" held BSP transfers\n";
  return 0;
} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
