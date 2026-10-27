#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_decal_vertex_cache.hpp"
#include "halo2/source_decal_draw.hpp"
#include "halo2/source_decal_frame_wire_tu15.hpp"
#include "helpers/source_decal_frame_checks.hpp"
#include "halo2/world_effects.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/math.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <thread>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;using Json=nlohmann::json;
namespace {void check(bool v,const char*s){if(!v)throw FormatError(s);}}
// Actual source weapon controls and target vitality, not a supplied blood
// definition, hit result, cache page or splat. The peer joins before stimulus;
// historical decals at late join and Xbox network ownership remain separate.
int main(int argc,char**argv)try {
 check(argc==7 || argc==8,"MAP SHARED XBE_OR_DASH REPORT offline|host-client OUTPUT_DIRECTORY [smg-human|smg-elite|plasma]");
 const std::string stimulus=argc==8?argv[7]:"smg-human";
 const bool plasma=stimulus=="plasma";check(plasma || stimulus=="smg-human" || stimulus=="smg-elite","invalid blood stimulus");
 if(!std::filesystem::is_regular_file(argv[1]))return 77;
 const bool online=std::string_view(argv[5])=="host-client";
 check(online||std::string_view(argv[5])=="offline","invalid mode");
 MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
 if(std::string_view(argv[3])!="-")options.gameplay_xbe=argv[3];
 options.source_object_savegame_directory=std::filesystem::path(argv[6])/"source";
 options.source_object_world=true;options.local_players=plasma || online?1:2;options.mute=true;
 std::optional<SourcePlayerConfiguration> remote_target_profile;
 options.starting_weapon=plasma?17:5;if(plasma)options.variant.values.set(83,17);options.session_random_seed=0x12345678u;
 if(!plasma) {
  std::array<std::uint8_t,9> appearance{10,0,0,10,0,0,0,0,0};
  options.selected_profiles.push_back(native_selected_player_configuration("Actual SMG shooter",appearance,-1));
  appearance[4]=stimulus=="smg-elite"?1:0;
  const auto target=native_selected_player_configuration("Actual blood target",appearance,-1);
  if(online)remote_target_profile=target;else options.selected_profiles.push_back(target);
 }
 {Cache placement(options.map,options.shared_maps,options.gameplay_xbe.empty()?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(options.gameplay_xbe));
  check(!placement.spawns().empty(),"actual authored spawn bank empty");
  auto start=placement.spawns().front();options.native_spawn_inputs={start};
  if(!plasma) {
   const auto content=placement.load_content(0,true);CollisionWorld geometry(content.collision);bool found{};
   for(const auto& candidate:placement.spawns()) {
    for(unsigned heading=0;heading<8 && !found;++heading) {
     const float yaw=float(heading)*.7853981633974483f;const Vec3 forward{std::cos(yaw),std::sin(yaw),0};
     const auto eye=candidate.position+Vec3{0,0,content.player.camera_height};
     if(geometry.raycast(eye,forward,2.5f) || geometry.raycast(eye,forward*-1.f,2.5f))continue;
     bool corridor_floor=true;
     for(float distance:{-2.5f,-1.25f,0.f,1.25f,2.5f}) {
      const auto hit=geometry.raycast(candidate.position+forward*distance+Vec3{0,0,.5f},{0,0,-1},1.f);
      corridor_floor&=hit && hit->normal.z>.9f;
     }
     if(!corridor_floor)continue;
     const auto floor=geometry.raycast(candidate.position+forward*.65f+Vec3{0,0,.5f},{0,0,-1},1.f);
     if(!floor || floor->normal.z<.9f)continue;
     start=candidate;start.facing=yaw;auto target=start;target.position=floor->position+Vec3{0,0,.002f};target.facing+=3.14159265358979323846f;
     options.native_spawn_inputs={start,target};found=true;
    }
    if(found)break;
   }
   check(found,"Actual authored spawn bank has no real clear SMG target corridor/floor");
  }
 }
 options.defer_initial_player_spawn=true;
 std::vector<std::uint32_t> players;
 options.player_birth_owner=[&](MultiplayerPlayer&p,unsigned){players.push_back(p.source_datum);p.source_flags=8;};
 options.pre_player_update_owner=[&](MultiplayerMatch&m){for(auto h:players){const auto*p=m.player_from_datum(h);check(p,"source player retired");if(!(p->source_flags&1u))m.apply_source_player_activation(h,p->source_flags,p->source_flags|1u);}};
 MultiplayerPreparedWorld world(options);auto&match=world.match;check(bool(world.source_objects),"actual source world absent");
 std::filesystem::create_directories(argv[6]);
 Json report={{"mode",argv[5]},{"stimulus",stimulus},{"map",std::filesystem::absolute(argv[1]).string()},{"mcc",world.cache->is_mcc()},
  {"scope","Actual source native SMG target controls (or explicitly labelled plasma diagnostic), completed source blood cache births and accepted UDP cache readback. Original network ownership and pixel equality remain separate; no fabricated splats or late-join-history requirement."},
  {"blood_births",Json::array()},{"client_consumer_errors",Json::array()}};
 const auto save=[&]{std::ofstream(argv[4])<<report.dump(2)<<'\n';};save();
 SourceDecalScene scene(*world.cache);report["source_decal_scene_active"]=scene.active();
 std::unique_ptr<LanHost>host;std::unique_ptr<LanClient>peer;
 std::array<Controls,multiplayer_capacity>input{},received{};std::array<Controls,1>neutral{};
 if(online){
#if defined(_WIN32)
  check(_putenv_s("H2_NET_PROFILE","original")==0,"Cannot declare original native network budget");
#else
  check(setenv("H2_NET_PROFILE","original",1)==0,"Cannot declare original native network budget");
#endif
  host=std::make_unique<LanHost>(match,0,0x424c4f4f4450524full);
  peer=std::make_unique<LanClient>(host->join_address(),0x424c4f4f4450524full,1,std::span<const std::int8_t>{},
      remote_target_profile?std::span<const SourcePlayerConfiguration>{&*remote_target_profile,1}:std::span<const SourcePlayerConfiguration>{});
  for(unsigned n=0;n<128&&!peer->ready();++n){peer->send(neutral);host->receive(received);host->broadcast();peer->receive();}
  check(peer->ready()&&peer->error().empty(),"actual UDP peer did not join before stimulus");
  check(peer->slots().size()==1 && peer->slots().front()==1,"Actual UDP blood target did not retain the declared second player owner");
  report["peer_joined_before_stimulus"]=true;report["network_profile"]="original";
  report["initial_native_channel_budget_bits_per_second"]=host->snapshot_diagnostics().stream_budget_bps;}
 Mesh client_mesh;NativeWorldEffects client_effects(*world.cache,client_mesh,match.simulation().collision(),0x12345678u);
 std::uint64_t bullet_serial{},melee_serial{},detonation_serial{},damage_serial{},source_detonation_serial{};
 unsigned received_detonations{},received_damage{},received_bullets{},peak_client_blood_vertices{},peak_source_blood{},new_blood{};
 bool baseline{},self_damage{},target_damage{},target_healthy_before_stimulus{};float initial_target_health=1,minimum_target_health=1,minimum_target_shield=1;
 std::uint32_t target_unit=null_datum,last_shot{},smg_damage=null_datum,smg_weapon=null_datum;std::string smg_name;unsigned actual_shots{},matching_native_damage_reports{};std::uint64_t damage_voice_serial{};unsigned source_detonations{},peer_blood{},peak_peer_blood{};std::size_t maximum_source_wire_bytes{};std::set<std::uint32_t>before_blood,seen_blood;std::map<std::uint32_t,std::string>decal_names;
 for(const auto&t:world.cache->select_tags({.group=tag_groups::decal,.require_address=true}))decal_names[t.datum]=t.name;
 const auto blood=[&](const auto&d){const auto at=decal_names.find(d.definition);return at!=decal_names.end()&&at->second.find("blood")!=std::string::npos;};
 struct FrameWriter {
  std::vector<std::byte> bytes;
  void word(std::uint64_t value,unsigned width){for(unsigned n=0;n<width;++n)bytes.push_back(std::byte(value>>(n*8)));}
  void real(float value){word(std::bit_cast<std::uint32_t>(value),4);}
  void anchor(SourceDecalWireAnchorTu15,std::uint32_t){}
 };
 const auto frame_bytes=[](const std::optional<SourceDecalFrameTu15>&frame){FrameWriter writer;encode_source_decal_frame_wire_tu15(writer,frame);return writer.bytes;};
 const auto client_markers=[&](const MultiplayerState&state){
  for(const auto&player:state.players)if(player.connected){const auto object=player.dead?player.source_previous_unit:player.source_unit;if(object==null_datum)continue;
   const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto&p){return p.object==object;});if(pose==state.source_body_poses.end()||pose->nodes.empty())continue;
   const auto representation=multiplayer_representation(player.source_configuration);check(representation<world.content.player_representations.size(),"peer representation outside actual bank");
   const auto definition=world.content.player_representations[representation].biped.datum;
   const auto mode=world.cache->load_initial_model_definition(definition).render_model;const auto metadata=world.cache->load_source_tag_metadata(mode);const Bytes root(metadata.bytes_at(metadata.source_address(),0x60));
   const Bytes groups(metadata.bytes_at(root.u32(0x5c),std::size_t(root.u32(0x58))*12));NativeWorldEffects::ObjectMarkers markers;
   const auto append=[&](const std::string&name,const Transform&t){markers[name].push_back({t.position,t.basis[2],t.basis[0],t.basis[0],t.basis[2],false,0,0});};
   for(unsigned n=0;n<root.u32(0x58);++n){const auto name=groups.u32(n*12);for(const auto&m:pose_model_markers(source_model_markers(metadata,name),pose->nodes))append(world.cache->string_id(name),m.world);}
   if(!markers.contains("root"))append("root",pose->nodes.front());client_effects.object_markers(object,std::move(markers));
  }
 };
 const auto stimulus_clock=std::chrono::steady_clock::now();
 for(unsigned tick=0;tick<(plasma?240u:360u);++tick){
  if(tick==79){
   std::vector<unsigned> views;std::vector<FirstPersonBindingDefinition> definitions;
   for(unsigned n=0;n<options.local_players;++n) {
    views.push_back(n);const auto representation=multiplayer_representation(match.player(n).source_configuration);
    const auto weapon=match.unit(n).weapon.definition;
    if(std::none_of(definitions.begin(),definitions.end(),[&](const auto& d){return d.weapon_definition==weapon && unsigned(d.representation)==representation;}))
      definitions.push_back(world.cache->load_first_person_binding(weapon,representation));
   }
   match.configure_local_views(views,definitions);
   if(!plasma) {
    smg_weapon=match.unit(0).weapon.definition;const auto weapon=world.content.weapons.find(smg_weapon);
    check(weapon!=world.content.weapons.end() && weapon->second.name.ends_with("\\smg"),"Actual shooter did not receive authored SMG loadout");
    smg_damage=weapon->second.damage;smg_name=weapon->second.name;last_shot=match.state().players[0].shot_tick;
    target_unit=match.unit(1).source_datum;initial_target_health=match.unit(1).vitality.body;
    target_healthy_before_stimulus=match.unit(1).exists && !match.unit(1).vitality.dead && initial_target_health>0;
    check(target_healthy_before_stimulus,"Actual SMG target is not alive before input stimulus; fixture invalid");
   }
   for(const auto&d:source_decal_vertex_cache_tu15(world.source_objects->memory()))if(blood(d))before_blood.insert(d.datum);baseline=true;
  }
  if(tick>=79)for(unsigned view=0;view<options.local_players;++view)
   world.source_objects->publish_native_observer_position(view,match.unit(view).body.position+Vec3{0,0,match.unit(view).definition.camera_height});
  input[0].yaw=match.unit(0).yaw;input[0].pitch=-1.2f;input[0].grenade=plasma && tick==80;
  if(!plasma) {
   const auto from=match.unit(0).body.position+Vec3{0,0,match.unit(0).definition.camera_height};
   const auto target=match.unit(1).body.position+Vec3{0,0,match.unit(1).body.height*.55f};const auto delta=target-from;
   input[0].yaw=std::atan2(delta.y,delta.x);input[0].pitch=std::atan2(delta.z,std::hypot(delta.x,delta.y));
   input[1].yaw=input[0].yaw+3.14159265358979323846f;
   input[1].forward=tick>=25 && tick<45?-1.f:0.f;
   input[0].fire=(tick>=90 && tick<240)||(tick>=270 && tick<340);input[0].reload=tick==240;
  }
  if(host){const std::array<Controls,1> command{plasma?Controls{}:input[1]};peer->send(command);host->receive(received);
   if(!plasma)input[1]=received[1];}
  const auto before_tick=match.simulation().ticks();match.step(input);
  const auto accepted=match.simulation().ticks()==before_tick+1?tick_seconds:0.f;
  world.source_objects->advance_effect_frame(accepted);
  const auto state=match.state();if(tick>=80)self_damage|=state.players[0].dead||state.players[0].health<1;
  if(!plasma && tick>=90) {
   if(state.players[0].shot_tick!=last_shot){last_shot=state.players[0].shot_tick;++actual_shots;}
   target_damage|=state.players[1].source_unit==target_unit && !state.players[1].dead && state.players[1].health<initial_target_health;
   for(const auto& event:state.unit_damage_voices)if(event.serial>damage_voice_serial) {
    damage_voice_serial=event.serial;matching_native_damage_reports+=event.target==target_unit && event.definition==smg_damage;
   }
   minimum_target_health=std::min(minimum_target_health,state.players[1].health);minimum_target_shield=std::min(minimum_target_shield,state.players[1].shield);
  }
  maximum_source_wire_bytes=std::max(maximum_source_wire_bytes,frame_bytes(state.source_decals).size());
  for(const auto&e:state.grenade_detonations)if(e.serial>source_detonation_serial){source_detonation_serial=e.serial;++source_detonations;}
  const auto records=source_decal_vertex_cache_tu15(world.source_objects->memory());unsigned active_blood{};
  for(const auto&d:records)if(blood(d)&&!d.quads.empty()){++active_blood;if(baseline&&tick>=80&&!before_blood.contains(d.datum)&&seen_blood.insert(d.datum).second){++new_blood;
   if(report["blood_births"].size()<64)report["blood_births"].push_back({{"datum",d.datum},{"definition",d.definition},{"name",decal_names.at(d.definition)},
    {"birth_tick",d.tick},{"observed_tick",state.tick},{"layer",d.layer},{"tint",d.tint},{"image",d.bitmap_index},{"lifetime",d.lifetime},{"decay",d.decay},{"quads",d.quads.size()}});}}
  peak_source_blood=std::max(peak_source_blood,active_blood);
  if(peer){host->broadcast();peer->receive();const auto&remote=peer->state();client_markers(remote);
   check(remote.source_decals.has_value(),"Actual source decal cache ownership absent from accepted UDP snapshot");
   check(test::same_source_decal_frame(state.source_decals,remote.source_decals),"Accepted UDP source cache differs from complete authority records/pages/heads");
   client_effects.source_decal_cache(scene.active() && remote.source_decals.has_value());peer_blood=0;
   for(const auto& record:remote.source_decals->records)peer_blood+=blood(record) && record.quad_count>0;
   peak_peer_blood=std::max(peak_peer_blood,peer_blood);
   const auto consume=[&](auto&&fn){try{fn();}catch(const FormatError&e){if(report["client_consumer_errors"].size()<64)report["client_consumer_errors"].push_back(e.what());}};
   for(const auto&e:remote.bullet_impacts)if(e.serial>bullet_serial){bullet_serial=e.serial;++received_bullets;consume([&]{client_effects.impact(e);});}
   for(const auto&e:remote.melee_impacts)if(e.serial>melee_serial){melee_serial=e.serial;consume([&]{client_effects.melee_impact(e);});}
   for(const auto&e:remote.grenade_detonations)if(e.serial>detonation_serial){detonation_serial=e.serial;++received_detonations;consume([&]{
    if(!e.source_locations.empty())client_effects.spawn_source(e.effect,e.projectile,e.tick,e.source_locations,e.colors,e.controls,e.replica_render);
    else client_effects.spawn(e.effect,e.projectile,e.tick,{e.position,e.up,e.forward,e.forward,e.up,false});});}
   for(const auto&e:remote.unit_damage_effects)if(e.serial>damage_serial){damage_serial=e.serial;++received_damage;consume([&]{client_effects.spawn_attached(e.effect.datum,e.target,e.owner.unit,e.tick,e.colors,e.controls);});}
   consume([&]{client_effects.advance(remote.tick,tick_seconds);});
   unsigned vertices{};for(const auto&v:client_effects.decals(remote.tick,0,tick_seconds))if(client_mesh.materials.at(v.material).name.find("blood")!=std::string::npos)++vertices;
   peak_client_blood_vertices=std::max(peak_client_blood_vertices,vertices);
  }
  std::this_thread::sleep_until(stimulus_clock+std::chrono::nanoseconds(std::uint64_t(tick+1)*1000000000u/30u));
 }
 report["stimulus_wall_seconds"]=std::chrono::duration<double>(std::chrono::steady_clock::now()-stimulus_clock).count();
 report["declared_caller_cadence_hz"]=30;
 report["source_new_blood_records"]=new_blood;report["source_peak_blood_records"]=peak_source_blood;report["actual_self_damage"]=self_damage;
 report["actual_source_detonations"]=source_detonations;report["actual_target_body_damage"]=target_damage;
 report["actual_target_healthy_before_stimulus"]=target_healthy_before_stimulus;report["actual_shooter_shot_transitions"]=actual_shots;
 report["matching_native_smg_damage_reports"]=matching_native_damage_reports;report["target_initial_unit"]=target_unit;
 report["actual_shooter_weapon"]=smg_weapon;report["actual_shooter_weapon_name"]=smg_name;report["actual_smg_damage_definition"]=smg_damage;
 report["minimum_target_health"]=minimum_target_health;report["minimum_target_shield"]=minimum_target_shield;
 report["maximum_source_decal_wire_bytes"]=maximum_source_wire_bytes;report["peer_peak_blood_records"]=peak_peer_blood;
 if(host){const auto wire=host->snapshot_diagnostics();report["actual_snapshot_wire"]={{"body_bytes",wire.body_bytes},{"stream_wire_bytes",wire.stream_wire_bytes},{"stream_messages",wire.stream_messages},{"stream_packets",wire.stream_packets},{"stream_over_budget",wire.stream_over_budget},{"stream_budget_bits_per_second",wire.stream_budget_bps}};}
 report["peer_received_detonations"]=received_detonations;report["peer_received_shield_effects"]=received_damage;report["peer_received_bullet_impacts"]=received_bullets;
 report["client_peak_blood_vertices"]=peak_client_blood_vertices;
 report["client_boundary_count"]=client_effects.diagnostics().unavailable.size();report["client_boundaries"]=Json::array();
 for(const auto&gap:client_effects.diagnostics().unavailable)if(report["client_boundaries"].size()<64)report["client_boundaries"].push_back(gap);
 save();check((plasma?self_damage:(target_healthy_before_stimulus && actual_shots>0 && target_damage)) && new_blood>0,"Actual source controls produced no admitted body damage/new dynamic blood cache geometry; trigger is not qualified");
 if(online)check(peak_peer_blood>0,"Actual born source blood absent from accepted UDP presentation cache");
 std::cout<<"PASS source blood births="<<new_blood<<" peer blood vertices="<<peak_client_blood_vertices<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
