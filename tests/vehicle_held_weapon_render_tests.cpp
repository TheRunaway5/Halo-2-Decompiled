#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/source_model_admission.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
Spawn entrance(const Cache &cache,const LevelContent &content,unsigned seat_index) {
  const auto &assets=*content.unit_interactions;
  for(const auto &[row,index]:assets.netgame_placements) {
    const auto &object=content.objects.at(index);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.datum==object.definition;});
    if(tag==cache.tags().end() || tag->name!="objects\\vehicles\\warthog\\warthog")continue;
    const auto &definition=assets.units.at(object.definition);
    const auto &seat=definition.seats.at(seat_index);
    const auto basis=authored_basis(object.rotation);
    const auto root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
    const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    const auto markers=pose_model_markers(definition.markers.at(seat.entry_marker),nodes,definition.permutations,{},1);
    check(!markers.empty(),"Missing authored Warthog entrance");
    Spawn spawn;spawn.position=markers.front().world.position+markers.front().world.basis[0]*(seat.radius*.5f);
    CollisionWorld terrain(content.collision);
    const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
    if(!floor)continue;
    spawn.position.z=floor->position.z+content.player.radius;
    const auto look=markers.front().world.position-(spawn.position+Vec3{0,0,content.player.camera_height});
    spawn.facing=std::atan2(look.y,look.x);return spawn;
  }
  throw FormatError("No authored Warthog approach");
}
}
int main(int argc,char **argv)try {
  check(argc==6,"Supply MAP SHARED TU1.5_XBE_OR_mcc SEAT STAGE");
  const bool mcc=std::string_view(argv[3])=="mcc";
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || (!mcc && !std::filesystem::exists(argv[3])))return 77;
  SDL_SetMainReady();
  const auto seat=unsigned(std::stoul(argv[4]));const std::string_view stage=argv[5];
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  if(!mcc)options.gameplay_xbe=argv[3];
  options.local_players=2;options.starting_weapon=5;options.source_object_world=true;
  options.variant.values.set(76,1);options.session_random_seed=0x13c40au;
  Cache cache(options.map,options.shared_maps,mcc?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(options.gameplay_xbe));
  check(cache.is_mcc()==mcc,"Vehicle render check received the wrong cache platform");
  const auto content=cache.load_content(0,true,source_game_variant(options.variant));
  const auto approach=entrance(cache,content,seat);
  options.native_spawn_inputs={approach,cache.spawns().back()};
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  auto &world=*options.prepared_world;world.bind_source_local_views(world.local_slots);
  const auto slot=world.local_slots[0];auto &objects=*world.source_objects;
  std::array<Controls,multiplayer_capacity> controls{};controls[slot].yaw=approach.facing;
  for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
  const auto before=world.match.state().players[slot];
  if(stage!="on-foot") {
    bool entered{};
    for(unsigned tick=0;tick<150;++tick) {
      controls[slot].pickup=true;controls[slot].pickup_milliseconds=900;world.match.step(controls);
      if(world.match.state().players[slot].source_parent!=null_datum){entered=true;break;}
    }
    check(entered,"Normal source vehicle entry failed");
    check(world.match.state().players[slot].source_seat==int(seat),"Entered a different seat");
    controls[slot].pickup=false;controls[slot].pickup_milliseconds=0;
    // Let the entry animation finish, then move the vehicle away from the
    // pre-entry position. A stale hidden weapon bank must never be submitted.
    for(unsigned tick=0;tick<90;++tick)world.match.step(controls);
    if(stage=="moving") {
      check(seat==0,"Moving case requires a driver");controls[slot].forward=1;
      for(unsigned tick=0;tick<60;++tick)world.match.step(controls);
      controls[slot].forward=0;
      check(length(world.match.state().players[slot].position-before.position)>1,"Vehicle did not move away from entry");
    }
    if(stage=="exited") {
      bool exited{};
      for(unsigned tick=0;tick<150;++tick) {
        controls[slot].pickup=true;controls[slot].pickup_milliseconds=900;world.match.step(controls);
        if(world.match.state().players[slot].source_parent==null_datum){exited=true;break;}
      }
      check(exited,"Normal source vehicle exit failed");
    }
  }
  SourceModelResourceWorld visibility;visibility.banks.objects=objects.source_object_pool();
  visibility.banks.read=[&](auto address,auto size){return objects.memory().view(address,size);};
  const auto handheld=world.match.state().players[slot].source_vehicle_hud.ownership.handheld_visible;
  unsigned hidden_frames{},visible_frames{},submitted{};
  options.animation_observer=[&](unsigned,unsigned player,const AnimationRenderObservation &draw) {
    if(player!=slot || draw.first_person)return;
    check(draw.weapon==before.source_weapon,"Entry changed the carried weapon identity");
    const bool hidden=source_model_owner_hidden(draw.weapon,visibility);
    std::cout<<"weapon="<<draw.weapon<<" hidden="<<hidden
        <<" handheld="<<handheld
        <<" vertices="<<draw.weapon_vertices.size()
        <<" stage="<<stage<<'\n';
    if(hidden) {
      ++hidden_frames;
      check(draw.weapon_vertices.empty(),"Vehicle entry submitted the hidden held weapon at its stale on-foot WORLD pose");
    } else {
      ++visible_frames;check(!draw.weapon_vertices.empty(),"Visible held weapon disappeared");
      check(!draw.weapon_nodes.empty() && !draw.body_nodes.empty() &&
          length(draw.weapon_nodes.front().position-draw.body_nodes.front().position)<3,
          "Visible held weapon did not follow its player");
    }
  };
  options.frames=3;options.width=640;options.height=480;options.mute=true;
  options.frame_observer=[&](const MultiplayerFrameTiming &frame){submitted+=frame.submitted;};
  check(run_multiplayer(options)==0 && submitted==3,"Vehicle held-weapon frames were not submitted");
  check(hidden_frames+visible_frames>=3,"No actual held-weapon draw observations");
  check(stage=="on-foot" || stage=="exited" || seat!=0 || hidden_frames>=3,"Driver did not exercise source weapon hiding");
  check((stage!="on-foot" && stage!="exited") || visible_frames>=3,"On-foot weapon visibility did not restore");
  std::cout<<"PASS seat="<<seat<<" stage="<<stage<<" hidden="<<hidden_frames<<" visible="<<visible_frames<<'\n';
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
