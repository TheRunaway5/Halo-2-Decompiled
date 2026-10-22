#include "halo2/multiplayer_world.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/math.hpp"
#include "halo2/source_bsp_breakable_visibility_tu15.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "helpers/gameplay_fixture.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Report {
  std::filesystem::path path;
  Json data{{"passed",false},{"runtime_cache_source","MCC"},
      {"damage_or_destruction_results_supplied",false},{"scene_placement_is_declared_test_input",true},
      {"render_audio_or_full_goal_acceptance",false},{"samples",Json::array()},{"detonations",Json::array()}};
  void save()const{if(!path.empty()){std::ofstream output(path);output<<data.dump(2)<<'\n';}}
  ~Report(){save();}
};
struct StoreObservation {
  MultiplayerObjectRuntime &source;
  ~StoreObservation(){source.observe_physics_stores({});}
};
void reciprocal_damage(MultiplayerPreparedWorld &world,const Cache &cache,const LevelContent &content,
    unsigned slot,std::array<Controls,multiplayer_capacity> &commands,bool coil_first,Report &report){
  auto &source=*world.source_objects;auto &native=source.tu15_world();
  std::vector<std::uint32_t> vehicles,coils;
  for(const auto &object:source.live_objects()){
    if(object.parent!=null_datum)continue;
    const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});if(!tag)continue;
    if(object.type==1 && tag->name.ends_with("\\warthog"))vehicles.push_back(object.object);
    if(object.type==11 && tag->name.ends_with("\\fusion_coil"))coils.push_back(object.object);
  }
  check(!vehicles.empty() && coils.size()>1,"Reciprocal scene requires actual authored Warthog and coil births");
  const auto vehicle=vehicles.front(),coil=coils.front();
  const auto vehicle_root=source.object_transform(vehicle),coil_root=source.object_transform(coil);
  const auto vehicle_center=source.object_sphere(vehicle).center;
  // Declared scene input: preserve the coil's actual root-to-floor height.
  // The position opposite the shooter separates rocket contact from the witness.
  // Original Headlong rocket jpt radius words are1/2; large vehicle1/3.
  // Actual contact/area admission and damage remain entirely source-owned.
  CollisionWorld terrain(content.collision);
  const auto original_floor=terrain.raycast(coil_root.position+Vec3{0,0,10},{0,0,-1},20);
  auto coil_position=vehicle_center+vehicle_root.basis[0]*2.8f;
  const auto floor=terrain.raycast(coil_position+Vec3{0,0,10},{0,0,-1},20);
  check(bool(original_floor) && bool(floor),"Reciprocal scene has no real terrain-supported coil position");
  coil_position.z=floor->position.z+coil_root.position.z-original_floor->position.z;
  native.set_object_position(coil,coil_position);
  const auto target=coil_first?coil:vehicle,witness=coil_first?vehicle:coil;
  const auto &controls=coil_first?vehicles:coils;
  std::uint32_t far_control=null_datum;float farthest=0;
  for(auto object:controls)if(object!=target && object!=witness){
    const auto distance=length(source.object_sphere(object).center-vehicle_center);
    if(distance>farthest){far_control=object;farthest=distance;}
  }
  check(far_control!=null_datum && farthest>10,"Reciprocal scene lacks an authored distant negative control");
  Spawn spawn;bool supported=false;
  const auto center=source.object_sphere(target).center;
  const auto away=normalized(coil_first?center-vehicle_center:vehicle_root.basis[0]*-1.f);
  for(unsigned side=0;side<32;++side){
    const float angle=float(side)*.19634954084936207f;
    const Vec3 direction{std::cos(angle),std::sin(angle),0};
    if(dot(direction,away)<.8f)continue;
    auto position=center+direction*(coil_first?6.f:12.f);
    const auto support=terrain.raycast(position+Vec3{0,0,10},{0,0,-1},20);
    if(!support)continue;position.z=support->position.z+content.player.radius;
    const auto eye=position+Vec3{0,0,content.player.camera_height},delta=center-eye;
    if(terrain.raycast(eye,normalized(delta),length(delta)))continue;
    spawn.position=position;supported=true;break;
  }
  check(supported,"Reciprocal scene lacks a supported clear shooter lane");
  const auto unit=world.match.state().players[slot].source_unit;
  test::place_player(world.match,slot,spawn);native.set_object_position(unit,spawn.position);
  const auto &memory=source.memory();
  const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
  const auto aim=[&]{
    constexpr std::uint32_t eye=0x6005b800u,velocity=eye+0x20u;
    auto abi=native.prepare_source_call(std::array{eye,velocity});abi.gp[0]=unit;native.run_actual(0x1582f0u,abi);
    const Vec3 position{std::bit_cast<float>(word(eye)),std::bit_cast<float>(word(eye+4)),std::bit_cast<float>(word(eye+8))};
    const auto delta=source.object_sphere(target).center-position;
    commands[slot].yaw=std::atan2(delta.y,delta.x);commands[slot].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
  };
  for(unsigned tick=0;tick<30;++tick){aim();world.match.step(commands);}
  const auto target_before=source.object_vitality(target),witness_before=source.object_vitality(witness),
      control_before=source.object_vitality(far_control);
  check(target_before.body==1.f && witness_before.body==1.f,"Reciprocal scene setup damaged an interaction participant");
  unsigned requests=0,shots=0;bool pristine_at_first_blast=false,dead=false,witness_retired=false;
  float witness_minimum=witness_before.body;std::optional<unsigned> first_blast,first_witness_damage,death_tick;
  const auto witness_body=source.source_object_body(witness);unsigned observed_tick=0;
  source.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
    if(store.instruction!=0xdc4a0u || Bytes(store.written).u32(0)!=target)return;
    if(!requests++){
      pristine_at_first_blast=std::bit_cast<float>(word(witness_body+0xecu))==witness_before.body &&
          std::bit_cast<float>(word(witness_body+0xf0u))==witness_before.shield;
      first_blast=observed_tick;
    }
  });
  StoreObservation observation{source};
  report.data["reciprocal_scene"]={{"target",target},{"witness",witness},{"far_control",far_control},
      {"far_control_distance",farthest},{"declared_coil_root",{coil_position.x,coil_position.y,coil_position.z}},
      {"declared_shooter",{spawn.position.x,spawn.position.y,spawn.position.z}}};
  const auto exists=[&](auto object){return std::ranges::any_of(source.live_objects(),[&](const auto &row){return row.object==object;});};
  for(unsigned tick=0;tick<900;++tick){
    auto state=world.match.state();check(!state.players[slot].dead && state.players[slot].source_unit==unit,
        "Reciprocal interaction lost its real shooter");
    const bool target_live=exists(target);if(target_live)aim();
    commands[slot].fire=target_live && !dead && (coil_first || tick%30==0);
    commands[slot].reload=target_live && !dead && state.players[slot].loaded==0;
    observed_tick=state.tick+1;
    world.match.step(commands);source.advance_effect_frame(tick_seconds);state=world.match.state();
    if(state.players[slot].shot_tick==state.tick)++shots;
    dead=!exists(target) || source.object_vitality(target).body<=0;
    if(dead && !death_tick)death_tick=tick;
    witness_retired=!exists(witness);
    const auto vitality=witness_retired?MultiplayerSourceVitality{}:source.object_vitality(witness);
    witness_minimum=std::min(witness_minimum,vitality.body);
    if((witness_retired || vitality.body<witness_before.body) && !first_witness_damage)first_witness_damage=state.tick;
    if(tick%30==0 || dead){report.data["samples"].push_back({{"tick",state.tick},{"shots",shots},
        {"target_dead",dead},{"witness_body",vitality.body},{"witness_retired",witness_retired},{"target_blasts",requests}});report.save();}
    if(death_tick && tick>=*death_tick+90)break;
  }
  const auto control_after=source.object_vitality(far_control);
  report.data.update({{"shots",shots},{"initiator_dead",dead},{"original_initiator_blasts",requests},
      {"witness_pristine_at_first_blast",pristine_at_first_blast},{"witness_minimum_body",witness_minimum},
      {"witness_retired",witness_retired}});
  if(first_blast)report.data["first_blast_tick"]=*first_blast;
  if(first_witness_damage)report.data["first_witness_damage_tick"]=*first_witness_damage;
  check(shots && dead && requests,"Ordinary weapon stimulus did not produce its original initiator destruction blast");
  check(pristine_at_first_blast && first_blast && first_witness_damage && *first_witness_damage>=*first_blast,
      "Reciprocal witness was directly damaged before the initiator's original blast, or received no blast damage");
  check(witness_retired || witness_minimum<witness_before.body,"Original blast did not damage the other authored object class");
  check(control_after.body==control_before.body && control_after.shield==control_before.shield,
      "Distant reciprocal negative control changed vitality");
  report.data["far_control_unchanged"]=true;report.data["passed"]=true;
  std::cout<<"PASS reciprocal MCC "<<(coil_first?"coil-to-vehicle":"vehicle-to-coil")<<" damage through ordinary weapon stimulus\n";
}
void terrain_breakage(MultiplayerPreparedWorld &world,const Cache &cache,
    const LevelContent &content,unsigned slot,std::array<Controls,multiplayer_capacity> &commands,Report &report) {
  auto &source=*world.source_objects;auto &native=source.tu15_world();
  const auto metadata=cache.load_source_bsp_metadata(0);
  const Bytes bsp(metadata.bytes_at(metadata.source_address(),metadata.size()));
  const auto read=[&](auto address,auto size){return Bytes(metadata.bytes_at(address,size));};
  const auto original_visibility=source_bsp_breakable_visibility_tu15(source.memory());
  const auto collisions=bsp.u32(16+0x18);
  CollisionWorld terrain(content.collision);std::set<std::pair<int,int>> visited;
  Spawn spawn;Vec3 target{},probe_origin{},probe_delta{};unsigned breakable=255;bool found=false;
  for(const auto &triangle:content.collision){
    if(!(triangle.flags&8u) || triangle.source.instance!=-1 || triangle.source.record<0 ||
        triangle.source.surface<0 || !visited.emplace(triangle.source.record,triangle.source.surface).second)continue;
    const auto record=read(collisions+unsigned(triangle.source.record)*0x40u,0x40);
    if(unsigned(triangle.source.surface)>=record.u32(0x28))throw FormatError("Terrain breakable source surface escapes its actual collision record");
    const auto surface=read(record.u32(0x2c)+unsigned(triangle.source.surface)*8u,8);
    const auto id=std::to_integer<unsigned>(surface.slice(5,1)[0]);
    if(id==255 || !source_bsp_material_visible(original_visibility,0,-1,int(id)))continue;
    const auto center=(triangle.a+triangle.b+triangle.c)*(1.f/3.f);
    for(unsigned side=0;side<16;++side){
      const float angle=float(side)*.39269908169872414f;
      auto position=center+Vec3{std::cos(angle)*12.f,std::sin(angle)*12.f,0};
      const auto floor=terrain.raycast(position+Vec3{0,0,10},{0,0,-1},20);
      if(!floor)continue;position.z=floor->position.z+content.player.radius;
      const auto eye=position+Vec3{0,0,content.player.camera_height},delta=center-eye;
      const auto hit=source.particle_collision(0x800005u,eye,delta*1.02f);
      if(!hit || hit->type!=1 || length(hit->position-center)>.02f)continue;
      spawn.position=position;target=center;probe_origin=eye;probe_delta=delta*1.02f;
      breakable=id;found=true;
      report.data["terrain_target"]={{"record",triangle.source.record},{"surface",triangle.source.surface},
          {"breakable",id},{"position",{center.x,center.y,center.z}},
          {"initial_contact_position",{hit->position.x,hit->position.y,hit->position.z}}};
      break;
    }
    if(found)break;
  }
  check(found,"No intact authored terrain breakable with a supported original-query firing lane");
  const auto unit=world.match.state().players[slot].source_unit;
  test::place_player(world.match,slot,spawn);native.set_object_position(unit,spawn.position);
  const auto &memory=source.memory();
  const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
  const auto bitmap=word(0x4f9b50u),selected_byte=bitmap+1u+(breakable>>5u)*4u+((breakable&31u)>>3u);
  const auto selected_bit=std::byte(1u<<(breakable&7u));
  source.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
    if(selected_byte<store.address || selected_byte-store.address>=store.written.size())return;
    const auto index=selected_byte-store.address;
    if((store.before.at(index)&selected_bit)!=std::byte{} &&
        (store.written.at(index)&selected_bit)==std::byte{})
      report.data["original_breakable_stores"].push_back({{"instruction",store.instruction},{"address",store.address},
          {"before",std::to_integer<unsigned>(store.before[index])},{"after",std::to_integer<unsigned>(store.written[index])}});
  });
  StoreObservation observation{source};
  report.data["original_breakable_stores"]=Json::array();
  unsigned shots=0,detonations=0;std::uint64_t frontier=0;bool broken=false;
  for(unsigned tick=0;tick<600;++tick){
    auto state=world.match.state();
    check(!state.players[slot].dead && state.players[slot].source_unit==unit,"Terrain blast stimulus lost its actual live shooter");
    constexpr std::uint32_t eye=0x6005b800u,velocity=eye+0x20u;
    auto abi=native.prepare_source_call(std::array{eye,velocity});abi.gp[0]=unit;native.run_actual(0x1582f0u,abi);
    const Vec3 position{std::bit_cast<float>(word(eye)),std::bit_cast<float>(word(eye+4)),std::bit_cast<float>(word(eye+8))};
    const auto delta=target-position;commands[slot].yaw=std::atan2(delta.y,delta.x);
    commands[slot].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
    commands[slot].fire=tick>=30 && tick%30==0;
    commands[slot].reload=state.players[slot].loaded==0;
    world.match.step(commands);source.advance_effect_frame(tick_seconds);state=world.match.state();
    if(state.players[slot].shot_tick==state.tick)++shots;
    for(const auto &event:state.grenade_detonations)if(event.serial>frontier){
      frontier=event.serial;++detonations;
      report.data["detonations"].push_back({{"tick",event.tick},{"projectile",event.projectile},
          {"definition",event.definition},{"effect_instance",event.source_effect_instance}});
    }
    const auto visibility=source_bsp_breakable_visibility_tu15(memory);
    broken=!source_bsp_material_visible(visibility,0,-1,int(breakable));
    if(tick%30==0 || broken){
      report.data["samples"].push_back({{"tick",state.tick},{"breakable_visible",!broken},
          {"loaded",state.players[slot].loaded},{"shots",shots}});report.save();
    }
    if(broken)break;
  }
  source.observe_physics_stores({});
  const auto after=source.particle_collision(0x800005u,probe_origin,probe_delta);
  const bool old_contact_removed=!after || length(after->position-target)>.02f;
  report.data.update({{"source_breakable_cleared",broken},{"shots",shots},{"explosions",detonations},
      {"source_collision_removed",old_contact_removed}});
  check(shots && detonations,"Terrain destruction produced no ordinary source rocket/explosion");
  check(broken && !report.data["original_breakable_stores"].empty(),"Actual explosion did not clear its source-owned terrain breakable bit");
  check(old_contact_removed,"Original collision query retained the destroyed terrain surface");
  report.data["passed"]=true;
  std::cout<<"PASS MCC terrain breakage, original bitmap writes and subsequent collision removal; shots="<<shots<<'\n';
}
}

int main(int argc,char **argv){Report report;try {
  check(argc==5,"Supply MCC_MAP MCC_MAPS TARGET_SUFFIX REPORT");report.path=argv[4];
  const std::string name=argv[3];report.data["target_suffix"]=name;report.data["map"]=argv[1];
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.local_players=1;options.source_object_world=true;options.mute=true;
  options.session_random_seed=0x22de570u;
  if(name=="ghost")options.variant.values.set(76,3);
  if(name=="spectre")options.variant.values.set(76,5);
  if(name=="warthog_gauss")options.variant.values.set(76,2);
  if(name=="wraith")options.variant.values.set(78,2);
  Cache cache(options.map,options.shared_maps);check(cache.is_mcc(),"Destruction runtime requires an MCC v13 cache");
  const auto content=cache.load_content(0,true,source_game_variant(options.variant));
  const auto choices=cache.multiplayer_weapon_choices();bool selected=false;
  const bool prop=name=="power_module" || name=="fusion_coil" || name=="gas_container_full" || name=="coil_to_warthog";
  const std::string suffix=prop?"\\smg":"\\rocket_launcher";
  for(unsigned option=3;option<=20;++option){std::uint32_t seed{};
    const auto weapon=source_starting_weapon(null_datum,option,null_datum,choices,seed);
    if(weapon!=null_datum && content.weapons.at(weapon).name.ends_with(suffix)){
      options.starting_weapon=option;selected=true;break;
    }
  }
  check(selected,"Map has no authored starting-weapon selection for this destruction regression");
  MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
  auto &source=*world.source_objects;auto &native=source.tu15_world();
  const auto slot=world.local_slots.at(0);
  std::array<Controls,multiplayer_capacity> commands{};
  for(unsigned tick=0;tick<60;++tick)world.match.step(commands);
  if(name=="coil_to_warthog" || name=="warthog_to_coil"){
    reciprocal_damage(world,cache,content,slot,commands,name=="coil_to_warthog",report);return 0;
  }
  if(name=="terrain_glass"){terrain_breakage(world,cache,content,slot,commands,report);return 0;}
  const auto target_suffix=name=="warthog_gauss"?std::string("warthog"):name;
  std::uint32_t target=null_datum,witness=null_datum,far_control=null_datum;
  std::vector<std::uint32_t> candidates;
  for(const auto &object:source.live_objects()){
    if(object.parent!=null_datum || (prop?object.type!=11:object.type!=1))continue;
    const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
    if(tag && tag->name.ends_with("\\"+target_suffix))candidates.push_back(object.object);
  }
  if(!candidates.empty())target=candidates.front();
  const bool chain=name=="power_module";
  if(chain){
    float nearest=INFINITY;
    for(auto a:candidates)for(auto b:candidates)if(a!=b){
      const auto distance=length(source.object_sphere(a).center-source.object_sphere(b).center);
      if(distance<nearest){nearest=distance;target=a;witness=b;}
    }
    report.data["chain_witness"]=witness;report.data["actual_pair_center_distance"]=nearest;
    check(witness!=null_datum && nearest<1.f,"No actual authored adjacent module pair for the chain regression");
    float farthest=0;
    for(auto object:candidates)if(object!=target && object!=witness){
      const auto distance=length(source.object_sphere(object).center-source.object_sphere(target).center);
      if(distance>farthest){farthest=distance;far_control=object;}
    }
    check(far_control!=null_datum,"Authored chain scene has no distant negative control");
    report.data["far_control"]=far_control;report.data["far_control_distance"]=farthest;
  }
  check(target!=null_datum,"No source-created target with the requested authored definition");
  const auto target_definition=source.object_definition(target);
  const auto sphere=source.object_sphere(target);const auto root=source.object_transform(target);
  report.data["target"]=target;report.data["definition"]=target_definition;
  // A supported clear firing lane is scene setup only. All subsequent shots,
  // damage requests, response timers, debris and motion run through the world.
  CollisionWorld terrain(content.collision);Spawn spawn;bool clear=false;
  for(unsigned side=0;side<32;++side){
    const float angle=float(side)*.19634954084936207f;
    auto candidate=sphere.center-(root.basis[0]*std::cos(angle)+root.basis[1]*std::sin(angle))*(prop?6.f:12.f);
    const auto floor=terrain.raycast(candidate+Vec3{0,0,10},{0,0,-1},20);
    if(!floor)continue;candidate.z=floor->position.z+content.player.radius;
    const auto eye=candidate+Vec3{0,0,content.player.camera_height},delta=sphere.center-eye;
    if(terrain.raycast(eye,normalized(delta),length(delta)))continue;
    spawn.position=candidate;clear=true;break;
  }
  check(clear,"No supported terrain-clear lane to the actual destruction target");
  const auto unit=world.match.state().players[slot].source_unit;
  test::place_player(world.match,slot,spawn);native.set_object_position(unit,spawn.position);
  report.data["declared_player_position"]={spawn.position.x,spawn.position.y,spawn.position.z};
  const auto &memory=source.memory();
  const auto word=[&](auto address){return Bytes(memory.view(address,4)).u32(0);};
  const auto aim=[&]{
    constexpr std::uint32_t eye=0x6005b800u,velocity=eye+0x20u;
    auto abi=native.prepare_source_call(std::array{eye,velocity});abi.gp[0]=unit;
    native.run_actual(0x1582f0u,abi);
    const Vec3 position{std::bit_cast<float>(word(eye)),std::bit_cast<float>(word(eye+4u)),std::bit_cast<float>(word(eye+8u))};
    const auto delta=source.object_sphere(target).center-position;
    commands[slot].yaw=std::atan2(delta.y,delta.x);
    commands[slot].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
  };
  aim();for(unsigned tick=0;tick<30;++tick){world.match.step(commands);aim();}
  std::set<std::uint32_t> initial,created_debris,projectiles;
  for(const auto &object:source.live_objects())initial.insert(object.object);
  const auto initial_vitality=source.object_vitality(target);float minimum=initial_vitality.body;
  bool dead=false,retired=false,regions_changed=false;unsigned shots=0;std::uint64_t frontier=0;
  std::optional<unsigned> death_tick;
  const auto regions=source.object_model_regions(target);
  const auto control_before=chain?source.object_vitality(far_control):MultiplayerSourceVitality{};
  std::uint32_t witness_body=chain?source.source_object_body(witness):0;
  bool witness_pristine_at_target_blast=false,witness_retired=false;
  std::map<std::uint32_t,unsigned> destruction_requests;
  source.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
    // Complete original DC3C0 writes its owning full object at DC4A0,
    // immediately before DC4A4 calls the original radius owner D91D0.
    // Observe the actual store, without admitting or reconstructing a blast.
    if(store.instruction!=0xdc4a0u)return;
    check(store.written.size()==4,"Original destruction-emitter store width differs");
    const auto emitter=Bytes(store.written).u32(0);
    ++destruction_requests[emitter];
    if(chain && emitter==target)
      witness_pristine_at_target_blast=std::bit_cast<float>(word(witness_body+0xecu))==1.f;
  });
  StoreObservation observation{source};
  report.data["initial_body"]=initial_vitality.body;
  for(unsigned tick=0;tick<900;++tick){
    auto state=world.match.state();
    check(!state.players[slot].dead && state.players[slot].source_unit==unit,
        "Destruction stimulus lost its actual live shooter");
    const auto live=source.live_objects();
    const bool exists=std::ranges::any_of(live,[&](const auto &object){return object.object==target;});
    if(exists)aim();
    commands[slot].fire=exists && !dead && (prop || tick%30==0);
    commands[slot].reload=exists && !dead && state.players[slot].loaded==0;
    world.match.step(commands);source.advance_effect_frame(tick_seconds);state=world.match.state();
    if(state.players[slot].shot_tick==state.tick)++shots;
    const auto after=source.live_objects();
    retired=!std::ranges::any_of(after,[&](const auto &object){return object.object==target;});
    witness_retired=chain && !std::ranges::any_of(after,[&](const auto &object){return object.object==witness;});
    for(const auto &object:after){
      if(object.type==5)projectiles.insert(object.object);
      if(!initial.contains(object.object) && object.type==11)created_debris.insert(object.object);
    }
    if(!retired){
      const auto vitality=source.object_vitality(target);minimum=std::min(minimum,vitality.body);
      const auto flags=word(source.object_body(target)+0x108u)>>16u;
      dead|=vitality.body<=0 && (flags&4u);
      if(dead && !death_tick)death_tick=tick;
      regions_changed|=source.object_model_regions(target).selection.permutations!=regions.selection.permutations;
      if(tick%30==0 || dead)report.data["samples"].push_back({{"tick",state.tick},
          {"body",vitality.body},{"shield",vitality.shield},{"damage_flags",flags},
          {"loaded",state.players[slot].loaded},{"reserve",state.players[slot].reserve}});
    }
    for(const auto &event:state.grenade_detonations)if(event.serial>frontier){
      frontier=event.serial;
      report.data["detonations"].push_back({{"tick",event.tick},{"projectile",event.projectile},
          {"definition",event.definition},{"effect_instance",event.source_effect_instance}});
    }
    if(tick%30==0){report.data["progress_tick"]=state.tick;report.save();}
    if((retired && (!chain || witness_retired)) || (!prop && dead)){
      if(!prop && dead)for(unsigned settle=0;settle<60;++settle){
        commands[slot].fire=commands[slot].reload=false;world.match.step(commands);source.advance_effect_frame(tick_seconds);
        for(const auto &object:source.live_objects())if(!initial.contains(object.object) && object.type==11)
          created_debris.insert(object.object);
      }
      break;
    }
    if(prop && death_tick && tick>=*death_tick+90)break;
  }
  source.observe_physics_stores({});
  report.data.update({{"shots",shots},{"minimum_body",minimum},{"source_dead",dead},
      {"target_retired",retired},{"regions_changed",regions_changed},
      {"source_projectiles",projectiles},{"source_debris",created_debris},
      {"original_destruction_emitter_stores",destruction_requests},
      {"witness_pristine_at_target_blast",witness_pristine_at_target_blast},
      {"chain_witness_retired",witness_retired}});
  check(shots>0,"Ordinary source weapon commands produced no shots");
  check(minimum<initial_vitality.body || retired,"Ordinary source projectiles never damaged the requested target");
  check(dead || retired,"Actual source damage did not complete the target death transition");
  if(prop)check(retired,"Explosive death did not deliver its delayed destruction/retirement response");
  if(chain){
    check(witness_pristine_at_target_blast,"Chain witness was already damaged before the target's original blast");
    check(witness_retired && destruction_requests[target]==1 && destruction_requests[witness]==1,
        "Original module blast did not cause one source-owned chained witness explosion");
    const auto control_after=source.object_vitality(far_control);
    check(control_after.body==control_before.body && control_after.shield==control_before.shield &&
        !destruction_requests.contains(far_control),"Distant source module was damaged or emitted a fabricated chain blast");
    report.data["far_control_unchanged"]=true;
  }
  if(!prop)check(!created_debris.empty(),"Vehicle death produced no source-created debris");
  report.data["passed"]=true;
  std::cout<<"PASS MCC destruction target="<<name<<" shots="<<shots<<" body="<<minimum
      <<" source_dead="<<dead<<" retired="<<retired<<" debris="<<created_debris.size()<<'\n';
  return 0;
}catch(const std::exception &error){report.data["error"]=error.what();std::cerr<<error.what()<<'\n';return 1;}}
