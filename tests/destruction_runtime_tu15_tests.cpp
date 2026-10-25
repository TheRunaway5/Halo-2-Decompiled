#include "halo2/multiplayer_world.hpp"
#include "halo2/custom_slayer.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/math.hpp"
#include "halo2/source_bsp_breakable_visibility_tu15.hpp"
#include "halo2/source_bsp_part_commands.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_vehicle_native_rates.hpp"
#include "halo2/source_damage_record_native_tu15.hpp"
#include "halo2/source_damage_model_native_tu15.hpp"
#include "halo2/source_damage_material_native_tu15.hpp"
#include "halo2/source_difficulty_parameter_native_tu15.hpp"
#include "halo2/posed_collision.hpp"
#include "helpers/gameplay_fixture.hpp"
#include "helpers/campaign_vehicle_observer_inputs_tu15.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_match_authority.hpp"
#include "halo2/source_authority_codec.hpp"
#include "halo2/source_execution_codec.hpp"
#include "halo2/raw_datum_pool.hpp"
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
  SourceVehicleNativeRateActivity native_rates_before=source_vehicle_native_rate_activity();
  SourceDamageRecordNativeActivity native_record_before=source_damage_record_native_activity();
  SourceDamageModelNativeActivity native_model_before=source_damage_model_native_activity();
  SourceDamageMaterialNativeActivity native_material_before=source_damage_material_native_activity();
  SourceDifficultyParameterNativeActivity native_difficulty_before=source_difficulty_parameter_native_activity();
  Json data{{"passed",false},{"runtime_cache_source","MCC"},
      {"damage_or_destruction_results_supplied",false},{"scene_placement_is_declared_test_input",true},
      {"render_audio_or_full_goal_acceptance",false},{"samples",Json::array()},{"detonations",Json::array()}};
  void save()const{if(!path.empty()){
    auto recorded=data;const auto rates=source_vehicle_native_rate_activity();
    recorded["native_vehicle_rate_activity"]={{"acceleration_calls",rates.acceleration_calls-native_rates_before.acceleration_calls},
        {"approach_calls",rates.approach_calls-native_rates_before.approach_calls},
        {"live_cursor_calls",rates.live_cursor_calls-native_rates_before.live_cursor_calls},
        {"scope","Process-local diagnostics from the actual native composition; never gameplay or replay inputs"}};
    const auto record=source_damage_record_native_activity();
    recorded["native_damage_record_activity"]={{"calls",record.calls-native_record_before.calls},
        {"admitted",record.admitted-native_record_before.admitted},{"rejected",record.rejected-native_record_before.rejected}};
    const auto model=source_damage_model_native_activity();
    recorded["native_damage_model_activity"]={{"calls",model.calls-native_model_before.calls},
        {"null_object",model.null_object-native_model_before.null_object},
        {"null_model",model.null_model-native_model_before.null_model},
        {"no_damage_information",model.no_damage_information-native_model_before.no_damage_information},
        {"resolved",model.resolved-native_model_before.resolved}};
    const auto material=source_damage_material_native_activity();
    recorded["native_damage_material_activity"]={{"calls",material.calls-native_material_before.calls},
        {"groups_found",material.groups_found-native_material_before.groups_found},
        {"modifiers_found",material.modifiers_found-native_material_before.modifiers_found}};
    const auto difficulty=source_difficulty_parameter_native_activity();
    recorded["native_difficulty_parameter_activity"]={
        {"parameter_calls",difficulty.parameter_calls-native_difficulty_before.parameter_calls},
        {"current_difficulty_calls",difficulty.current_difficulty_calls-native_difficulty_before.current_difficulty_calls}};
    std::ofstream output(path);output<<recorded.dump(2)<<'\n';
  }}
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
    std::array<Controls,multiplayer_capacity> commands,Report &report,
    std::string_view report_key="post_destruction_continuation",bool return_to_cut=false){
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
  const auto clock_milliseconds=report.clock?report.clock->milliseconds_value:0u;
  const auto clock_cycles=report.clock?report.clock->cycles_value:0ull;
  const auto clock_filetime=report.clock?report.clock->filetime_value:0ull;
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
  if(return_to_cut) {
    // Rewind the declared external input before the native commit computes
    // its local clock-domain adjustments from that input.
    if(report.clock) {
      report.clock->milliseconds_value=clock_milliseconds;
      report.clock->cycles_value=clock_cycles;report.clock->filetime_value=clock_filetime;
    }
    auto prepared=authority.prepare(before,binding);
    check(authority.commit(std::move(prepared)),"Pending destruction cut did not recommit");
    check(encode_source_authority(authority.checkpoint(binding))==initial,
        "Pending destruction cut changed after its qualification replay");
  }
  report.data[std::string(report_key)]={{"passed",true},{"repeated_restores",2},
      {"complete_encoded_world_sha256_comparisons",compared},{"cut_bytes",initial.size()},
      {"returned_to_initial_cut",return_to_cut},
      {"peer_transport_or_presentation_acceptance",false},{"raw_snapshots_retained",false}};
}
void explicit_cleanup_receipt(MultiplayerPreparedWorld &world,std::uint32_t target,Report &report){
  if(!std::getenv("HALO2_DESTRUCTION_FINAL_CLEANUP_INPUT"))return;
  auto &source=*world.source_objects;unsigned calls{};
  check(std::ranges::any_of(source.live_objects(),[&](const auto &object){return object.object==target;}),
      "Explicit original teardown requires the actual remaining target generation");
  source.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
    // Complete1186D0's CALL pushes this literal return address before
    // the actual16E8A0 pool deletion. No callee result is supplied here.
    if(store.instruction==0x1186dau && store.written.size()==4 &&
        Bytes(store.written).u32(0)==0x1186dfu)++calls;
  });
  StoreObservation observation{source};
  source.destroy_object(target);
  const auto after=source.live_objects();
  const bool gone=std::ranges::none_of(after,[&](const auto &object){return object.object==target;});
  report.data["explicit_original_teardown"]={{"requested_full_object",target},
      {"original_light_receiver_delete_calls",calls},{"full_generation_retired",gone},
      {"scope","Explicit original deletion API input after natural destruction/replay; automatic retirement timing is not certified"}};
  check(calls && gone,"Original explicit teardown did not execute its light-receiver deletion and retire the full object");
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
        const auto blast_approach=vehicle_center-(candidate+center_offset);
        if(length(blast_approach)<2.f &&
            support->normal.z>=content.player.maximum_slope_cosine &&
            !terrain.raycast(candidate+center_offset,normalized(blast_approach),length(blast_approach)) &&
            interaction_firing_lane(content,terrain,candidate+center_offset,candidate-vehicle_center,coil_distances)){
          coil_position=candidate;floor=support;planned=true;
          report.data["declared_coil_plan"]={{"side",side},{"floor_layer",layer},
              {"predicted_center_bsp_ray_clear",true}};break;
        }
        const float step=content.player.radius*.1f;remaining-=support->distance+step;
        origin=support->position-Vec3{0,0,step};
      }
    }
    check(planned,"Actual BSP has no supported coil position with clear shooting and blast approaches");
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
  const bool named_default_wraith=name=="wraith_minigun" || name=="wraith_mortar";
  const bool campaign_scenery=name=="campaign_flood_clump";
  const bool campaign_no_damage=name=="campaign_guntower_no_damage";
  const bool campaign=name=="campaign_c_turret_ap" || name=="campaign_h_turret_mp" ||
      name=="campaign_plasma_cannon" || name=="campaign_scorpion_cannon" ||
      name=="campaign_sentinel_emitter" || campaign_scenery || campaign_no_damage;
  const bool campaign_disposable_turret=name=="campaign_h_turret_mp" || name=="campaign_plasma_cannon";
  const std::string_view campaign_weapon_suffix=campaign_scenery?"\\flak_cannon":
      name=="campaign_plasma_cannon"?"\\smg":"\\rocket_launcher";
  const auto requested_suffix=campaign_scenery?std::string("flood_floor_clump_large"):
      campaign_no_damage?std::string("cov_guntower_turret"):campaign?name.substr(9):name;
  const auto campaign_parent_suffix=name=="campaign_scorpion_cannon"?std::string("scorpion"):requested_suffix;
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.local_players=1;options.source_object_world=true;options.mute=true;
  options.session_random_seed=0x22de570u;
  const std::map<std::string,std::string> attached_targets{
      {"warthog_chaingun","objects\\vehicles\\warthog\\turrets\\chaingun\\chaingun"},
      {"warthog_gauss_turret","objects\\vehicles\\warthog\\turrets\\gauss\\gauss"},
      {"scorpion_cannon_mp","objects\\vehicles\\scorpion\\turrets\\cannon\\cannon_mp"},
      {"campaign_scorpion_cannon","objects\\vehicles\\scorpion\\turrets\\cannon\\cannon"},
      {"wraith_mortar_mp","objects\\vehicles\\wraith\\turrets\\mortar\\mortar_mp"},
      {"wraith_mortar","objects\\vehicles\\wraith\\turrets\\mortar\\mortar"},
      {"wraith_minigun","objects\\vehicles\\wraith\\turrets\\minigun\\minigun"},
      {"spectre_plasma_turret","objects\\vehicles\\spectre\\turrets\\plasma\\plasma_turret"}};
  const auto attached=attached_targets.find(name);
  if(name=="ghost")options.variant.values.set(76,3);
  if(name=="spectre" || name=="spectre_plasma_turret")options.variant.values.set(76,5);
  if(name=="warthog_gauss" || name=="warthog_gauss_turret")options.variant.values.set(76,2);
  if(name=="scorpion" || name=="scorpion_cannon_mp")options.variant.values.set(78,1);
  if(name=="wraith" || name=="wraith_mortar_mp" || named_default_wraith)options.variant.values.set(78,2);
  Cache cache(options.map,options.shared_maps);check(cache.is_mcc(),"Destruction runtime requires an MCC v13 cache");
  auto content=cache.load_content(0,!campaign,source_game_variant(options.variant));
  std::optional<ScenarioObject> campaign_placement;
  std::uint32_t campaign_weapon=null_datum;
  if(campaign) {
    const auto at=std::ranges::find_if(content.objects,[&](const auto &placement){
      const auto tag=cache.query_tag({.handle=TagHandle{placement.definition}});
      return placement.type==(campaign_scenery?6:1) && placement.placement>=0 && !placement.model_attachment &&
          tag && tag->name.ends_with("\\"+campaign_parent_suffix);
    });
    check(at!=content.objects.end(),"Campaign destruction target has no authored scenario placement");
    campaign_placement=*at;
    check(at->origin_bsp>=0 && std::size_t(at->origin_bsp)<cache.bsps().size(),
        "Campaign target needs an admitted authored BSP before destruction stimulus");
    options.bsp=std::size_t(at->origin_bsp);options.campaign_difficulty=1;
    content=cache.load_content(options.bsp,false);
    const auto starts=[&] {
      if(!campaign_no_damage)
        return test::campaign_vehicle_observer_starts<1>(cache,content,options.bsp,*campaign_placement);
      report.data["campaign_initial_observer_survey"]=Json::array();
      for(const auto &placement:content.objects) {
        const auto tag=cache.query_tag({.handle=TagHandle{placement.definition}});
        if(placement.type!=1 || placement.placement<0 || placement.model_attachment ||
            placement.origin_bsp!=std::int16_t(options.bsp) || !tag ||
            !tag->name.ends_with("\\"+campaign_parent_suffix))continue;
        try {
          const auto result=test::campaign_vehicle_observer_starts<1>(cache,content,options.bsp,placement);
          campaign_placement=placement;
          report.data["campaign_initial_observer_survey"].push_back({{"placement",placement.placement},{"admitted",true}});
          return result;
        } catch(const FormatError &error) {
          report.data["campaign_initial_observer_survey"].push_back({{"placement",placement.placement},
              {"admitted",false},{"reason",error.what()}});
          auto &diagnostic=report.data["campaign_initial_observer_survey"].back();
          const CollisionWorld survey(content.collision);
          const auto atmosphere=cache.load_bsp_atmosphere(options.bsp);
          const auto radius=std::abs(placement.scale)*placement.bounding_radius;
          const auto center=placement.position+authored_basis(placement.rotation).rotate(placement.bounding_offset*placement.scale);
          unsigned floors{},steep{},outside{},overlap{},blocked{};
          for(unsigned n=0;n<128;++n) {
            const float angle=float(n%32)*6.28318530718f/32.f;
            const auto candidate=center+Vec3{std::cos(angle),std::sin(angle),0}*((radius+2*content.player.height)*float(1+n/32));
            auto origin=candidate+Vec3{0,0,2*radius+2*content.player.height};
            float reach=4*radius+8*content.player.height;
            for(unsigned layer=0;layer<16 && reach>0;++layer) {
              const auto floor=survey.raycast(origin,{0,0,-1},reach);if(!floor)break;++floors;
              const auto foot=floor->position+Vec3{0,0,content.player.radius};
              const auto location=source_object_point_location(atmosphere,std::int16_t(options.bsp),foot+Vec3{0,0,.5f*content.player.height});
              const auto eye=foot+Vec3{0,0,content.player.camera_height},sight=center-eye;
              if(floor->normal.z<content.player.maximum_slope_cosine)++steep;
              else if(location.cluster<0)++outside;
              else if(survey.overlaps(foot,content.player.height,content.player.radius))++overlap;
              else {
                if(length(sight)>0 && survey.raycast(eye,normalized(sight),length(sight)))++blocked;
                // Initial placement is a declared scene input. A center ray
                // can be blocked by the authored tower while another part of
                // the authored bound is visible. Actual projectile/request
                // evaluation, not this survey, decides the negative result.
                for(const auto offset:std::array{Vec3{0,0,0},Vec3{radius,0,0},Vec3{-radius,0,0},
                    Vec3{0,radius,0},Vec3{0,-radius,0},Vec3{0,0,radius},Vec3{0,0,-radius}}) {
                  const auto ray=center+offset-eye;const auto distance=length(ray);
                  if(!(distance>0) || survey.raycast(eye,ray/distance,distance))continue;
                  Spawn spawn;spawn.position=foot;spawn.facing=std::atan2(ray.y,ray.x);
                  campaign_placement=placement;
                  diagnostic.update({{"center_observer_admitted",false},{"admitted",true},
                      {"bound_survey",true},{"position",{foot.x,foot.y,foot.z}},
                      {"visible_bound_offset",{offset.x,offset.y,offset.z}}});
                  return std::array{spawn};
                }
              }
              reach-=floor->distance+content.player.radius*.1f;
              origin=floor->position-Vec3{0,0,content.player.radius*.1f};
            }
          }
          diagnostic.update({{"center",{center.x,center.y,center.z}},{"radius",radius},{"scale",placement.scale},
              {"floor_hits",floors},{"steep",steep},{"outside_cluster",outside},{"overlap",overlap},{"sight_blocked",blocked}});
        }
      }
      throw FormatError("No authored guntower placement admits the supported initial observer survey");
    }();
    options.native_spawn_inputs.assign(starts.begin(),starts.end());
    options.campaign_spawn_input=[starts](unsigned slot,bool initial)->std::optional<Spawn>{
      if(initial && slot<starts.size())return starts[slot];return {};
    };
    for(const auto &tag:cache.tags())if(tag.group=="weap" && tag.address && tag.name.ends_with(campaign_weapon_suffix)){
      check(campaign_weapon==null_datum,"Campaign equipment identity is ambiguous");campaign_weapon=tag.datum;
    }
    check(campaign_weapon!=null_datum,"Campaign map has no resident declared weapon");
    report.data["campaign_inputs"]={{"placement",campaign_placement->placement},{"definition",campaign_placement->definition},
        {"authored_flags",campaign_placement->flags},{"origin_bsp",campaign_placement->origin_bsp},{"difficulty",1},
        {"finite_empty_script_input",true},{"story_ai_or_campaign_completion_certified",false}};
  }
  bool selected=campaign;
  const bool prop=name=="power_module" || name=="fusion_coil" || name=="gas_container_full" ||
      name=="coil_to_warthog" || name=="coil_to_scenery" || name=="coil_to_crate";
  const std::string suffix=prop?"\\smg":"\\rocket_launcher";
  if(!campaign) {
    const auto choices=cache.multiplayer_weapon_choices();
    for(unsigned option=3;option<=20;++option){std::uint32_t seed{};
      const auto weapon=source_starting_weapon(null_datum,option,null_datum,choices,seed);
      if(weapon!=null_datum && content.weapons.at(weapon).name.ends_with(suffix)){
        options.starting_weapon=option;selected=true;break;
      }
    }
  }
  check(selected,"Map has no authored starting-weapon selection for this destruction regression");
  if(std::getenv("HALO2_DESTRUCTION_FIXED_CLOCK"))report.clock=std::make_shared<FixtureClock>();
  report.data["declared_external_clock"]=bool(report.clock);
  MultiplayerPreparedWorld world(options,campaign?std::optional<ScriptProgram>{ScriptProgram{}}:std::nullopt,report.clock);world.bind_source_local_views(world.local_slots);
  auto &source=*world.source_objects;auto &native=source.tu15_world();
  const auto slot=world.local_slots.at(0);
  std::optional<test::LivePhysicsEntryCaptureTu15> campaign_query_capture;
  SourcePhysicsEntryObserver campaign_area_observer;
  std::optional<SourcePhysicsEntryObserverBinding> campaign_query_observation;
  std::filesystem::path campaign_query_directory;
  std::uint32_t model_capture_target=null_datum;
  bool model_capture_active=false;
  const bool capture_record=name=="wraith_minigun" && std::getenv("HALO2_DESTRUCTION_CAPTURE_RECORD");
  const bool capture_model=std::getenv("HALO2_DESTRUCTION_CAPTURE_MODEL");
  const auto campaign_capture_entry=capture_model?0xd80a0u:capture_record?0x105460u:std::getenv("HALO2_DESTRUCTION_CAPTURE_AREA")?0xd91d0u:
      std::getenv("HALO2_DESTRUCTION_CAPTURE_DAMAGE")?0xe89b0u:0x304c90u;
  if(capture_model || capture_record || (campaign_scenery && (std::getenv("HALO2_DESTRUCTION_CAPTURE_COLLISION") ||
      std::getenv("HALO2_DESTRUCTION_CAPTURE_DAMAGE") || std::getenv("HALO2_DESTRUCTION_CAPTURE_AREA")))) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Campaign query capture needs its owned scratch");
    campaign_query_directory=std::filesystem::path(scratch)/
        (capture_model?"model-lookup-capture":capture_record?"minigun-record-capture":campaign_capture_entry==0xd91d0u?"flood-area-capture":
        campaign_capture_entry==0xe89b0u?"flood-damage-capture":"flood-query-capture");
    campaign_query_capture.emplace(campaign_query_directory,true);
    report.data["original_entry_capture"]={{"entry",campaign_capture_entry},{"directory",campaign_query_directory.string()}};
  }
  if(capture_model || campaign_scenery || campaign_no_damage || name=="wraith_minigun")campaign_query_observation.emplace(source.memory(),[&](bool before,std::uint32_t entry,const auto &abi){
    if(campaign_query_capture && entry==campaign_capture_entry) {
      if(capture_model && before)model_capture_active=model_capture_target!=null_datum && abi.gp[1]==model_capture_target &&
          (!campaign_no_damage || Bytes(source.memory().view(abi.gp[4],4)).u32(0)==0xda180u);
      if(!capture_model || model_capture_active)campaign_query_capture->observe(before,abi,source.memory(),entry);
      if(capture_model && !before)model_capture_active=false;
    }
    if(campaign_area_observer)campaign_area_observer(before,entry,abi);
  });
  if(campaign) {
    const auto game=Bytes(source.memory().view(0x4f3218u,4)).u32(0);
    const auto mode=Bytes(source.memory().view(game+8u,4)).u32(0);
    const auto difficulty=Bytes(source.memory().view(game+0x132u,2)).u16(0);
    report.data["campaign_inputs"]["actual_source_game_mode"]=mode;
    report.data["campaign_inputs"]["actual_source_difficulty"]=difficulty;
    check(mode==1 && difficulty==1,"Campaign destruction stimulus lost its original mode/difficulty admission");
    const auto unit=world.match.state().players[slot].source_unit;
    const auto weapon=native.equip_weapon(unit,campaign_weapon);
    check(weapon!=null_datum,"Original campaign equipment API did not attach the declared weapon");
    report.data["campaign_inputs"]["original_equipment_birth"]={{"unit",unit},{"weapon",weapon},{"definition",campaign_weapon}};
  }
  std::array<Controls,multiplayer_capacity> commands{};
  const auto initial_view=world.match.state();
  for(const auto local:world.local_slots){
    commands[local].yaw=initial_view.players[local].yaw;
    commands[local].pitch=initial_view.players[local].pitch;
  }
  for(unsigned tick=0;tick<60;++tick) {
    world.match.step(commands);
    if(campaign_query_capture && std::getenv("HALO2_DESTRUCTION_QUERY_CAPTURE_ONLY") &&
        std::filesystem::exists(campaign_query_directory/"capture.json")) {
      report.data["capture_only"]=true;
      report.data["capture_scope"]="Actual original entry input/output only; no destruction acceptance";
      report.save();return 0;
    }
  }
  if(name=="coil_to_warthog" || name=="warthog_to_coil"){
    reciprocal_damage(world,cache,content,slot,commands,name=="coil_to_warthog",report);return 0;
  }
  if(name=="coil_to_scenery" || name=="coil_to_crate"){
    reciprocal_damage(world,cache,content,slot,commands,true,report,
        name=="coil_to_scenery"?6:11,name=="coil_to_scenery"?"\\no_swimming":"\\military\\case\\case");return 0;
  }
  if(name=="terrain_glass"){terrain_breakage(world,cache,content,slot,commands,report);return 0;}
  const auto target_suffix=attached!=attached_targets.end()?attached->second:
      name=="warthog_gauss"?std::string("warthog"):requested_suffix;
  std::uint32_t target=null_datum,witness=null_datum,far_control=null_datum;
  std::vector<std::uint32_t> candidates;
  for(const auto &object:source.live_objects()){
    if((attached==attached_targets.end()?object.parent!=null_datum:object.parent==null_datum) ||
        (campaign_scenery?object.type!=6:prop?object.type!=11:object.type!=1))continue;
    const auto tag=cache.query_tag({.handle=TagHandle{object.definition}});
    if(tag && (attached!=attached_targets.end()?tag->name==target_suffix:
        tag->name.ends_with("\\"+target_suffix)))candidates.push_back(object.object);
  }
  if(named_default_wraith) {
    check(candidates.empty(),"Multiplayer Wraith unexpectedly created the requested default-variant child");
    // The original multiplayer variant declares only mortar_mp. Exercise the
    // separately authored default variant through B9960/B9B70, using the same
    // explicit constructor-input seam as vehicle_variants_world_tu15_tests.
    // The source constructor owns the mortar, both miniguns and their physics.
    const auto parent=std::ranges::find_if(cache.tags(),[](const auto &tag){
      return tag.group=="vehi" && tag.name=="objects\\vehicles\\wraith\\wraith";
    });
    check(parent!=cache.tags().end(),"Default Wraith constructor definition absent");
    const auto model=cache.load_initial_model_definition(parent->datum);
    check(cache.string_id(model.default_variant)=="default","Wraith default variant differs from original constructor input");
    const auto variant=std::ranges::find_if(model.variants,[&](const auto &row){return row.name==model.default_variant;});
    check(variant!=model.variants.end(),"Wraith default model variant absent");
    constexpr std::uint32_t packet=0x60010000u;
    auto initialize=native.prepare_source_call(std::array{parent->datum,null_datum,0u});
    initialize.gp[6]=packet;native.run_actual(0xb9960u,initialize);
    const auto input_word=[&](auto address,std::uint32_t value){
      const std::array bytes{std::byte(value),std::byte(value>>8),std::byte(value>>16),std::byte(value>>24)};
      native.write_source_call_input(address,bytes);
    };
    const auto position=world.match.state().players[slot].position+Vec3{0,0,.5f};
    input_word(packet+0xc,model.default_variant);
    input_word(packet+0x1c,std::bit_cast<std::uint32_t>(position.x));
    input_word(packet+0x20,std::bit_cast<std::uint32_t>(position.y));
    input_word(packet+0x24,std::bit_cast<std::uint32_t>(position.z));
    auto birth=native.prepare_source_call(std::array{packet});native.run_actual(0xb9b70u,birth);
    const auto created=birth.gp[0];
    report.data["declared_named_variant_birth"]={{"definition",parent->datum},{"variant",model.default_variant},
        {"position",{position.x,position.y,position.z}},{"actual_result",created},
        {"original_variant_contract","e29b94a392598c24142d51a33783d64732ad08ed0b3368e8d819ad6535421f06"}};
    check(created!=null_datum && source.object_definition(created)==parent->datum,
        "Original default Wraith constructor did not admit its declared definition");
    check(source.object_model_regions(created).selection.variant_index==std::distance(model.variants.begin(),variant),
        "Original Wraith constructor selected another model variant");
    for(const auto &child:source.live_objects())if(child.parent==created && child.type==1) {
      const auto tag=cache.query_tag({.handle=TagHandle{child.definition}});
      if(tag && tag->name==attached->second)candidates.push_back(child.object);
    }
    check(candidates.size()==(name=="wraith_minigun"?2u:1u),
        "Original default Wraith constructor did not create its declared child count");
  }
  if(campaign && candidates.empty()) {
    // An initially-not-placed campaign row needs an explicit runtime creation
    // input. C1120 reads the unchanged original scenario row and palette;
    // it owns admission, constructor flags, physics, vitality and the result.
    const auto created=native.create_scenario_object(*campaign_placement,SourceScenarioBirth::runtime);
    report.data["campaign_inputs"]["explicit_original_runtime_creation"]=created;
    check(created!=null_datum,"Original campaign runtime creation did not admit the authored target");
    check(source.object_definition(created)==campaign_placement->definition,
        "Original campaign birth selected a different target definition");
    if(attached==attached_targets.end())candidates.push_back(created);
    else for(const auto &child:source.live_objects())if(child.parent==created && child.type==1) {
      const auto tag=cache.query_tag({.handle=TagHandle{child.definition}});
      if(tag && tag->name==attached->second)candidates.push_back(child.object);
    }
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
  std::optional<ModelCollision> legacy_bullet_collision;
  std::optional<ModelCollision> negative_aim_collision;
  if(campaign_no_damage) {
    const auto object=cache.load_source_tag_metadata(target_definition);
    const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38u));
    check(model.group()=="hlmt","Negative target lost its authored model owner");
    negative_aim_collision=cache.load_model_collision(model.datum());
  }
  if(campaign_scenery) {
    const auto object=cache.load_source_tag_metadata(target_definition);
    const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
    check(model.group()=="hlmt","Campaign scenery lost its authored model owner");
    const Bytes head(model.bytes_at(model.source_address(),model.size()));
    check(head.u32(0x60)==1,"Flood-clump damage shape differs from original contract");
    const Bytes info(cache.read_tag_bytes(head.u32(0x64),0xf8));
    check(info.u32(0xbc)==1,"Flood-clump section shape differs from original contract");
    const Bytes section(cache.read_tag_bytes(info.u32(0xc0),0x38));
    check(section.u32(4)==0 && section.u32(0xc)==1,"Flood-clump section/response differs from original contract");
    const Bytes response(cache.read_tag_bytes(section.u32(0x10),0x50));
    check(response.u32(4)==0x80 && response.u32(0xc)==0x65666665 && response.u32(0x10)==null_datum,
        "Native Flood-clump response did not receive the original literal null effect");
    report.data["original_scenery_contract"]={{"section_flags",0},{"response_flags",0x80},
        {"effect_group",0x65666665},{"effect",null_datum},
        {"original_contract","078d0e9df82268e8a7f6980d04a02cf14b4df20d2ce97fa61c473c849f5b5204"}};
  }
  if(campaign_disposable_turret) {
    // The retained original/MCC census has this exact response tuple for
    // both legacy fixed turrets. Require its literal authored input before
    // testing the observed original death/retirement path. No flag meaning
    // or destruction result is supplied by the fixture.
    const auto object=cache.load_source_tag_metadata(target_definition);
    const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38u));
    check(model.group()=="hlmt","Legacy turret has a different model owner");
    if(name=="campaign_plasma_cannon")legacy_bullet_collision=cache.load_model_collision(model.datum());
    const Bytes head(model.bytes_at(model.source_address(),model.size()));
    check(head.u32(0x60u)==1,"Legacy turret damage shape differs from original contract");
    const Bytes info(cache.read_tag_bytes(head.u32(0x64u),0xf8u));
    check(info.u32(0xbcu)==1,"Legacy turret section shape differs from original contract");
    const Bytes section(cache.read_tag_bytes(info.u32(0xc0u),0x38u));
    check(section.u32(4)==6 && section.u32(0xcu)==(name=="campaign_h_turret_mp"?1u:2u),
        "Legacy turret section/response input differs from original contract");
    const Bytes response(cache.read_tag_bytes(section.u32(0x10u),0x50u));
    for(const auto [offset,expected]:std::array<std::pair<unsigned,unsigned>,8>{{
        {0,0},{4,0x81},{8,0},{0x20,4},{0x2c,0},{0x30,0x65666665},{0x44,0},{0x4c,0}}})
      check(response.u32(offset)==expected,"Legacy turret response tuple differs from original contract");
    report.data["legacy_turret_contract"]={{"section_flags",6},{"first_response_flags",0x81},
        {"original_contract",name=="campaign_h_turret_mp"?
          "6d00062e3682e4a48f7f9c9577165c7273ef6438f1fe89b955b033bff0375318":
          "0bd0486ecc08a48c0a4e7411aec43ee87cc4b0b1d2ef7811a2e45ed70196f69a"}};
  }
  if(attached!=attached_targets.end()) {
    const auto objects=source.live_objects();
    const auto found=std::ranges::find_if(objects,[&](const auto &object){return object.object==target;});
    check(found!=objects.end() && found->parent!=null_datum,"Attached destruction target lost its original parent");
    report.data["actual_attached_target"]={{"name",attached->second},{"object",target},
        {"parent",found->parent},{"parent_definition",source.object_definition(found->parent)}};
  }
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
  const auto vehicle_lane_distances=name=="campaign_plasma_cannon"?
      std::array{2.f,3.f,4.f,6.f,8.f,12.f}:std::array{12.f,8.f,6.f,4.f,3.f,2.f};
  const bool surveyed_vehicle_lane=campaign_disposable_turret || campaign_no_damage || name=="campaign_sentinel_emitter";
  if(!prop)for(float distance:vehicle_lane_distances) {
  if(distance!=12.f && !surveyed_vehicle_lane)break;
  for(unsigned side=0;side<32 && !clear;++side){
    const float angle=float(side)*.19634954084936207f;
    auto candidate=sphere.center-(root.basis[0]*std::cos(angle)+root.basis[1]*std::sin(angle))*distance;
    auto origin=candidate+Vec3{0,0,10};float remaining=20;
    for(unsigned layer=0;layer<(surveyed_vehicle_lane?8u:1u) && remaining>0;++layer) {
      const auto floor=terrain.raycast(origin,{0,0,-1},remaining);if(!floor)break;
      candidate.z=floor->position.z+content.player.radius;
      const bool supported=!surveyed_vehicle_lane ||
          (floor->normal.z>=content.player.maximum_slope_cosine &&
           !terrain.overlaps(candidate,content.player.height,content.player.radius));
      const auto eye=candidate+Vec3{0,0,content.player.camera_height},delta=sphere.center-eye;
      bool visible=false;
      if(supported) {
        if(campaign_no_damage) {
          const auto radius=sphere.radius;
          for(const auto offset:std::array{Vec3{0,0,0},Vec3{radius,0,0},Vec3{-radius,0,0},
              Vec3{0,radius,0},Vec3{0,-radius,0},Vec3{0,0,radius},Vec3{0,0,-radius}}) {
            const auto ray=sphere.center+offset-eye;const auto distance=length(ray);
            if(distance>0 && !terrain.raycast(eye,ray/distance,distance)){visible=true;break;}
          }
        } else visible=!terrain.raycast(eye,normalized(delta),length(delta));
      }
      if(visible) {
        spawn.position=candidate;clear=true;
        report.data["declared_lane"]={{"distance",distance},{"floor_layer",layer},
            {"floor_normal",{floor->normal.x,floor->normal.y,floor->normal.z}},
            {"scope","Supported cached BSP initial scene input; no impact or damage result supplied"}};
        break;
      }
      const auto step=content.player.radius*.1f;remaining-=floor->distance+step;
      origin=floor->position-Vec3{0,0,step};
    }
  }
  if(clear)break;
  }
  report.data["target_pose_before_lane"]={{"root",{root.position.x,root.position.y,root.position.z}},
      {"center",{sphere.center.x,sphere.center.y,sphere.center.z}},{"radius",sphere.radius},
      {"authored_campaign_placement",campaign_placement?Json(campaign_placement->placement):Json(nullptr)}};
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
    current_eye=position;const auto bound=source.object_sphere(target);current_target=bound.center;
    const auto aiming_collision=legacy_bullet_collision?&*legacy_bullet_collision:
        negative_aim_collision?&*negative_aim_collision:nullptr;
    if(aiming_collision) {
      const auto nodes=source.world_nodes(target);
      const auto active=source.object_model_regions(target).selection.permutations;
      check(!nodes.empty() && content.original_data,"Legacy bullet aiming lost its actual source pose/data");
      unsigned queries{};
      std::optional<std::pair<Vec3,PosedCollisionHit>> chosen;
      const auto admit=[&](Vec3 point) {
        ++queries;const auto ray=(point-position)*2.f;
        if(const auto hit=posed_collision(*aiming_collision,nodes,active,*content.original_data,position,ray,2u)) {
          const auto surface=position+ray*hit->fraction;
          const auto sight=surface-position;const auto reach=length(sight);
          if(reach>.01f && !terrain.raycast(position,sight/reach,reach-.01f)) {
            chosen=std::pair{surface,*hit};return true;
          }
        }
        return false;
      };
      const bool center_hit=admit(bound.center);
      if(!chosen)for(const auto &node:nodes)if(admit(node.position))break;
      if(!chosen)for(float z:{0.f,-.25f,.25f,-.5f,.5f,-.75f,.75f}) {
        for(float x:{0.f,-.25f,.25f,-.5f,.5f,-.75f,.75f}) {
          for(float y:{0.f,-.25f,.25f,-.5f,.5f,-.75f,.75f})
            if(admit(bound.center+(nodes.front().basis[0]*x+nodes.front().basis[1]*y+nodes.front().basis[2]*z)*bound.radius))break;
          if(chosen)break;
        }
        if(chosen)break;
      }
      const auto aim_key=campaign_no_damage?"negative_posed_aim":"legacy_bullet_aim";
      report.data[aim_key]={{"center_ray_admitted",center_hit},{"queries",queries},
          {"posed_surface_found",bool(chosen)},{"scope","Read-only current posed collision/BSP input aiming; projectile hits and damage are not supplied"}};
      check(bool(chosen),"Legacy bullet fixture has no clear actual posed collision surface");
      current_target=chosen->first;const auto &hit=chosen->second;
      check(hit.model_material>=0 && unsigned(hit.model_material)<aiming_collision->materials.size(),
          "Legacy posed input hit has an invalid authored model material");
      report.data[aim_key].update({{"point",{current_target.x,current_target.y,current_target.z}},
          {"region",hit.region},{"node",hit.node},{"surface",hit.surface},{"model_material",hit.model_material},
          {"global_material",hit.global_material},
          {"authored_damage_section",aiming_collision->materials[hit.model_material].damage_section}});
    }
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
  const auto target_body=source.source_object_body(target);
  const auto target_pool=source.source_object_pool();
  model_capture_target=target;
  check(!(word(target_body+0x108u)&0x40000u),"Destruction target already carries the original dead flag");
  report.data["original_dead_flag_stores"]=Json::array();
  bool dead=false,retired=false,regions_changed=false;unsigned shots=0;std::uint64_t frontier=0;
  std::optional<unsigned> death_tick;
  const auto regions=source.object_model_regions(target);
  const auto control_before=chain?source.object_vitality(far_control):MultiplayerSourceVitality{};
  std::uint32_t witness_body=chain?source.source_object_body(witness):0;
  bool witness_pristine_at_target_blast=false,witness_retired=false;
  float witness_body_at_target_blast=1.f;
  std::map<std::uint32_t,unsigned> destruction_requests;
  std::vector<Json> active_area_calls;
  if(campaign_no_damage) {
    const auto tags=word(0x4f0414u);
    const auto definition=word(tags+(target_definition&0xffffu)*16u+8u);
    const auto model=word(definition+0x38u);
    check(model!=null_datum,"Negative guntower target has no authored model");
    const auto model_body=word(tags+(model&0xffffu)*16u+8u);
    const auto count=word(model_body+0x60u);
    check(count==0,"Negative guntower target differs from its original absent damage-information contract");
    report.data["authored_no_damage_information"]={{"definition",target_definition},{"model",model},{"actual_count",count}};
    report.data["actual_target_damage_requests"]=Json::array();
    report.data["actual_target_model_rejections"]=Json::array();
    report.data["native_lookup_observer_entries"]=0;
    campaign_area_observer=[&,active=false](bool before,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi &abi)mutable{
      if(entry!=0xd80a0u)return;
      if(before) {
        report.data["native_lookup_observer_entries"]=report.data["native_lookup_observer_entries"].get<unsigned>()+1;
        active=abi.gp[1]==target && word(abi.gp[4])==0xda180u;
        if(!active)return;
        const SourceRawDatumPoolServices readback{.read=[&](auto address,auto size){return memory.view(address,size);}};
        const auto header=source_raw_datum_lookup(target_pool,target,readback);
        check(header && word(header+8u)==target_body && word(target_body)==target_definition,
            "Negative model lookup lost its full target generation/body/definition");
        report.data["actual_target_model_rejections"].push_back({{"object",target},{"body",target_body},
            {"definition",target_definition},{"caller_return",word(abi.gp[4])},{"full_generation_live",true},
            {"returned",false}});
      } else if(active) {
        check(abi.gp[0]==0,"Original no-information target unexpectedly resolved a damage block");
        auto &event=report.data["actual_target_model_rejections"].back();
        event["returned"]=true;event["actual_result"]=abi.gp[0];active=false;
      }
    };
  } else
  if(campaign_scenery) {
    report.data["actual_original_area_calls"]=Json::array();
    report.data["original_area_attributed_death_stores"]=Json::array();
    campaign_area_observer=[&](bool before,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi &abi){
      if(entry!=0xd91d0u)return;
      if(before) {
        const auto request=word(abi.gp[4]+4u),caller=word(abi.gp[4]);
        Json words=Json::array();for(unsigned offset=0;offset<0x60;offset+=4)words.push_back(word(request+offset));
        Json event{{"caller_return",caller},{"request",request},{"damage",word(request)},
            {"gp",abi.gp},{"request_words",words}};
        // Original 101330 loads its full projectile argument into ESI at
        // 101AB2, then calls D91D0 at 101B84. Verify its live generation and
        // EBP body before attaching that identity to this observed call.
        if(caller==0x101b89u) {
          const SourceRawDatumPoolServices readback{.read=[&](auto address,auto size){return memory.view(address,size);}};
          const auto header=source_raw_datum_lookup(target_pool,abi.gp[6],readback);
          if(header && word(header+8)==abi.gp[5]) {
            event["projectile"]=abi.gp[6];event["projectile_definition"]=word(abi.gp[5]);
          }
        }
        active_area_calls.push_back(event);
        report.data["actual_original_area_calls"].push_back(event);
      } else {
        check(!active_area_calls.empty(),"Original area-damage observation became unbalanced");
        active_area_calls.pop_back();
      }
    };
  } else if(name=="wraith_minigun") {
    report.data["actual_native_record_calls"]=Json::array();
    campaign_area_observer=[&](bool before,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi &abi){
      if(!before || entry!=0x105460u)return;
      const SourceRawDatumPoolServices readback{.read=[&](auto address,auto size){return memory.view(address,size);}};
      const auto header=source_raw_datum_lookup(target_pool,abi.gp[0],readback);
      check(header,"Original indexed-record call lost its full object generation");
      const auto body=word(header+8u);
      const auto target_header=source_raw_datum_lookup(target_pool,target,readback);
      bool target_weapon=false;std::int32_t selected_weapon=-1;
      if(target_header && word(target_header+8u)==target_body && word(target_body)==target_definition &&
          Bytes(memory.view(target_header+6u,2)).u16(0)>0x212u) {
        // Actual DC8EE/DC8FF select the unit's current weapon from +212/+218
        // before DC923 calls 105460 on that weapon. Check the stored full
        // handle and the unit's real extent; a weapon is a separate object.
        selected_weapon=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(memory.view(target_body+0x212u,1).front()));
        const auto bytes=Bytes(memory.view(target_header+6u,2)).u16(0);
        if(selected_weapon>=0 && 0x218u+std::uint64_t(selected_weapon)*4u+4u<=bytes)
          target_weapon=word(target_body+0x218u+std::uint32_t(selected_weapon)*4u)==abi.gp[0];
      }
      report.data["actual_native_record_calls"].push_back({{"object",abi.gp[0]},{"definition",word(body)},
          {"body",body},{"selector",abi.gp[1]},{"full_generation_live",true},
          {"target_selected_weapon_slot",selected_weapon},{"is_target_selected_weapon",target_weapon},
          {"is_target",abi.gp[0]==target && body==target_body && word(body)==target_definition}});
    };
  }
  std::optional<std::pair<std::uint32_t,unsigned>> legacy_sections;
  const auto raw_bytes=[&](std::uint32_t address,unsigned size) {
    Json result=Json::array();for(const auto byte:memory.view(address,size))result.push_back(std::to_integer<unsigned>(byte));return result;
  };
  if(legacy_bullet_collision) {
    const Bytes descriptor(memory.view(target_body+0x120u,4));
    const auto size=std::bit_cast<std::int16_t>(descriptor.u16(0)),offset=std::bit_cast<std::int16_t>(descriptor.u16(2));
    check(size>0 && size<=64 && size%8==0 && offset>=0,"Legacy section diagnostic has a different original descriptor");
    legacy_sections=std::pair{target_body+unsigned(offset),unsigned(size)};
    report.data["legacy_section_storage_before"]=raw_bytes(legacy_sections->first,legacy_sections->second);
    report.data["legacy_section_changed_store_count"]=0;
    report.data["legacy_section_changed_stores"]=Json::array();
  }
  if(legacy_bullet_collision) {
    report.data["legacy_direct_damage_request_count"]=0;
    report.data["legacy_direct_damage_target_request_count"]=0;
    report.data["legacy_direct_damage_requests"]=Json::array();
  }
  const auto observe_destruction=[&](std::uint32_t,const SourcePhysicsContactRecordStore &store){
    if(campaign_no_damage && store.instruction==0xda0eeu) {
      check(store.written.size()==1 && store.address>=0xbu,"Original damage request prologue differs");
      // Authenticated DA0D0 frame, also used by the legacy direct-hit observer:
      // DA0EE writes ESP+B after SUB ESP,F4 and two saved registers.
      const auto stack=store.address-0xbu;
      const auto request=word(stack+0x100u),recipient=word(stack+0x104u);
      if(recipient==target) {
        const auto damage=word(request);
        const auto tag=cache.query_tag({.handle=TagHandle{damage}});
        check(tag && tag->group=="jpt!","Negative target request lost its actual damage-effect definition");
        if(report.data["actual_target_damage_requests"].size()<128)
          report.data["actual_target_damage_requests"].push_back({{"target",recipient},{"damage",damage},
              {"damage_name",tag->name},{"source_player",word(request+8u)},{"source_object",word(request+0xcu)}});
      }
    }
    if(legacy_bullet_collision && store.instruction==0xda0eeu) {
      // Unchanged TU1.5 DA0D0: SUB ESP,F4; PUSH EBX; PUSH EBP;
      // DA0EE writes AL at ESP+B. Actual caller request/target arguments
      // are consequently ESP+100/+104. Read them without supplying a hit.
      check(store.written.size()==1 && store.address>=0xbu,"Original direct-damage prologue store differs");
      const auto stack=store.address-0xbu;
      const auto request=word(stack+0x100u),recipient=word(stack+0x104u);
      report.data["legacy_direct_damage_request_count"]=report.data["legacy_direct_damage_request_count"].get<unsigned>()+1;
      if(recipient==target)
        report.data["legacy_direct_damage_target_request_count"]=report.data["legacy_direct_damage_target_request_count"].get<unsigned>()+1;
      if(report.data["legacy_direct_damage_requests"].size()<128)
        report.data["legacy_direct_damage_requests"].push_back({{"instruction",store.instruction},
            {"target",recipient},{"definition",word(request)},{"flags",word(request+4u)},
            {"source_player",word(request+8u)},{"source_object",word(request+0xcu)},
            {"scale_a_bits",word(request+0x54u)},{"scale_b_bits",word(request+0x58u)},
            {"strength_bits",word(request+0x5cu)},{"distance_bits",word(request+0x60u)},
            {"raw_arguments",{word(stack+0x108u),word(stack+0x10cu),word(stack+0x110u)}}});
    }
    if(legacy_sections && store.before!=store.written) {
      const auto low=std::max(std::uint64_t(store.address),std::uint64_t(legacy_sections->first));
      const auto high=std::min(std::uint64_t(store.address)+store.written.size(),
          std::uint64_t(legacy_sections->first)+legacy_sections->second);
      if(low<high) {
        report.data["legacy_section_changed_store_count"]=report.data["legacy_section_changed_store_count"].get<unsigned>()+1;
        if(report.data["legacy_section_changed_stores"].size()<128) {
          Json old=Json::array(),now=Json::array();
          for(auto at=low;at<high;++at) {
            old.push_back(std::to_integer<unsigned>(store.before.at(at-store.address)));
            now.push_back(std::to_integer<unsigned>(store.written.at(at-store.address)));
          }
          report.data["legacy_section_changed_stores"].push_back({{"instruction",store.instruction},
              {"relative_offset",low-legacy_sections->first},{"before",old},{"after",now}});
        }
      }
    }
    const auto flag=target_body+0x10au;
    if(store.address<=flag && flag-store.address<store.written.size()) {
      const auto offset=flag-store.address;
      if(!(std::to_integer<unsigned>(store.before.at(offset))&4u) &&
          (std::to_integer<unsigned>(store.written.at(offset))&4u)) {
        if(campaign_scenery || named_default_wraith) {
          const SourceRawDatumPoolServices readback{.read=[&](auto address,auto size){return memory.view(address,size);}};
          const auto header=source_raw_datum_lookup(target_pool,target,readback);
          if(!header || word(header+8)!=target_body || word(target_body)!=target_definition)return;
        }
        Json death{{"instruction",store.instruction},
            {"address",store.address},{"before",std::to_integer<unsigned>(store.before[offset])},
            {"after",std::to_integer<unsigned>(store.written[offset])}};
        if(campaign_scenery || named_default_wraith)death.update({{"object",target},
            {"definition",target_definition},{"full_generation_live",true}});
        report.data["original_dead_flag_stores"].push_back(death);
        if(campaign_scenery && !active_area_calls.empty()) {
          auto event=active_area_calls.back();event.update({{"object",target},{"definition",target_definition},
              {"full_generation_live",true},{"instruction",store.instruction}});
          report.data["original_area_attributed_death_stores"].push_back(event);
        }
      }
    }
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
  };
  source.observe_physics_stores(observe_destruction);
  StoreObservation observation{source};
  report.data["initial_body"]=initial_vitality.body;
  bool pending_qualified=false;
  const auto native_difficulty_firing_before=source_difficulty_parameter_native_activity();
  for(unsigned tick=0;tick<900;++tick){
    auto state=world.match.state();
    check(!state.players[slot].dead && state.players[slot].source_unit==unit,
        "Destruction stimulus lost its actual live shooter");
    const auto live=source.live_objects();
    const bool exists=std::ranges::any_of(live,[&](const auto &object){return object.object==target;});
    if(exists)aim();
    commands[slot].fire=exists && !dead && (chain?tick%10==0:(prop || name=="campaign_plasma_cannon" || tick%30==0));
    commands[slot].reload=exists && !dead && state.players[slot].loaded==0;
    const auto previous_shot=state.players[slot].shot_tick;
    world.match.step(commands);source.advance_effect_frame(tick_seconds);state=world.match.state();
    if(campaign_query_capture && std::getenv("HALO2_DESTRUCTION_QUERY_CAPTURE_ONLY") &&
        std::filesystem::exists(campaign_query_directory/"capture.json")) {
      report.data["capture_only"]=true;
      report.data["capture_scope"]="Actual original entry input/output only; no destruction acceptance";
      report.save();return 0;
    }
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
      // OriginalD8D8A publishes death independently of the remaining body
      // fraction. Attached damage-sharing units may retain a positive floor.
      // Observe that original transition rather than imposing a host threshold.
      dead|=bool(flags&4u);
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
      if(chain && !pending_qualified && std::getenv("HALO2_DESTRUCTION_PENDING_CONTINUATION")) {
        // Literal D73E0: object+120 is the signed byte size of 8-byte
        // section states; +122 is their signed body-relative offset. The
        // pending word is section+4, with countdown above the low3 bits.
        const auto [size,offset]=[&]{
          const Bytes descriptor(memory.view(target_body+0x120u,4));
          return std::pair{std::bit_cast<std::int16_t>(descriptor.u16(0)),
              std::bit_cast<std::int16_t>(descriptor.u16(2))};
        }();
        check(size>=0 && size%8==0 && offset>=0,"Original damage-section storage descriptor differs");
        Json pending=Json::array();
        for(unsigned section=0;section<unsigned(size)/8;++section) {
          const auto word=Bytes(memory.view(target_body+unsigned(offset)+section*8u+4u,2)).u16(0);
          if(word>>3u)pending.push_back({{"section",section},{"pending_word",word}});
        }
        if(!pending.empty()) {
          check(std::getenv("HALO2_DESTRUCTION_CONTINUATION"),"Pending cut requires complete-world continuation qualification");
          report.data["actual_pending_response_cut"]={{"tick",state.tick},{"target",target},{"sections",pending}};
          destruction_continuation(world,commands,report,"pending_response_continuation",true);
          source.observe_physics_stores(observe_destruction);pending_qualified=true;
        }
      }
    }
    for(const auto &event:state.grenade_detonations)if(event.serial>frontier){
      frontier=event.serial;
      report.data["detonations"].push_back({{"tick",event.tick},{"projectile",event.projectile},
          {"definition",event.definition},{"effect_instance",event.source_effect_instance},
          {"position",{event.position.x,event.position.y,event.position.z}},
          {"target_center",{current_target.x,current_target.y,current_target.z}}});
      if(campaign_scenery)report.data["detonations"].back()["effect"]=event.effect;
    }
    if(tick%30==0){report.data["progress_tick"]=state.tick;report.save();}
    if(campaign_no_damage && shots>=2 && !report.data["detonations"].empty() &&
        !report.data["actual_target_damage_requests"].empty() && !report.data["actual_target_model_rejections"].empty())break;
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
  if(campaign_scenery) {
    check(active_area_calls.empty(),"Original area-damage dispatch has not returned");
    campaign_area_observer={};
  }
  if(name=="wraith_minigun")campaign_area_observer={};
  if(campaign_no_damage)campaign_area_observer={};
  const auto native_difficulty_firing_after=source_difficulty_parameter_native_activity();
  report.data["native_difficulty_firing_activity"]={
      {"parameter_calls",native_difficulty_firing_after.parameter_calls-native_difficulty_firing_before.parameter_calls},
      {"current_difficulty_calls",native_difficulty_firing_after.current_difficulty_calls-native_difficulty_firing_before.current_difficulty_calls}};
  report.data.update({{"shots",shots},{"minimum_body",minimum},{"source_dead",dead},
      {"target_retired",retired},{"regions_changed",regions_changed},
      {"source_projectiles",projectiles},{"source_debris",created_debris},
      {"original_destruction_emitter_stores",destruction_requests},
      {"witness_pristine_at_target_blast",witness_pristine_at_target_blast},
      {"witness_body_at_target_blast",witness_body_at_target_blast},
      {"chain_witness_retired",witness_retired}});
  if(legacy_sections && !retired)
    report.data["legacy_section_storage_after"]=raw_bytes(legacy_sections->first,legacy_sections->second);
  check(shots>0,"Ordinary source weapon commands produced no shots");
  if(campaign_no_damage) {
    check(shots>=2 && !projectiles.empty() && !report.data["detonations"].empty(),
        "Negative target received no ordinary source rocket/explosion stimulus");
    check(!report.data["actual_target_damage_requests"].empty() &&
        !report.data["actual_target_model_rejections"].empty() &&
        std::ranges::all_of(report.data["actual_target_model_rejections"],[](const auto &event){
          return event.at("returned").template get<bool>() && event.at("actual_result")==0 &&
              event.at("caller_return")==0xda180u &&
              event.at("full_generation_live").template get<bool>();}),
        "Negative result has no actual full-target dispatcher/model-lookup rejection");
    check(std::ranges::any_of(report.data["actual_target_damage_requests"],[&](const auto &request){
          return request.at("source_player")==world.match.state().players[slot].source_player &&
              request.at("source_object")==unit;
        }),"Negative target has no damage request from the actual full shooter/player handles");
    check(!dead && !retired && !regions_changed && minimum==initial_vitality.body &&
        source.object_vitality(target).body==initial_vitality.body &&
        source.object_vitality(target).shield==initial_vitality.shield &&
        report.data["original_dead_flag_stores"].empty() && !destruction_requests.contains(target),
        "Authored no-information target changed vitality, regions, generation or death state");
    report.data["original_no_damage_information_gate_verified"]=true;
    report.data["campaign_acceptance_scope"]="Original campaign placement, ordinary rockets, actual full-target damage dispatcher and native model lookup returning no authored damage information, unchanged vitality/regions and exact continuation; other producers and full-definition gates remain open";
    commands[slot].fire=commands[slot].reload=false;
    destruction_continuation(world,commands,report);report.data["passed"]=true;
    std::cout<<"PASS authored no-damage-information gate target="<<name<<" shots="<<shots<<'\n';return 0;
  }
  const bool recorded_minigun_death=name=="wraith_minigun" && dead && regions_changed &&
      !report.data["original_dead_flag_stores"].empty();
  check(minimum<initial_vitality.body || retired || recorded_minigun_death,
      "Ordinary source projectiles never damaged the requested target");
  check(dead || retired,"Actual source damage did not complete the target death transition");
  if(dead)check(!report.data["original_dead_flag_stores"].empty(),
      "Dead target has no observed original source death-flag transition");
  if(prop)check(retired,"Explosive death did not deliver its delayed destruction/retirement response");
  if(chain){
    if(std::getenv("HALO2_DESTRUCTION_PENDING_CONTINUATION"))
      check(pending_qualified,"Module chain never reached an actual pending-response cut");
    check(witness_pristine_at_target_blast,"Chain witness was already damaged before the target's original blast");
    check(witness_retired && destruction_requests[target]==1 && destruction_requests[witness]==1,
        "Original module blast did not cause one source-owned chained witness explosion");
    const auto control_after=source.object_vitality(far_control);
    check(control_after.body==control_before.body && control_after.shield==control_before.shield &&
        !destruction_requests.contains(far_control),"Distant source module was damaged or emitted a fabricated chain blast");
    report.data["far_control_unchanged"]=true;
  }
  if(!prop && !campaign && attached==attached_targets.end() && name!="h_turret_ap")
    check(!created_debris.empty(),"Vehicle death produced no source-created debris");
  if(name=="h_turret_ap") {
    // The original destroyed_explosion graph has light, sound, decal and
    // particle systems, with no authored object part. Particle fragments
    // require their separate presentation/physics gate, not a fabricated bloc.
    check(regions_changed && destruction_requests[target]==1,
        "Fixed human turret did not change its original regions and emit its authored destruction blast");
    report.data["fixed_turret_acceptance_scope"]="Original death/region/blast; authored particle fragments, sound and complete visual delivery remain separate";
    report.data["object_debris_contract_evidence"]="human-turret-object-parts-v1.json";
  }
  if(attached!=attached_targets.end()) {
    check(regions_changed,"Attached target death did not change its actual authored model regions");
    report.data["attached_acceptance_scope"]="Original death-flag transition and model-region change under ordinary rockets; full parent/debris/occupant gates remain separate";
    if(name=="wraith_minigun") {
      check(std::ranges::any_of(report.data["actual_native_record_calls"],[](const auto &call){
        return call.at("is_target").template get<bool>() || call.at("is_target_selected_weapon").template get<bool>();}),
          "Native indexed-record callback did not receive the full target or its actual selected weapon");
      report.data["global_vitality_unchanged_original_death"]=minimum==initial_vitality.body;
      report.data["attached_acceptance_scope"]="Original named-default child death/regions and actual full target/selected-weapon native indexed-record callback under ordinary rockets; direct/cascade causality, full parent, section and material gates remain open";
    }
  }
  if(campaign_disposable_turret) {
    check(retired && !report.data["original_dead_flag_stores"].empty() && !created_debris.empty(),
        "Legacy turret did not deliver original death, retirement and authored object parts");
    report.data["campaign_acceptance_scope"]="Actual campaign mode, ordinary resident weapon, original death flag, full target retirement and object-part births; occupants, complete response/presentation and transport remain separate";
  } else if(campaign_scenery) {
    check(!report.data["original_dead_flag_stores"].empty(),"Flood-clump damage has no original death transition");
    bool matched=false;
    for(auto &death:report.data["original_area_attributed_death_stores"]) {
      const auto damage=cache.query_tag({.handle=TagHandle{death.at("damage").get<std::uint32_t>()}});
      check(damage && damage->group=="jpt!","Observed original area call has no actual damage-effect definition");
      death["damage_name"]=damage->name;
      for(const auto &event:report.data["detonations"])
        matched|=death.contains("projectile") && death.at("projectile")==event.at("projectile") &&
            death.at("projectile_definition")==event.at("definition");
    }
    check(matched,"Flood-clump death was not inside its actual projectile's original area-damage call");
    report.data["original_explosion_death_attribution_verified"]=true;
    report.data["source_debris_scope"]="New class11 objects in this world; target ownership is not established by this set";
    report.data["campaign_acceptance_scope"]="Actual campaign scenery, original resident fuel-rod equipment and damage/death, original null effect input and exact continuation; lifetime, complete effect delivery and transport remain separate";
  } else if(campaign) {
    check(regions_changed,"Campaign target death did not change its authored model regions");
    report.data["campaign_acceptance_scope"]="Original campaign mode, authored runtime creation, ordinary rocket death and region changes; debris, occupants, lifetime, story, transport and presentation remain separate";
  }
  destruction_continuation(world,commands,report);explicit_cleanup_receipt(world,target,report);
  if(std::getenv("HALO2_DESTRUCTION_REQUIRE_NATIVE_RATES")) {
    const auto rates=source_vehicle_native_rate_activity();
    check(rates.live_cursor_calls>report.native_rates_before.live_cursor_calls,
        "Actual source composition admitted no qualified native vehicle-rate kernel");
  }
  if(name=="wraith_minigun")check(source_damage_record_native_activity().calls>report.native_record_before.calls,
      "Wraith minigun damage executed no native indexed-record kernel");
  if(std::getenv("HALO2_DESTRUCTION_REQUIRE_NATIVE_MODEL"))
    check(source_damage_model_native_activity().calls>report.native_model_before.calls,
        "Actual source composition executed no native model-damage lookup");
  if(std::getenv("HALO2_DESTRUCTION_REQUIRE_NATIVE_MATERIAL") && name!="campaign_guntower_no_damage")
    check(source_damage_material_native_activity().calls>report.native_material_before.calls,
        "Actual source composition executed no native material-damage lookup");
  if(std::getenv("HALO2_DESTRUCTION_REQUIRE_NATIVE_DIFFICULTY") && name!="campaign_guntower_no_damage")
    check(source_difficulty_parameter_native_activity().parameter_calls>native_difficulty_firing_before.parameter_calls,
        "Actual source composition executed no native difficulty parameter kernel");
  report.data["passed"]=true;
  std::cout<<"PASS MCC destruction target="<<name<<" shots="<<shots<<" body="<<minimum
      <<" source_dead="<<dead<<" retired="<<retired<<" debris="<<created_debris.size()<<'\n';
  return 0;
}catch(const std::exception &error){report.data["error"]=error.what();std::cerr<<error.what()<<'\n';return 1;}}
