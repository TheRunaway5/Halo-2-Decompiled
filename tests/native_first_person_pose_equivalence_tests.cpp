#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/first_person_ik.hpp"
#include "halo2/model.hpp"
#include "halo2/math.hpp"
#include "halo2/unit_animation_graph_metadata.hpp"
#include "halo2/source_first_person_pose_tu15.hpp"
#include "halo2/source_first_person_packet_tu15.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif

using namespace halo2;
namespace {
void need(bool value,const char *message){if(!value)throw FormatError(message);}
struct Difference {
  float position{},basis{},scale{};
  unsigned node{},count{};
  void compare(std::span<const Transform> actual,std::span<const Transform> expected,
      const std::function<bool(unsigned)> &selected={}) {
    need(actual.size()==expected.size(),"FP equivalence lost model-order node identity");
    for(unsigned n=0;n<actual.size();++n) {
      if(selected && !selected(n))continue;
      const auto p=length(actual[n].position-expected[n].position);
      if(p>position){position=p;node=n;}
      scale=std::max(scale,std::abs(actual[n].scale-expected[n].scale));
      for(unsigned i=0;i<3;++i)for(const auto delta:{actual[n].basis[i].x-expected[n].basis[i].x,
          actual[n].basis[i].y-expected[n].basis[i].y,actual[n].basis[i].z-expected[n].basis[i].z})
        basis=std::max(basis,std::abs(delta));
      ++count;
    }
  }
};
struct NativePoseInput final:SourcePhysicsContactMemory {
  const SourceRuntimeMemory &source;
  std::vector<std::pair<std::uint32_t,std::uint32_t>> denied;
  explicit NativePoseInput(const SourceRuntimeMemory &source):source(source){}
  void read(std::uint32_t at,std::span<std::byte> bytes)const override {
    for(const auto &[base,count]:denied)
      if(std::uint64_t(at)<std::uint64_t(base)+count && std::uint64_t(base)<std::uint64_t(at)+bytes.size())
        throw FormatError("Native pose consumer attempted to read a previously completed output");
    source.read(at,bytes);
  }
  void write(std::uint32_t,std::span<const std::byte>)override {
    throw FormatError("Native pose input owner must remain read-only");
  }
};
}

int main(int argc,char **argv)try {
  need(argc==5 || (argc==6 && (std::string_view(argv[5])=="extras" || std::string_view(argv[5])=="dynamic" || std::string_view(argv[5])=="native" || std::string_view(argv[5])=="packet" || std::string_view(argv[5])=="packet_single" || std::string_view(argv[5])=="packet_mixed" || std::string_view(argv[5])=="packet_render_order" || std::string_view(argv[5])=="legacy")),
      "Supply prepared MCC map, shared maps, report, representation2|3 and optional extras|dynamic|native|packet|packet_single|packet_mixed|packet_render_order|legacy");
  const bool extras_only=argc==6 && std::string_view(argv[5])=="extras";
  // Ordinary/default and dynamic acceptance follow the production joined
  // packet consumer. The retired clip approximation remains an explicit
  // diagnostic mode so earlier red receipts can still be reproduced.
  const bool mixed=argc==6 && std::string_view(argv[5])=="packet_mixed";
  const bool single=argc==6 && std::string_view(argv[5])=="packet_single";
  const bool render_order=argc==6 && std::string_view(argv[5])=="packet_render_order";
  const bool packet_only=argc==5 || mixed || single || render_order || (argc==6 &&
      (std::string_view(argv[5])=="packet" || std::string_view(argv[5])=="dynamic"));
  const bool native_only=packet_only || (argc==6 && std::string_view(argv[5])=="native");
  const bool dynamic=native_only || (argc==6 && std::string_view(argv[5])=="dynamic");
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
  need(scratch && std::filesystem::path(scratch).is_absolute(),"Run FP equivalence through tools/run_test.py");
  const unsigned representation=unsigned(std::stoul(argv[4]));
  need(representation==2 || representation==3,"FP equivalence requires human or Elite");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.local_players=1;options.starting_weapon=5;options.mute=true;
  options.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};
  options.source_object_world=true;
  options.source_object_savegame_directory=std::filesystem::path(scratch)/"native-fp";
  if(representation==3){auto profile=source_default_player_configuration();profile[0x44]=1;options.selected_profiles={profile};}
  MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
  need(world.cache->is_mcc() && world.source_objects && world.source_objects->owns_physics() &&
      world.match.source_engine_owned(),"FP equivalence requires canonical native MCC authority");
  std::unique_ptr<MultiplayerPreparedWorld> packet_replica;
  std::shared_ptr<const SourceRuntimeResources> packet_resources;
  std::unique_ptr<SourceFirstPersonPoseReceiverTu15> packet_receiver;
  std::shared_ptr<MultiplayerObjectRuntime> packet_assets;
  if(packet_only) {
    auto recipient=options;recipient.join="native-fp-recipient-constructor-fixture";
    packet_replica=std::make_unique<MultiplayerPreparedWorld>(recipient);
    need(!packet_replica->source_objects && !packet_replica->match.source_object_runtime() &&
        packet_replica->rules.requires_source_engine,
        "Joined PreparedWorld unexpectedly supplied a native gameplay owner");
    // An actual independent native asset owner with no local player/FP actor.
    // It uses the real joined PreparedWorld's independently admitted Cache;
    // it cannot borrow the authority's player/camera/cache or attach to Match.
    packet_assets=std::make_shared<MultiplayerObjectRuntime>(*packet_replica->cache,options.bsp,
        std::filesystem::path(scratch)/"recipient-assets");
    packet_assets->configure_session(source_game_variant(options.variant),1,30,world.rules.random_seed);
    packet_resources=std::shared_ptr<const SourceRuntimeResources>(packet_assets,&packet_assets->tu15_world().runtime_resources());
    packet_receiver=std::make_unique<SourceFirstPersonPoseReceiverTu15>(*packet_replica->cache,packet_resources);
  }
  const auto slot=world.local_slots.at(0);
  const auto binding=world.cache->load_first_person_binding(world.cache->slayer_starting_weapon(5),representation);
  const auto gun=world.cache->load_model(binding.weapon_model,false);
  const auto arms=world.cache->load_model(binding.arms_model,false);
  const auto ik=world.cache->load_first_person_ik(binding.graph,binding.weapon_model,binding.arms_model);
  const auto graph_metadata=world.cache->load_source_unit_animation_graph_metadata(binding.graph);
  const auto record_name=[&](std::uint32_t identifier) {
    const auto index=identifier>>16;
    const Bytes header(graph_metadata.header());
    if(index>=header.u32(0x2c))return std::string("absent");
    const Bytes record(graph_metadata.bytes_at(header.u32(0x30)+index*0x6cu,0x6c));
    return world.cache->string_id(record.u32(0));
  };
  const auto &definition=world.content.weapons.at(binding.weapon_definition);
  auto pickup_definition=binding.weapon_definition;
  if(mixed) {
    const auto other=std::find_if(world.content.weapons.begin(),world.content.weapons.end(),[](const auto &w){
      return w.second.name.ends_with("plasma_pistol");
    });
    need(other!=world.content.weapons.end(),"Mixed FP fixture lacks authored plasma pistol");pickup_definition=other->first;
  }
  const NetgameEquipment *pickup{};
  for(const auto &row:world.content.netgame_equipment)
    if(!row.classification && source_netgame_equipment_selector(row,true,world.rules.source_engine)>=0 &&
        row.items.size()==1 && row.items.front().definition==pickup_definition){pickup=&row;break;}
  need(pickup,"FP equivalence lacks an authored same-weapon dual pickup");
  const Transform camera=camera_transform({0,0,0},{1,0,0},{0,0,1});
  std::map<std::uint32_t,AnimationClip> clips;
  std::array<Difference,2> weapon_difference,arms_difference;
  std::array<std::array<Difference,2>,3> phase_weapon,phase_arms;
  std::array<std::array<Difference,2>,7> action_weapon,action_arms;
  constexpr std::array action_names{"idle","look","movement","primary_fire","secondary_fire","reload","combined"};
  std::array<bool,3> actions_observed{};
  nlohmann::json samples=nlohmann::json::array();
  std::array<Controls,multiplayer_capacity> controls{};
  std::optional<unsigned> acquired;
  unsigned frames{};
  bool extras_equal=true;
  bool native_equal=true;
  Difference native_difference;
  Difference completed_input_difference;
  bool feedback_advanced_after_composition{};
  std::set<std::uint32_t> native_instructions;
  std::vector<std::pair<std::uint32_t,std::uint32_t>> native_input_ranges;
  nlohmann::json native_roots;
  for(unsigned tick=0;tick<220;++tick) {
    const auto state=world.match.state();const auto &player=state.players[slot];
    if(!single && player.source_unit!=null_datum && !player.dead) {
      auto position=pickup->position;position.x-=.2f;
      world.source_objects->tu15_world().set_object_position(player.source_unit,position);
    }
    if(dynamic && acquired) {
      const auto elapsed=tick-*acquired;
      controls[slot]=Controls{};
      if(elapsed>=20 && elapsed<40){controls[slot].yaw=.5f*std::sin(elapsed*.1f);controls[slot].pitch=.3f*std::cos(elapsed*.1f);}
      if(elapsed>=40 && elapsed<60){controls[slot].forward=.75f;controls[slot].right=.25f;}
      if(elapsed>=60 && elapsed<90)controls[slot].fire=true;
      if(elapsed>=90 && elapsed<120)controls[slot].fire_secondary=true;
      if(elapsed==120)controls[slot].reload=true;
      if(elapsed>=150){controls[slot].yaw=.2f*std::sin(elapsed*.1f);controls[slot].pitch=.2f;controls[slot].forward=.5f;controls[slot].fire=true;controls[slot].fire_secondary=true;}
    }
    controls[slot].dual_wield=!single && !acquired;
    controls[slot].dual_wield_milliseconds=std::uint16_t(std::min(1000u,tick*34u));
    world.match.step(controls);
    const auto current=world.match.state();const auto &p=current.players[slot];
    if(single) {
      if(p.dead || p.source_unit==null_datum || p.source_weapon==null_datum)continue;
      need(p.secondary_weapon.datum==null_datum,"Single-weapon FP fixture unexpectedly acquired an offhand");
    } else if(p.secondary_weapon.datum==null_datum)continue;
    if(!acquired)acquired=tick;
    if(mixed)need(p.weapon==binding.weapon_definition && p.secondary_weapon.definition==pickup_definition,
        "Native mixed pickup did not preserve independent authored hand bindings");
    const auto view=world.source_objects->first_person_view(0);
    need(view.unit==p.source_unit && view.hands[0].weapon==p.source_weapon &&
        view.hands[1].weapon==p.secondary_weapon.datum,"Native FP hand ownership changed");
    need(single?(!view.hands[0].dual_wield && !view.hands[1].dual_wield):
        (view.hands[0].dual_wield && view.hands[1].dual_wield),"Actual native registration has the wrong single/dual class");
    if(extras_only) {
      const auto word=[&](std::uint32_t address){return Bytes(world.source_objects->memory().view(address,4)).u32(0);};
      const auto view_address=word(0x4f6498u);
      for(unsigned hand=0;hand<2;++hand) {
        const auto address=view_address+0xcu+hand*0x1010u;
        const auto canonical_class=word(address+0x7cu);
        for(const auto &[offset,expected]:std::array{
            std::pair{0xd8u,std::string("first_person:dual:pitch_and_turn")},
            std::pair{0xdcu,std::string("first_person:dual:overlays")}}) {
          const auto identifier=word(address+offset);
          const auto actual=record_name(identifier);
          const bool equal=actual==expected && canonical_class==0x0400054bu;
          extras_equal&=equal;
          samples.push_back({{"tick",current.tick},{"hand",hand},{"offset",offset},
              {"canonical_class",canonical_class},{"identifier",identifier},
              {"actual",actual},{"expected",expected},{"passed",equal}});
        }
      }
      if(++frames>=4)break;
      continue;
    }
    std::optional<SourceFirstPersonPosePacketTu15> portable;
    if(packet_only) {
      SourceFirstPersonContextIdentityTu15 identity;
      identity.player=p.source_player;identity.unit=p.source_unit;identity.representation=representation;
      const auto unit=world.source_objects->tu15_world().source_object_body(p.source_unit);
      identity.unit_definition=Bytes(world.source_objects->memory().view(unit,4)).u32(0);
      identity.weapons={p.source_weapon,p.secondary_weapon.datum};
      identity.weapon_definitions={p.weapon,p.secondary_weapon.definition};
      need(p.first_person_pose.has_value(),"Production Match FP phase did not publish its recipient packet");
      const auto &captured=*p.first_person_pose;
      need(captured.temporal.source_tick+1u==current.tick,
          "Production FP packet does not retain its genuine pre-increment phase tick");
      const auto encoded=source_encode_first_person_packet_tu15(captured,identity);
      auto decoded=source_decode_first_person_packet_tu15(encoded,identity);
      need(decoded.consumed==encoded.size(),"Portable native FP codec did not consume its exact packet");
      portable=std::move(decoded.packet);
    }
    if(render_order) {
      // Hold one actual pre-increment phase packet across genuine render
      // completion. The exact collector calls/metrics are the existing native
      // body-grip fixture, with real model residency and original BF640.
      SourcePhysicsWorldRuntimeServices cpu;
#if defined(__i386__) || defined(__x86_64__)
      cpu.reciprocal_sqrt=[](auto bits){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
          _mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
      cpu.timestamp_counter=[](){return __rdtsc();};
#else
      throw FormatError("Native FP render-order test requires actual CPU instruction services");
#endif
      auto &authority=*world.source_objects;auto &receiver_world=packet_assets->tu15_world();
      const auto world_nodes=[&] {
        auto &memory=authority.memory();const auto body=authority.tu15_world().source_object_body(p.source_unit);
        const auto layout=Bytes(memory.view(body+0x114u,4)).u32(0);
        const auto bytes=std::int16_t(layout&65535u),offset=std::int16_t(layout>>16);
        need(bytes>0 && bytes%52==0 && offset>=0,"Render-order unit lost original BAC30 bank");
        std::vector<Transform> nodes(unsigned(bytes)/52u);
        memory.read(body+unsigned(offset),std::as_writable_bytes(std::span(nodes)));return nodes;
      };
      const auto initial_nodes=world_nodes();nlohmann::json cases=nlohmann::json::array();
      Difference baseline,after_collector,foreground,body_only,late_body,late_foreground,cache_matches_direct;
      std::array<std::vector<SourceFirstPersonModelTu15>,2> cached;
      unsigned models_checked{},native_cache_producers{};
      const auto measure=[&](bool after) {
        for(unsigned camera_case=0;camera_case<2;++camera_case) {
          const Vec3 position{1,-2,.5f},forward=camera_case?Vec3{.6f,0,-.8f}:Vec3{1,0,0};
          const Vec3 up=camera_case?Vec3{.8f,0,.6f}:Vec3{0,0,1};
          std::vector<SourceFirstPersonModelTu15> actual;
          authority.render_model_view([&](auto &source){
            actual=source.prepare_first_person_models(0,position,forward,up);
            if(!after) {
              // Actual source_bsp_effect_lights caller input and complete
              // original2BABC producer, before the first WORLD collector.
              const auto no_override=null_datum;
              source.write_source_call_input(0x4c3ae0u,std::as_bytes(std::span(&no_override,1)));
              auto abi=source.prepare_source_call();source.run_actual(0x3d2b0u,abi);++native_cache_producers;
              cached[camera_case]=source_cached_first_person_models_tu15(*world.cache,source.memory(),p.source_unit);
            }
          });
          const auto received=packet_receiver->sample(receiver_world.memory(),receiver_world.prepare_source_call(),
              *portable,portable->temporal.identity,position,forward,up,cpu);
          const auto &reference=cached[camera_case];
          need(actual.size()==reference.size() && reference.size()==received.size(),"Render-order packet changed native collector count");
          Difference frame;unsigned body_models{};
          for(unsigned n=0;n<actual.size();++n) {
            need(reference[n].owner==received[n].owner && reference[n].render_model==received[n].render_model && reference[n].mode==received[n].mode &&
                actual[n].owner==reference[n].owner && actual[n].render_model==reference[n].render_model && actual[n].mode==reference[n].mode,
                "Render-order packet changed native model identity");
            frame.compare(received[n].nodes,reference[n].nodes);++models_checked;
            (after?after_collector:baseline).compare(received[n].nodes,reference[n].nodes);
            if(after) {
              (reference[n].mode==0?body_only:foreground).compare(received[n].nodes,reference[n].nodes);
              (reference[n].mode==0?late_body:late_foreground).compare(actual[n].nodes,reference[n].nodes);
            } else cache_matches_direct.compare(reference[n].nodes,actual[n].nodes);
            body_models+=reference[n].mode==0;
          }
          cases.push_back({{"after_native_third_person_collector",after},{"downward_camera",bool(camera_case)},
              {"models",actual.size()},{"body_models",body_models},{"nodes_checked",frame.count},
              {"position_difference",frame.position},{"basis_difference",frame.basis}});
        }
      };
      authority.begin_model_frame();measure(false);
      for(const auto definition:{portable->temporal.identity.unit_definition,p.weapon,p.secondary_weapon.definition})
        authority.preload_render_model(world.cache->load_initial_model_definition(definition).render_model);
      authority.begin_model_frame();
      authority.render_model_view([&](auto &source) {
        for(const auto object:{p.source_unit,p.source_weapon,p.secondary_weapon.datum})
          (void)source.build_world_model(object,0,1,false,1.f,1.f);
      });
      const auto completed_nodes=world_nodes();Difference changed;changed.compare(completed_nodes,initial_nodes);
      measure(true);
      const bool before_equal=baseline.position<=.0001f && baseline.basis<=.0001f && baseline.scale<=.0001f;
      const bool after_equal=after_collector.position<=.0001f && after_collector.basis<=.0001f && after_collector.scale<=.0001f;
      const bool cache_equal=cache_matches_direct.position<=.0001f && cache_matches_direct.basis<=.0001f && cache_matches_direct.scale<=.0001f;
      const bool regression_seen=changed.basis>.0001f && late_body.basis>.0001f;
      const bool equal=before_equal&&after_equal&&cache_equal&&regression_seen&&native_cache_producers==2;
      nlohmann::json report{{"passed",equal},{"scope","Actual original3D2B0 pre-WORLD FP cache retained across the genuine3Pcollector, compared with one production phase packet and independent native recipient"},
          {"representation",representation},{"native_kernels",true},{"runtime_emulation",false},
          {"phase_tick",portable->temporal.source_tick},{"completed_tick",current.tick},{"phase_packet_held_constant",true},
          {"baseline_passed",before_equal},{"after_collector_passed",after_equal},{"models_checked",models_checked},
          {"native_cache_producers",native_cache_producers},{"cached_group_matches_direct_pre_collector",cache_equal},
          {"late_mutable_reread_regression_observed",regression_seen},
          {"late_mutable_body_position_difference",late_body.position},{"late_mutable_body_basis_difference",late_body.basis},
          {"late_mutable_foreground_position_difference",late_foreground.position},{"late_mutable_foreground_basis_difference",late_foreground.basis},
          {"unit_sim_position_change",changed.position},{"unit_sim_basis_change",changed.basis},
          {"foreground_position_difference",foreground.position},{"foreground_basis_difference",foreground.basis},
          {"body_position_difference",body_only.position},{"body_basis_difference",body_only.basis},{"cases",cases}};
      std::ofstream(argv[3])<<report.dump(2)<<'\n';
      need(equal,"Held native FP cache or packet changed across actual3Pcompletion, or the regression stimulus was absent");
      std::cout<<"PASS native FP phase packet across actual 3P completion\n";return 0;
    }
    world.source_objects->begin_model_frame();
    std::vector<SourceFirstPersonModelTu15> native;
    const auto camera_case=native_only?frames%4u:0u;
    const Vec3 camera_position=camera_case?Vec3{1,-2,.5f}:Vec3{};
    const Vec3 camera_forward=camera_case==3?Vec3{.6f,0,-.8f}:camera_case==2?Vec3{.6f,0,.8f}:camera_case==1?Vec3{.6f,.8f,0}:Vec3{1,0,0};
    const Vec3 camera_up=camera_case==3?Vec3{.8f,0,.6f}:camera_case==2?Vec3{.64f,.6f,-.48f}:Vec3{0,0,1};
    world.source_objects->render_model_view([&](auto &source){
      native=source.prepare_first_person_models(0,camera_position,camera_forward,camera_up);
    });
    if(single && native.empty())continue; // Genuine spawn/draw visibility may omit this frame.
    need(!native.empty(),"Actual native168DED produced no first-person records");
    if(native_only) {
      const auto caller=world.source_objects->tu15_world().prepare_source_call();
      const auto word=[&](std::uint32_t at){return Bytes(world.source_objects->memory().view(at,4)).u32(0);};
      const auto view_at=word(0x4f6498u),locals=word(0x4f6494u);
      if(!frames)native_roots={{"view",view_at},{"locals",locals},{"tag_table",word(0x4f0414u)},
          {"unit_body",world.source_objects->tu15_world().source_object_body(view.unit)},
          {"primary_weapon_body",world.source_objects->tu15_world().source_object_body(view.hands[0].weapon)},
          {"secondary_weapon_body",view.hands[1].weapon==null_datum?0u:
              world.source_objects->tu15_world().source_object_body(view.hands[1].weapon)}};
      NativePoseInput input(world.source_objects->memory());
      for(unsigned hand=0;hand<2;++hand) {
        input.denied.emplace_back(view_at+0xcu+hand*0x1010u+0x304u,0xd04u);
        input.denied.emplace_back(locals+hand*0x1000u,0x800u);
      }
      input.denied.emplace_back(view_at+0x2098u,0x34u);
      input.denied.emplace_back(0x60050000u,4u*0xd0cu);
      SourcePhysicsWorldRuntimeServices cpu;
#if defined(__i386__) || defined(__x86_64__)
      cpu.reciprocal_sqrt=[](auto bits){return std::bit_cast<std::uint32_t>(_mm_cvtss_f32(
          _mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
      cpu.timestamp_counter=[](){return __rdtsc();};
#else
      throw FormatError("Native FP consumer test requires its actual CPU instruction services");
#endif
      cpu.instruction=[&](auto pc){native_instructions.insert(pc);};
      if(packet_only) {
        const auto rebuilt=packet_receiver->sample(packet_assets->memory(),packet_assets->tu15_world().prepare_source_call(),*portable,
            portable->temporal.identity,camera_position,camera_forward,camera_up,cpu);
        need(rebuilt.size()==native.size(),"Portable native FP endpoint changed model count");
        Difference frame,full_source_difference,packet_vs_full;
        std::vector<SourceFirstPersonModelTu15> full_source;
        if(single)full_source=source_sample_first_person_pose_tu15(*world.cache,input,caller,0,
            camera_position,camera_forward,camera_up,cpu);
        if(single)need(full_source.size()==native.size(),"Native single-FP full-source control changed model count");
        nlohmann::json model_samples=nlohmann::json::array();
        for(unsigned n=0;n<native.size();++n) {
          need(rebuilt[n].render_model==native[n].render_model && rebuilt[n].owner==native[n].owner &&
              rebuilt[n].mode==native[n].mode,"Portable native FP endpoint changed model identity");
          frame.compare(rebuilt[n].nodes,native[n].nodes);native_difference.compare(rebuilt[n].nodes,native[n].nodes);
          if(single) {
            need(full_source[n].owner==native[n].owner && full_source[n].render_model==native[n].render_model &&
                full_source[n].mode==native[n].mode,"Native single-FP full-source control changed model identity");
            Difference model,full,delta;model.compare(rebuilt[n].nodes,native[n].nodes);
            full.compare(full_source[n].nodes,native[n].nodes);delta.compare(rebuilt[n].nodes,full_source[n].nodes);
            full_source_difference.compare(full_source[n].nodes,native[n].nodes);
            completed_input_difference.compare(full_source[n].nodes,native[n].nodes);
            packet_vs_full.compare(rebuilt[n].nodes,full_source[n].nodes);
            model_samples.push_back({{"owner",native[n].owner},{"model",native[n].render_model},{"mode",native[n].mode},
                {"nodes",native[n].nodes.size()},{"packet_position_difference",model.position},{"packet_basis_difference",model.basis},
                {"maximum_position_node",model.node},{"full_source_position_difference",full.position},{"full_source_basis_difference",full.basis},
                {"packet_vs_full_source_position_difference",delta.position},{"packet_vs_full_source_basis_difference",delta.basis}});
          }
        }
        native_equal&=frame.position<=.0001f && frame.basis<=.0001f && frame.scale<=.0001f;
        const auto elapsed=tick-*acquired;
        actions_observed[0]|=elapsed>=60 && elapsed<90 && view.hands[0].activity==0x06000006u;
        actions_observed[1]|=elapsed>=90 && elapsed<120 && view.hands[1].activity==0x06000006u;
        actions_observed[2]|=elapsed>=120 && elapsed<150 && view.hands[0].activity==0x0b000065u;
        nlohmann::json sample{{"tick",current.tick},{"elapsed",elapsed},{"position_difference",frame.position},
            {"basis_difference",frame.basis},{"nodes_checked",frame.count},{"camera_case",camera_case}};
        if(single) {
          const auto unit=world.source_objects->tu15_world().source_object_body(p.source_unit);
          const auto extension=std::bit_cast<std::int16_t>(std::uint16_t(word(unit+0x33eu)));
          need(extension>=0,"Actual single-FP unit lost its native transition component");
          std::array<std::byte,3> actual_clock;world.source_objects->memory().read(unit+unsigned(extension)+0x80u,actual_clock);
          const auto bytes=[](const auto &input){nlohmann::json values=nlohmann::json::array();
            for(const auto b:input)values.push_back(std::to_integer<unsigned>(b));return values;};
          sample["phase_unit_transition"]=bytes(portable->actors.single_hand_ik_transition);
          sample["completed_unit_transition"]=bytes(actual_clock);
          sample["full_source_position_difference"]=full_source_difference.position;
          sample["full_source_basis_difference"]=full_source_difference.basis;
          sample["packet_vs_full_source_position_difference"]=packet_vs_full.position;
          sample["packet_vs_full_source_basis_difference"]=packet_vs_full.basis;
          sample["models"]=std::move(model_samples);
          std::array<std::byte,0x34> completed_feedback;
          world.source_objects->memory().read(view_at+0x202cu,completed_feedback);
          const bool advanced=completed_feedback!=portable->temporal.view_feedback;
          feedback_advanced_after_composition|=advanced;
          sample["feedback_advanced_after_composition"]=advanced;
          sample["sampling_feedback"]=bytes(portable->temporal.view_feedback);
          sample["completed_feedback"]=bytes(completed_feedback);
        }
        samples.push_back(std::move(sample));
        ++frames;if(((mixed || single) && frames>=6) || (!mixed && !single && tick>=*acquired+180))break;continue;
      }
      std::vector<std::pair<std::uint32_t,std::uint32_t>> reads;
      const auto rebuilt=source_sample_first_person_pose_tu15(*world.cache,input,caller,0,
          camera_position,camera_forward,camera_up,cpu,[&](auto at,auto bytes){reads.emplace_back(at,bytes);});
      std::sort(reads.begin(),reads.end());
      std::vector<std::pair<std::uint32_t,std::uint32_t>> ranges;
      for(const auto &[at,count]:reads) {
        if(!ranges.empty() && at<=std::uint64_t(ranges.back().first)+ranges.back().second)
          ranges.back().second=std::uint32_t(std::max(std::uint64_t(ranges.back().first)+ranges.back().second,
              std::uint64_t(at)+count)-ranges.back().first);
        else ranges.emplace_back(at,count);
      }
      SourceRuntimeMemory sparse;
      for(const auto &[at,count]:ranges) {
        std::vector<std::byte> bytes(count);input.read(at,bytes);
        sparse.mount(at,bytes,SourceRuntimeMemory::Access::ReadOnly,"Actual initial FP consumer inputs");
      }
      const auto repeated=source_sample_first_person_pose_tu15(*world.cache,sparse,caller,0,
          camera_position,camera_forward,camera_up,cpu);
      need(rebuilt.size()==native.size() && repeated.size()==native.size(),"Native pose consumer changed draw cardinality");
      Difference frame;
      for(unsigned n=0;n<native.size();++n) {
        need(rebuilt[n].render_model==native[n].render_model && rebuilt[n].owner==native[n].owner &&
            rebuilt[n].mode==native[n].mode,"Native pose consumer changed the source model identity");
        frame.compare(rebuilt[n].nodes,native[n].nodes);
        frame.compare(repeated[n].nodes,native[n].nodes);
        native_difference.compare(rebuilt[n].nodes,native[n].nodes);
        native_difference.compare(repeated[n].nodes,native[n].nodes);
      }
      native_equal&=frame.position<=.0001f && frame.basis<=.0001f && frame.scale<=.0001f;
      native_input_ranges.insert(native_input_ranges.end(),ranges.begin(),ranges.end());
      const auto elapsed=tick-*acquired;
      actions_observed[0]|=elapsed>=60 && elapsed<90 && view.hands[0].activity==0x06000006u;
      actions_observed[1]|=elapsed>=90 && elapsed<120 && view.hands[1].activity==0x06000006u;
      actions_observed[2]|=elapsed>=120 && elapsed<150 && view.hands[0].activity==0x0b000065u;
      samples.push_back({{"tick",current.tick},{"elapsed",elapsed},{"position_difference",frame.position},
          {"basis_difference",frame.basis},{"nodes_checked",frame.count},{"initial_input_ranges",ranges.size()},
          {"camera_case",camera_case}});
      ++frames;if(tick>=*acquired+180)break;continue;
    }
    const auto native_arms=std::find_if(native.begin(),native.end(),[&](const auto &m){
      return m.mode==1 && m.owner==view.unit && m.render_model==binding.arms_model;
    });
    need(native_arms!=native.end(),"Actual native first-person arms bank is absent");
    Transform primary_adjustment;
    for(unsigned hand=0;hand<2;++hand) {
      const auto &h=view.hands[hand];
      if(!clips.contains(h.animation_identifier))
        clips.emplace(h.animation_identifier,world.cache->load_animation_identifier(h.graph,h.animation_identifier));
      const auto &clip=clips.at(h.animation_identifier);
      const auto &ammo=binding.ammunition[h.dual_wield?1:0];
      const int loaded=hand?p.secondary_weapon.loaded:p.loaded;
      const int available=hand?p.secondary_weapon.ammunition_available:p.ammunition_available;
      const auto reload=hand?p.secondary_weapon.reload_tick:p.reload_tick;
      const float ammo_frame=ammo?first_person_ammunition_frame(std::int16_t(loaded),
          std::bit_cast<std::int16_t>(std::uint16_t(available)),std::int16_t(definition.capacity),
          std::bit_cast<std::int16_t>(std::uint16_t(current.tick-reload)),definition.weapon_type,
          h.activity,ammo->frame_count):0;
      // These are the actual joined renderer's exact completed-clock inputs,
      // with no interpolation and the same declared native camera.
      const auto replica=pose_first_person_models_world_frame(gun,arms,clip,h.frame,
          definition.first_person_offset,camera,ik,0,hand!=0,hand?&primary_adjustment:nullptr,
          ammo?&*ammo:nullptr,ammo_frame);
      if(!hand)primary_adjustment=replica.camera_adjustment;
      const auto original=std::find_if(native.begin(),native.end(),[&](const auto &m){
        return m.mode==1 && m.owner==h.weapon && m.render_model==binding.weapon_model;
      });
      need(original!=native.end(),"Actual native weapon bank is absent");
      Difference weapon_sample,arms_sample;
      weapon_sample.compare(replica.weapon_nodes,original->nodes);
      weapon_difference[hand].compare(replica.weapon_nodes,original->nodes);
      const auto selected=[&](unsigned node) {
        const auto at=std::find(clip.nodes.begin(),clip.nodes.end(),arms.bones.at(node).name);
        need(at!=clip.nodes.end(),"Replica FP arm lost authored graph identity");
        return bool(clip.node_flags.at(at-clip.nodes.begin())&0x20u)==bool(hand);
      };
      arms_sample.compare(replica.arms_nodes,native_arms->nodes,selected);
      arms_difference[hand].compare(replica.arms_nodes,native_arms->nodes,selected);
      const auto elapsed=tick-*acquired;
      const unsigned phase=elapsed<15?0:elapsed<40?1:2;
      const unsigned action=elapsed<20?0:elapsed<40?1:elapsed<60?2:elapsed<90?3:elapsed<120?4:elapsed<150?5:6;
      action_weapon[action][hand].compare(replica.weapon_nodes,original->nodes);
      action_arms[action][hand].compare(replica.arms_nodes,native_arms->nodes,selected);
      if(dynamic) {
        actions_observed[0]|=action==3 && hand==0 && h.activity==0x06000006u;
        actions_observed[1]|=action==4 && hand==1 && h.activity==0x06000006u;
        actions_observed[2]|=action==5 && hand==0 && h.activity==0x0b000065u;
      }
      phase_weapon[phase][hand].compare(replica.weapon_nodes,original->nodes);
      phase_arms[phase][hand].compare(replica.arms_nodes,native_arms->nodes,selected);
      const auto delta=compose_transform(original->nodes.front(),inverse_transform(replica.weapon_nodes.front()));
      auto rigid_weapon=replica.weapon_nodes,rigid_arms=replica.arms_nodes;
      for(auto &node:rigid_weapon)node=compose_transform(delta,node);
      for(auto &node:rigid_arms)node=compose_transform(delta,node);
      Difference rigid_weapon_sample,rigid_arms_sample;
      rigid_weapon_sample.compare(rigid_weapon,original->nodes);
      rigid_arms_sample.compare(rigid_arms,native_arms->nodes,selected);
      if(samples.size()<512)samples.push_back({{"tick",current.tick},{"hand",hand},{"elapsed",elapsed},{"phase",phase},
          {"action_phase",action},{"action_name",action_names[action]},
          {"declared_input",{{"yaw",controls[slot].yaw},{"pitch",controls[slot].pitch},
              {"forward",controls[slot].forward},{"right",controls[slot].right},
              {"primary_fire",controls[slot].fire},{"secondary_fire",controls[slot].fire_secondary},{"reload",controls[slot].reload}}},
          {"activity",h.activity},{"identifier",h.animation_identifier},{"frame",h.frame},
          {"weapon_position_difference",weapon_sample.position},{"weapon_basis_difference",weapon_sample.basis},
          {"arms_position_difference",arms_sample.position},{"arms_basis_difference",arms_sample.basis},
          {"weapon_maximum_node",weapon_sample.node},{"arms_maximum_node",arms_sample.node},
          {"weapon_maximum_node_name",world.cache->string_id(gun.bones.at(weapon_sample.node).name)},
          {"arms_maximum_node_name",world.cache->string_id(arms.bones.at(arms_sample.node).name)},
          {"rigid_weapon_position_residual",rigid_weapon_sample.position},
          {"rigid_weapon_basis_residual",rigid_weapon_sample.basis},
          {"rigid_arms_position_residual",rigid_arms_sample.position},
          {"rigid_arms_basis_residual",rigid_arms_sample.basis}});
    }
    ++frames;
    if(tick>=*acquired+(dynamic?180u:70u))break;
  }
  need(acquired && frames>=(extras_only?4u:(mixed || single)?6u:60u),"Native FP equivalence did not obtain the complete actual binding scenario");
  if(extras_only) {
    nlohmann::json report{{"scope","Actual native dual FP cached extra channels against authored named records"},
        {"native_kernels",true},{"runtime_emulation",false},{"representation",representation},
        {"dual_acquired_tick",*acquired},{"frames",frames},{"samples",samples},{"passed",extras_equal}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    need(extras_equal,"Native dual FP cached extras selected a single-weapon authored record");
    std::cout<<"PASS native dual first-person cached extra channels\n";return 0;
  }
  if(native_only) {
    std::sort(native_input_ranges.begin(),native_input_ranges.end());
    std::vector<std::pair<std::uint32_t,std::uint32_t>> ranges;
    for(const auto &[at,count]:native_input_ranges) {
      if(!ranges.empty() && at<=std::uint64_t(ranges.back().first)+ranges.back().second)
        ranges.back().second=std::uint32_t(std::max(std::uint64_t(ranges.back().first)+ranges.back().second,
            std::uint64_t(at)+count)-ranges.back().first);
      else ranges.emplace_back(at,count);
    }
    const bool sampled=native_instructions.contains(0x16a4ddu) && native_instructions.contains(0x168dedu);
    const bool transitions=native_instructions.contains(0x1cf210u) && native_instructions.contains(0x1cf3b0u) && native_instructions.contains(0x1d4560u);
    native_equal&=sampled && (mixed || single || (transitions && std::all_of(actions_observed.begin(),actions_observed.end(),[](bool v){return v;})));
    const bool temporal_regression=single && representation==3;
    if(temporal_regression)native_equal&=feedback_advanced_after_composition && completed_input_difference.basis>.0001f;
    nlohmann::json report{{"scope","Production native FP consumer against actual native authority; previous completed outputs forbidden"},
        {"native_kernels",true},{"runtime_emulation",false},{"representation",representation},{"frames",frames},
        {"position_units","world units"},{"current_completed_banks_denied",true},{"sparse_initial_inputs_only",!packet_only},
        {"typed_actor_inputs_only",packet_only},
        {"local_camera_cases",4},
        {"native_sampler_executed",sampled},{"native_sampler_and_transition_instructions_executed",sampled&&transitions},
        {"source_actions_observed",actions_observed},
        {"position_difference",native_difference.position},{"basis_difference",native_difference.basis},
        {"scale_difference",native_difference.scale},{"nodes_checked",native_difference.count},{"samples",samples},
        {"native_roots",native_roots},{"initial_input_ranges",ranges},{"passed",native_equal}};
    report["portable_packet_endpoint"]=packet_only;
    report["mixed_authored_bindings"]=mixed;
    report["single_weapon_binding"]=single;
    report["independent_native_asset_owner"]=packet_only;
    report["joined_prepared_world_has_no_gameplay_owner"]=packet_only;
    report["actual_phase_registry_packet"]=packet_only;
    report["feedback_advanced_after_composition"]=feedback_advanced_after_composition;
    report["completed_input_position_difference"]=completed_input_difference.position;
    report["completed_input_basis_difference"]=completed_input_difference.basis;
    report["precomposition_input_regression_required"]=temporal_regression;
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    need(native_equal,"Production native FP consumer differs from the completed authority pose");
    if(packet_only)std::cout<<"PASS production native FP packet consumer with typed inputs and independent local assets\n";
    else std::cout<<"PASS production native FP consumer, denied completed outputs and sparse initial inputs\n";
    return 0;
  }
  nlohmann::json report{{"scope","Actual native168DED vs actual joined completed-clock reconstruction"},
      {"native_kernels",true},{"runtime_emulation",false},{"representation",representation},
      {"dynamic_scenario",dynamic},{"source_actions_observed",actions_observed},
      {"dual_acquired_tick",*acquired},{"frames",frames},{"samples",samples},{"position_units","world units"},
      {"supplied_inputs","Same canonical source clock/frame/graph, camera and authoritative ammo; actual native dual pickup"}};
  bool equal=true;
  for(unsigned hand=0;hand<2;++hand) {
    const auto &w=weapon_difference[hand],&a=arms_difference[hand];
    report["hand"][hand]={{"weapon_position_difference",w.position},{"weapon_basis_difference",w.basis},
        {"arms_position_difference",a.position},{"arms_basis_difference",a.basis},
        {"weapon_nodes_checked",w.count},{"arms_nodes_checked",a.count}};
    equal&=w.position<=.0001f && w.basis<=.0001f && a.position<=.0001f && a.basis<=.0001f;
  }
  for(unsigned phase=0;phase<3;++phase)for(unsigned hand=0;hand<2;++hand) {
    const auto &w=phase_weapon[phase][hand],&a=phase_arms[phase][hand];
    report["phase"][phase][hand]={{"weapon_position_difference",w.position},{"weapon_basis_difference",w.basis},
        {"arms_position_difference",a.position},{"arms_basis_difference",a.basis},
        {"weapon_nodes_checked",w.count},{"arms_nodes_checked",a.count}};
  }
  for(unsigned action=0;action<7;++action)for(unsigned hand=0;hand<2;++hand) {
    const auto &w=action_weapon[action][hand],&a=action_arms[action][hand];
    report["action_phase"][action][hand]={{"name",action_names[action]},
        {"weapon_position_difference",w.position},{"weapon_basis_difference",w.basis},
        {"arms_position_difference",a.position},{"arms_basis_difference",a.basis},
        {"weapon_nodes_checked",w.count},{"arms_nodes_checked",a.count}};
  }
  equal&=!dynamic || std::all_of(actions_observed.begin(),actions_observed.end(),[](bool v){return v;});
  report["passed"]=equal;std::ofstream(argv[3])<<report.dump(2)<<'\n';
  std::cout<<report["hand"].dump()<<'\n';
  need(equal,"Joined first-person completed pose differs from the original native pose at the same inputs");
  std::cout<<"PASS actual native and joined completed first-person pose equivalence\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
