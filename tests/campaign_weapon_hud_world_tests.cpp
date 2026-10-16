#include "halo2/campaign_source_runtime.hpp"
#include "halo2/native_weapon_hud.hpp"
#include "halo2/original_data.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/collision.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>
using namespace halo2;
namespace {void check(bool yes,const char *message){if(!yes)throw FormatError(message);}}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;check(argc==4,"Supply original CAMPAIGN_MAP SHARED TU1.5_XBE");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  auto content=cache.load_content(0);
  const auto start=std::ranges::find_if(cache.spawns(),[](const auto &s){return s.bsp==0;});
  check(start!=cache.spawns().end(),"Campaign has no authored BSP0 player start");
  std::cout<<"Constructing retained campaign authority\n";
  auto source=std::make_shared<CampaignSourceRuntime>(cache,argv[3],0,1,0x13c4b8u);
  std::cout<<"Constructing campaign scene/player over its actual pool\n";
  Simulation simulation(content,0,*start,{},[&](auto bsp){return cache.load_bsp_content(bsp);},{},
      {},{}, {},source);
  check(simulation.object_lifetimes().pool().source_memory()==&source->objects().memory(),
      "Campaign simulation does not borrow its actual source object bank");
  Mesh mesh;NativeWeaponHud hud(cache,content,mesh,cache.shared_directory()/"fonts",{});
  const auto representation=cache.load_hud(content.player.datum,null_datum).unit_representation;
  check(representation.has_value(),"Campaign HUD has no authored biped representation");
  Controls input;input.yaw=start->facing;
  for(unsigned tick=0;tick<3;++tick) {
    simulation.step(input);const auto player=source->hud_player(simulation);
    check(player.source_unit==simulation.player().source_datum && player.source_player!=null_datum,
        "Campaign HUD lost its retained player/unit identity");
    check(player.source_configuration && (*player.source_configuration)[0x44]==*representation &&
        (*player.source_configuration)[0x7c]==1,"Campaign source profile lost its original scenario representation/team");
    check(player.source_hud_target_flags==source->objects().weapon_hud_target_flags(player.source_unit),
        "Campaign HUD target flags differ from original retained publication");
    const auto body=source->objects().object_body(player.source_unit);
    const auto &memory=source->objects().memory();
    const auto action=std::bit_cast<std::int16_t>(Bytes(memory.view(body+0x346u,2)).u16(0));
    check(action>=0 && player.source_action_word==Bytes(memory.view(body+unsigned(action)+4u,4)).u32(0),
        "Campaign HUD substituted control flags for the actual action component");
    const auto vertices=hud.frame(player,std::uint32_t(simulation.ticks()),1280,720);
    std::cout<<"PASS actual campaign tick="<<simulation.ticks()<<" weapon="<<player.weapon
        <<" HUD vertices="<<vertices.size()<<" dead="<<player.dead<<" action="<<player.source_action_word
        <<" position="<<player.position.x<<','<<player.position.y<<','<<player.position.z<<'\n';
    check(!player.dead,"Actual campaign player died before HUD weapon selection");
  }
  unsigned weapons{},optics{},binoculars{};
  unsigned expected_weapons{},expected_optics{};
  for(const auto &[id,definition]:content.weapons)if(definition.first_person_model!=null_datum) {
    const auto authored=cache.load_hud(content.player.datum,id);
    if(std::ranges::none_of(authored.widgets,[](const auto &row){return row.weapon_type==1;}))continue;
    ++expected_weapons;expected_optics+=unsigned(definition.zoom.levels);
  }
  for(const auto &[id,definition]:content.weapons)if(definition.first_person_model!=null_datum) {
    const auto authored=cache.load_hud(content.player.datum,id);
    if(std::ranges::none_of(authored.widgets,[](const auto &row){return row.weapon_type==1;}))continue;
    const bool binocular=definition.zoom.levels==0 && content.binocular_zoom &&
        content.binocular_zoom->levels>0 && !(definition.zoom.flags&0x10000u);
    simulation.equip(id);
    bool selected=false;
    for(unsigned tick=0;tick<60;++tick) {
      simulation.step(input);
      const auto &unit=simulation.player();
      const std::array modes{unit.weapon.magazine.mode};
      const ZoomWeaponView view{definition.zoom,definition.magazine?std::span(modes):std::span<const std::int16_t>{}};
      if(unit.weapon.definition==id && unit.source_inventory.current[0]>=0 &&
          ((!definition.zoom.levels && !binocular) || source_zoom_admitted({unit.source_parent,unit.grenade_action.active_actions(),{&view,nullptr}}))) {selected=true;break;}
    }
    if(!selected) {
      const auto &unit=simulation.player();
      std::cerr<<"Rejected selection "<<definition.name<<" wanted="<<id<<" current="<<unit.weapon.definition
          <<" hand="<<int(unit.source_inventory.current[0])<<" requested="<<int(unit.source_inventory.requested[0])
          <<" action="<<unit.source_action_word<<" dead="<<unit.vitality.dead<<" magazine="<<unit.weapon.magazine.mode<<'\n';
    }
    check(selected,"Original campaign equipment did not become the current hand");
    const auto player=source->hud_player(simulation);
    const auto original=source->objects().tu15_world().weapon_hud_state(player.source_weapon);
    const auto projected=multiplayer_hud_weapon_state(player,authored,0);
    check(original.loaded==projected.loaded && original.reserve==projected.reserve && original.capacity==projected.capacity &&
        original.maximum_reserve==projected.maximum_reserve && original.age==projected.age && original.heat==projected.heat &&
        original.charge==projected.charge && original.magazine_working==projected.magazine_working &&
        original.charging==projected.charging && original.locking==projected.locking && original.locked==projected.locked,
        "Campaign HUD numeric inputs differ from complete original13DA89");
    check(!hud.frame(player,std::uint32_t(simulation.ticks()),1280,720).empty(),"Actual selected campaign weapon has no authored crosshair");
    const auto zoom_levels=binocular?content.binocular_zoom->levels:definition.zoom.levels;
    for(int zoom=0;zoom<zoom_levels;++zoom) {
      input.zoom=std::int16_t(zoom);input.zoom_reset_serial=simulation.player().zoom_reset_serial;
      simulation.step(input);const auto scoped=source->hud_player(simulation);
      check(scoped.source_zoom.current==int(zoom),"Actual campaign optic did not accept its delivered selector");
      check(!hud.frame(scoped,std::uint32_t(simulation.ticks()),1280,720).empty(),"Actual campaign optic submitted no authored HUD");
      if(binocular) {
        check(scoped.source_hud_zoom_mask&0x400u,"Campaign binocular selector lost its authored mask condition");++binoculars;
      } else ++optics;
    }
    input.zoom=-1;input.zoom_reset_serial=simulation.player().zoom_reset_serial;simulation.step(input);
    ++weapons;std::cout<<"PASS actual campaign weapon="<<definition.name<<" zoom levels="<<definition.zoom.levels<<'\n';
  }
  check(expected_weapons>0 && weapons==expected_weapons && optics==expected_optics && binoculars,
      "Campaign did not exercise every resident handheld weapon, optic and binocular HUD");
  std::cout<<weapons<<" actual campaign handhelds, "<<optics<<" optic levels and "<<binoculars<<" binocular levels passed\n";
  simulation.request_checkpoint();simulation.step(input);
  const auto saved=source->hud_player(simulation);const auto saved_tick=simulation.ticks();
  input.zoom=0;input.zoom_reset_serial=simulation.player().zoom_reset_serial;simulation.step(input);
  simulation.request_revert();simulation.step(input);
  const auto restored=source->hud_player(simulation);
  check(simulation.ticks()==saved_tick && restored.source_unit==saved.source_unit && restored.source_weapon==saved.source_weapon &&
      restored.source_zoom.current==saved.source_zoom.current && restored.source_hud_target_flags==saved.source_hud_target_flags,
      "Campaign revert did not restore the coupled source HUD owner/state");
  check(simulation.object_lifetimes().pool().source_memory()==&source->objects().memory(),"Campaign revert left a stale object-pool borrower");
  std::cout<<"PASS campaign checkpoint restored source player/weapon/zoom/targeting and pool backing\n";
  if(cache.path().filename()=="01b_spacestation.map") {
    simulation.set_team_allegiance(1,2,true);
    const auto atmosphere=cache.load_bsp_atmosphere(0);
    const auto squad=std::ranges::find_if(content.squads,[&](const auto &s) {
      return s.team==2 && s.count>0 && !s.starts.empty() &&
          std::ranges::all_of(s.starts,[&](const auto &p){return p.reference_frame==-1 && atmosphere.cluster_at(p.position)>=0;});
    });
    check(squad!=content.squads.end(),"Cairo has no admitted authored friendly squad");
    std::cout<<"TARGET squad="<<squad->name<<" team="<<squad->team<<'\n';
    try {source->place_ai(simulation,unsigned(squad-content.squads.begin()),std::int16_t(1));}
    catch(...) {
      for(const auto &object:source->objects().live_objects())if(object.type==2 && !content.weapons.contains(object.definition)) {
        const auto tag=cache.find_tag({object.definition});
        std::cerr<<"Unprepared original item "<<object.definition<<" "<<(tag?tag->name:"<absent tag>")<<'\n';
      }
      throw;
    }
    const auto actor=std::ranges::find_if(simulation.actors(),[](const auto &u){return u.exists && !u.vitality.dead && u.source_membership==2;});
    check(actor!=simulation.actors().end(),"Original friendly placement produced no retained actor view");
    const auto target=actor->source_datum;
    CollisionWorld terrain(content.collision);bool acquired{};
    for(unsigned side=0;side<8 && !acquired;++side) {
      const auto &npc=*std::ranges::find_if(simulation.actors(),[&](const auto &u){return u.source_datum==target;});
      const auto angle=float(side)*.7853981633974483f;
      auto position=npc.body.position+Vec3{3.f*std::cos(angle),3.f*std::sin(angle),0};
      const auto floor=terrain.raycast(position+Vec3{0,0,2.f},{0,0,-1},4.f);
      if(!floor || atmosphere.cluster_at(floor->position)<0)continue;
      position.z=floor->position.z+.002f;
      source->objects().tu15_world().set_object_position(simulation.player().source_datum,position);
      source->publish(simulation);input.zoom=-1;
      for(unsigned tick=0;tick<6 && !acquired;++tick) {
        const auto found=std::ranges::find_if(simulation.actors(),[&](const auto &u){return u.source_datum==target;});
        check(found!=simulation.actors().end() && found->exists,"Actual friendly actor retired during targeting");
        const auto eye=simulation.player().body.position+Vec3{0,0,content.player.camera_height};
        const auto actor_position=found->body.position;
        const auto look=found->body.position+Vec3{0,0,found->definition.height*.5f}-eye;
        input.yaw=std::atan2(look.y,look.x);input.pitch=std::atan2(look.z,std::hypot(look.x,look.y));
        input.zoom_reset_serial=simulation.player().zoom_reset_serial;
        source->objects().publish_native_observer_position(0,eye);
        // Exercise the production command/targeting stage against the real
        // placed actor. The earlier weapon checks exercise complete ticks;
        // actor behavior decisions are independent of HUD target publication.
        source->deliver_controls(simulation,input);
        const std::array command_frame{0x60028000u};
        source->objects().tu15_world().call_actual(0x14fec0u,command_frame);
        source->publish(simulation);
        const auto player=source->hud_player(simulation);
        if(tick==0) {
          const auto &memory=source->objects().memory();
          const Bytes eye_query(memory.view(0x60041200u,12));
          const Bytes target_query(memory.view(0x60041300u,0x58));
          const auto body=source->objects().object_body(player.source_unit);
          const Bytes published(memory.view(body+0x1c8u,0x20));
          std::cout<<"TARGET side="<<side<<" actual_eye="<<eye_query.f32(0)<<','<<eye_query.f32(4)<<','<<eye_query.f32(8)
              <<" native_eye="<<eye.x<<','<<eye.y<<','<<eye.z<<" actor="<<target
              <<" actor_position="<<actor_position.x<<','<<actor_position.y<<','<<actor_position.z
              <<" query="<<target_query.u32(0)<<" published="<<published.u32(0)<<','<<published.u32(4)
              <<" confidence="<<published.f32(0x1c)<<" flags="<<player.source_hud_target_flags<<'\n';
        }
        if(!(player.source_hud_target_flags&1u))continue;
        const auto body=source->objects().object_body(player.source_unit);
        const Bytes targets(source->objects().memory().view(body+0x1c8u,8));
        check(targets.u32(0)==target || targets.u32(4)==target,"Friendly HUD flag did not name the actual original actor target");
        auto authored=cache.load_hud(content.player.datum,player.weapon);authored.action_messages.enabled=false;
        for(const auto &row:authored.widgets)if(row.weapon_type==1)
          std::cout<<"TARGET authored="<<row.name<<" yes_aim="<<row.yes[1]<<" no_aim="<<row.no[1]<<'\n';
        std::erase_if(authored.widgets,[](const auto &row){return row.weapon_type!=1 || !(row.yes[1]&1u);});
        check(!authored.widgets.empty(),"Actual selected weapon lacks its authored friendly reticle");
        Mesh friendly_mesh;MultiplayerHud friendly(authored,cache.shared_directory()/"fonts",friendly_mesh,1,"",
            cache.original_data()->hud_bitmap_geometry_constants,{});
        MultiplayerState state;state.tick=std::uint32_t(simulation.ticks());state.players[0]=player;
        friendly.animate(state,0,0,0,0,state.tick*tick_seconds);
        check(!friendly.draw(state,0,0,1,1280,720,0,0).empty(),"Original acquired friendly target did not submit its authored reticle");
        acquired=true;
      }
    }
    check(acquired,"Original campaign targeting never acquired the actual authored friendly actor");
    simulation.set_team_allegiance(1,2,false);
    check(!(source->hud_player(simulation).source_hud_target_flags&1u),
        "Original allegiance removal left the friendly HUD flag set");
    simulation.request_revert();simulation.step(input);
    check(source->hud_player(simulation).source_weapon==saved.source_weapon &&
        std::ranges::none_of(simulation.actors(),[&](const auto &u){return u.exists && u.source_datum==target;}),
        "Campaign target encounter survived its whole-authority checkpoint restore");
    input.yaw=saved.yaw;input.pitch=saved.pitch;input.zoom=-1;
    input.zoom_reset_serial=simulation.player().zoom_reset_serial;
    check(!simulation.allied(1,2),"Campaign checkpoint retained a discarded allegiance command");
    std::cout<<"PASS original allegiance, actor acquisition, friendly reticle removal and checkpoint retirement\n";
  }
  if(content.bsp_count>1) {
    simulation.request_bsp(1);input.zoom=-1;simulation.step(input);
    check(simulation.bsp()==1,"Campaign source BSP transition was not committed");
    const auto after_bsp=source->hud_player(simulation);
    check(after_bsp.source_unit==saved.source_unit && after_bsp.source_weapon==saved.source_weapon,
        "Campaign BSP transition replaced its retained unit/weapon identity");
    simulation.request_revert();simulation.step(input);
    check(simulation.bsp()==0 && source->hud_player(simulation).source_weapon==saved.source_weapon,
        "Campaign checkpoint failed to restore its original BSP and HUD owner");
    std::cout<<"PASS campaign BSP transition and cross-BSP checkpoint preserved actual HUD owners\n";
  }
  std::cout<<"Campaign source player, targeting publication and authored HUD submission passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
