#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/math.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/menu_music.hpp"
#include "halo2/source_loop_track_transition_tu15.hpp"
#include "halo2/source_loop_track_control_tu15.hpp"
#include "halo2/desktop_session.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool value,const char *why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address,unsigned size=4) {
  const auto bytes=memory.view(address,size);std::uint32_t value{};
  for(unsigned i=0;i<size;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
  return value;
}
struct Entrance {Spawn spawn;Vec3 point;std::uint32_t definition=null_datum;};
Entrance entrance(const Cache &cache,const LevelContent &content,const std::string &owner) {
  if(owner!="banshee"&&owner!="warthog") {
    const std::map<std::string,std::string> paths{{"extra-rocks","\\extra_rocks"},{"skittery-bugs","\\skittery_bugs"},
        {"flies","\\monopoint_flies"},{"birds-insects","\\outdoor_birds_insects"},
        {"powerhaus","\\powerhaus"},{"under-glass","\\hq_under_glass"},
        {"green-hum","\\green_hum"},
        {"lift","\\lockout_lift_tube\\lockout_lift_tube"}};
    for(const auto &placed:content.objects) {
      const auto tag=cache.query_tag({.handle=TagHandle{placed.definition}});
      if(!tag || (owner=="teleporter" ? !tag->name.ends_with("\\2way_teleporter") :
          owner=="sound-scenery"?placed.type!=10:!tag->name.ends_with(paths.at(owner))))continue;
      if(owner=="lift") {
        // The actual authored lower phantom hull is a declared post-birth
        // position input. Original contact, force, activation and loop controls
        // still run normally; neither a lift result nor a sound request is supplied.
        const auto object=cache.load_source_tag_metadata(placed.definition);
        const Bytes object_root(object.bytes_at(object.source_address(),object.size()));
        const auto model=cache.load_source_tag_metadata(object_root.u32(0x38));
        const Bytes model_root(model.bytes_at(model.source_address(),model.size()));
        const auto physics=cache.load_source_tag_metadata(model_root.u32(0x24));
        const Bytes p(physics.bytes_at(physics.source_address(),physics.size()));
        check(p.u32(0x70)==2 && p.u32(0x108)==2,"Original Lockout lower lift hull differs");
        const Bytes hull(physics.bytes_at(p.u32(0x74),0x100));
        Transform local;local.position={hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)};
        const auto basis=authored_basis(placed.rotation);
        const auto root=source_object_transform(placed.position,basis.forward,basis.up,placed.scale,false);
        const auto position=compose_transform(root,local).position;
        return {{position,0},placed.position,placed.definition};
      }
      const CollisionWorld terrain(content.collision);
      const auto audio_locations=cache.load_bsp_atmosphere(0);
      auto position=placed.position+Vec3{2,0,0};
      const auto floor=terrain.raycast(position+Vec3{0,0,2*content.player.height},{0,0,-1},4*content.player.height);
      if(floor)position.z=floor->position.z+content.player.radius;
      if(!floor || audio_locations.cluster_at(position+Vec3{0,0,content.player.camera_height})<0) {
        // Ambient origins can lie below floors or above the playable BSP.
        // A collision hit alone does not establish a valid listener location.
        // Choose the nearest actual scenario spawn with a real BSP cluster.
        const Spawn* nearest=nullptr;float distance=1e30f;
        for(const auto& spawn:cache.spawns()) {
          if(audio_locations.cluster_at(spawn.position+Vec3{0,0,content.player.camera_height})<0)continue;
          const auto current=length(spawn.position-placed.position);
          if(current<distance){nearest=&spawn;distance=current;}
        }
        if(!nearest)continue;
        auto spawn=*nearest;const auto delta=placed.position-(spawn.position+Vec3{0,0,content.player.camera_height});
        spawn.facing=std::atan2(delta.y,delta.x);
        return {spawn,placed.position,placed.definition};
      }
      const auto delta=placed.position-(position+Vec3{0,0,content.player.camera_height});
      return {{position,std::atan2(delta.y,delta.x)},placed.position,placed.definition};
    }
    throw FormatError("No requested authored scenery approach");
  }
  check(bool(content.unit_interactions),"Vehicle loop test has no authored seats");
  const auto &assets=*content.unit_interactions;
  for(const auto &[row,index]:assets.netgame_placements) {
    const auto &placed=content.objects.at(index);
    const auto tag=cache.query_tag({.handle=TagHandle{placed.definition}});
    if(!tag||!tag->name.ends_with("\\"+owner)||placed.model_attachment)continue;
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
      return {{position,std::atan2(delta.y,delta.x)},points.front().world.position,
          owner=="banshee"?placed.definition:null_datum};
    }
  }
  throw FormatError("No authored vehicle driver approach");
}
}
int main(int argc,char **argv) {
  Json report{{"passed",false},{"source_requests",Json::array()},
      {"actual_target_function_samples",Json::array()},
      {"source_admission_or_voice_results_supplied",false},{"full_audio_parity",false}};
  std::filesystem::path output;
  try {
    check(argc==5||argc==6,"Supply MCC_MAP MCC_MAPS REPORT MODE [warthog|banshee|teleporter|sound-scenery]");output=argv[3];
    const std::string owner=argc==6?argv[5]:"warthog";
    check(owner=="warthog"||owner=="banshee"||owner=="teleporter"||owner=="sound-scenery"||
        owner=="extra-rocks"||owner=="skittery-bugs"||owner=="flies"||owner=="birds-insects"||
        owner=="powerhaus"||owner=="under-glass"||owner=="green-hum"||owner=="lift","Unknown authored audio owner");
    const bool vehicle=owner=="warthog"||owner=="banshee";
    const std::string mode=argv[4];check(mode=="baseline"||mode=="source-frame"||mode=="listeners-and-source-frame"||mode=="selected-loop"||mode=="nonspatial-control"||mode=="native-views-frame"||mode=="native-views-empty"||mode=="native-views-far"||mode=="native-loop-packet"||mode=="native-loop-packet-converted"||mode=="native-object-loop"||mode=="native-object-loop-omit"||mode=="native-object-loop-retire"||mode=="native-object-loop-maneuvers","Unknown object-loop mode");
    const bool object_mode=mode.starts_with("native-object-loop");
    const bool packets_mode=mode.starts_with("native-loop-packet")||object_mode;
    report["mode"]=mode;report["map"]=argv[1];report["owner"]=owner;
    MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
    options.local_players=(packets_mode||mode.starts_with("native-views"))?2:1;
    options.starting_weapon=5;options.source_object_world=true;options.mute=true;
    options.session_random_seed=0x22a0d10u;
    Vec3 entry_point;std::optional<Spawn> lift_spawn;
    std::set<std::uint32_t> target_loops;std::uint32_t target_definition=null_datum;
    {
      Cache cache(options.map,options.shared_maps);
      const auto content=cache.load_content(0,true,source_game_variant(options.variant));
      const auto setup=entrance(cache,content,owner);
      if(owner=="lift")lift_spawn=setup.spawn;else options.native_spawn_inputs={setup.spawn};
      entry_point=setup.point;target_definition=setup.definition;
      report["declared_entry_spawn"]={setup.spawn.position.x,setup.spawn.position.y,setup.spawn.position.z};
      report["authored_entry_point"]={setup.point.x,setup.point.y,setup.point.z};
      if(setup.definition!=null_datum) {
        const auto meta=cache.load_source_tag_metadata(setup.definition);
        const Bytes root(meta.bytes_at(meta.source_address(),0x9c));
        if(root.u32(0x94)) {
          const Bytes attachments(meta.bytes_at(root.u32(0x98),std::size_t(root.u32(0x94))*0x18));
          for(unsigned n=0;n<root.u32(0x94);++n)
            if(attachments.u32(n*0x18)==0x6c736e64u)target_loops.insert(attachments.u32(n*0x18+4));
        }
        check(!target_loops.empty(),"Chosen object has no actual authored loop attachment");
      }
    }
    MultiplayerPreparedWorld world(options,std::nullopt);world.bind_source_local_views(world.local_slots);
    report["actual_local_player_rows"]=world.local_slots.size();
    check(world.cache->is_mcc(),"Object-loop runtime requires the prepared MCC cache");
    auto &source=*world.source_objects;auto &native=source.tu15_world();
    const auto native_audio_locations=world.cache->load_bsp_atmosphere(0);
    auto &memory=native.memory();
    std::optional<float> placed_lift_height;
    if(lift_spawn) {
      check(options.native_spawn_inputs.empty() && world.match.source_engine_owned(),
          "Local lift fixture requires canonical source-engine birth before placement");
      auto born=world.match.state();unsigned birth_wait_ticks{};
      const auto ready=[&] {
        return std::ranges::all_of(world.local_slots,[&](auto slot) {
          const auto& player=born.players[slot];
          return player.source_unit!=null_datum && !player.dead && player.first_person_pose.has_value();
        });
      };
      while(!ready() && birth_wait_ticks<3u) {
        world.match.step({});++birth_wait_ticks;born=world.match.state();
      }
      check(world.local_slots.size()==options.local_players && ready(),
          "Canonical local lift birth omitted an actual local unit or completed FP packet");
      Json birth_rows=Json::array();std::vector<Vec3> before_placement;
      for(unsigned view=0;view<world.local_slots.size();++view) {
        const auto slot=world.local_slots[view];const auto& player=born.players[slot];
        const auto& temporal=player.first_person_pose->temporal;
        const auto row=word(memory,word(memory,0x4f54f4u)+0x44u)+(player.source_player&65535u)*0x21cu;
        const auto input_user=word(memory,row+0x24u),local_view=word(memory,row+0x28u,2);
        check((word(memory,row,2)<<16 | (player.source_player&65535u))==player.source_player &&
            input_user==view && local_view==view && source.first_person_view(view).unit==player.source_unit &&
            temporal.generation && temporal.source_tick+1u==born.tick &&
            temporal.identity.player==player.source_player && temporal.identity.unit==player.source_unit,
            "Canonical local lift birth lost its full player/unit/view/input/FP identity");
        before_placement.push_back(source.biped_motion(player.source_unit).position);
        birth_rows.push_back({{"slot",slot},{"view",view},{"input_user",input_user},
            {"player",player.source_player},{"unit",player.source_unit},
            {"packet_source_tick",temporal.source_tick},{"packet_generation",temporal.generation}});
      }
      report["lift_native_birth"]={{"canonical_source_engine_owned",true},
          {"completed_tick",born.tick},{"additional_wait_ticks",birth_wait_ticks},{"local_players",birth_rows}};
      const auto same_point=[](Vec3 a,Vec3 b) {
        return std::bit_cast<std::uint32_t>(a.x)==std::bit_cast<std::uint32_t>(b.x) &&
            std::bit_cast<std::uint32_t>(a.y)==std::bit_cast<std::uint32_t>(b.y) &&
            std::bit_cast<std::uint32_t>(a.z)==std::bit_cast<std::uint32_t>(b.z);
      };
      const auto slot=world.local_slots.at(0),unit=born.players[slot].source_unit;
      // The complete original B95D0 API moves only this genuinely born unit.
      // Capture its actual result before stepping; Match still publishes its
      // pre-placement position until the next completed native source phase.
      native.set_object_position(unit,lift_spawn->position);
      const auto placed=source.biped_motion(unit).position;
      check(same_point(placed,lift_spawn->position),"Original B95D0 did not deliver the declared lower lift position");
      for(unsigned view=1;view<world.local_slots.size();++view)
        check(same_point(source.biped_motion(born.players[world.local_slots[view]].source_unit).position,before_placement[view]),
            "Local lift placement moved another actual local player");
      placed_lift_height=placed.z;
      report["lift_native_placement"]={{"canonical_birth_then_b95d0",true},{"view",0},{"slot",slot},{"unit",unit},
          {"source_tick",born.tick},{"actual_position",{placed.x,placed.y,placed.z}},
          {"actual_height_before_next_step",placed.z},{"other_local_units_unchanged",true}};
      world.match.step({});const auto completed=world.match.state();
      const auto& player=completed.players[slot];
      check(completed.tick==born.tick+1u && player.source_unit==unit && !player.dead && player.first_person_pose &&
          player.first_person_pose->temporal.identity.player==born.players[slot].source_player &&
          player.first_person_pose->temporal.identity.unit==unit &&
          player.first_person_pose->temporal.source_tick+1u==completed.tick &&
          same_point(player.position,source.biped_motion(unit).position),
          "Local lift placement lost its genuine completed source/FP continuation");
      report["lift_native_placement"]["first_completed_tick"]=completed.tick;
      report["lift_native_placement"]["first_completed_position"]={player.position.x,player.position.y,player.position.z};
    }
    std::uint32_t target_object=null_datum;
    const auto select_target=[&](unsigned tick) {
      if(target_definition==null_datum||target_object!=null_datum)return;
      float nearest=1e30f;
      for(const auto &object:source.live_objects())if(object.definition==target_definition&&object.parent==null_datum) {
        const auto distance=length(source.object_sphere(object.object).center-entry_point);
        if(distance<nearest){nearest=distance;target_object=object.object;}
      }
      if(target_object!=null_datum){report["target_object"]=target_object;report["target_object_observed_tick"]=tick;}
    };
    select_target(0);
    if(!vehicle)check(target_object!=null_datum,"Chosen audio owner lacks an actual source object generation");
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
    report["target_loop_definitions"]=target_loops;
    report["target_loop_metadata"]=Json::array();
    for(const auto tag:target_loops) {
      const auto definition=world.cache->load_looping_sound(tag);Json tracks=Json::array();
      for(const auto& track:definition.tracks)tracks.push_back({{"flags",track.flags},{"start",track.start},
          {"loop",track.loop},{"end",track.end},{"alternate",track.alternate}});
      Json details=Json::array();for(const auto& detail:definition.details)details.push_back({
          {"sound",detail.sound},{"flags",detail.flags},{"period",detail.period},{"distance",detail.distance}});
      report["target_loop_metadata"].push_back({{"definition",tag},{"flags",definition.flags},
          {"authored_detail_rows",details},
          {"details",definition.details.size()},{"tracks",tracks},
          {"actual_native_root",word(memory,word(memory,0x4f0414u)+(tag&65535u)*16u+8u)},
          {"actual_native_flags",word(memory,word(memory,word(memory,0x4f0414u)+(tag&65535u)*16u+8u))},
          {"actual_native_track_count",word(memory,word(memory,word(memory,0x4f0414u)+(tag&65535u)*16u+8u)+0x1cu)},
          {"actual_native_detail_count",word(memory,word(memory,word(memory,0x4f0414u)+(tag&65535u)*16u+8u)+0x24u)}});
      const auto& row=report["target_loop_metadata"].back();
      check(row["actual_native_flags"]==(definition.flags&~0x300u) &&
          row["actual_native_track_count"]==definition.tracks.size() &&
          row["actual_native_detail_count"]==definition.details.size(),
          "Original object-loop root is still MCC layout");
    }
    report["native_loop_root_layout_verified"]=!target_loops.empty();
    if(!vehicle&&!target_loops.empty())check(std::any_of(report["initial_loops"].begin(),report["initial_loops"].end(),[&](const auto &row){
      return target_loops.contains(row.at("definition").template get<std::uint32_t>())&&
          row.at("object").template get<std::uint32_t>()==target_object;
    }),"Original world did not register the chosen authored scenery loop");
    report["source_game_loaded"]=word(memory,word(memory,0x4f3218u)+0x1120u,1);
    unsigned tick{},requests{},target_requests{},control_requests{};
    std::uint32_t control_loop=null_datum;
    std::vector<SourceLoopSoundRequestTu15> packet_requests;
    std::uint32_t packet_auxiliary=0x6d2b1943u;const auto packet_auxiliary_before=packet_auxiliary;
    unsigned native_packets{},native_boost_sides{},native_contrail_sides{},native_lift_activation_packets{},lift_activation_requests{};
    double native_peak{};bool packet_mutation_rejected=false;
    std::unique_ptr<MultiplayerAudio> packet_audio;
    std::optional<MccSoundVariationProvider> packet_metadata;
    struct PacketState {
      std::optional<MultiplayerAudio::VoiceReceipt> voice;
      LoopingSoundSelection selection;std::uint32_t seed=0x19d3217bu;
    };
    std::map<std::tuple<std::uint32_t,unsigned,unsigned>,PacketState> packet_states;
    std::map<std::pair<std::uint32_t,unsigned>,float> track_controls;
    if(packets_mode) {
      packet_audio=std::make_unique<MultiplayerAudio>(world.cache,0x2a178e5bu,
          SdlSoundPlatformConfiguration{SdlSoundPlatformConfiguration::Delivery::Manual,256,{}},
          [&](const MultiplayerAudioSubmission& event) {
            ++native_packets;
            if(!report.contains("native_packets"))report["native_packets"]=Json::array();
            const auto tag=world.cache->query_tag({.handle=TagHandle{event.sound}});
            if(tag && event.owner==target_object) {
              if(tag->name.find("\\banshee_boost_left\\")!=std::string::npos)native_boost_sides|=1u;
              if(tag->name.find("\\banshee_boost_right\\")!=std::string::npos)native_boost_sides|=2u;
              if(tag->name.find("\\banshee_contrail_left\\")!=std::string::npos)native_contrail_sides|=1u;
              if(tag->name.find("\\banshee_contrail_right\\")!=std::string::npos)native_contrail_sides|=2u;
              if(tag->name.find("\\midship_activate_lift\\")!=std::string::npos)++native_lift_activation_packets;
            }
            if(report["native_packets"].size()<128)report["native_packets"].push_back({
                {"sound",event.sound},{"owner",event.owner},{"view",event.view},
                {"name",tag?tag->name:""},{"tick",tick},{"voice",event.voice},
                {"pitch_range",event.pitch_range},{"permutation",event.permutation},
                {"source_pitch_bits",event.source_pitch_bits},{"frames",event.frames}});
            if(object_mode && !packet_mutation_rejected) {
              // The real packet is readable before Core publication. A
              // callback must not erase the object-loop record being used.
              (void)packet_audio->playback_position(event.voice);
              try{packet_audio->stop();}catch(const FormatError& error){
                packet_mutation_rejected=std::string(error.what()).find("borrowed playback owner")!=std::string::npos;
              }
              check(packet_mutation_rejected,"Object packet observer mutated its borrowed playback owner");
            }
          },&packet_auxiliary);
      packet_audio->enroll_continuation();
      if(mode=="native-loop-packet-converted" || object_mode) {
        packet_metadata=native.sound_variation_provider(*world.cache);
        packet_audio->bind_native_sound_metadata(*packet_metadata);
        auto desktop=std::make_shared<DesktopSession>();
        packet_audio->bind_object_loop_random_source([desktop]{return desktop->fresh_secondary_random();});
      }
      report["packet_metadata_domain"]=packet_metadata?"actual converted native World":"raw MCC boundary control, not original flag policy";
      if(owner=="banshee" && packet_metadata) {
        std::set<std::uint32_t> sounds;report["native_phase_sound_metadata"]=Json::array();
        for(const auto definition:target_loops)for(const auto& track:world.cache->load_looping_sound(definition).tracks)
          for(const auto sound:{track.start,track.loop,track.end,track.alternate,track.alternate_end,
              track.alternate_transition_in,track.alternate_transition_out})if(sound!=null_datum)sounds.insert(sound);
        const auto words=[](auto& bytes){Json out=Json::array();const Bytes raw(bytes);
          for(unsigned at=0;at<bytes.size();at+=4)out.push_back(raw.u32(at));return out;};
        for(const auto sound:sounds) {
          auto definition=packet_metadata->load(*world.cache,TagHandle{sound},0);Json intervals=Json::array();
          for(const auto& range:definition.pitch_ranges){Json interval=Json::array();for(unsigned n=0;n<5;++n)
              interval.push_back(std::bit_cast<std::int16_t>(Bytes(range.interval).u16(n*2)));intervals.push_back(interval);}
          const auto tag=world.cache->query_tag({.handle=TagHandle{sound}});
          report["native_phase_sound_metadata"].push_back({{"sound",sound},{"name",tag?tag->name:""},
              {"native_flags",Bytes(definition.sound).u16(0)},{"sound_class",std::to_integer<unsigned>(definition.sound[2])},
              {"playback_words",words(definition.playback)},{"scale_words",words(definition.scale)},
              {"class_words",words(definition.sound_class)},{"pitch_intervals",intervals}});
        }
      }
      report["packet_boundary_scope"]="Actual source frame requests and queued normal-loop PCM packets; entry/exit/alternate/detail scheduling and launcher gameplay are separate";
      report["packet_selection_seed_is_declared_component_input"]=true;
      report["native_local_views"]=options.local_players;
      if(object_mode) {
        unsigned route_controls{};
        for(const auto& tag:world.cache->tags())if(tag.group=="lsnd" &&
            (world.cache->load_looping_sound(tag.datum).flags&0x200u)) {
          SourceLoopSoundRequestTu15 control{};control.arguments[0]=tag.datum;
          control.arguments[2]=0x5a000000u+route_controls;control.arguments[3]=null_datum;
          const auto one=std::bit_cast<std::uint32_t>(1.f);
          for(unsigned n=0;n<4;++n)control.location[4+n]=std::byte(one>>(n*8));
          packet_audio->object_loop_request(control);
          check(packet_audio->interface_loop(tag.datum,control.arguments[2],0).empty(),
              "Remastered-only looping tag reached the native Classic interface lane");
          check(packet_audio->voices()==0 && native_packets==0,"Remastered-only looping tag reached native Classic output");
          ++route_controls;
        }
        report["declared_component_remastered_loop_route_controls"]=route_controls;
      }
    }
    native.observe_loop_sound_requests([&](const auto &request) {
      const auto &args=request.arguments;
      const auto tag=world.cache->query_tag({.handle=TagHandle{args[0]}});
      ++requests;
      const auto pool=word(memory,0x4f9b5cu),index=args[2]&65535u;
      bool selected=false;
      if(index<word(memory,pool+0x38u)&&
          (word(memory,word(memory,pool+0x48u)+(index>>5)*4u)&(1u<<(index&31u)))) {
        const auto record=word(memory,pool+0x44u)+index*24u;
        selected=((word(memory,record,2)<<16)|index)==args[2]&&
            word(memory,record+16u)==target_object&&word(memory,record+12u)==args[0];
      }
      target_requests+=selected&&target_loops.contains(args[0]);
      if(selected && tag && tag->name.ends_with("\\midship_activate_lift\\midship_activate_lift") &&
          args[5]!=2 && Bytes(request.location).f32(4)>0)++lift_activation_requests;
      if(selected)check(request.emitter_owner==target_object,
          "Attached loop observer lost its actual source emitter identity");
      if(packet_audio && selected && target_loops.contains(args[0]))packet_requests.push_back(request);
      control_requests+=control_loop!=null_datum&&args[2]==control_loop;
      if(report["source_requests"].size()<128)report["source_requests"].push_back({{"tick",tick},{"arguments",args},
          {"name",tag?tag->name:""},{"location_type",std::to_integer<unsigned>(request.location[3])&15u},
          {"control_bits",Bytes(request.location).u32(4)},{"emitter_owner",request.emitter_owner},
          {"selected_source_object",selected}});
      if(selected && target_loops.contains(args[0])) {
        if(!report.contains("target_source_requests"))report["target_source_requests"]=Json::array();
        if(report["target_source_requests"].size()<32)report["target_source_requests"].push_back({
            {"tick",tick},{"arguments",args},{"emitter_owner",request.emitter_owner}});
      }
    });
    if(mode=="nonspatial-control") {
      // Independent boundary control: actual18D390 takes EAX=lsnd,
      // EDI=object and XMM0=control. Its unchanged18BD80 child allocates
      // the actual pool record;18D390 owns flags/control. This explicit
      // nonspatial API command is not vehicle/listener/audio acceptance.
      check(!report["initial_loops"].empty(),"Nonspatial control lacks a resident authored loop");
      const auto tag=report["initial_loops"][0]["definition"].get<std::uint32_t>();
      auto abi=native.prepare_source_call({});abi.gp[0]=tag;abi.gp[7]=null_datum;
      abi.xmm[0][0]=std::bit_cast<std::uint32_t>(1.f);native.run_actual(0x18d390u,abi);
      control_loop=abi.gp[0];check(control_loop!=null_datum,"Actual original nonspatial loop allocation failed");
      report["explicit_nonspatial_control"]={{"definition",tag},{"datum",control_loop},
          {"vehicle_playback_verified",false},{"native_PCM_verified",false}};
    }
    std::array<Controls,multiplayer_capacity> commands{};
    const auto initial=world.match.state();const auto slot=world.local_slots.at(0);
    const float initial_height=placed_lift_height.value_or(initial.players[slot].position.z);float peak_height=initial_height;
    commands[slot].yaw=initial.players[slot].yaw;commands[slot].pitch=initial.players[slot].pitch;
    const auto look=entry_point-(initial.players[slot].position+Vec3{0,0,world.content.player.camera_height});
    commands[slot].pitch=std::atan2(look.z,std::hypot(look.x,look.y));
    unsigned seated{};
    const auto threshold=source_pickup_hold_threshold(world.content.pickup_hold_seconds);
    // Lockout's selected wind detail has an authored8..15 second deadline;
    // a six-second fixture cannot require that cue to have played yet.
    const auto test_ticks=owner=="sound-scenery" &&
        std::filesystem::path(argv[1]).filename()=="lockout.map"?600u:
        mode=="native-object-loop-maneuvers"?360u:180u;
    for(tick=0;tick<test_ticks;++tick) {
      packet_requests.clear();
      select_target(tick);
      const auto current=world.match.state().players[slot];
      peak_height=std::max(peak_height,current.position.z);
      native.publish_native_observer_position(0,current.position+Vec3{0,0,world.content.player.camera_height});
      if(mode=="nonspatial-control" && tick==120u) {
        const auto active=loops();
        const bool retained=std::any_of(active.begin(),active.end(),[&](const auto &row) {
          return row.at("datum").template get<std::uint32_t>()==control_loop;
        });
        if(retained) {
          auto abi=native.prepare_source_call({});abi.gp[0]=control_loop;native.run_actual(0x18d3d0u,abi);
        }else report["original_control_retired_before_stop"]=true;
      }
      // Explicit composition of actual13A9D8's audio child. The ordinary
      // object step and its clock are left responsible for all simulation.
      if(mode.starts_with("native-views-") || packets_mode) {
        SourceObjectAudioViewsTu15 views;
        if(mode!="native-views-empty") {
          auto point=world.match.state().players[slot].position+
              Vec3{0,0,world.content.player.camera_height};
          if(mode=="native-views-far")point=point+Vec3{10000,10000,10000};
          views[0]=SourceObjectAudioViewTu15{point,std::int16_t(native_audio_locations.cluster_at(point))};
        }
        if(packet_audio) {
          std::vector<Vec3> listeners;
          for(unsigned view=0;view<world.local_slots.size();++view) {
            auto point=world.match.state().players[world.local_slots[view]].position+
                Vec3{0,0,world.content.player.camera_height};
            if(mode=="native-object-loop-retire" && tick>=60)point=point+Vec3{10000,10000,10000};
            views[view]=SourceObjectAudioViewTu15{point,std::int16_t(native_audio_locations.cluster_at(point))};
            listeners.push_back(point);
          }
          packet_audio->listeners(listeners);
        }
        native.step_native_object_audio(views,tick_seconds);
        if(tick==0 || tick==60 || tick==179) {
          Json view_rows=Json::array();
          for(unsigned index=0;index<views.size();++index)if(views[index]) {
            const auto& view=*views[index];view_rows.push_back({{"slot",index},
                {"position",{view.position.x,view.position.y,view.position.z}},{"cluster",view.cluster}});
          }
          Json sample{{"tick",tick},{"views",view_rows},{"visibility_words",Json::array()}};
          const auto bsp=word(memory,0x4ecc18u),clusters=word(memory,bsp+0x9cu);
          for(unsigned n=0;n<(clusters+31)/32;++n)
            sample["visibility_words"].push_back(word(memory,0x55d730u+n*4u));
          if(target_object!=null_datum) {
            const auto pool=word(memory,0x4ecbd0u);
            const auto header=word(memory,pool+0x44u)+(target_object&65535u)*12u;
            check((word(memory,header,2)<<16 | (target_object&65535u))==target_object,
                "Target object observation lost its actual full salt");
            const auto body=word(memory,header+8u);
            sample["object_header_flags"]=word(memory,header+2u,1);
            sample["object_location_words"]={word(memory,body+0x28u),word(memory,body+0x2cu)};
          }
          report["actual_audio_spatial_samples"].push_back(std::move(sample));
        }
      }
      if(mode=="listeners-and-source-frame")native.call_actual(0x129f10u,std::array{0u});
      if(mode=="selected-loop") {
        // Explicit per-object API composition isolates the whole18D5A0
        // owner from18EE50's frame/PVS admission. No loop, listener,
        // visibility bit, function result or device readiness is supplied.
        for(const auto &row:loops())if(row.at("object").template get<std::uint32_t>()==target_object&&
            target_loops.contains(row.at("definition").template get<std::uint32_t>()))
          native.call_actual(0x18d5a0u,std::array{row.at("datum").template get<std::uint32_t>()});
      }else if(mode=="nonspatial-control") {
        const auto active=loops();
        if(std::any_of(active.begin(),active.end(),[&](const auto &row) {
            return row.at("datum").template get<std::uint32_t>()==control_loop;
        }))native.call_actual(0x18d5a0u,std::array{control_loop});
      }else if(mode!="baseline" && !mode.starts_with("native-views-") && !packets_mode)native.call_actual(0x18ee50u,std::array{std::bit_cast<std::uint32_t>(tick_seconds)});
      if(packet_audio) {
        packet_audio->update();
        if(object_mode) {
          if(mode!="native-object-loop-omit")for(const auto& request:packet_requests)packet_audio->object_loop_request(request);
          packet_audio->object_loop_frame(tick_seconds);
        }else for(const auto& request:packet_requests) {
          const auto &args=request.arguments;const Bytes location(request.location);
          if(args[5]==2)continue; // This boundary fixture does not implement the stop controller.
          const auto definition=world.cache->load_looping_sound(args[0]);
          for(unsigned track_index=0;track_index<definition.tracks.size();++track_index) {
            const auto &track=definition.tracks[track_index];
            const auto next=source_loop_track_transition_tu15({track.start,track.loop,track.end,
                track.alternate_transition_in,track.alternate,track.alternate_transition_out,track.alternate_end},
                track.flags,0,0,false);
            if(next.sound==null_datum)continue;
            const auto base=packet_metadata?packet_metadata->load(*world.cache,TagHandle{next.sound}):world.cache->load_sound_variation(next.sound);
            const auto key=std::to_string(next.sound);
            if(!report["packet_metadata"].contains(key)) {
              const auto raw=world.cache->load_sound_variation(next.sound);
              report["packet_metadata"][key]={{"raw_mcc_flags",Bytes(raw.sound).u16(0)},
                  {"playback_flags",Bytes(base.sound).u16(0)},
                  {"playback_distance",{Bytes(base.playback).f32(0),Bytes(base.playback).f32(4)}},
                  {"class_distance",{Bytes(base.sound_class).f32(0x18),Bytes(base.sound_class).f32(0x1c)}}};
            }
            const Bytes playback(base.playback);const float control=location.f32(4);
            auto [at,created]=track_controls.try_emplace({args[2],track_index},control);
            if(!created)at->second=source_loop_track_gain_control_tu15(at->second,control,playback.f32(0xc),tick_seconds);
            const auto base_pitch=std::bit_cast<std::int16_t>(playback.u16(0x18));
            const float pitch=source_sound_control_pitch(base,float(base_pitch),at->second);
            const bool layered=Bytes(base.sound).u16(0)&0x2000u;
            const auto primary=source_sound_pitch_range(base.pitch_ranges,-1,pitch);
            for(unsigned range=0;range<base.pitch_ranges.size();++range) {
              const Bytes interval(base.pitch_ranges[range].interval);std::array<std::int16_t,5> bounds;
              for(unsigned n=0;n<5;++n)bounds[n]=std::bit_cast<std::int16_t>(interval.u16(n*2));
              const auto layer=source_loop_pitch_layer_tu15(bounds,pitch,layered);
              if(layered?!layer.active:int(range)!=primary)continue;
              auto &state=packet_states[{args[2],track_index,range}];
              const MultiplayerLoopPacketParametersTu15 parameters{pitch,track.gain_db+location.f32(8)+layer.gain_db,control};
              std::optional<Vec3> position;
              if((std::to_integer<unsigned>(request.location[3])&15u)==1)
                position=Vec3{location.f32(0xc),location.f32(0x10),location.f32(0x14)};
              if(state.voice && packet_audio->voice_active(*state.voice)) {
                packet_audio->loop_packet_parameters(*state.voice,parameters);
                if(position)packet_audio->emitter(*state.voice,*position);
              }
              const auto metadata=packet_metadata?packet_metadata->load(*world.cache,TagHandle{next.sound},range):world.cache->load_sound_variation(next.sound,range);
              std::vector<std::uint16_t> chances;
              for(const auto &permutation:metadata.permutations)chances.push_back(Bytes(permutation).u16(2));
              if(!state.voice || !packet_audio->voice_active(*state.voice)) {
                const auto selected=source_looping_sound_permutation(chances,state.selection,state.seed);
                state.voice=packet_audio->play_loop_packet({next.sound,target_object,0,range,unsigned(selected),parameters,position});
              }
              if(state.voice && packet_audio->loop_packet_accepts_input(*state.voice)) {
                const auto selected=source_looping_sound_permutation(chances,state.selection,state.seed);
                check(packet_audio->queue_loop_packet(*state.voice,unsigned(selected)),"Actual loop queue lost its retained free slot");
              }
            }
          }
        }
        if(mode=="native-object-loop-maneuvers" && tick==110) {
          // This fixture owns live World metadata, not a prospective World
          // publication. Preserve the aggregate boundary: a scalar-only
          // Audio replacement must not infer a target native lease.
          const auto checkpoint=packet_audio->checkpoint();
          bool rejected=false;
          try{(void)packet_audio->prepare_restore(checkpoint);}catch(const FormatError& error){
            rejected=std::string(error.what()).find("genuine target metadata binding")!=std::string::npos;
          }
          check(rejected,"Live object-loop restoration inferred a prospective native metadata owner");
          report["native_metadata_restore_rejected_without_target"]=true;
          report["native_controller_checkpoint_suffix_verified"]=false;
        }
        for(const auto sample:packet_audio->render(1600)) {
          check(std::isfinite(sample),"Selected loop packet produced nonfinite native PCM");
          native_peak=std::max(native_peak,std::abs(double(sample)));
        }
        packet_audio->update();
      }
      if(!tick) {
        const auto sound=word(memory,0x4f2c50u);
        report["source_sound_availability"]={word(memory,sound+0x78u,1),word(memory,sound+0x79u,1),word(memory,sound+0x7au,1)};
        report["listener_active"]=Json::array();
        for(unsigned view=0;view<4;++view)report["listener_active"].push_back(word(memory,sound+0x8eu+view*0x48u,1));
      }
      const auto player=world.match.state().players[slot];
      commands[slot].vehicle_interaction_target=player.action_prompt.target;
      if(!vehicle) {
        commands[slot].pickup=false;commands[slot].pickup_milliseconds=0;
      }else if(player.source_parent==null_datum) {
        commands[slot].pickup=tick<90;
        commands[slot].pickup_milliseconds=std::uint16_t(std::min<unsigned>(tick*33u,threshold+100u));
      }else {
        ++seated;commands[slot].pickup=false;commands[slot].pickup_milliseconds=0;
        commands[slot].forward=tick>=90&&tick<150?1.f:0.f;
        if(mode=="native-object-loop-maneuvers") {
          commands[slot].forward=tick>=60&&tick<240?1.f:0.f;
          commands[slot].fire_secondary=(tick>=90&&tick<150)||(tick>=180&&tick<210);
          commands[slot].crouch=commands[slot].fire_secondary;
          commands[slot].jump=tick>=45&&tick<90;
        }
      }
      world.match.step(commands);
      if(vehicle && target_object!=null_datum && (tick==60 || tick==100 || tick==140 || tick==179 || tick==200 || tick==239 || tick==359)) {
        const auto body=source.source_object_body(target_object),driver=source.source_object_body(player.source_unit);
        if(!report.contains("actual_vehicle_audio_control_samples"))report["actual_vehicle_audio_control_samples"]=Json::array();
        report["actual_vehicle_audio_control_samples"].push_back({{"tick",tick},{"vehicle",target_object},
            {"body",body},{"source_control_word",word(memory,body+0x148)},
            {"source_driver_control_word",word(memory,driver+0x148)},
            {"command_crouch",commands[slot].crouch},{"command_jump",commands[slot].jump},
            {"command_secondary",commands[slot].fire_secondary},
            {"position_bits",{word(memory,body+0x64),word(memory,body+0x68),word(memory,body+0x6c)}},
            {"velocity_bits",{word(memory,body+0x88),word(memory,body+0x8c),word(memory,body+0x90)}},
            {"boost_state_bits",{word(memory,body+0x334),word(memory,body+0x338)}}});
      }
      if(tick==60 || tick==100 || tick==140 || tick==179 || tick==200 || tick==239 || tick==359)for(const auto &row:loops()) {
        if(row["type"]!=1 || row["object"]!=target_object || !target_loops.contains(row["definition"].get<std::uint32_t>()))continue;
        const auto object=row["object"].get<std::uint32_t>(),parameter=row["parameter"].get<std::uint32_t>();
        constexpr std::uint32_t output=0x6005bd00u;
        const auto active=native.call_actual(0xbcb70u,std::array{object,parameter,output});
        report["actual_target_function_samples"].push_back({{"tick",tick},{"loop",row["datum"]},
            {"object",object},{"parameter",parameter},
            {"parameter_name",parameter?world.cache->string_id(parameter):std::string("implicit one")},
            {"value_bits",word(memory,output)},{"active",bool(active&255u)}});
      }
    }
    native.observe_loop_sound_requests({});report["final_loops"]=loops();report["ticks"]=tick;report["seated_ticks"]=seated;
    report["request_count"]=requests;report["request_samples_clipped"]=requests>report["source_requests"].size();
    if(packet_audio){report["native_loop_final_voices"]=packet_audio->voices();packet_audio->stop();}
    report["native_PCM_verified"]=bool(packet_audio)&&native_peak>0&&native_packets>0;
    report["native_loop_packet_peak"]=native_peak;report["native_loop_packet_submissions"]=native_packets;
    report["native_banshee_boost_sides"]=native_boost_sides;
    report["native_banshee_contrail_sides"]=native_contrail_sides;
    if(owner=="lift")report["lift_activation"]={{"initial_height",initial_height},{"peak_height",peak_height},
        {"positive_source_requests",lift_activation_requests},{"actual_native_packets",native_lift_activation_packets}};
    report["packet_output_did_not_advance_auxiliary_rng"]=packet_auxiliary==packet_auxiliary_before;
    report["completed_native_loop_scheduler_verified"]=false;
    report["target_original_request_count"]=target_requests;
    report["nonspatial_control_request_count"]=control_requests;
    report["passed"]=(mode=="native-views-empty" || mode=="native-views-far")?target_requests==0:
        mode=="nonspatial-control"?control_requests>0:
        ((!vehicle||seated>0)&&(target_loops.empty()?requests>0:target_requests>0)&&!report["initial_loops"].empty());
    const bool synchronized=std::ranges::any_of(target_loops,[&](auto tag){return world.cache->load_looping_sound(tag).flags&0x10u;});
    report["controller_random_domain"]=synchronized?"actual native platform group seed and private track/voice words":"actual common secondary word";
    report["ordinary_controller_used_common_secondary_rng"]=object_mode&&packet_auxiliary!=packet_auxiliary_before;
    report["native_packet_observer_mutation_rejected"]=packet_mutation_rejected;
    if(packet_audio)report["passed"]=report["passed"].get<bool>()&&report["native_PCM_verified"].get<bool>()&&
        (object_mode&&!synchronized?packet_auxiliary!=packet_auxiliary_before:packet_auxiliary==packet_auxiliary_before);
    if(mode=="native-object-loop-retire")report["passed"]=report["passed"].get<bool>()&&report["native_loop_final_voices"]==0;
    if(object_mode)report["passed"]=report["passed"].get<bool>()&&packet_mutation_rejected;
    if(mode=="native-object-loop-maneuvers")report["passed"]=report["passed"].get<bool>()&&
        report.value("native_metadata_restore_rejected_without_target",false)&&native_boost_sides==3u&&native_contrail_sides==3u;
    if(owner=="lift" && packet_audio)report["passed"]=report["passed"].get<bool>()&&
        peak_height>initial_height && lift_activation_requests>0 && native_lift_activation_packets>0;
    report["ordinary_object_loop_profile_verified"]=object_mode&&report["passed"].get<bool>();
    if(!report["passed"].get<bool>())report["error"]=mode=="native-object-loop-retire"?
        "Native object loop streams survived source-owner retirement":mode=="native-object-loop-maneuvers"&&native_boost_sides!=3u?
        "Actual secondary intent reached neither complete Banshee boost audio pair":packet_audio&&target_requests?
        "Actual source requests reached no native loop PCM":"Original object loops produced no playback requests";
  }catch(const std::exception &error){report["error"]=error.what();}
  if(!output.empty()){std::ofstream file(output);file<<report.dump(2)<<'\n';}
  std::cout<<report.dump(2)<<'\n';return report["passed"].get<bool>()?0:1;
}
