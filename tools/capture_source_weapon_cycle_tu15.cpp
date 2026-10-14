#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/math.hpp"
#include "halo2/source_object_velocity_read_only_tu15.hpp"
#include "sandbox_scheduled_effect_udp_tu15.hpp"
#include "halo2/animation_presentation.hpp"
#include "halo2/particle_draw.hpp"
#include <cstring>
#include <algorithm>
#include <bit>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <set>
#include "halo2/source_portal_visibility.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/source_camera.hpp"
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool ok,const char *message) {if(!ok)throw FormatError(message);}
std::uint32_t word(const SourceRuntimeMemory &memory,NativeRuntimeAddress address,unsigned count=4) {
  const auto bytes=memory.view(address,count);std::uint32_t value{};
  for(unsigned i=0;i<count;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
  return value;
}
std::string raw_hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string out;out.reserve(bytes.size()*2);
  for(const auto b:bytes) {const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15u];}
  return out;
}
}

namespace {
nlohmann::json phase_abi(const SourcePhysicsWorldRuntimeAbi &a) {
  check(!a.x87_depth && !a.active_mmx,"FP probe entry/exit has an unsupported live x87/MMX stack");
  return {{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},
      {"x87_depth",a.x87_depth},{"x87_status",a.x87_status},
      {"x87_control",a.floating_point ? a.floating_point->x87_control_word : 0x27f},
      {"mxcsr",a.floating_point ? a.floating_point->mxcsr : 0x1f80}};
}
void save_phase(const std::filesystem::path &directory,unsigned tick,bool before,
    const SourcePhysicsWorldRuntimeAbi &a,const SourceRuntimeMemory &memory,
    std::span<const std::uint32_t> trace,std::span<const SourceFirstPersonCpuInputTu15> cpu,std::uint32_t entry=0x168ca1u) {
  std::filesystem::create_directories(directory);
  const auto stem=std::to_string(tick)+(before ? "-before" : "-after");
  std::ofstream bytes(directory/(stem+".bin"),std::ios::binary);check(bool(bytes),"Cannot create source FP bank capture");
  nlohmann::json regions=nlohmann::json::array(),inputs=nlohmann::json::array();std::uint64_t offset{};
  memory.visit_committed([&](auto address,auto data) {
    bytes.write(reinterpret_cast<const char *>(data.data()),data.size());
    regions.push_back({{"address",address},{"offset",offset},{"size",data.size()}});offset+=data.size();
  });check(bool(bytes),"Incomplete source FP bank capture");
  for(const auto &sample:cpu)inputs.push_back({{"kind",sample.kind},{"input",sample.input},{"value",sample.value}});
  nlohmann::json report{{"tick",tick},{"entry",entry},{"original_sha256",
      "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"},
      {"abi",phase_abi(a)},{"regions",regions},{"instructions",trace},{"cpu_inputs",inputs}};
  std::ofstream json(directory/(stem+".json"));json<<report.dump(2)<<'\n';
  check(bool(json),"Incomplete source FP phase manifest");
}
}

namespace {
struct CapturedFpPhase {
  nlohmann::json manifest;
  std::vector<std::byte> bytes;
};
void retain_phase(CapturedFpPhase &capture,unsigned tick,const SourcePhysicsWorldRuntimeAbi &abi,
    const SourceRuntimeMemory &memory) {
  capture.bytes.clear();nlohmann::json regions=nlohmann::json::array();
  memory.visit_committed([&](auto address,auto data) {
    const auto offset=capture.bytes.size();capture.bytes.insert(capture.bytes.end(),data.begin(),data.end());
    regions.push_back({{"address",address},{"offset",offset},{"size",data.size()}});
  });
  capture.manifest={{"tick",tick},{"entry",0x168ca1},{"original_sha256",
      "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"},
      {"abi",phase_abi(abi)},{"regions",regions},{"instructions",nlohmann::json::array()},{"cpu_inputs",nlohmann::json::array()}};
}
void save_retained_phase(const std::filesystem::path &directory,unsigned tick,const CapturedFpPhase &capture) {
  std::filesystem::create_directories(directory);const auto stem=std::to_string(tick)+"-before";
  std::ofstream data(directory/(stem+".bin"),std::ios::binary);data.write(reinterpret_cast<const char *>(capture.bytes.data()),capture.bytes.size());
  std::ofstream manifest(directory/(stem+".json"));manifest<<capture.manifest.dump(2)<<'\n';
  check(bool(data) && bool(manifest),"Incomplete original FP event-phase input capture");
}
}

// Normal match inputs must reach the retained original weapon. Native shadow
// ammo, native effects or an independently requested source effect cannot pass.
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  check(argc==5 || argc==6,"Supply MAP SHARED TU15 OUTPUT_DIRECTORY [reload|first-person|first-person-start|first-person-oracle|first-person-render|first-person-runtime|first-person-events]");
  const unsigned runtime_views=argc==6 && std::string_view(argv[5])=="first-person-particle-runtime-4"?4u:
      argc==6 && std::string_view(argv[5])=="first-person-particle-runtime-2"?2u:1u;
#ifdef HALO2_CAPTURE_SOURCE_FP_RUNTIME
  const bool runtime=runtime_views>1 || argc==6 && (std::string_view(argv[5])=="first-person-runtime" || std::string_view(argv[5])=="first-person-scheduled-runtime" || std::string_view(argv[5])=="first-person-particle-runtime");
#else
  const bool runtime=false;
#endif
  const bool particle_cycle=runtime_views>1 || argc==6 && (std::string_view(argv[5])=="first-person-particle-cycle" || std::string_view(argv[5])=="first-person-particle-runtime");
  const bool scheduled=particle_cycle || argc==6 && (std::string_view(argv[5])=="first-person-scheduled-effects" || std::string_view(argv[5])=="first-person-scheduled-runtime");
  const bool events=scheduled || (argc==6 && std::string_view(argv[5])=="first-person-events");
  const bool source_render=events || runtime || (argc==6 && std::string_view(argv[5])=="first-person-render");
  const bool oracle=source_render || (argc==6 && std::string_view(argv[5])=="first-person-oracle");
  const bool early_first_person=oracle || (argc==6 && std::string_view(argv[5])=="first-person-start");
  const bool first_person=early_first_person || (argc==6 && std::string_view(argv[5])=="first-person");
  const bool reload=first_person || (argc==6 && std::string_view(argv[5])=="reload");
  check(argc==5 || reload,"Unknown source weapon cycle mode");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.gameplay_xbe=argv[3];options.starting_weapon=5;options.local_players=runtime_views;
  options.session_random_seed=0x12345678u;
  options.source_object_savegame_directory=std::filesystem::absolute(argv[4])/"source-savegames";
  options.defer_initial_player_spawn=true;
  std::vector<std::uint32_t> players;
  options.player_birth_owner=[&](MultiplayerPlayer &p,unsigned) {
    players.push_back(p.source_datum);p.source_flags=8;
  };
  options.pre_player_update_owner=[&](MultiplayerMatch &match) {
    for(const auto datum:players) {
      const auto *p=match.player_from_datum(datum);check(p,"Source weapon player disappeared");
      if(!(p->source_flags&1u))match.apply_source_player_activation(datum,p->source_flags,p->source_flags|1u);
    }
  };
  MultiplayerPreparedWorld world(options);auto &match=world.match;
  check(world.source_objects && world.source_objects->owns_physics(),"TU1.5 source world was not admitted");
  auto &memory=world.source_objects->memory();std::array<Controls,16> controls{};
  unsigned loaded_before{},loaded_after{},retained_trigger_frames{};
  unsigned reload_frames{},loaded_final{};
  unsigned reserve_before{},reserve_final{};
  unsigned source_channel_changes{};std::array<std::uint32_t,2> prior_channel{};
  unsigned source_event_callbacks{},source_sound_requests{},source_event_phases{};
  unsigned sound_submissions{},sound_returns{},observed_tick{};
  nlohmann::json sound_records=nlohmann::json::array();
  nlohmann::json scheduled_records=nlohmann::json::array(),velocity_records=nlohmann::json::array(),scheduled_call_records=nlohmann::json::array();
  unsigned scheduled_calls{},scheduled_returns{},legacy_effect_calls{};std::uint32_t scheduled_serial{};
  if(scheduled) {
    match.configure_weapon_effect_owner([&](const Unit&,const WeaponEffectInput &)->std::uint32_t {++legacy_effect_calls;throw FormatError("Scheduled source firing entered a second effect creator");});
    world.source_objects->observe_weapon_effect_phase([&](bool before,const auto &abi,const auto &bank,auto trace,auto cpu) {
      if(observed_tick<80 || observed_tick>=100)return;
      if(before) {
        ++scheduled_calls;const auto sp=bank.native_address(abi.gp[4],16);
        scheduled_call_records.push_back({{"id",scheduled_calls},{"loop_tick",observed_tick},
            {"source_tick",word(bank,bank.native_address(word(bank,bank.native_address(0x51df54u,4))+8u,4))+1u},
            {"return_address",word(bank,sp)},{"effect",abi.gp[0]},{"weapon",abi.gp[1]},
            {"previous_instance",word(bank,sp+4u)},{"a_bits",word(bank,sp+8u)},{"b_bits",word(bank,sp+12u)}});
      }else {++scheduled_returns;scheduled_call_records.back()["result_instance"]=abi.gp[0];}
      check(scheduled_calls==scheduled_returns+(before?1u:0u),"Scheduled effect caller ordering differs");
      save_phase(std::filesystem::path(argv[4])/"weapon-effect-oracle",scheduled_calls,before,abi,bank,trace,cpu,0x108a70u);
    });
  }
  CapturedFpPhase before_event_phase;
  std::uint32_t expected_graph=null_datum;
  if(first_person) {
    const auto definition=world.cache->load_first_person_binding(world.rules.weapon,2);
    expected_graph=definition.graph;match.configure_replicated_first_person(definition);
    if(early_first_person) {
      const std::array<unsigned,1> views{0};match.configure_local_views(views,definition);
    }
  }
  std::map<std::uint32_t,RenderModel> source_render_models;
  if(events)world.source_objects->observe_sound_submissions([&](bool before,std::uint32_t sound,std::uint32_t request,
      const auto &abi,const auto &bank,auto trace,auto cpu) {
    if(before) {
      ++sound_submissions;
      const auto system=word(bank,bank.native_address(0x4f2c50u,4));
      check(system!=0,"Actual sound submit lacks its constructed system owner");
      sound_records.push_back({{"id",sound_submissions},{"tick",observed_tick},{"sound",sound},{"request",request},
          {"request_before_hex",raw_hex(bank.view(request,160))},
          {"system",system},{"availability_bytes_hex",raw_hex(bank.view(system+0x78u,3))},
          {"driver",word(bank,bank.native_address(0x52d674u,4))}});
    } else {
      ++sound_returns;check(sound_returns==sound_submissions,"Original sound submit observation order differs");
      check(!trace.empty() && trace.front()==0x223b20u,"Sound submit observation bypassed its original body");
      sound_records.back()["result"]=abi.gp[0];
      sound_records.back()["request_after_hex"]=raw_hex(bank.view(request,160));
      sound_records.back()["original_instruction_events"]=trace.size();
      std::cout<<"source_sound_submit tick="<<observed_tick<<" tag="<<std::hex<<sound
          <<" result="<<abi.gp[0]<<std::dec<<'\n';
    }
    save_phase(std::filesystem::path(argv[4])/"sound-submit-oracle",sound_submissions,before,abi,bank,trace,cpu,0x223b20u);
  });
  nlohmann::json particle_frames=nlohmann::json::array();
  const auto portal_definition=particle_cycle?world.cache->load_source_portal_mask_definition(0):SourcePortalMaskDefinition{};
  SourcePortalMaskState portal_mask;std::vector<SourcePortalOwnerState> portal_owners;
  if(particle_cycle) {
    check(portal_definition.associations.empty(),"Particle cycle requires real dynamic portal owners for this BSP");
    portal_mask.initialize(portal_definition,portal_owners);
  }
  std::uint32_t weapon=null_datum;
  for(unsigned tick=0;tick<(reload?235u:120u);++tick) {
    observed_tick=tick;
    controls[0].fire=tick>=80 && tick<92;
    if(scheduled)controls[0].forward=tick>=55 && tick<79?1.f:0.f;
    controls[0].reload=reload && tick==125;
    const auto previous_view=first_person ? match.first_person_view(0) : FirstPersonViewBinding{};
    const bool capture_phase=oracle && (tick==84 || tick==130 || tick==220);
    if(capture_phase || events)world.source_objects->observe_first_person_phase([&](bool before,const auto &abi,const auto &bank,auto trace,auto cpu) {
      if(events && before)retain_phase(before_event_phase,tick,abi,bank);
      if(!before) {
        const auto callbacks=std::count(trace.begin(),trace.end(),0x169ba2u);
        const auto requests=std::count(trace.begin(),trace.end(),0x18be50u);
        source_event_callbacks+=callbacks;source_sound_requests+=requests;
        if(events && callbacks) {
          ++source_event_phases;
          const auto directory=std::filesystem::path(argv[4])/"fp-event-oracle";
          save_retained_phase(directory,tick,before_event_phase);
          save_phase(directory,tick,false,abi,bank,trace,cpu);
          std::cout<<"source_fp_events tick="<<tick<<" callbacks="<<callbacks<<" sound_requests="<<requests
              <<" sound_submit="<<std::count(trace.begin(),trace.end(),0x223b20u)<<'\n';
        }
      }
      if(capture_phase) {
        if(!before)check(std::find(trace.begin(),trace.end(),0x16a4ddu)!=trace.end(),
            "Source first-person phase did not execute the original positive pose owner");
        save_phase(std::filesystem::path(argv[4])/"fp-oracle",tick,before,abi,bank,trace,cpu);
      }
    });
    match.step(controls);
    if(capture_phase || events)world.source_objects->observe_first_person_phase({});
    const auto &unit=match.unit(0);
    if(particle_cycle && unit.source_datum!=null_datum) {
      const auto clock=word(memory,memory.native_address(0x51df54u,4));
      const auto delta=std::bit_cast<float>(word(memory,memory.native_address(clock+4u,4)));
      SourceFirstPersonPhaseObserverTu15 observer;
      if(tick==81 || tick==84 || tick==130)observer=[&](bool before,const auto& abi,const auto& bank,auto trace,auto cpu) {
        save_phase(std::filesystem::path(argv[4])/"particle-frame-oracle",tick,before,abi,bank,trace,cpu,0x179df0u);
      };
      world.source_objects->advance_effect_particles(delta,observer);
      // Explicit native view input after the source particle phase. The
      // following phase consumes the preceding view's genuine visibility.
      const Vec3 forward{std::cos(unit.yaw)*std::cos(unit.pitch),std::sin(unit.yaw)*std::cos(unit.pitch),std::sin(unit.pitch)};
      const Vec3 up{-std::cos(unit.yaw)*std::sin(unit.pitch),-std::sin(unit.yaw)*std::sin(unit.pitch),std::cos(unit.pitch)};
      const Vec3 eye=unit.body.position+Vec3{0,0,world.content.player.camera_height};
      SourceFrustumProjectionInput input;input.projection_angle=source_camera_angles(1.2217304763960306f).projection_angle;
      input.outer={0,0,480,640};input.safe=input.outer;input.scenario_present=true;input.scenario_type=world.cache->load_source_geometry_scenario_type();
      const auto projection=source_frustum_projection(input);
      const auto camera=source_frustum_camera({eye,forward,up,.015625f,1024.f,projection.clipping_rectangle()});
      const auto cluster=world.cache->load_bsp_atmosphere(0).cluster_at(eye);
      const auto visibility=source_portal_visibility(world.cache->load_source_portal_bsp(0),std::span{&camera,1},cluster,portal_mask.words);
      check(cluster>=0 && !visibility.clusters.empty(),"Actual native view has no completed authored visibility");
      world.source_objects->begin_model_frame();world.source_objects->publish_render_visibility(visibility);
      nlohmann::json pools=nlohmann::json::object();
      for(const auto& [name,address]:std::array<std::pair<const char*,std::uint32_t>,4>{{{"systems",0x51df74u},{"locations",0x52d71cu},{"emitters",0x52d718u},{"particles",0x52d714u}}}) {
        const auto pool=word(memory,memory.native_address(address,4));
        pools[name]={{"pool",pool},{"count",word(memory,memory.native_address(pool+0x3cu,4))},{"stride",word(memory,memory.native_address(pool+0x24u,4))}};
      }
      particle_frames.push_back({{"tick",tick},{"delta_bits",std::bit_cast<std::uint32_t>(delta)},{"cluster",cluster},{"visible_entries",visibility.clusters.size()},{"pools",pools}});
    }
    if(scheduled) {
      for(const auto &event:match.state().weapon_effects)if(event.serial>scheduled_serial) {
        scheduled_serial=event.serial;
        check(event.source_scheduled && event.source_presentation && event.source_instance!=null_datum &&
            event.unit==unit.source_datum && event.weapon==unit.weapon.source_datum,
            "Scheduled source effect lost its actual instance/unit/weapon ownership");
        scheduled_records.push_back({{"serial",event.serial},{"tick",event.tick},{"unit",event.unit},{"weapon",event.weapon},
            {"effect",event.effect},{"a_bits",event.a_bits},{"b_bits",event.b_bits},{"source_instance",event.source_instance},
            {"colors_a",event.colors.a},{"colors_b",event.colors.b},{"velocity",{event.velocity.x,event.velocity.y,event.velocity.z}}});
      }
      if((tick==70 || tick==84 || tick==130) && unit.weapon.source_datum!=null_datum) {
        const auto unchanged=memory.checkpoint();
        const auto velocity=source_object_linear_velocity_read_only_tu15(memory,unit.weapon.source_datum,{0x27f,0x1f80},
            [&](bool before,const auto &abi,auto scratch,auto trace) {
          SourceRuntimeMemory capture;
          memory.visit_committed([&](auto p,auto b){capture.mount(p,b,SourceRuntimeMemory::Access::ReadOnly,"actual source read-only query input");});
          capture.mount(source_object_velocity_private_address,scratch,SourceRuntimeMemory::Access::ReadWrite,"actual private native query stack/output");
          save_phase(std::filesystem::path(argv[4])/"velocity-oracle",tick,before,abi,capture,trace,{},0xbc200u);
        });
        check(memory.checkpoint()==unchanged,"Read-only presentation query changed source state");
        velocity_records.push_back({{"tick",tick},{"weapon",unit.weapon.source_datum},{"velocity",{velocity.x,velocity.y,velocity.z}},
            {"all_source_bytes_unchanged",true}});
      }
    }
    if(unit.source_datum==null_datum)continue;
    const auto body=world.source_objects->object_body(unit.source_datum);
    if(unit.weapon.source_datum!=null_datum) {
      const auto source_weapon=world.source_objects->object_body(unit.weapon.source_datum);
      check(unit.weapon.source_runtime_published,"Weapon presentation has no source publication");
      check(unit.weapon.loaded==std::bit_cast<std::int16_t>(std::uint16_t(word(memory,source_weapon+0x22cu,2))),
          "Presented ammunition diverged from the actual source weapon");
      check(unit.weapon.reserve==std::bit_cast<std::int16_t>(std::uint16_t(word(memory,source_weapon+0x22au,2))),
          "Presented reserve diverged from the actual source weapon");
      check(match.simulation().shots()==0,"A parallel native weapon owner fired during source simulation");
      if(tick>=125 && word(memory,source_weapon+0x224u,2)!=0)++reload_frames;
      loaded_final=word(memory,source_weapon+0x22cu,2);
      reserve_final=word(memory,source_weapon+0x22au,2);
      if(tick>=76 && tick<100)std::cout<<"tick="<<tick
        <<" input="<<word(memory,source_weapon+0x16eu,2)
        <<" pressure="<<word(memory,source_weapon+0x170u,1)
        <<" ready="<<word(memory,source_weapon+0x19eu,2)
        <<" state="<<word(memory,source_weapon+0x178u)
        <<" flags="<<word(memory,source_weapon+0x16cu,2)
        <<" trigger="<<word(memory,source_weapon+0x20cu,1)
        <<" trigger_flags="<<word(memory,source_weapon+0x210u)
        <<" barrel="<<word(memory,source_weapon+0x1a5u,1)
        <<" barrel_flags="<<word(memory,source_weapon+0x1a8u,2)
        <<" rounds="<<unit.weapon.loaded<<'\n';
    }
    if(tick==79) {
      weapon=unit.weapon.source_datum;
      check(weapon!=null_datum,"Source player has no actual weapon");
      const auto weapon_body=world.source_objects->object_body(weapon);
      loaded_before=word(memory,weapon_body+0x22cu,2);
      reserve_before=word(memory,weapon_body+0x22au,2);
      check(loaded_before>0,"Original SMG has no loaded rounds before input");
      if(first_person) {
        const auto definition=world.cache->load_first_person_binding(unit.weapon.definition,2);
        expected_graph=definition.graph;
        if(!early_first_person) {
          const std::array<unsigned,1> views{0};match.configure_local_views(views,definition);
        }
        const auto local=word(memory,memory.native_address(0x4f54f0u,4));
        const auto local_player=word(memory,memory.native_address(local+0xcu,4));
        const auto player_data=word(memory,world.source_objects->player_pool()+0x44u);
        const auto player=memory.native_address(player_data+(unit.source_player&0xffffu)*0x21cu,0x21c);
        const auto local_index=word(memory,player+0x28u,2);
        std::cout<<"source_local_player="<<std::hex<<local_player<<" source_local_index="<<local_index<<std::dec<<'\n';
        check(local_player==unit.source_player && local_index==0,
            "Configured native local view is absent from original player/view ownership");
      }
    }
    if(first_person && (tick>=80 || early_first_person)) {
      const auto base=word(memory,memory.native_address(0x4f6498u,4));
      const auto view=memory.native_address(base,0x20cc);
      check(word(memory,view+4u)==unit.source_datum,"Original first-person view lost its actual unit");
      check(word(memory,view+0x10u)==unit.weapon.source_datum,"Original first-person hand has no actual weapon");
      check(word(memory,view+0x14u)==expected_graph,"Original first-person hand has no authored animation graph");
      check(word(memory,memory.native_address(0x4f5520u,4))==0x245400u,
          "Original director lifecycle has no first-person callback");
      check((word(memory,view+0xcu)&1u)!=0 && word(memory,view+0x310u)>0,
          "Original first-person pose gate/count is absent");
      const std::array channel{word(memory,view+0x18u),word(memory,view+0x30u)};
      const auto &published=match.first_person_view(0);
      const auto &origin=match.first_person_clock_origin(0);
      for(unsigned h=0;h<2;++h) {
        const bool stable=previous_view.unit==published.unit && previous_view.representation==published.representation &&
            previous_view.hands[h].weapon==published.hands[h].weapon && previous_view.hands[h].graph==published.hands[h].graph;
        check(same_first_person_channel(first_person_channel_state(origin.hands[h]),
            first_person_channel_state(stable ? previous_view.hands[h] : published.hands[h])),
            "Published source clock origin used a different identity or time sample");
      }
      const auto packet=match.state().players[0].first_person_clock;
      check(packet.present && same_first_person_channel(packet.current,first_person_channel_state(published.hands[0])) &&
          same_first_person_channel(packet.origin,first_person_channel_state(match.first_person_clock_origin(0).hands[0])),
          "First-person snapshot does not carry the actual source clock endpoints");
      check(published.unit==unit.source_datum && published.representation==2,
          "Published first-person view has different source ownership");
      for(unsigned h=0;h<2;++h) {
        const auto source=view+h*0x1010u;const auto &hand=published.hands[h];
        check(hand.flags==word(memory,source+0xcu) && hand.weapon==word(memory,source+0x10u) &&
            hand.graph==word(memory,source+0x14u) && hand.animation_identifier==word(memory,source+0x18u),
            "Published first-person registration differs from original storage");
        check(std::bit_cast<std::uint32_t>(hand.rate)==word(memory,source+0x2cu) &&
            std::bit_cast<std::uint32_t>(hand.frame)==word(memory,source+0x30u) &&
            hand.activity==word(memory,source+0x90u),
            "Published first-person clock differs from the original owner");
        check(hand.channel_delay==word(memory,source+0x24u,1) && hand.channel_flags==word(memory,source+0x25u,1) &&
            hand.channel_options==word(memory,source+0x26u,2) && hand.events==word(memory,source+0x28u,2) &&
            hand.future_events==word(memory,source+0x2au,2) && std::uint16_t(hand.fire_ticks)==word(memory,source+0xfeu,2),
            "Published first-person event/delay state differs from the original owner");
        check(hand.weapon_node_count==word(memory,source+0x100u) && hand.arms_node_count==word(memory,source+0x104u),
            "Published first-person node counts differ from the original owner");
        for(unsigned n=0;n<64;++n)check(std::uint32_t(hand.weapon_nodes[n])==word(memory,source+0x108u+n*4u) &&
            std::uint32_t(hand.arms_nodes[n])==word(memory,source+0x208u+n*4u),
            "Published first-person node maps differ from the original owner");
      }
      if(tick>80 && channel!=prior_channel)++source_channel_changes;
      prior_channel=channel;
    }
    if(source_render && unit.source_datum!=null_datum) {
      const auto definition=world.cache->load_first_person_binding(unit.weapon.definition,2);
      const Vec3 forward{std::cos(unit.yaw)*std::cos(unit.pitch),std::sin(unit.yaw)*std::cos(unit.pitch),std::sin(unit.pitch)};
      const Vec3 up{-std::cos(unit.yaw)*std::sin(unit.pitch),-std::sin(unit.yaw)*std::sin(unit.pitch),std::cos(unit.pitch)};
      const auto eye=unit.body.position+Vec3{0,0,world.content.player.camera_height};
      SourceFirstPersonPhaseObserverTu15 render_observer;
      if(capture_phase)render_observer=[&](bool before,const auto &abi,const auto &bank,auto trace,auto cpu) {
        if(!before)check(std::find(trace.begin(),trace.end(),0x168dedu)!=trace.end(),"FP render owner was bypassed");
        save_phase(std::filesystem::path(argv[4])/"fp-render-oracle",tick,before,abi,bank,trace,cpu,0x168dedu);
      };
      const auto models=world.source_objects->prepare_first_person_models(0,eye,forward,up,render_observer);
      check(models.size()==2,"Source single-SMG FP draw does not contain exactly arms and weapon");
      check(models[0].render_model==definition.arms_model && models[1].render_model==definition.weapon_model &&
          models[0].owner==unit.source_datum && models[1].owner==unit.weapon.source_datum &&
          models[0].mode==1 && models[1].mode==1,"Source FP draw records changed model/owner identity");
      for(const auto &model:models) {
        if(!source_render_models.contains(model.render_model))
          source_render_models.emplace(model.render_model,world.cache->load_model(model.render_model,false));
        const auto &decoded=source_render_models.at(model.render_model);
        check(model.nodes.size()==decoded.bones.size(),"Original FP draw model-order node count differs");
        const auto skinned=skin_model_world_nodes(decoded,model.nodes);
        check(skinned.size()==decoded.mesh.vertices.size() && !skinned.empty(),"Original FP draw bank did not skin its actual mesh");
      }
      if(capture_phase)std::cout<<"source_fp_render tick="<<tick<<" arms_nodes="<<models[0].nodes.size()<<" weapon_nodes="<<models[1].nodes.size()<<'\n';
    }
    if(tick>=80 && tick<92)
      retained_trigger_frames+=word(memory,body+0x1c0u)==std::bit_cast<std::uint32_t>(1.f);
    if(tick==119) {
      check(unit.weapon.source_datum==weapon,"Source input unexpectedly changed weapon identity");
      loaded_after=word(memory,world.source_objects->object_body(weapon)+0x22cu,2);
    }
  }
  if(scheduled) {
    world.source_objects->observe_weapon_effect_phase({});
    check(!legacy_effect_calls && scheduled_calls==scheduled_returns && scheduled_calls>0 && !scheduled_records.empty() && velocity_records.size()==3,
        "Source scheduled firing did not publish its completed original effect/velocity ownership");
    std::ofstream f(std::filesystem::path(argv[4])/"source-scheduled-effects.json");
    f<<nlohmann::json{{"original_sha256","21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"},
        {"scheduled_calls",scheduled_calls},{"scheduled_returns",scheduled_returns},{"legacy_effect_creations",legacy_effect_calls},
        {"events",scheduled_records},{"original_calls",scheduled_call_records},{"read_only_velocity_cases",velocity_records},{"parallel_native_shots",match.simulation().shots()},
        {"native_device_or_pcm_verified",false},{"matched_xbox_framebuffer_verified",false}}.dump(2)<<'\n';
    check(bool(f),"Incomplete scheduled effect evidence");
    std::cout<<"PASS original scheduled weapon effects: requests="<<scheduled_calls<<" published="<<scheduled_records.size()<<" private velocity cases="<<velocity_records.size()<<'\n';
  }
  if(events) {
    world.source_objects->observe_sound_submissions({});
    check(sound_submissions>0 && sound_returns==sound_submissions,"Original sound submit owner was not observed completely");
    std::ofstream file(std::filesystem::path(argv[4])/"source-sound-submissions.json");file<<sound_records.dump(2)<<'\n';
    check(bool(file),"Incomplete original sound request publication record");
    std::cout<<"source_fp_event_callbacks="<<source_event_callbacks<<" source_fp_sound_requests="<<source_sound_requests
        <<" source_fp_event_phases="<<source_event_phases<<'\n';
    check(source_event_callbacks>0 && source_sound_requests>0,"Actual first-person cycle did not request its original sound events");
  }
  std::cout<<"original_loaded_before="<<loaded_before<<" original_loaded_after="<<loaded_after
    <<" retained_primary_trigger_frames="<<retained_trigger_frames<<'\n';
  check(retained_trigger_frames==12,"Normal primary input did not reach the original unit trigger");
  check(loaded_after<loaded_before,"Normal primary input did not consume original weapon ammunition");
  if(reload) {
    std::cout<<"source_reload_frames="<<reload_frames<<" original_loaded_final="<<loaded_final<<'\n';
    check(reload_frames>0,"Normal reload input did not enter the original magazine owner");
    check(loaded_final==loaded_before,"Original reload did not restore its actual magazine");
    check(reserve_before-reserve_final==loaded_before-loaded_after,
          "Original reload did not spend the actual shared reserve");
  }
  if(first_person) {
    std::cout<<"source_channel_changes="<<source_channel_changes<<'\n';
    check(source_channel_changes>0,"Original first-person animation owner did not advance");
    const auto local=word(memory,memory.native_address(0x4f54f0u,4));
    match.configure_local_views({},std::span<const FirstPersonBindingDefinition>{});
    check(word(memory,memory.native_address(local+0xcu,4))==null_datum &&
        word(memory,memory.native_address(local+8u,2),2)==0,
        "Original local-player release retained its view map or count");
    const auto fp=word(memory,memory.native_address(0x4f6498u,4));
    check(word(memory,memory.native_address(fp+4u,4))==null_datum &&
        word(memory,memory.native_address(fp+0x10u,4))==null_datum,
        "Original local-player release retained a first-person unit or weapon");
    const auto definition=world.cache->load_first_person_binding(match.unit(0).weapon.definition,2);
    const std::array<unsigned,1> views{0};match.configure_local_views(views,definition);
    check(word(memory,memory.native_address(local+0xcu,4))==match.unit(0).source_player &&
        word(memory,memory.native_address(local+8u,2),2)==1,
        "Original local-player rebind did not restore its actual association");
    controls[0]={};match.step(controls);
    check(match.first_person_view(0).hands[0].weapon==match.unit(0).weapon.source_datum,
        "Original local-player rebind lost its actual first-person weapon");
    std::cout<<"PASS source local-view release and rebind\n";
    LanHost host(match,0,0x545531354650434cull);
    LanClient peer("127.0.0.1:"+std::to_string(host.port()),0x545531354650434cull,1);
    for(unsigned n=0;n<64 && !peer.ready();++n) {peer.send(std::span(controls).first(1));host.receive(controls);host.broadcast();peer.receive();}
    check(peer.ready() && peer.error().empty(),"Source first-person LAN handshake incomplete");
    controls={};
    check(peer.slots().size()==1,"Source first-person peer has no unique actual slot");
    const auto remote=peer.slots().front();
    for(unsigned n=0;n<180 && match.unit(remote).source_datum==null_datum;++n)match.step(controls);
    host.broadcast();peer.receive();
    const auto expected=match.state().players[0].first_person_clock;
    const auto actual=peer.state().players[0].first_person_clock;
    check(expected.present && actual.present && same_first_person_channel(actual.current,expected.current) &&
        same_first_person_channel(actual.origin,expected.origin),"Loopback UDP changed the original first-person clock");
    check(match.unit(remote).source_datum!=null_datum,
        "Joined remote player did not receive an original source unit within180 source ticks");
    check(!match.state().players[remote].first_person_clock.present,
        "Source authority manufactured a local first-person clock for its remote player");
    std::cout<<"PASS source first-person clock loopback UDP and unbound remote availability\n";
#ifndef _WIN32
    if(scheduled) {
      using halo2::sandbox_effect_udp_probe::ScheduledEffectUdpProxy;
      ScheduledEffectUdpProxy proxy(host.port());
      LanClient receiver("127.0.0.1:"+std::to_string(proxy.port()),0x545531354650434cull,1);
      const auto same_journal=[&] {
        const auto expected=match.state().weapon_effects;const auto &received=receiver.state().weapon_effects;
        check(expected.size()==received.size(),"UDP lost an original scheduled weapon event");
        for(unsigned n=0;n<expected.size();++n) {
          const auto &a=expected[n],&b=received[n];
          check(a.serial==b.serial && a.tick==b.tick && a.unit==b.unit && a.weapon==b.weapon && a.effect==b.effect &&
              a.a_bits==b.a_bits && a.b_bits==b.b_bits && a.source_instance==b.source_instance &&
              a.source_presentation==b.source_presentation && a.source_scheduled==b.source_scheduled &&
              a.colors.a==b.colors.a && a.colors.b==b.colors.b &&
              std::bit_cast<std::uint32_t>(a.velocity.x)==std::bit_cast<std::uint32_t>(b.velocity.x) &&
              std::bit_cast<std::uint32_t>(a.velocity.y)==std::bit_cast<std::uint32_t>(b.velocity.y) &&
              std::bit_cast<std::uint32_t>(a.velocity.z)==std::bit_cast<std::uint32_t>(b.velocity.z),
              "UDP changed source scheduled identity, flags, scalars, colors or velocity");
        }
      };
      for(unsigned n=0;n<64 && !receiver.ready();++n) {
        receiver.send(std::span(controls).first(1));proxy.pump();host.receive(controls);host.broadcast();
        proxy.deliver(proxy.pump(),true,true);receiver.receive();peer.receive();
      }
      check(receiver.ready() && receiver.error().empty(),"Scheduled-effect UDP handshake failed");same_journal();
      std::vector<ScheduledEffectUdpProxy::Packet> stale;unsigned delivered{},fragmented{},lost{},late{};
      for(unsigned n=0;n<5;++n) {
        match.step(controls);receiver.send(std::span(controls).first(1));proxy.pump();host.receive(controls);host.broadcast();
        auto packets=proxy.pump();check(packets.size()>1,"Scheduled source snapshot was not genuinely fragmented");++fragmented;
        const auto prior_tick=receiver.state().tick;
        if(n==1) {
          stale=packets;packets.erase(packets.begin()+packets.size()/2);proxy.deliver(packets,true,true);receiver.receive();
          check(receiver.state().tick==prior_tick,"Partial UDP snapshot published source events");++lost;peer.receive();continue;
        }
        proxy.deliver(packets,true,true);receiver.receive();peer.receive();same_journal();
        check(receiver.state().tick==match.state().tick,"UDP source event snapshot did not recover after loss");++delivered;
        if(n==2) {
          proxy.deliver(stale,true,true);receiver.receive();same_journal();
          check(receiver.state().tick==match.state().tick,"Late source event snapshot rolled back authority");++late;
        }
      }
      std::ofstream report(std::filesystem::path(argv[4])/"source-scheduled-effects-lan.json");
      report<<nlohmann::json{{"real_loopback_udp",true},{"fragmented_snapshots",fragmented},{"complete_comparisons",delivered+1},
          {"incomplete_snapshot_rejected",lost},{"late_snapshot_rejected",late},{"reversed_and_duplicated_fragments",true},
          {"source_scheduled_flag_preserved",true},{"actual_source_journal_events",match.state().weapon_effects.size()},
          {"late_join_retained_metadata_verified",true},{"lan_visual_or_pcm_playback_verified",false}}.dump(2)<<'\n';
      check(bool(report),"Scheduled-effect UDP evidence was not retained");
      std::cout<<"PASS actual scheduled-effect journal through fragmented UDP, loss, reorder, duplicates and late snapshots\n";
    }
#endif

  }
#ifdef HALO2_CAPTURE_SOURCE_FP_RUNTIME
  if(runtime) {
    options.prepared_world=std::shared_ptr<MultiplayerPreparedWorld>(&world,[](auto*){});
    options.hidden=true;options.mute=true;options.frames=4;options.width=640;options.height=480;
    options.reference_tick_cadence=true;
    options.screenshot=std::filesystem::absolute(argv[4])/"source-first-person.ppm";
    options.input_observer=[](auto,auto,const auto&){};
    unsigned observed{},particle_frames{},particle_draws{},source_particle_phases{};std::set<std::uint32_t> particle_ticks;
    std::vector<unsigned> view_particle_draws(runtime_views),rounds_before(runtime_views),rounds_consumed;
    std::vector<std::set<std::uint32_t>> view_particle_ticks(runtime_views);
    for(unsigned view=0;view<runtime_views;++view)rounds_before[view]=match.unit(view).weapon.loaded;
    const auto prior_events=match.state().weapon_effects;
    const auto prior_serial=prior_events.empty()?0u:prior_events.back().serial;
    if(scheduled) {
      options.frames=0;options.capture_tick=std::uint32_t(match.state().tick)+18u;
      options.input_script=std::filesystem::absolute(argv[4])/"scheduled-runtime-input.txt";
      std::ofstream script(options.input_script);
      for(unsigned tick=0;tick<18;++tick)for(unsigned view=0;view<runtime_views;++view)
        script<<tick<<' '<<view<<' '<<(tick>=8u+2u*view?1:0)<<" 0\n";
      check(bool(script),"Cannot retain actual native firing input script");script.close();
      options.effect_observer=[&](unsigned view,std::uint32_t tick,std::span<const ParticleDraw> draws) {
        if(view>=runtime_views)return;
        const auto state=match.state();unsigned retained{};
        for(const auto &draw:draws)if(draw.effect_owner==match.unit(view).weapon.source_datum &&
            std::any_of(state.weapon_effects.begin(),state.weapon_effects.end(),[&](const auto &event) {
              return event.source_scheduled && event.weapon==draw.effect_owner && event.effect==draw.effect_definition && event.tick<=tick;
            }))++retained;
        if(retained){particle_ticks.insert(tick);particle_frames=particle_ticks.size();particle_draws+=retained;
          view_particle_ticks[view].insert(tick);view_particle_draws[view]+=retained;}
      };
      options.input_observer=[&](auto,auto,const auto&){check(match.simulation().shots()==0,"Runtime source firing entered a parallel native shot owner");};
      if(particle_cycle)options.source_particle_phase_observer=[&](bool before,const auto& abi,const auto& bank,auto trace,auto cpu) {
        if(before)++source_particle_phases;
        if(source_particle_phases==10 || source_particle_phases==12 || source_particle_phases==18)
          save_phase(std::filesystem::path(argv[4])/"runtime-particle-frame-oracle",source_particle_phases,before,abi,bank,trace,cpu,0x179df0u);
        if(!before)check(!trace.empty() && trace.front()==0x179df0u,"Runtime particle phase bypassed its original frame owner");
      };
    }
    options.animation_observer=[&](unsigned view,unsigned,const AnimationRenderObservation &draw) {
      if(!draw.first_person)return;
      check(view<runtime_views && draw.weapon==match.unit(view).weapon.source_datum,"Native FP draw changed its actual view/weapon");
      constexpr std::uint32_t record=0x60050000u+0xd0cu;
      check(word(memory,memory.native_address(record+4u,4))==draw.weapon,
          "Native FP geometry has no original draw-record owner");
      const auto bytes=memory.view(memory.native_address(record+0xcu,draw.weapon_nodes.size_bytes()),draw.weapon_nodes.size_bytes());
      check(!draw.weapon_nodes.empty() && bytes.size()==draw.weapon_nodes.size_bytes() &&
          std::memcmp(bytes.data(),draw.weapon_nodes.data(),bytes.size())==0,
          "Native foreground did not consume the exact original world-node bank");
      check(!draw.vertices.empty() && !draw.interpolated,"Native source foreground is missing or used a second interpolation owner");
      ++observed;
    };
    check(run_multiplayer(options)==0,"Source FP native renderer returned failure");
    check(observed>0 && std::filesystem::is_regular_file(options.screenshot),"Native source FP render was never observed/captured");
    std::cout<<"PASS native FP foreground consumes original world-node bank: draws="<<observed<<'\n';
    if(scheduled) {
      for(unsigned view=0;view<runtime_views;++view)rounds_consumed.push_back(rounds_before[view]-match.unit(view).weapon.loaded);
      const bool every_view_particles=std::ranges::all_of(view_particle_draws,[](auto count){return count>0;});
      const auto current=match.state();const auto created=std::count_if(current.weapon_effects.begin(),current.weapon_effects.end(),[&](const auto &e){return e.serial>prior_serial && e.source_scheduled;});
      std::ofstream report(std::filesystem::absolute(argv[4])/"source-scheduled-effects-render.json");
      report<<nlohmann::json{{"runtime_source_effect_events",created},{"source_fp_model_draws",observed},
          {"source_particle_phases",source_particle_phases},{"local_views",runtime_views},{"original_particle_draw_callbacks_verified",false},
          {"particle_draw_frames",particle_frames},{"particle_draw_records",particle_draws},
          {"particle_records_per_view",view_particle_draws},{"source_rounds_consumed_per_view",rounds_consumed},
          {"parallel_native_shots",match.simulation().shots()},{"mute",true},{"original_node_bank_exact",true},
          {"native_authored_particle_presentation_verified",created>0 && particle_frames>0 && particle_draws>0 && every_view_particles},{"source_particle_pool_or_xbox_pixel_parity_verified",false},
          {"original_pcm_verified",false}}.dump(2)<<'\n';check(bool(report),"Cannot retain actual scheduled runtime evidence");
      check(created>0 && particle_frames>0 && particle_draws>0,"Live original scheduled firing never reached native particle drawing");
      if(particle_cycle)check(source_particle_phases==18,"Native render frames did not advance the original particle phase exactly once");
      if(particle_cycle)check(observed==18u*runtime_views,"Native split views did not consume their own original foreground node banks");
      if(particle_cycle)for(unsigned view=0;view<runtime_views;++view)
        check(rounds_consumed[view]==5u-view && view_particle_draws[view]>0,"A local view did not independently fire its actual source weapon and draw its authored particles");
      std::cout<<"PASS live scheduled source effects to native particle draw: events="<<created<<" frames="<<particle_frames<<" records="<<particle_draws<<'\n';
    }
  }
#endif
  if(particle_cycle) {
    std::uint32_t peak{};
    for(const auto& frame:particle_frames)peak=std::max(peak,frame["pools"]["particles"]["count"].get<std::uint32_t>());
    bool drained=true;
    for(const auto* name:{"systems","locations","emitters","particles"})drained &= particle_frames.back()["pools"][name]["count"]==0;
    const auto instance=scheduled_records.back()["source_instance"].get<std::uint32_t>();
    const auto pool=word(memory,memory.native_address(0x4f720cu,4));
    const auto record=word(memory,memory.native_address(pool+0x44u,4))+(instance&65535u)*word(memory,memory.native_address(pool+0x24u,4));
    const auto slot_salt=word(memory,memory.native_address(record,2),2);
    const bool retired=slot_salt!=(instance>>16u);
    std::ofstream report(std::filesystem::path(argv[4])/"source-particle-cycle.json");
    report<<nlohmann::json{{"frames",particle_frames},{"normal_source_firing",true},{"explicit_native_camera",true},{"full_original_frame_schedule_verified",false},{"source_draw_callbacks_or_gpu_verified",false},
        {"peak_live_particles",peak},{"all_particle_pools_drained",drained},{"actual_effect_instance_retired",retired},{"retired_instance",instance},{"current_slot_salt",slot_salt},{"stale_draw_removal_not_ordinary_rendered_aging",true}}.dump(2)<<'\n';
    check(bool(report),"Cannot retain source particle cycle evidence");
    check(particle_frames.size()==235 && peak>0 && drained && retired,"Source firing did not emit, drain and retire its actual particle graph");
    std::cout<<"PASS actual source firing particle phase: frames="<<particle_frames.size()<<" peak="<<peak<<"; graph and instance retired\n";
  }
  std::cout<<"PASS normal primary input reaches actual TU1.5 unit and weapon\n";
  return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
