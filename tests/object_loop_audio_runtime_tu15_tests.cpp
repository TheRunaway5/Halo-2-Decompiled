#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/math.hpp"
#include "halo2/player_action_edges.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address,unsigned size=4) {
  const auto bytes=memory.view(address,size);std::uint32_t value{};
  for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
  return value;
}
struct Entrance {Spawn spawn;Vec3 point;};
Entrance entrance(const Cache &cache,const LevelContent &content) {
  check(bool(content.unit_interactions),"Vehicle loop test has no authored seats");
  const auto &assets=*content.unit_interactions;
  for(const auto &[row,index]:assets.netgame_placements) {
    const auto &placed=content.objects.at(index);
    const auto tag=cache.query_tag({.handle=TagHandle{placed.definition}});
    if(!tag||!tag->name.ends_with("\\warthog")||placed.model_attachment)continue;
    const auto basis=authored_basis(placed.rotation);
    const auto root=source_object_transform(placed.position,basis.forward,basis.up,placed.scale,false);
    const auto &definition=assets.units.at(placed.definition);
    const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    for(const auto &seat:definition.seats) {
      if(!(seat.flags&4u))continue;
      const auto marker=definition.markers.find(seat.entry_marker);
      if(marker==definition.markers.end())continue;
      const auto points=pose_model_markers(marker->second,nodes,definition.permutations,{},1);
      if(points.empty()||seat.radius<=0)continue;
      auto position=points.front().world.position+points.front().world.basis[0]*(seat.radius*.5f);
      const CollisionWorld terrain(content.collision);
      const auto floor=terrain.raycast(position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
      if(!floor)continue;
      position.z=floor->position.z+content.player.radius;
      const auto delta=points.front().world.position-(position+Vec3{0,0,content.player.camera_height});
      return {{position,std::atan2(delta.y,delta.x)},points.front().world.position};
    }
  }
  throw FormatError("No authored Warthog driver approach");
}
}
int main(int argc,char **argv) {
  Json report{{"passed",false},{"source_requests",Json::array()},
      {"source_admission_or_voice_results_supplied",false},{"full_audio_parity",false}};
  std::filesystem::path output;
  try {
    check(argc==5,"Supply MCC_MAP MCC_MAPS REPORT baseline|source-frame|listeners-and-source-frame");output=argv[3];
    const std::string mode=argv[4];check(mode=="baseline"||mode=="source-frame"||mode=="listeners-and-source-frame","Unknown object-loop mode");
    report["mode"]=mode;report["map"]=argv[1];
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
    options.local_players=1;options.starting_weapon=5;options.source_object_world=true;options.mute=true;
    options.session_random_seed=0x22a0d10u;
    Vec3 entry_point;
    {
      Cache cache(options.map,options.shared_maps);
      const auto content=cache.load_content(0,true,source_game_variant(options.variant));
      const auto setup=entrance(cache,content);options.native_spawn_inputs={setup.spawn};
      entry_point=setup.point;
      report["declared_entry_spawn"]={setup.spawn.position.x,setup.spawn.position.y,setup.spawn.position.z};
      report["authored_entry_point"]={setup.point.x,setup.point.y,setup.point.z};
    }
    MultiplayerPreparedWorld world(options,std::nullopt);world.bind_source_local_views(world.local_slots);
    check(world.cache->is_mcc(),"Object-loop runtime requires the prepared MCC cache");
    auto &source=*world.source_objects;auto &native=source.tu15_world();
    auto &memory=native.memory();
    const auto loops=[&] {
      Json rows=Json::array();const auto pool=word(memory,0x4f9b5cu);
      check(pool && word(memory,pool+0x24u)==24,"Original looping pool lost its actual layout");
      const auto count=word(memory,pool+0x38u),bitmap=word(memory,pool+0x48u),data=word(memory,pool+0x44u);
      for(unsigned i=0;i<count;++i)if(word(memory,bitmap+(i>>5)*4u)&(1u<<(i&31u))) {
        const auto record=data+i*24u,definition=word(memory,record+12u),owner=word(memory,record+16u);
        const auto tag=world.cache->query_tag({.handle=TagHandle{definition}});
        Json row{{"datum",(word(memory,record,2)<<16)|i},{"definition",definition},
            {"type",word(memory,record+3u,1)},{"state",word(memory,record+2u,1)},
            {"flags",word(memory,record+4u,2)},{"control_bits",word(memory,record+8u)},
            {"object",owner},{"parameter",word(memory,record+20u)}};
        if(tag)row["name"]=tag->name;
        rows.push_back(std::move(row));
      }
      return rows;
    };
    report["initial_loops"]=loops();
    unsigned tick{},requests{};
    native.observe_loop_sound_requests([&](const auto &request) {
      const auto &args=request.arguments;
      const auto tag=world.cache->query_tag({.handle=TagHandle{args[0]}});
      ++requests;
      if(report["source_requests"].size()<128)report["source_requests"].push_back({{"tick",tick},{"arguments",args},
          {"name",tag?tag->name:""},{"location_type",std::to_integer<unsigned>(request.location[3])&15u},
          {"control_bits",Bytes(request.location).u32(4)}});
    });
    std::array<Controls,multiplayer_capacity> commands{};
    const auto initial=world.match.state();const auto slot=world.local_slots.at(0);
    commands[slot].yaw=initial.players[slot].yaw;commands[slot].pitch=initial.players[slot].pitch;
    const auto look=entry_point-(initial.players[slot].position+Vec3{0,0,world.content.player.camera_height});
    commands[slot].pitch=std::atan2(look.z,std::hypot(look.x,look.y));
    unsigned seated{};
    const auto threshold=source_pickup_hold_threshold(world.content.pickup_hold_seconds);
    for(tick=0;tick<180;++tick) {
      // Explicit composition of actual13A9D8's audio child. The ordinary
      // object step and its clock are left responsible for all simulation.
      if(mode=="listeners-and-source-frame")native.call_actual(0x129f10u,std::array{0u});
      if(mode!="baseline")native.call_actual(0x18ee50u,std::array{std::bit_cast<std::uint32_t>(tick_seconds)});
      const auto player=world.match.state().players[slot];
      commands[slot].vehicle_interaction_target=player.action_prompt.target;
      if(player.source_parent==null_datum) {
        commands[slot].pickup=tick<90;
        commands[slot].pickup_milliseconds=std::uint16_t(std::min<unsigned>(tick*33u,threshold+100u));
      }else {
        ++seated;commands[slot].pickup=false;commands[slot].pickup_milliseconds=0;
        commands[slot].forward=tick>=90&&tick<150?1.f:0.f;
      }
      world.match.step(commands);
    }
    native.observe_loop_sound_requests({});report["final_loops"]=loops();report["ticks"]=tick;report["seated_ticks"]=seated;
    report["request_count"]=requests;report["request_samples_clipped"]=requests>report["source_requests"].size();
    report["passed"]=seated>0&&!report["initial_loops"].empty()&&!report["source_requests"].empty();
    if(!report["passed"].get<bool>())report["error"]="Original object loops produced no playback requests";
  }catch(const std::exception &error){report["error"]=error.what();}
  if(!output.empty()){std::ofstream file(output);file<<report.dump(2)<<'\n';}
  std::cout<<report.dump(2)<<'\n';return report["passed"].get<bool>()?0:1;
}
