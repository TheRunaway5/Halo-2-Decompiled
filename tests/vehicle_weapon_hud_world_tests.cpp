#include "halo2/multiplayer_world.hpp"
#include "halo2/hud.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
using namespace halo2;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
struct Entrance {Spawn spawn;unsigned seat{};std::size_t placement{};};
Entrance entrance(const Cache &cache,const LevelContent &content,std::string_view name,bool child) {
  const auto &assets=*content.unit_interactions;
  for(const auto &[row,placement]:assets.netgame_placements) {
    const auto &object=content.objects.at(placement);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.datum==object.definition;});
    if(tag==cache.tags().end() || tag->name.find("\\"+std::string(name)+"\\")==std::string::npos)continue;
    const auto basis=authored_basis(object.rotation);
    auto root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
    auto target=placement;
    if(child) {
      const auto found=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &p){return p.model_attachment && p.model_attachment->parent==placement;});
      if(found==content.objects.end())continue;
      target=std::size_t(found-content.objects.begin());const auto &attachment=*found->model_attachment;
      const auto &parent=assets.units.at(object.definition),&definition=assets.units.at(found->definition);
      const auto nodes=pose_model_nodes_world(parent.hierarchy,root);
      const auto markers=pose_model_markers(parent.markers.at(attachment.parent_marker),nodes,parent.permutations,{},1);
      check(!markers.empty(),"Missing authored turret attachment marker");Transform local;
      if(attachment.child_marker) {
        const auto child_nodes=pose_model_nodes_world(definition.hierarchy,Transform{});
        const auto child_markers=pose_model_markers(definition.markers.at(attachment.child_marker),child_nodes,definition.permutations,{},1);
        if(!child_markers.empty())local=child_markers.front().world;
      }
      root=compose_transform(markers.front().world,inverse_transform(local));
    }
    const auto &definition=assets.units.at(content.objects.at(target).definition);
    const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    for(unsigned index=0;index<definition.seats.size();++index) {
      const auto &seat=definition.seats[index];if(child?index!=0:!(seat.flags&4u))continue;
      const auto marker=definition.markers.find(seat.entry_marker);if(marker==definition.markers.end())continue;
      const auto entrances=pose_model_markers(marker->second,nodes,definition.permutations,{},1);if(entrances.empty())continue;
      Spawn spawn;spawn.position=entrances.front().world.position+entrances.front().world.basis[0]*(seat.radius*.5f);
      CollisionWorld terrain(content.collision);const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
      if(!floor)continue;spawn.position.z=floor->position.z+content.player.radius;
      const auto look=entrances.front().world.position-(spawn.position+Vec3{0,0,content.player.camera_height});
      spawn.facing=std::atan2(look.y,look.x);return {spawn,index,target};
    }
  }
  throw FormatError("Missing authored vehicle HUD approach: "+std::string(name));
}
}
int main(int argc,char **argv)try {
#ifdef HALO2_VEHICLE_HUD_GPU
  check(argc==5,"Supply Coagulation MAP SHARED TU1.5_XBE SCREENSHOT");
#else
  check(argc==4,"Supply Coagulation MAP SHARED TU1.5_XBE");
#endif
  unsigned worlds{},seated{},weapon_huds{};
  for(const auto &[name,child,parameter,choice]:std::array<std::tuple<std::string_view,bool,unsigned,int>,7>{{
      {"ghost",false,76,3},{"banshee",false,76,1},{"scorpion",false,78,1},
      {"wraith",false,78,2},{"warthog",true,76,1},{"warthog",true,76,2},{"spectre",true,76,5}}}) {
#ifdef HALO2_VEHICLE_HUD_GPU
    const auto selected=std::getenv("HALO2_VEHICLE_HUD_GPU_NAME");
    if(name!=(selected?selected:"spectre"))continue;
#endif
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.local_players=2;options.starting_weapon=5;options.source_object_world=true;
    options.variant.values.set(parameter,choice);options.session_random_seed=0x13c40au;
    Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
    const auto content=cache.load_content(0,true,source_game_variant(options.variant));
    const auto approach=entrance(cache,content,name,child);options.native_spawn_inputs={approach.spawn,cache.spawns().back()};
    options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
    auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
    std::array<Controls,multiplayer_capacity> controls{};const auto player=world.local_slots[0];
    controls[player].yaw=approach.spawn.facing;
    for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
    for(unsigned tick=0;tick<150;++tick) {
      controls[player].pickup=true;controls[player].pickup_milliseconds=900;
      world.match.step(controls);auto state=world.match.state();const auto &p=state.players[player];
      if(p.source_parent==null_datum)continue;
      check(!state.players[world.local_slots[1]].source_vehicle_hud.ownership.seated &&
          !state.players[world.local_slots[1]].source_vehicle_hud.weapon.present,"Vehicle HUD leaked into the other player");
      ++seated;check(p.source_vehicle_hud.ownership.seated,"Actual seated source unit lost vehicle HUD ownership");
      const auto &actual=p.source_vehicle_hud;
      check(actual.ownership.vehicle_weapon!=null_datum && actual.weapon.present,"Actual armed seat lost its vehicle weapon HUD");
      check(p.source_seat==int(approach.seat),"Original owner selected another seat");
      check(actual.weapon.datum==actual.ownership.vehicle_weapon &&
          world.source_objects->object_definition(actual.weapon.datum)==actual.definition,"Vehicle HUD weapon identity differs from its original object");
      const auto body=world.source_objects->object_body(actual.weapon.datum);float age{},heat{};
      world.source_objects->memory().read(body+0x180u,std::as_writable_bytes(std::span(&age,1)));
      world.source_objects->memory().read(body+0x184u,std::as_writable_bytes(std::span(&heat,1)));
      check(actual.weapon.age==age && actual.weapon.heat==heat,"Vehicle HUD differs from actual source heat/age fields");
      auto authored=cache.load_hud(content.player_representations.at(2).biped.datum,actual.definition);
      authored.action_messages.enabled=false;
      std::erase_if(authored.widgets,[](const auto &w){return w.weapon_type!=1;});
      for(auto &w:authored.widgets)w.weapon_type=4;
      Mesh mesh;MultiplayerHud hud(authored,options.shared_maps/"fonts",mesh,1,"Slayer",cache.original_data()->hud_bitmap_geometry_constants,{},2);
      hud.animate(state,player,0,0,0,state.tick*tick_seconds);
      auto vertices=hud.draw(state,player,0,1,1280,720,0,0);
      if(actual.ownership.parent_hud!=null_datum) {
        auto parent_content=cache.load_hud_definition(content.player_representations.at(2).biped.datum,actual.ownership.parent_hud);
        MultiplayerHud parent(parent_content,options.shared_maps/"fonts",mesh,1,"Slayer",cache.original_data()->hud_bitmap_geometry_constants);
        parent.animate(state,player,0,0,0,state.tick*tick_seconds);
        const auto parent_vertices=parent.draw(state,player,0,1,1280,720,0,0);
        vertices.insert(vertices.end(),parent_vertices.begin(),parent_vertices.end());
      }
      check(!vertices.empty(),"Live source vehicle and seat HUDs submitted no authored widgets");
#ifdef HALO2_VEHICLE_HUD_GPU
      options.frames=4;options.mute=true;options.screenshot=argv[4];
      unsigned submitted{};
      options.view_state_observer=[&](unsigned view,const MultiplayerState &frame,const Camera &) {
        if(view)return;
        check(frame.players[player].source_vehicle_hud.weapon.present &&
            !frame.players[player].source_vehicle_hud.ownership.handheld_visible,
            "Rendered seated view lost its source weapon HUD");++submitted;
      };
      check(run_multiplayer(options)==0 && submitted>=4,"Actual vehicle HUD Vulkan submission stopped early");
      std::cout<<submitted<<" actual seated Vulkan frames submitted; screenshot="<<argv[4]<<'\n';
#endif
      ++weapon_huds;std::cout<<"PASS "<<name<<(child?" turret":" driver")<<" choice="<<choice
          <<" weapon="<<actual.definition<<" handheld="<<actual.ownership.handheld_visible<<'\n'<<std::flush;
#ifdef HALO2_VEHICLE_HUD_GPU
      // The runtime releases its view/session callbacks on return. Source
      // entry/exit is verified by the separate world-only executable above.
      return 0;
#endif
      break;
    }
    ++worlds;check(weapon_huds==worlds,"Normal original vehicle entry did not reach its armed HUD");
    controls[player].pickup=false;controls[player].pickup_milliseconds=0;
    for(unsigned tick=0;tick<90;++tick)world.match.step(controls);
    bool exited=false;
    for(unsigned tick=0;tick<150;++tick) {
      controls[player].pickup=true;controls[player].pickup_milliseconds=900;world.match.step(controls);
      const auto state=world.match.state();const auto &p=state.players[player];
      if(p.source_parent!=null_datum)continue;
      check(!p.source_vehicle_hud.ownership.seated && p.source_vehicle_hud.ownership.handheld_visible &&
          !p.source_vehicle_hud.weapon.present && p.source_vehicle_hud.ownership.vehicle_weapon==null_datum &&
          p.source_vehicle_hud.ownership.parent_hud==null_datum,
          "Vehicle HUD did not release its weapon on actual source exit");exited=true;break;
    }
    check(exited,"Normal original exit did not release vehicle HUD ownership");
  }
  std::cout<<worlds<<" actual TU1.5 vehicle worlds, "<<weapon_huds<<" selected source weapon HUDs passed\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
