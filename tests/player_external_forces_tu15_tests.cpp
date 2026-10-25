#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/math.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_memory_read_alias_observation_tu15.hpp"
#include "halo2/source_sound_timing_input_observation_tu15.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *message){if(!value)throw FormatError(message);}
Json vector(Vec3 v){return Json::array({v.x,v.y,v.z});}
struct Report {
  std::filesystem::path path;
  Json data{{"passed",false},{"reference","OG Xbox TU1.5"},{"runtime_assets","prepared MCC"},
      {"supplied_damage_velocity_or_contact_results",false},{"samples",Json::array()}};
  void save()const{if(!path.empty()){std::ofstream out(path);out<<data.dump(2)<<'\n';}}
  ~Report(){save();}
};
struct Observations {
  MultiplayerObjectRuntime &source;
  ~Observations(){source.observe_body_impulses({});source.observe_physics_stores({});}
};
struct FloorStart {Vec3 position,direction;};
std::array<Vec3,3> wall_floor(const Cache &cache,const LevelContent &content,PhysicsLiveWorldTu15 &native) {
  const CollisionWorld terrain(content.collision);
  for(const auto &triangle:content.collision) {
    const auto n=cross(triangle.b-triangle.a,triangle.c-triangle.a);
    if(length(n)<.5f || std::abs(normalized(n).z)>.01f)continue;
    const auto center=(triangle.a+triangle.b+triangle.c)/3.f;
    const auto normal=normalized(Vec3{n.x,n.y,0});
    for(float distance:{.5f,.75f,1.f}) {
      auto a=center-normal*distance,b=center+normal*distance;
      const auto fa=terrain.raycast(a+Vec3{0,0,2},{0,0,-1},5);
      const auto fb=terrain.raycast(b+Vec3{0,0,2},{0,0,-1},5);
      if(!fa || !fb || fa->normal.z<.99f || fb->normal.z<.99f ||
          std::abs(fa->position.z-fb->position.z)>.02f)continue;
      a.z=fa->position.z+.05f;b.z=fb->position.z+.05f;
      bool lethal=false;
      for(const auto trigger:content.kill_triggers)for(const auto point:{a,b})
        for(float height:{0.f,content.player.height*.5f,content.player.height})
          lethal|=native.trigger_contains(trigger,point+Vec3{0,0,height});
      if(lethal)continue;
      // An exterior wall can have supported mesh on its lethal outside.
      // Choose an interior scene using actual authored spawn context on both
      // sides; source settling still has to keep both participants alive.
      const auto has_spawn=[&](Vec3 p){return std::ranges::any_of(cache.spawns(),[&](const auto &spawn){
        return length(spawn.position-p)<12.f &&
            dot(spawn.position-center,normal)*dot(p-center,normal)>.1f;
      });};
      if(!has_spawn(a) || !has_spawn(b))continue;
      if(terrain.overlaps(a,content.player.height,content.player.radius) ||
          terrain.overlaps(b,content.player.height,content.player.radius))continue;
      bool blocked=true;
      for(float height:{.1f,content.player.height*.5f,content.player.height*.9f}) {
        const auto delta=b-a;
        const auto hit=terrain.raycast(a+Vec3{0,0,height},normalized(delta),length(delta));
        blocked&=hit && hit->distance<length(delta)-content.player.radius;
      }
      if(blocked)return {a,b,Vec3{-normal.y,normal.x,0}};
    }
  }
  throw FormatError("No supported actual BSP wall separates the blast and player fixture");
}
FloorStart clear_floor(const Cache &cache,const LevelContent &content) {
  // These BSP queries choose initial scene inputs only. The original live
  // world supplies every later contact, projectile, damage and motion result.
  const CollisionWorld terrain(content.collision);
  std::vector<Vec3> anchors;
  for(const auto &spawn:cache.spawns())anchors.push_back(spawn.position);
  for(const auto &triangle:content.collision) {
    const auto normal=cross(triangle.b-triangle.a,triangle.c-triangle.a);
    if(length(normal)<.5f || std::abs(normalized(normal).z)<.99f)continue;
    const auto center=(triangle.a+triangle.b+triangle.c)/3.f;
    if(std::ranges::any_of(cache.spawns(),[&](const auto &spawn){
      return length(center-spawn.position)<8.f && std::abs(center.z-spawn.position.z)<content.player.height*.5f;
    }))anchors.push_back(center);
  }
  for(const auto anchor:anchors)for(unsigned direction=0;direction<8;++direction) {
    const float angle=float(direction)*.7853981633974483f;
    const Vec3 forward{std::cos(angle),std::sin(angle),0},side{-forward.y,forward.x,0};
    const auto floor=terrain.raycast(anchor+Vec3{0,0,1},{0,0,-1},3);
    if(!floor || floor->normal.z<.99f)continue;
    bool clear=true;
    for(int x=-2;x<=16 && clear;++x)for(int y=-2;y<=6 && clear;++y) {
      const auto p=floor->position+forward*(float(x)*.25f)+side*(float(y)*.25f);
      const auto support=terrain.raycast(p+Vec3{0,0,.5f},{0,0,-1},1);
      clear=support && support->normal.z>.99f && std::abs(support->position.z-floor->position.z)<.02f &&
          !terrain.overlaps(p+Vec3{0,0,.05f},content.player.height*2.f,content.player.radius);
    }
    if(clear)return {floor->position+Vec3{0,0,.05f},forward};
  }
  throw FormatError("No clear actual BSP floor for the external player force fixture");
}
Vec3 eye(MultiplayerObjectRuntime &source,std::uint32_t unit) {
  // Actual source1582F0, with caller-owned output pointers. This avoids an
  // inferred eye offset when aiming an ordinary source rocket or grenade.
  constexpr std::uint32_t position=0x6005b800u,velocity=position+0x20u;
  auto &native=source.tu15_world();
  auto input=native.prepare_source_call(std::array{position,velocity});input.gp[0]=unit;
  native.run_actual(0x1582f0u,input);
  const Bytes bytes(source.memory().view(position,12));
  return {bytes.f32(0),bytes.f32(4),bytes.f32(8)};
}
}
int main(int argc,char **argv) {
  Report report;
  try {
    std::cout<<std::unitbuf;
    check(argc==5,"Supply prepared MCC map, external-force case, target representation and report path");
    report.path=argv[4];
    if(!std::filesystem::is_regular_file(argv[1]))return 77;
    {
      std::array<std::byte,12> header{};std::ifstream input(argv[1],std::ios::binary);
      input.read(reinterpret_cast<char *>(header.data()),header.size());
      check(bool(input) && Bytes(header).u32(0)==0x68656164u && Bytes(header).u32(4)==13,
          "External player force acceptance requires a prepared MCC map before world activation");
    }
    const std::string scenario=argv[2];
    const bool lan=scenario.starts_with("lan-");
    std::string name=lan?scenario.substr(4):scenario;
    std::string vehicle_kind="warthog";
    for(const std::string kind:{"ghost","spectre","scorpion","wraith","banshee","warthog_gauss"})
      if(name.starts_with(kind+"-")){vehicle_kind=kind;name="vehicle-"+name.substr(kind.size()+1);break;}
    const bool vehicle=name.starts_with("vehicle-");
    const bool plasma=name.starts_with("plasma-");
    const bool grenade=name.starts_with("frag-") || plasma;
    const bool blocked=name=="frag-blocked" || name=="plasma-blocked";
    const bool negative=name=="frag-far" || name=="plasma-far" || blocked || name=="rocket-far" ||
        name=="vehicle-miss" || name=="vehicle-neutral";
    check(name=="frag-near" || name=="frag-middle" || name=="frag-far" || name=="frag-blocked" ||
        name=="plasma-near" || name=="plasma-far" || name=="plasma-blocked" ||
        name=="rocket-near" || name=="rocket-far" || name=="rocket-direct" ||
        name=="vehicle-hit" || name=="vehicle-splatter" || name=="vehicle-miss" || name=="vehicle-neutral",
        "Unknown external player force case");
    const unsigned representation=std::stoul(argv[3]);
    check(representation==2 || representation==3,"External force target must be Spartan or Elite");
    report.data.update({{"scenario",scenario},{"representation",representation},{"negative_control",negative},
        {"vehicle_kind",vehicle_kind}});
    MultiplayerOptions options;options.map=argv[1];options.local_players=lan?2:3;
    options.starting_weapon=grenade || vehicle?5:8;options.source_object_world=true;
    options.session_random_seed=0x15e470u;
    if(plasma)options.variant.values.set(83,15); // Original plasma-grenade weapon-set option.
    if(vehicle) {
      // Original variant parameters select authored vehicle births; no vehicle,
      // seat, velocity or contact result is manufactured by this fixture.
      options.variant.values.set(76,vehicle_kind=="ghost"?3:vehicle_kind=="spectre"?5:
          vehicle_kind=="warthog_gauss"?2:1);
      if(vehicle_kind=="scorpion" || vehicle_kind=="wraith")
        options.variant.values.set(78,vehicle_kind=="scorpion"?1:2);
    }
    for(unsigned i=0;i<options.local_players;++i){auto profile=source_default_player_configuration();
      profile[0x44]=!lan && i==1?representation:2;options.selected_profiles.push_back(profile);}
    MultiplayerPreparedWorld world(options);
    check(world.cache->is_mcc() && world.match.source_engine_owned(),
        "External force acceptance requires the actual native TU1.5 match and prepared MCC assets");
    auto &match=world.match;auto &source=*world.source_objects;auto &native=source.tu15_world();
    std::array<Controls,multiplayer_capacity> commands{};
    std::array<unsigned,3> slots{world.local_slots[0],lan?0:world.local_slots[1],world.local_slots.back()};
    std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
    if(lan) {
      host=std::make_unique<LanHost>(match,0,0x15e470u);
      auto profile=source_default_player_configuration();profile[0x44]=representation;
      const std::array<SourcePlayerConfiguration,1> profiles{profile};
      client=std::make_unique<LanClient>("127.0.0.1:"+std::to_string(host->port()),0x15e470u,1,
          std::span<const std::int8_t>{},profiles);
      const std::array<Controls,1> input{};
      for(unsigned i=0;i<30 && !client->ready();++i){client->send(input);host->receive(commands);host->broadcast();client->receive();}
      check(client->ready() && client->error().empty(),"External-force native LAN participant failed to join");
      slots[1]=client->slots().front();
    }
    unsigned exact_snapshots{},exact_pose_nodes{},exact_corpse_poses{};
    const auto step=[&] {
      if(client){std::this_thread::sleep_for(std::chrono::duration<float>(tick_seconds));
        const std::array<Controls,1> input{commands[slots[1]]};client->send(input);host->receive(commands);}
      match.step(commands);source.advance_effect_frame(tick_seconds);
      if(client) {
        const auto state=match.state();host->broadcast();
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(100);
        do {client->receive();if(client->state().tick==state.tick || !client->error().empty())break;
          std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<deadline);
        check(client->error().empty() && client->state().tick==state.tick,"External force LAN snapshot is rejected or stale");
        for(const auto slot:slots){const auto &a=state.players[slot],&b=client->state().players[slot];
          check(!std::memcmp(&a.position,&b.position,sizeof(Vec3)) && a.dead==b.dead &&
              !std::memcmp(&a.velocity,&b.velocity,sizeof(Vec3)) &&
              a.health==b.health && a.shield==b.shield && a.source_unit==b.source_unit &&
              a.kills==b.kills && a.deaths==b.deaths,
              "External force LAN changed a source player pose, vitality, generation or attribution");}
        check(state.corpses.size()==client->state().corpses.size(),"External force LAN lost an actual corpse");
        for(unsigned i=0;i<state.corpses.size();++i){const auto &a=state.corpses[i],&b=client->state().corpses[i];
          check(a.object==b.object && a.player==b.player && a.definition==b.definition && a.death_tick==b.death_tick &&
              a.physical_state==b.physical_state && a.health==b.health && a.shield==b.shield &&
              !std::memcmp(&a.velocity,&b.velocity,sizeof(Vec3)),
              "External force LAN changed an original corpse generation, motion, vitality or death clock");}
        for(const auto &pose:state.source_body_poses) {
          const auto remote=std::ranges::find_if(client->state().source_body_poses,
              [&](const auto &candidate){return candidate.object==pose.object;});
          check(remote!=client->state().source_body_poses.end() && pose.nodes.size()==remote->nodes.size() &&
              (pose.nodes.empty() || !std::memcmp(pose.nodes.data(),remote->nodes.data(),pose.nodes.size()*sizeof(Transform))),
              "External force LAN changed an actual solved body/corpse WORLD pose");
          exact_pose_nodes+=pose.nodes.size();
          if(!pose.nodes.empty() && std::ranges::any_of(state.corpses,
              [&](const auto &corpse){return corpse.object==pose.object;}))++exact_corpse_poses;
        }
        ++exact_snapshots;
      }
    };
    for(unsigned i=0;i<32 && match.player(slots[1]).source_current_object==null_datum;++i)step();
    std::vector<FirstPersonBindingDefinition> bindings;
    bindings.push_back(world.cache->load_first_person_binding(match.unit(slots[0]).weapon.definition,2));
    if(!lan && representation==3)
      bindings.push_back(world.cache->load_first_person_binding(match.unit(slots[1]).weapon.definition,3));
    match.configure_local_views(world.local_slots,bindings);
    std::array<std::uint32_t,3> units;
    std::array<std::uint32_t,3> definitions;
    for(unsigned i=0;i<slots.size();++i){units[i]=match.player(slots[i]).source_current_object;
      check(units[i]!=null_datum,"External force fixture lost a genuinely born biped");
      definitions[i]=source.object_definition(units[i]);
      check(definitions[i]==multiplayer_biped(world.content,i==1?representation:2).datum,
          "External force participant did not receive its actual selected Spartan/Elite biped");
      commands[slots[i]].yaw=match.unit(slots[i]).yaw;commands[slots[i]].pitch=match.unit(slots[i]).pitch;}
    step(); // Establish the source look adapter before requesting a turn.
    if(plasma) {
      check(match.state().players[slots[0]].grenades[1]>0,
          "Original plasma weapon-set option did not grant its authored starting grenades");
      if(source.current_grenade(units[0])!=1) {
        commands[slots[0]].switch_grenade=true;step();commands[slots[0]].switch_grenade=false;step();
      }
      check(source.current_grenade(units[0])==1,"Ordinary grenade selection did not select actual plasma inventory");
      report.data["original_selected_grenade"]=source.current_grenade(units[0]);
    }
    report.data["source_units"]=units;
    report.data["source_definitions"]=definitions;
    report.data["phase"]="declared scene setup";report.save();
    std::uint32_t driven=null_datum;Vec3 driving_direction{1,0,0},blast_side{0,1,0},vehicle_aim{};
    if(vehicle) {
      for(unsigned tick=0;tick<=90 && driven==null_datum;++tick) {
        // WORLD presentation lists contain registered scene placements. The
        // actual source factory can also own dynamic/native-only vehicles.
        // Select the genuine complete object generation from its owner pool.
        for(const auto &candidate:source.live_objects()){
          if(candidate.type!=1 || candidate.parent!=null_datum)continue;
          const auto tag=world.cache->query_tag({.handle=TagHandle{candidate.definition}});
          const auto suffix=vehicle_kind=="warthog_gauss"?"warthog":vehicle_kind;
          if(tag && tag->name.ends_with("\\"+suffix)) {
            if(vehicle_kind=="warthog_gauss") {
              const auto model=world.cache->load_initial_model_definition(candidate.definition);
              const auto selected=source.object_model_regions(candidate.object).selection.variant_index;
              if(selected<0 || std::size_t(selected)>=model.variants.size() ||
                  world.cache->string_id(model.variants[selected].name)!="gauss")continue;
            }
            driven=candidate.object;break;
          }
        }
        if(driven==null_datum && tick<90)step();
      }
      if(driven==null_datum) {
        for(const auto &candidate:source.live_objects())if(candidate.type==1) {
          const auto tag=world.cache->query_tag({.handle=TagHandle{candidate.definition}});
          report.data["available_native_vehicles"].push_back({{"object",candidate.object},
              {"parent",candidate.parent},{"definition",candidate.definition},{"name",tag?tag->name:""}});
        }
      }
      check(driven!=null_datum,"External player-impact fixture has no original-born requested vehicle");
      const auto vehicle_definition=source.object_definition(driven);
      const auto vehicle_tag=world.cache->query_tag({.handle=TagHandle{vehicle_definition}});
      const auto model=world.cache->load_initial_model_definition(vehicle_definition);
      const auto selected=source.object_model_regions(driven).selection.variant_index;
      const bool selected_variant=selected>=0 && std::size_t(selected)<model.variants.size();
      check(vehicle_tag && (selected_variant || (selected==-1 && model.variants.empty())),
          "Native vehicle has no actual authored model variant");
      report.data["vehicle_identity"]={{"object",driven},{"definition",vehicle_definition},
          {"path",vehicle_tag->name},{"variant_index",selected},
          {"variant_name",selected_variant?Json(world.cache->string_id(model.variants[selected].name)):Json(nullptr)}};
      const auto frame=source.object_transform(driven);
      const auto &definition=world.content.unit_interactions->units.at(source.object_definition(driven));
      check(!definition.seats.empty(),"Warthog has no authored driver seat");const auto &seat=definition.seats.front();
      const auto markers=definition.markers.find(seat.entry_marker);
      check(markers!=definition.markers.end(),"Warthog has no authored driver entry marker");
      const auto posed=pose_model_markers(markers->second,pose_model_nodes_world(definition.hierarchy,frame),definition.permutations,{},1);
      check(!posed.empty(),"Warthog driver marker produced no posed approach");
      const CollisionWorld terrain(world.content.collision);
      auto approach=posed.front().world.position+posed.front().world.basis[0]*(seat.radius*.5f);
      const auto floor=terrain.raycast(approach+Vec3{0,0,world.content.player.radius},{0,0,-1},4);
      check(bool(floor),"Warthog driver approach lacks real BSP support");approach.z=floor->position.z+world.content.player.radius;
      native.set_object_position(units[0],approach);
      commands[slots[0]].vehicle_interaction_target=SourcePlayerActionTarget{5,0,driven};
      commands[slots[0]].pickup=true;commands[slots[0]].pickup_milliseconds=900;
      unsigned entry_frames{};
      for(;entry_frames<150 && source.biped_motion(units[0]).parent!=driven;++entry_frames) {
        const auto live_markers=pose_model_markers(markers->second,source.world_nodes(driven),definition.permutations,{},1);
        check(!live_markers.empty(),"Actual vehicle entry WORLD marker disappeared");
        const auto delta=live_markers.front().world.position-eye(source,units[0]);
        const auto state=match.state();
        commands[slots[0]].yaw+=std::remainder(std::atan2(delta.y,delta.x)-state.players[slots[0]].yaw,6.283185307179586f);
        commands[slots[0]].pitch+=std::atan2(delta.z,std::hypot(delta.x,delta.y))-state.players[slots[0]].pitch;
        commands[slots[0]].forward=length(delta)>seat.radius*.75f && std::hypot(delta.x,delta.y)>.2f?1.f:0.f;
        const auto selected=source.vehicle_seat_target(units[0],driven);
        commands[slots[0]].vehicle_interaction_target=selected.kind?
            SourcePlayerActionTarget{std::int16_t(selected.kind+3),selected.seat,selected.object}:SourcePlayerActionTarget{};
        if(entry_frames%10==0)report.data["entry_queries"].push_back({{"frame",entry_frames},
            {"kind",selected.kind},{"seat",selected.seat},{"radius",seat.radius},{"distance",length(delta)},
            {"player",vector(state.players[slots[0]].position)},{"marker",vector(live_markers.front().world.position)}});
        step();
      }
      report.data["ordinary_entry_frames"]=entry_frames;report.save();
      check(source.biped_motion(units[0]).parent==driven,"Ordinary native vehicle entry failed before player-impact stimulus");
      commands[slots[0]].forward=0;
      commands[slots[0]].pickup=false;commands[slots[0]].pickup_milliseconds=0;
      commands[slots[0]].vehicle_interaction_target.reset();
      const auto root=source.object_transform(driven);
      driving_direction=normalized(Vec3{root.basis[0].x,root.basis[0].y,0});
      const Vec3 side{-driving_direction.y,driving_direction.x,0};
      vehicle_aim=root.position+driving_direction*(name=="vehicle-splatter"?10.f:6.f);
      if(vehicle_kind=="banshee" && name=="vehicle-neutral")
        vehicle_aim=root.position-driving_direction*6.f; // Neutral aircraft still coasts under original physics.
      auto target=vehicle_aim+side*(name=="vehicle-miss"?4.f:name=="vehicle-splatter"?-.75f:0.f);
      const auto support=terrain.raycast(target+Vec3{0,0,vehicle_kind=="banshee" && !negative?world.content.player.height:4.f},{0,0,-1},8);
      check(bool(support),"Vehicle target initial placement lacks actual BSP support");
      target.z=support->position.z+.05f;native.set_object_position(units[1],target);
      if(vehicle_kind=="banshee")vehicle_aim.z=target.z+world.content.player.height*.5f;
      auto far=root.position-driving_direction*6.f+side*4.f;
      const auto far_floor=terrain.raycast(far+Vec3{0,0,4},{0,0,-1},8);
      check(bool(far_floor),"Vehicle distant player control lacks actual BSP support");
      far.z=far_floor->position.z+.05f;native.set_object_position(units[2],far);
      commands[slots[0]].yaw=std::atan2(driving_direction.y,driving_direction.x);
      commands[slots[0]].pitch=std::atan2(root.basis[0].z,std::hypot(root.basis[0].x,root.basis[0].y));
      report.data["driven_vehicle"]=driven;
      report.data["declared_vehicle_lane"]={{"root",vector(root.position)},
          {"forward",vector(driving_direction)},{"target",vector(target)},{"far",vector(far)}};
    } else {
      auto start=clear_floor(*world.cache,world.content);
      std::optional<Vec3> wall_target;
      if(blocked) {
        const auto wall=wall_floor(*world.cache,world.content,native);start={wall[0],wall[2]};wall_target=wall[1];
        report.data["declared_wall"]={{"shooter",vector(wall[0])},{"target",vector(wall[1])}};
      }
      const auto floor=start.position;driving_direction=start.direction;
      blast_side={-start.direction.y,start.direction.x,0};
      native.set_object_position(units[0],floor);
      native.set_object_position(units[1],wall_target.value_or(floor+start.direction*
          (grenade?(name=="frag-far" || name=="plasma-far"?4.f:
              name=="frag-middle" || plasma?1.5f:.7f):3.f)));
      native.set_object_position(units[2],floor+Vec3{0,2.f,0});
      // The distant negative control is outside both grenade and rocket
      // radius; select support from the actual BSP rather than hold it up.
      const CollisionWorld terrain(world.content.collision);
      bool distant=false;
      for(const auto &spawn:world.cache->spawns())if(length(spawn.position-floor)>10.f){
        const auto support=terrain.raycast(spawn.position+Vec3{0,0,1},{0,0,-1},3);
        if(support){native.set_object_position(units[2],support->position+Vec3{0,0,.05f});distant=true;break;}}
      check(distant,"Blast fixture has no distant actual BSP-supported player control");
      commands[slots[0]].yaw=std::atan2(start.direction.y,start.direction.x);
      commands[slots[0]].pitch=grenade?-1.5f:0.f;
    }
    for(unsigned i=0;i<60;++i) {
      if(vehicle) {
        const auto state=match.state();const auto root=source.object_transform(driven);
        const auto yaw=std::atan2(driving_direction.y,driving_direction.x);
        const auto pitch=std::atan2(root.basis[0].z,std::hypot(root.basis[0].x,root.basis[0].y));
        // Controls carries a raw look accumulator. Driver entry reorients the
        // original retained input independently of that accumulator. Supply
        // ordinary look deltas toward the actual published source heading.
        commands[slots[0]].yaw+=std::remainder(yaw-state.players[slots[0]].yaw,6.283185307179586f);
        commands[slots[0]].pitch+=pitch-state.players[slots[0]].pitch;
      } else {
        const auto state=match.state();
        const auto yaw=std::atan2(driving_direction.y,driving_direction.x);
        const auto pitch=grenade?-1.5f:0.f;
        commands[slots[0]].yaw+=std::remainder(yaw-state.players[slots[0]].yaw,6.283185307179586f);
        commands[slots[0]].pitch+=pitch-state.players[slots[0]].pitch;
      }
      step();
    }
    if(std::getenv("HALO2_PLAYER_FORCE_REFERENCE_INPUT_PREPARE")) {
      // Populate the original cached language through its real string query.
      // The native interface supplies English at its platform boundary; the
      // independent game replay must not attempt Xbox NVRAM discovery later.
      const auto strings=std::ranges::find_if(world.cache->tags(),[](const auto &tag){
        return tag.group=="unic" && tag.address && tag.size>=0x30u;
      });
      check(strings!=world.cache->tags().end(),"Reference input preparation lacks an actual string list");
      auto locale=native.prepare_source_call(std::array{0x6005bc00u});locale.gp[0]=strings->handle().value;
      native.run_actual(0x1a9d90u,locale);
      report.data["reference_locale_query"]={{"entry",0x1a9d90u},{"tag",strings->handle().value},
          {"name",strings->name},{"cached_language",Bytes(source.memory().view(0x48c750u,4)).u32(0)},
          {"supplied_gameplay_or_physics_results",false}};
      if(plasma && blocked) {
        // A genuine first wall detonation loads the effect's cold resources.
        // The second ordinary throw is the measured original-call comparison.
        // Keep its real aftermath; never inject an I/O return or restore banks.
        const auto origin=source.biped_motion(units[0]).position;
        for(unsigned tick=0;tick<180;++tick){
          commands[slots[0]].grenade=tick==0;
          commands[slots[0]].forward=tick>=8 && tick<64?-1.f:0.f;step();
        }
        commands[slots[0]].grenade=false;commands[slots[0]].forward=0;
        check(!match.state().players[slots[0]].dead,"Reference resource warm-up shooter did not survive ordinary retreat");
        check(!match.state().players[slots[1]].dead,"Reference wall warm-up killed the blocked target");
        // Establish the measured scene again through original B95D0, then
        // let original contact/friction settle; no velocity/vitality is set.
        native.set_object_position(units[0],origin);
        for(unsigned tick=0;tick<60;++tick)step();
        report.data["reference_resource_warmup"]={{"ordinary_plasma_wall_throws",1},{"ticks",180},
            {"ordinary_retreat_ticks",56},{"scene_reset_owner",0xb95d0u},{"settling_ticks",60},
            {"supplied_gameplay_or_physics_results",false}};
      }
      // Original D54D0 is a palette query, not a damage/physics producer.
      // Prepare the actual authored frames through that complete owner so
      // offline replay has immutable CPU bitmap inputs in its BEFORE image.
      // No native after-state or allocation/admission return is supplied.
      const auto word=[&](std::uint32_t p){return Bytes(source.memory().view(p,4)).u32(0);};
      const auto globals=word(0x4ecc14u);
      check(globals && word(globals+0x80u)>0,"Reference palette preparation has no actual rasterizer globals");
      const auto bitmap=word(word(globals+0x84u)+0x1cu);
      const auto tag=world.cache->query_tag({.handle=TagHandle{bitmap},.group=tag_groups::bitmap});
      check(bool(tag),"Reference palette preparation lost its actual typed bitmap");
      const auto count=world.cache->bitmap_count(bitmap);
      check(count>0 && count<=32768,"Reference palette frames escape the original signed selector");
      for(unsigned frame=0;frame<count;++frame) {
        auto input=native.prepare_source_call(std::array{0u,frame,0u,
            std::bit_cast<std::uint32_t>(.5f),std::bit_cast<std::uint32_t>(.5f)});
        input.gp[3]=0x6005ba00u;native.run_actual(0xd54d0u,input);
      }
      report.data["reference_input_preparation"]={{"original_owner",0xd54d0u},{"bitmap",bitmap},
          {"name",tag->name},{"authored_frames",count},{"coordinates",{.5f,.5f}},
          {"supplied_gameplay_or_physics_results",false}};
    }
    const auto initial=source.biped_motion(units[1]);
    const auto vitality=source.object_vitality(units[1]);
    const auto far_vitality=source.object_vitality(units[2]);
    report.data["initial_target"]={{"position",vector(initial.position)},{"velocity",vector(initial.velocity)},
        {"physical_state",initial.physical_state},{"body",vitality.body},{"shield",vitality.shield}};
    report.data["phase"]="ordinary stimulus";report.save();
    {const auto shooter=match.state().players[slots[0]];
      report.data["initial_shooter"]={{"position",vector(shooter.position)},
          {"dead",shooter.dead},{"grenades",shooter.grenades},{"yaw",shooter.yaw},{"pitch",shooter.pitch}};
      report.save();check(!shooter.dead,"Shooter died during declared scene settling");}
    check(!match.state().players[slots[1]].dead,"Target died before the external-force stimulus");
    unsigned target_impulses{},blast_calls{},detonations{},shots{};
    float maximum_impulse{},maximum_speed{},maximum_travel{},minimum_body=vitality.body,minimum_shield=vitality.shield;
    float far_minimum_body=far_vitality.body,far_minimum_shield=far_vitality.shield,vehicle_travel{};
    bool target_dead=false,corpse_seen=false,damaged=false,respawn_seen=false;
    std::optional<test::LivePhysicsEntryCaptureTu15> capture;
    std::optional<SourcePhysicsEntryObserverBinding> capture_observation;
    std::filesystem::path capture_directory;
    std::unique_ptr<SourceExecutionInputs> captured_inputs;
    std::optional<SourceExecutionInputBinding> input_binding;
    unsigned input_depth{};bool inputs_complete=false;
    Json read_aliases=Json::array();
    Json sound_timing_views=Json::array();
    std::vector<std::pair<std::uint64_t,std::uint64_t>> captured_regions;
    std::optional<SourceMemoryReadAliasObserverBindingTu15> read_alias_observation;
    std::optional<SourceSoundTimingInputObserverBindingTu15> sound_input_observation;
    if(std::getenv("HALO2_PLAYER_FORCE_CAPTURE_ROOT"))
      sound_input_observation.emplace(source.memory(),[&](const auto &entry,const auto &original,const auto &binding){
        if(!input_depth || inputs_complete)return;
        const auto hex=[](const auto &bytes){std::string result;constexpr char digits[]="0123456789abcdef";
          for(auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);result+=digits[value>>4];result+=digits[value&15];}return result;};
        sound_timing_views.push_back({{"entry",entry.pc},{"return_address",entry.return_address},
            {"gp",{entry.eax,entry.ecx,entry.edx,entry.ebx,entry.esp,0u,entry.esi,entry.edi}},
            {"stack",entry.stack},{"sound",binding.sound.value},{"gestalt",binding.gestalt.value},
            {"root",binding.native_root},{"range",binding.native_range},{"permutation",binding.native_permutation},
            {"path",binding.path},{"range_name",binding.range_name},{"permutation_name",binding.permutation_name},
            {"named_key",hex(binding.named_key)},{"encoded_payload",hex(binding.encoded_payload)},
            {"original_operands",{original.sample_rate,original.encoding,original.compression,original.encoded_bytes}},
            {"canonical_codec",hex(source.memory().view(binding.native_root+3u,3))}});
      });
    if(std::getenv("HALO2_PLAYER_FORCE_CAPTURE_ROOT"))
      read_alias_observation.emplace(source.memory(),[&](const SourceMemoryReadAliasTu15 &alias){
        if(input_depth && !inputs_complete)read_aliases.push_back({{"entry",alias.entry},{"caller",alias.caller},
            {"address",alias.address},{"canonical_value",alias.canonical_value},{"read_value",alias.read_value},
            {"tag",alias.tag},{"selector",alias.selector}});
      });
    std::uint32_t capture_entry=0xd91d0u;
    std::ofstream store_trace;
    unsigned trace_depth{};bool trace_started=false,trace_complete=false;
    if(const auto path=std::getenv("HALO2_PLAYER_FORCE_STORE_TRACE"))store_trace.open(path,std::ios::binary);
    if(const auto root=std::getenv("HALO2_PLAYER_FORCE_CAPTURE_ROOT")) {
      const auto entry_text=std::getenv("HALO2_PLAYER_FORCE_CAPTURE_ENTRY");
      if(entry_text)capture_entry=std::stoul(entry_text,nullptr,0);
      const auto path=std::filesystem::path(root)/scenario;
      check(path.is_absolute(),"Actual force capture requires an absolute owned output path");
      capture.emplace(path);capture_directory=path;report.data["original_call_capture"]=path.string();
    }
    float closest_blast=std::numeric_limits<float>::infinity();
    capture_observation.emplace(source.memory(),[&](bool before,std::uint32_t called,const auto &abi){
      if(called==0xd91d0u && store_trace && !trace_complete) {
        if(before){trace_started=true;++trace_depth;}
        else if(trace_depth && --trace_depth==0){trace_complete=true;store_trace.flush();}
      }
      if(before && called==0xd91d0u) {
        const auto request=Bytes(source.memory().view(abi.gp[4]+4u,4)).u32(0);
        const Bytes point(source.memory().view(request+0x24u,12));
        const Vec3 position{point.f32(0),point.f32(4),point.f32(8)};
        closest_blast=std::min(closest_blast,length(position-initial.position));
        report.data["original_area_requests"].push_back({{"request",request},{"position",vector(position)},
            {"initial_target_distance",length(position-initial.position)}});
      }
      const bool contact_ready=!std::getenv("HALO2_PLAYER_FORCE_CAPTURE_AFTER_CONTACT") || maximum_travel>.2f;
      if(capture && called==capture_entry && contact_ready && (!std::getenv("HALO2_PLAYER_FORCE_CAPTURE_AFTER_DAMAGE") || damaged)) {
        if(before && !inputs_complete && input_depth++==0) {
          source.memory().visit_committed([&](auto address,auto bytes){
            captured_regions.emplace_back(address,std::uint64_t(address)+bytes.size());
          });
          captured_inputs=std::make_unique<SourceExecutionInputs>();
          input_binding.emplace(source.memory(),*captured_inputs);
        }
        capture->observe(before,abi,source.memory(),called);
        if(!before && !inputs_complete && input_depth && --input_depth==0) {
          input_binding.reset();Json inputs=Json::array();
          for(const auto &input:captured_inputs->finish())inputs.push_back({{"kind",unsigned(input.kind)},
              {"operand",input.operand},{"value",input.value}});
          Json new_regions=Json::array();
          source.memory().visit_committed([&](auto address,auto bytes){
            auto cursor=std::uint64_t(address);const auto end=cursor+bytes.size();
            for(const auto &[first,last]:captured_regions) {
              if(last<=cursor || first>=end)continue;
              if(cursor<first)new_regions.push_back({{"address",cursor},{"size",first-cursor}});
              cursor=std::max(cursor,last);if(cursor>=end)break;
            }
            if(cursor<end)new_regions.push_back({{"address",cursor},{"size",end-cursor}});
          });
          std::ofstream out(capture_directory/"external-inputs.json");
          out<<Json{{"actual_external_inputs",inputs},{"memory_read_aliases",read_aliases},
              {"newly_committed_regions",new_regions},
              {"sound_timing_input_views",sound_timing_views},
              {"supplied_gameplay_results",false}}.dump(2)<<'\n';
          check(bool(out),"Cannot export the actual call's external platform inputs");inputs_complete=true;
        }
      }
    });
    std::optional<unsigned> first_damage,death_tick;
    const auto vehicle_start=vehicle?source.object_transform(driven).position:Vec3{};
    bool vehicle_aim_complete=false;
    source.observe_body_impulses([&](const auto &event){if(event.object!=units[1])return;
      ++target_impulses;const Bytes bytes(event.impulse);
      const Vec3 impulse{bytes.f32(0),bytes.f32(4),bytes.f32(8)};
      maximum_impulse=std::max(maximum_impulse,length(impulse));});
    source.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
      if(trace_started && trace_depth && !trace_complete) {
        const std::array<std::uint32_t,3> header{store.instruction,store.address,std::uint32_t(store.written.size())};
        store_trace.write(reinterpret_cast<const char *>(header.data()),sizeof(header));
        store_trace.write(reinterpret_cast<const char *>(store.before.data()),store.before.size());
        store_trace.write(reinterpret_cast<const char *>(store.written.data()),store.written.size());
      }
      // D91FF is the original D91D0 radius owner's first damage-definition
      // store, reached by projectile damage as well as effect-part damage.
      // The narrower17D8DA effect observer omits ordinary projectile callers.
      if(store.instruction==0xd91ffu){
        check(store.written.size()==4,"Original radius definition store width changed");++blast_calls;
        const auto definition=Bytes(store.written).u32(0);
        const Bytes authored(source.memory().view(definition,0x6c));
        report.data["original_radius_operands"]={{"definition",definition},{"+04",authored.f32(4)},
            {"+58",authored.f32(0x58)},{"+68",authored.f32(0x68)}};
      }
    });
    Observations observation{source};
    std::uint64_t detonation_frontier=0;
    const auto old_shot=match.state().players[slots[0]].shot_tick;
    const bool fatal=name=="rocket-direct" || name=="vehicle-splatter";
    const unsigned total=fatal?450:180;
    for(unsigned tick=0;tick<total;++tick) {
      if(vehicle) {
        commands[slots[0]].forward=name=="vehicle-neutral" || tick>=180?0.f:1.f;
        const auto root=source.object_transform(driven);
        const auto delta=vehicle_aim-root.position;
        if(dot(delta,driving_direction)<2.f || damaged)vehicle_aim_complete=true;
        if(!vehicle_aim_complete) {
          const auto yaw=std::atan2(delta.y,delta.x);
          commands[slots[0]].yaw+=std::remainder(yaw-match.state().players[slots[0]].yaw,6.283185307179586f);
          if(vehicle_kind=="banshee")commands[slots[0]].pitch+=
              std::atan2(delta.z,std::hypot(delta.x,delta.y))-match.state().players[slots[0]].pitch;
        }
      }
      else if(grenade)commands[slots[0]].grenade=tick==0;
      else if(tick<45 && !match.state().players[slots[0]].dead) {
        const auto target=initial.position+blast_side*(name=="rocket-far"?4.f:name=="rocket-near"?1.25f:0.f)+
            Vec3{0,0,name=="rocket-direct"?.35f:.01f};
        const auto delta=target-eye(source,units[0]);const auto aiming=match.state();
        const auto desired_yaw=std::atan2(delta.y,delta.x);
        const auto desired_pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
        // A source frame can initialize look while a remote player joins.
        // Raw Controls yaw is an accumulator, so use ordinary deltas from
        // the actual source aim rather than assume an absolute baseline.
        commands[slots[0]].yaw+=std::remainder(desired_yaw-aiming.players[slots[0]].yaw,6.283185307179586f);
        commands[slots[0]].pitch+=desired_pitch-aiming.players[slots[0]].pitch;
        commands[slots[0]].fire=tick==30;
        if(tick%10==0) {
          const auto state=match.state();
          report.data["rocket_aim"].push_back({{"relative_tick",tick},
              {"desired_target",vector(target)},{"actual_source_eye",vector(eye(source,units[0]))},
              {"desired_yaw",desired_yaw},{"desired_pitch",desired_pitch},
              {"input_yaw",commands[slots[0]].yaw},{"input_pitch",commands[slots[0]].pitch},
              {"source_yaw",state.players[slots[0]].yaw},{"source_pitch",state.players[slots[0]].pitch}});
        }
      }
      step();const auto state=match.state();
      if(blocked && tick%10==0) {
        const auto &shooter=state.players[slots[0]];
        report.data["wall_stimulus"].push_back({{"tick",tick},{"position",vector(shooter.position)},
            {"dead",shooter.dead},{"grenades",shooter.grenades},{"yaw",shooter.yaw},{"pitch",shooter.pitch},
            {"projectiles",source.grenade_projectiles().size()}});
      }
      if(state.players[slots[0]].shot_tick!=old_shot)shots=1;
      for(const auto &event:state.grenade_detonations)if(event.serial>detonation_frontier){
        detonation_frontier=event.serial;++detonations;
        report.data["detonations"].push_back({{"tick",event.tick},{"position",vector(event.position)},
            {"target_distance",length(event.position-initial.position)}});}
      const auto live=source.live_objects();
      if(std::ranges::any_of(live,[&](const auto &o){return o.object==units[1];})) {
        const auto motion=source.biped_motion(units[1]);const auto value=source.object_vitality(units[1]);
        minimum_body=std::min(minimum_body,value.body);minimum_shield=std::min(minimum_shield,value.shield);
        maximum_speed=std::max(maximum_speed,length(motion.velocity));
        maximum_travel=std::max(maximum_travel,length(motion.position-initial.position));
        const bool hit=value.body<vitality.body || value.shield<vitality.shield;
        damaged|=hit;if(hit && !first_damage)first_damage=tick;
        if(tick%10==0 || (first_damage && tick<*first_damage+10)) {
          Json sample{{"tick",state.tick},{"position",vector(motion.position)},
              {"velocity",vector(motion.velocity)},{"body",value.body},{"shield",value.shield},
              {"dead",state.players[slots[1]].dead}};
          if(vehicle)sample.update({{"vehicle_position",vector(source.object_transform(driven).position)},
              {"vehicle_velocity",vector(source.object_linear_velocity(driven))},
              {"driver_parent",state.players[slots[0]].source_parent},
              {"driver_yaw",state.players[slots[0]].yaw}});
          report.data["samples"].push_back(std::move(sample));
        }
      }
      const auto &p=state.players[slots[1]];
      if(p.dead && !target_dead){target_dead=true;death_tick=tick;}
      if(target_dead && !p.dead && p.source_unit!=null_datum && p.source_unit!=units[1])respawn_seen=true;
      for(const auto &corpse:state.corpses)if(corpse.object==units[1])corpse_seen=true;
      const auto far=source.object_vitality(units[2]);far_minimum_body=std::min(far_minimum_body,far.body);
      far_minimum_shield=std::min(far_minimum_shield,far.shield);
      if(vehicle)vehicle_travel=std::max(vehicle_travel,length(source.object_transform(driven).position-vehicle_start));
    }
    report.data.update({{"target_damaged",damaged},{"target_dead",target_dead},{"source_corpse_seen",corpse_seen},
        {"target_minimum_body",minimum_body},{"target_minimum_shield",minimum_shield},
        {"target_point_impulses",target_impulses},{"maximum_target_impulse",maximum_impulse},
        {"maximum_target_speed",maximum_speed},{"maximum_target_travel",maximum_travel},
        {"original_radius_damage_calls",blast_calls},{"projectile_effect_requests",detonations},{"ordinary_weapon_shots",shots},
        {"vehicle_travel",vehicle_travel},{"exact_lan_snapshots",exact_snapshots},
        {"exact_lan_world_nodes",exact_pose_nodes},{"exact_lan_corpse_poses",exact_corpse_poses},
        {"source_salted_respawn_seen",respawn_seen},
        {"source_shooter_kills",match.player(slots[0]).kills},{"source_target_deaths",match.player(slots[1]).deaths},
        {"far_control_undamaged",far_minimum_body==far_vitality.body && far_minimum_shield==far_vitality.shield}});
    if(first_damage)report.data["first_damage_tick"]=*first_damage;
    if(death_tick)report.data["death_tick"]=*death_tick;
    report.save();
    check(far_minimum_body==far_vitality.body && far_minimum_shield==far_vitality.shield,
        "External-force stimulus damaged the distant player control");
    if(vehicle && name!="vehicle-neutral")check(vehicle_travel>5.f,"Vehicle never traversed the declared impact approach");
    if(!vehicle)check(detonations>0 && blast_calls>0,"Ordinary input produced no actual original explosion damage request");
    if(blocked)check(closest_blast<2.f,
        "Wall-obstruction control never received a nearby actual source blast");
    if(negative)check(!damaged && !target_dead,"Missed/distant external-force negative control damaged its target");
    else {
      // At ordinary tank/hover speed, original contact can push a biped
      // without crossing the authored collision-damage threshold. Require
      // real solved motion, then qualify that whole frame against TU1.5.
      if(!(vehicle && (vehicle_kind=="scorpion" || vehicle_kind=="wraith" || vehicle_kind=="banshee")))
        check(damaged,"Actual external-force stimulus never damaged its target player");
      check(maximum_speed>.2f && maximum_travel>.01f,
          "External player damage produced no measurable original physical consequence");
      if(name=="frag-near" || name=="rocket-near" || (name=="vehicle-hit" && vehicle_kind=="warthog"))
        check(!target_dead,"Living-player knockback fixture unexpectedly became a fatal contact");
      if(name=="rocket-direct" || name=="vehicle-splatter") {
        check(target_dead && corpse_seen,"Fatal external player impact did not publish its original death/corpse transition");
        check(match.player(slots[0]).kills==1 && match.player(slots[1]).deaths==1,
            "Fatal external player impact lost or duplicated its original kill attribution");
        check(first_damage && death_tick && first_damage==death_tick,
            "Fatal contact fixture died later instead of from its first actual impact");
        check(respawn_seen,"Fatal external player impact never completed its original salted respawn");
      }
    }
    if(lan) {
      check(exact_pose_nodes>0,"External-force LAN pose comparisons were vacuous");
      if(fatal)check(exact_corpse_poses>0,"External-force LAN never delivered actual solved corpse nodes");
    }
    report.data["phase"]="complete";report.data["passed"]=true;report.save();std::cout<<report.data.dump()<<'\n';return 0;
  } catch(const std::exception &error) {
    report.data["error"]=error.what();report.save();std::cerr<<error.what()<<'\n';return 1;
  }
}
