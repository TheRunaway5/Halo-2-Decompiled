#include "halo2/multiplayer_world.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/math.hpp"
#include "halo2/source_bsp_breakable_visibility_tu15.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "helpers/gameplay_fixture.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>
#include <cstdlib>
#include <openssl/sha.h>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct FixtureClock final:PhysicsLiveClockSource {
  std::uint32_t milliseconds_value=1000;
  std::uint64_t cycles_value=1'000'000,filetime_value=132'000'000'000'000'000ull;
  std::uint32_t milliseconds()const noexcept override{return milliseconds_value;}
  std::uint64_t cycles()const noexcept override{return cycles_value;}
  std::uint64_t utc_filetime()const noexcept override{return filetime_value;}
  void advance(){milliseconds_value+=33;cycles_value+=33'000;filetime_value+=330'000;}
};
struct Report {
  std::filesystem::path path;
  std::shared_ptr<FixtureClock> clock;
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
std::optional<Spawn> interaction_firing_lane(const LevelContent &content,const CollisionWorld &terrain,
    Vec3 center,Vec3 away,std::span<const float> distances){
  away=normalized(Vec3{away.x,away.y,0});
  for(float distance:distances)for(unsigned side=0;side<32;++side){
    const float angle=float(side)*.19634954084936207f;
    const Vec3 direction{std::cos(angle),std::sin(angle),0};
    if(dot(direction,away)<.8f)continue;
    auto origin=center+direction*distance+Vec3{0,0,10};float remaining=20;
    for(unsigned layer=0;layer<8 && remaining>0;++layer){
      const auto support=terrain.raycast(origin,{0,0,-1},remaining);if(!support)break;
      const auto position=support->position+Vec3{0,0,content.player.radius};
      const auto eye=position+Vec3{0,0,content.player.camera_height},delta=center-eye;
      if(support->normal.z>=content.player.maximum_slope_cosine &&
          !terrain.overlaps(position,content.player.height,content.player.radius) &&
          !terrain.raycast(eye,normalized(delta),length(delta))){Spawn spawn;spawn.position=position;return spawn;}
      const float step=content.player.radius*.1f;remaining-=support->distance+step;
      origin=support->position-Vec3{0,0,step};
    }
  }
  return {};
}
void destruction_continuation(MultiplayerPreparedWorld &world,
    std::array<Controls,multiplayer_capacity> commands,Report &report){
  if(!std::getenv("HALO2_DESTRUCTION_CONTINUATION"))return;
  // This local binding supplies no peer identity or transport readiness.
  // Retain only owning in-memory cuts. Each replay executes the original
  // APIs with recorded caller/clock inputs and verifies their actual results.
  constexpr SourceMatchAuthority::AuthorityDigest binding{std::byte{0x22},std::byte{0xde},std::byte{0x57}};
  SourceMatchAuthority authority(world.match,*world.source_objects);
  world.source_objects->observe_physics_stores({});
  for(auto &command:commands)command.fire=command.reload=false;
  auto before=authority.checkpoint(binding);
  const auto initial=encode_source_authority(before);
  const auto digest=[](const std::vector<std::byte> &bytes){
    std::array<unsigned char,SHA256_DIGEST_LENGTH> value;
    SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),value.data());return value;
  };
  std::vector<SourceExecutionPhase> phases;
  std::vector<std::array<unsigned char,SHA256_DIGEST_LENGTH>> expected;
  for(unsigned tick=0;tick<8;++tick){
    phases.push_back(decode_source_execution_phase(encode_source_execution_phase(
        authority.capture_execution_phase([&]{
          if(report.clock)report.clock->advance();
          world.match.step(commands);world.source_objects->advance_effect_frame(tick_seconds);
        }))));
    expected.push_back(digest(encode_source_authority(authority.checkpoint(binding))));
  }
  unsigned compared=0;
  for(unsigned repetition=0;repetition<2;++repetition){
    auto prepared=authority.prepare(before,binding);
    check(authority.commit(std::move(prepared)),"Destruction continuation cut did not commit");
    auto restored=authority.checkpoint(binding);
    if(encode_source_authority(restored)!=initial){
      report.data["restore_mismatch"]={{"source_bank_equal",restored.world.world.bank==before.world.world.bank},
          {"milliseconds",{before.world.world.logical_milliseconds,restored.world.world.logical_milliseconds}},
          {"cycles",{before.world.world.logical_cycles,restored.world.world.logical_cycles}},
          {"filetime",{before.world.world.logical_utc_filetime,restored.world.world.logical_utc_filetime}},
          {"result_seconds",{before.match.logical_result_seconds,restored.match.logical_result_seconds}}};
      // Diagnostic copies only: identify external input differences without
      // changing imported source bytes or accepting a mismatching replay.
      restored.world.world.logical_milliseconds=before.world.world.logical_milliseconds;
      restored.world.world.logical_cycles=before.world.world.logical_cycles;
      restored.world.world.logical_utc_filetime=before.world.world.logical_utc_filetime;
      restored.match.logical_result_seconds=before.match.logical_result_seconds;
      report.data["restore_mismatch"]["equal_after_external_clock_fields_only"]=encode_source_authority(restored)==initial;
      check(false,"Repeated destruction restore changed the complete native cut");
    }
    for(unsigned tick=0;tick<phases.size();++tick){
      authority.replay_execution_phase(phases[tick]);
      check(digest(encode_source_authority(authority.checkpoint(binding)))==expected[tick],
          "Destruction continuation changed original objects, response/effect state or coupled match bytes");
      ++compared;
    }
  }
  report.data["post_destruction_continuation"]={{"passed",true},{"repeated_restores",2},
      {"complete_encoded_world_sha256_comparisons",compared},{"cut_bytes",initial.size()},
      {"peer_transport_or_presentation_acceptance",false},{"raw_snapshots_retained",false}};
}
void reciprocal_damage(MultiplayerPreparedWorld &world,const Cache &cache,const LevelContent &content,
    unsigned slot,std::array<Controls,multiplayer_capacity> &commands,bool coil_first,Report &report,
    unsigned witness_type=1,std::string_view witness_suffix="\\warthog"){
  auto &source=*world.source_objects;auto &native=source.tu15_world();
  std::vector<std::uint32_t> vehicles,coils;
  for(const auto &object:source.live_objects()){
    if(object.parent!=null_datum)continue;
    const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});if(!tag)continue;
    if(object.type==witness_type && tag->name.ends_with(witness_suffix))vehicles.push_back(object.object);
    if(object.type==11 && tag->name.ends_with("\\fusion_coil"))coils.push_back(object.object);
  }
  check(!vehicles.empty() && coils.size()>1,"Interaction scene requires actual authored witness and coil births");
  const auto vehicle=vehicles.front(),coil=coils.front();
  const auto vehicle_root=source.object_transform(vehicle),coil_root=source.object_transform(coil);
  const auto vehicle_center=source.object_sphere(vehicle).center;
  // Declared scene input: preserve the coil's actual root-to-floor height.
  // The position opposite the shooter separates rocket contact from the witness.
  // Original Headlong rocket jpt radius words are1/2; large vehicle1/3.
  // Actual contact/area admission and damage remain entirely source-owned.
  CollisionWorld terrain(content.collision);
  const auto original_floor=terrain.raycast(coil_root.position+Vec3{0,0,.2f},{0,0,-1},20);
  check(bool(original_floor),"Reciprocal scene lacks its original coil support context");
  auto coil_position=vehicle_center+vehicle_root.basis[0]*(witness_type==1?2.8f:1.2f);
  auto floor=terrain.raycast(coil_position+Vec3{0,0,.2f},{0,0,-1},20);
  constexpr std::array coil_distances{6.f,4.f,3.f,2.f};
  constexpr std::array vehicle_distances{12.f};
  if(witness_type==11){
    // Admit the declared scene input from actual BSP support and an actual
    // terrain-clear approach, before moving any source object. The source
    // still owns the coil's subsequent collision, motion and damage.
    const auto center_offset=source.object_sphere(coil).center-coil_root.position;
    bool planned=false;
    for(unsigned side=0;side<32 && !planned;++side){
      const float angle=float(side)*.19634954084936207f;
      auto candidate=vehicle_center+Vec3{std::cos(angle),std::sin(angle),0}*1.2f;
      auto origin=candidate+Vec3{0,0,10};float remaining=20;
      for(unsigned layer=0;layer<8 && remaining>0;++layer){
        const auto support=terrain.raycast(origin,{0,0,-1},remaining);if(!support)break;
        candidate.z=support->position.z+coil_root.position.z-original_floor->position.z;
        if(length(candidate+center_offset-vehicle_center)<2.f &&
            support->normal.z>=content.player.maximum_slope_cosine &&
            interaction_firing_lane(content,terrain,candidate+center_offset,candidate-vehicle_center,coil_distances)){
          coil_position=candidate;floor=support;planned=true;
          report.data["declared_coil_plan"]={{"side",side},{"floor_layer",layer}};break;
        }
        const float step=content.player.radius*.1f;remaining-=support->distance+step;
        origin=support->position-Vec3{0,0,step};
      }
    }
    check(planned,"Actual BSP has no supported coil position with a clear shooting approach");
  }
  check(bool(floor),"Reciprocal scene has no real terrain-supported coil position");
  coil_position.z=floor->position.z+coil_root.position.z-original_floor->position.z;
  report.data["interaction_setup"]={{"original_coil_root",{coil_root.position.x,coil_root.position.y,coil_root.position.z}},
      {"original_support",{original_floor->position.x,original_floor->position.y,original_floor->position.z}},
      {"witness_center",{vehicle_center.x,vehicle_center.y,vehicle_center.z}},
      {"new_support",{floor->position.x,floor->position.y,floor->position.z}},
      {"declared_coil_root",{coil_position.x,coil_position.y,coil_position.z}}};
  native.set_object_position(coil,coil_position);
  const auto target=coil_first?coil:vehicle,witness=coil_first?vehicle:coil;
  const auto &controls=coil_first?vehicles:coils;
  std::uint32_t far_control=null_datum;float farthest=0;
  for(auto object:controls)if(object!=target && object!=witness){
    const auto distance=length(source.object_sphere(object).center-vehicle_center);
    if(distance>farthest){far_control=object;farthest=distance;}
  }
  if(far_control==null_datum || farthest<=10){
    // Some ordinary netgame variants create only one vehicle of this class.
    // Declare an additional constructor input for the negative control; the
    // complete original B9960/B9B70 owns all object/physics/vitality results.
    const auto original=coil_first?vehicle:coil;
    const auto original_root=source.object_transform(original);
    const auto original_support=terrain.raycast(original_root.position+Vec3{0,0,.2f},{0,0,-1},20);
    check(bool(original_support),"Distant control lacks its original root-to-floor context");
    std::optional<Vec3> control_position;
    for(unsigned side=0;side<32 && !control_position;++side){
      const float angle=float(side)*.19634954084936207f;
      auto position=vehicle_center+Vec3{std::cos(angle),std::sin(angle),0}*32.f;
      const auto support=terrain.raycast(position+Vec3{0,0,.2f},{0,0,-1},20);
      if(!support || support->normal.z<content.player.maximum_slope_cosine)continue;
      position.z=support->position.z+original_root.position.z-original_support->position.z;
      control_position=position;
    }
    check(bool(control_position),"No supported declared distant control constructor location");
    constexpr std::uint32_t request=0x60010000u;
    auto input=native.prepare_source_call(std::array{source.object_definition(original),null_datum,0u});
    input.gp[6]=request;native.run_actual(0xb9960u,input);
    native.write_source_call_input(request+0x1cu,std::as_bytes(std::span(&*control_position,1)));
    native.write_source_call_input(request+0x28u,std::as_bytes(std::span(&original_root.basis[0],1)));
    far_control=native.call_actual(0xb9b70u,std::array{request});
    check(far_control!=null_datum,"Original generic factory rejected the declared distant control input");
    farthest=length(source.object_sphere(far_control).center-vehicle_center);
    report.data["declared_control_constructor"]={{"definition",source.object_definition(original)},
        {"position",{control_position->x,control_position->y,control_position->z}},
        {"original_entries",{0xb9960u,0xb9b70u}},{"object",far_control}};
  }
  check(far_control!=null_datum && farthest>10,"Reciprocal scene lacks an actual distant same-class negative control");
  // Complete normal source frames refresh posed spheres and solve the moved
  // coil before those published values are used for the shooter approach.
  for(unsigned tick=0;tick<30;++tick)world.match.step(commands);
  const auto center=source.object_sphere(target).center;
  const auto lane=interaction_firing_lane(content,terrain,center,
      coil_first?center-vehicle_center:vehicle_root.basis[0]*-1.f,
      coil_first?std::span<const float>(coil_distances):std::span<const float>(vehicle_distances));
  check(bool(lane),"Reciprocal scene lacks a supported clear shooter lane");
  const auto spawn=*lane;
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
  struct BlastPosition {std::array<std::uint32_t,3> words{};unsigned mask{};};
  std::map<std::uint32_t,BlastPosition> blast_positions;
  source.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
    const auto component=store.instruction==0xdc47bu?0:store.instruction==0xdc486u?1:
        store.instruction==0xdc464u?2:-1;
    if(component>=0){
      const auto stack=store.address-0x2cu-4u*unsigned(component);
      auto &position=blast_positions[stack];if(component==2)position.mask=0;
      position.words[component]=Bytes(store.written).u32(0);position.mask|=1u<<component;
      return;
    }
    if(store.instruction!=0xdc4a0u || Bytes(store.written).u32(0)!=target)return;
    if(!requests++){
      pristine_at_first_blast=std::bit_cast<float>(word(witness_body+0xecu))==witness_before.body &&
          std::bit_cast<float>(word(witness_body+0xf0u))==witness_before.shield;
      first_blast=observed_tick;
      // DC47B/DC486/DC464 publish request+24's actual XYZ from the caller's
      // F0/F4/F8 inputs. D9249 passes that exact point to the radius query.
      const auto vector=[&](auto at){return Vec3{std::bit_cast<float>(word(at)),
          std::bit_cast<float>(word(at+4u)),std::bit_cast<float>(word(at+8u))};};
      const auto at=blast_positions.find(store.address-0x20u);
      check(at!=blast_positions.end() && at->second.mask==7,"Original blast marker stores were not observed completely");
      // DC491/DC497 push two arguments after the marker stores. The final
      // emitter address is therefore the earlier SP+20, not the earlier SP+28.
      // Consume the actual produced XYZ bytes at that verified stack identity.
      const Vec3 origin{std::bit_cast<float>(at->second.words[0]),std::bit_cast<float>(at->second.words[1]),
          std::bit_cast<float>(at->second.words[2])};
      const auto position=vector(witness_body+0x30u),delta=position-origin;
      check(std::isfinite(length(delta)),"Original blast marker stores were nonfinite");
      report.data["first_original_blast"]={{"position",{origin.x,origin.y,origin.z}},
          {"witness_center",{position.x,position.y,position.z}},{"center_distance",length(delta)}};
      if(const auto obstruction=terrain.raycast(origin,normalized(delta),length(delta)))
        report.data["first_original_blast"]["independent_bsp_ray"]={{"hit",true},
            {"surface",obstruction->source.surface},{"plane",obstruction->source.plane},
            {"material",obstruction->material},{"distance",obstruction->distance}};
      else report.data["first_original_blast"]["independent_bsp_ray"]={{"hit",false}};
    }
  });
  StoreObservation observation{source};
  report.data["reciprocal_scene"]={{"target",target},{"witness",witness},{"far_control",far_control},
      {"witness_type",witness_type},{"witness_suffix",witness_suffix},
      {"far_control_distance",farthest},{"declared_coil_root",{coil_position.x,coil_position.y,coil_position.z}},
      {"declared_shooter",{spawn.position.x,spawn.position.y,spawn.position.z}}};
  const auto exists=[&](auto object){return std::ranges::any_of(source.live_objects(),[&](const auto &row){return row.object==object;});};
  for(unsigned tick=0;tick<900;++tick){
    auto state=world.match.state();check(!state.players[slot].dead && state.players[slot].source_unit==unit,
        "Reciprocal interaction lost its real shooter");
    const bool target_live=exists(target);if(target_live)aim();
    commands[slot].fire=target_live && !dead && (coil_first || tick%30==0);
    commands[slot].reload=target_live && !dead && state.players[slot].loaded==0;
    const auto previous_shot=state.players[slot].shot_tick;observed_tick=state.tick+1;
    world.match.step(commands);source.advance_effect_frame(tick_seconds);state=world.match.state();
    if(state.players[slot].shot_tick!=previous_shot)++shots;
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
  report.data["far_control_unchanged"]=true;
  destruction_continuation(world,commands,report);report.data["passed"]=true;
  std::cout<<"PASS MCC "<<(coil_first?"coil-to-object":"vehicle-to-coil")<<" damage through ordinary weapon stimulus\n";
}
void terrain_breakage(MultiplayerPreparedWorld &world,const Cache &cache,
    const LevelContent &content,unsigned slot,std::array<Controls,multiplayer_capacity> &commands,Report &report) {
  auto &source=*world.source_objects;auto &native=source.tu15_world();
  std::optional<test::LivePhysicsEntryCaptureTu15> collision_capture;
  std::optional<SourcePhysicsEntryObserverBinding> collision_observation;
  if(std::getenv("HALO2_DESTRUCTION_CAPTURE_COLLISION")){
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Collision capture requires its owned test scratch");
    const auto path=std::filesystem::path(scratch)/"collision-query-capture";
    report.data["collision_query_capture"]=path.string();collision_capture.emplace(path);
    collision_observation.emplace(source.memory(),[&](bool before,std::uint32_t entry,const auto &abi){
      if(entry==0x2fc690u)collision_capture->observe(before,abi,source.memory(),entry);
    });
  }
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
    const auto previous_shot=state.players[slot].shot_tick;
    world.match.step(commands);source.advance_effect_frame(tick_seconds);state=world.match.state();
    if(state.players[slot].shot_tick!=previous_shot)++shots;
    for(const auto &event:state.grenade_detonations)if(event.serial>frontier){
      frontier=event.serial;++detonations;
      report.data["detonations"].push_back({{"tick",event.tick},{"projectile",event.projectile},
          {"definition",event.definition},{"effect_instance",event.source_effect_instance},
          {"position",{event.position.x,event.position.y,event.position.z}}});
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
  destruction_continuation(world,commands,report);report.data["passed"]=true;
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
  if(name=="scorpion")options.variant.values.set(78,1);
  if(name=="wraith")options.variant.values.set(78,2);
  Cache cache(options.map,options.shared_maps);check(cache.is_mcc(),"Destruction runtime requires an MCC v13 cache");
  const auto content=cache.load_content(0,true,source_game_variant(options.variant));
  const auto choices=cache.multiplayer_weapon_choices();bool selected=false;
  const bool prop=name=="power_module" || name=="fusion_coil" || name=="gas_container_full" ||
      name=="coil_to_warthog" || name=="coil_to_scenery" || name=="coil_to_crate";
  const std::string suffix=prop?"\\smg":"\\rocket_launcher";
  for(unsigned option=3;option<=20;++option){std::uint32_t seed{};
    const auto weapon=source_starting_weapon(null_datum,option,null_datum,choices,seed);
    if(weapon!=null_datum && content.weapons.at(weapon).name.ends_with(suffix)){
      options.starting_weapon=option;selected=true;break;
    }
  }
  check(selected,"Map has no authored starting-weapon selection for this destruction regression");
  if(std::getenv("HALO2_DESTRUCTION_FIXED_CLOCK"))report.clock=std::make_shared<FixtureClock>();
  report.data["declared_external_clock"]=bool(report.clock);
  MultiplayerPreparedWorld world(options,std::nullopt,report.clock);world.bind_source_local_views(world.local_slots);
  auto &source=*world.source_objects;auto &native=source.tu15_world();
  const auto slot=world.local_slots.at(0);
  std::array<Controls,multiplayer_capacity> commands{};
  const auto initial_view=world.match.state();
  for(const auto local:world.local_slots){
    commands[local].yaw=initial_view.players[local].yaw;
    commands[local].pitch=initial_view.players[local].pitch;
  }
  for(unsigned tick=0;tick<60;++tick)world.match.step(commands);
  if(name=="coil_to_warthog" || name=="warthog_to_coil"){
    reciprocal_damage(world,cache,content,slot,commands,name=="coil_to_warthog",report);return 0;
  }
  if(name=="coil_to_scenery" || name=="coil_to_crate"){
    reciprocal_damage(world,cache,content,slot,commands,true,report,
        name=="coil_to_scenery"?6:11,name=="coil_to_scenery"?"\\no_swimming":"\\military\\case\\case");return 0;
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
  if(prop)for(float distance:{4.f,3.f,6.f,2.f}){
    for(unsigned side=0;side<32 && !clear;++side){
      const float angle=float(side)*.19634954084936207f;
      const auto candidate=sphere.center-(root.basis[0]*std::cos(angle)+root.basis[1]*std::sin(angle))*distance;
      auto origin=candidate+Vec3{0,0,10};float remaining=20;
      for(unsigned layer=0;layer<8 && remaining>0;++layer){
        const auto floor=terrain.raycast(origin,{0,0,-1},remaining);if(!floor)break;
        const auto position=floor->position+Vec3{0,0,content.player.radius};
        const auto eye=position+Vec3{0,0,content.player.camera_height},delta=sphere.center-eye;
        if(floor->normal.z>=content.player.maximum_slope_cosine &&
            !terrain.overlaps(position,content.player.height,content.player.radius) &&
            !terrain.raycast(eye,normalized(delta),length(delta))){
          spawn.position=position;clear=true;report.data["declared_lane"]={{"distance",distance},{"floor_layer",layer}};break;
        }
        const float step=content.player.radius*.1f;remaining-=floor->distance+step;
        origin=floor->position-Vec3{0,0,step};
      }
    }
    if(clear)break;
  }
  if(!prop)
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
  Vec3 current_eye{},current_target{};
  const auto aim=[&]{
    constexpr std::uint32_t eye=0x6005b800u,velocity=eye+0x20u;
    auto abi=native.prepare_source_call(std::array{eye,velocity});abi.gp[0]=unit;
    native.run_actual(0x1582f0u,abi);
    const Vec3 position{std::bit_cast<float>(word(eye)),std::bit_cast<float>(word(eye+4u)),std::bit_cast<float>(word(eye+8u))};
    current_eye=position;current_target=source.object_sphere(target).center;
    const auto delta=current_target-position;
    commands[slot].yaw=std::atan2(delta.y,delta.x);
    commands[slot].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
  };
  aim();for(unsigned tick=0;tick<30;++tick){world.match.step(commands);aim();}
  const auto settled=world.match.state();const auto unit_position=source.object_transform(unit).position;
  report.data["settled_scene"]={{"source_unit_position",{unit_position.x,unit_position.y,unit_position.z}},
      {"published_unit_position",{settled.players[slot].position.x,settled.players[slot].position.y,settled.players[slot].position.z}},
      {"source_eye",{current_eye.x,current_eye.y,current_eye.z}},
      {"target_center",{current_target.x,current_target.y,current_target.z}},
      {"input_aim",{commands[slot].yaw,commands[slot].pitch}},
      {"published_aim",{settled.players[slot].yaw,settled.players[slot].pitch}}};
  std::set<std::uint32_t> initial,created_debris,projectiles;
  for(const auto &object:source.live_objects())initial.insert(object.object);
  const auto initial_vitality=source.object_vitality(target);float minimum=initial_vitality.body;
  bool dead=false,retired=false,regions_changed=false;unsigned shots=0;std::uint64_t frontier=0;
  std::optional<unsigned> death_tick;
  const auto regions=source.object_model_regions(target);
  const auto control_before=chain?source.object_vitality(far_control):MultiplayerSourceVitality{};
  std::uint32_t witness_body=chain?source.source_object_body(witness):0;
  bool witness_pristine_at_target_blast=false,witness_retired=false;
  float witness_body_at_target_blast=1.f;
  std::map<std::uint32_t,unsigned> destruction_requests;
  source.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
    // Complete original DC3C0 writes its owning full object at DC4A0,
    // immediately before DC4A4 calls the original radius owner D91D0.
    // Observe the actual store, without admitting or reconstructing a blast.
    if(store.instruction!=0xdc4a0u)return;
    check(store.written.size()==4,"Original destruction-emitter store width differs");
    const auto emitter=Bytes(store.written).u32(0);
    ++destruction_requests[emitter];
    if(chain){
      const auto fraction=std::bit_cast<float>(word(witness_body+0xecu));
      report.data["chain_original_blast_order"].push_back({{"emitter",emitter},{"witness_body",fraction}});
      if(emitter==target){witness_body_at_target_blast=fraction;witness_pristine_at_target_blast=fraction==1.f;}
    }
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
    commands[slot].fire=exists && !dead && (chain?tick%10==0:(prop || tick%30==0));
    commands[slot].reload=exists && !dead && state.players[slot].loaded==0;
    const auto previous_shot=state.players[slot].shot_tick;
    world.match.step(commands);source.advance_effect_frame(tick_seconds);state=world.match.state();
    if(state.players[slot].shot_tick!=previous_shot)++shots;
    const auto after=source.live_objects();
    retired=!std::ranges::any_of(after,[&](const auto &object){return object.object==target;});
    witness_retired=chain && !std::ranges::any_of(after,[&](const auto &object){return object.object==witness;});
    for(const auto &object:after){
      if(object.type==5 && projectiles.insert(object.object).second){
        const auto frame=source.object_transform(object.object);
        report.data["source_projectile_births"].push_back({{"object",object.object},{"tick",state.tick},
            {"position",{frame.position.x,frame.position.y,frame.position.z}},
            {"forward",{frame.basis[0].x,frame.basis[0].y,frame.basis[0].z}}});
      }
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
          {"loaded",state.players[slot].loaded},{"reserve",state.players[slot].reserve},
          {"shot_tick",state.players[slot].shot_tick},
          {"source_eye",{current_eye.x,current_eye.y,current_eye.z}},
          {"target_center",{current_target.x,current_target.y,current_target.z}},
          {"input_aim",{commands[slot].yaw,commands[slot].pitch}},
          {"published_aim",{state.players[slot].yaw,state.players[slot].pitch}}});
    }
    for(const auto &event:state.grenade_detonations)if(event.serial>frontier){
      frontier=event.serial;
      report.data["detonations"].push_back({{"tick",event.tick},{"projectile",event.projectile},
          {"definition",event.definition},{"effect_instance",event.source_effect_instance},
          {"position",{event.position.x,event.position.y,event.position.z}},
          {"target_center",{current_target.x,current_target.y,current_target.z}}});
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
      {"witness_body_at_target_blast",witness_body_at_target_blast},
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
  destruction_continuation(world,commands,report);report.data["passed"]=true;
  std::cout<<"PASS MCC destruction target="<<name<<" shots="<<shots<<" body="<<minimum
      <<" source_dead="<<dead<<" retired="<<retired<<" debris="<<created_debris.size()<<'\n';
  return 0;
}catch(const std::exception &error){report.data["error"]=error.what();std::cerr<<error.what()<<'\n';return 1;}}
