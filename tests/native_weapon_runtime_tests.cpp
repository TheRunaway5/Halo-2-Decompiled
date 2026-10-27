#include "helpers/gameplay_fixture.hpp"
#include "helpers/first_person_dual_variants.hpp"
#define SDL_MAIN_HANDLED
#include "halo2/animation_presentation.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/model.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <set>
#include <thread>
using namespace halo2;
namespace {
struct HistoryReport {
  const char *path=std::getenv("HALO2_TEST_DUAL_HISTORY_REPORT");
  nlohmann::json data{{"samples",nlohmann::json::array()},{"render_loop_completed",false},
      {"full_weapon_parity",false},{"original_position_owner",true},
      {"diagnostic_placement","Pinned at the current authored pickup row through original B95D0"}};
  ~HistoryReport(){if(path){std::ofstream file(path);file<<data.dump()<<'\n';}}
};
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
void key(Uint32 window,SDL_Scancode scan) {
  SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=window;e.key.keysym.scancode=scan;e.key.state=SDL_PRESSED;
  check(SDL_PushEvent(&e)==1,"Queue weapon key press");e.type=SDL_KEYUP;e.key.state=SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue weapon key release");
}
void hold_key(Uint32 window,SDL_Scancode scan,bool down) {
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;e.key.keysym.scancode=scan;
  e.key.state=down?SDL_PRESSED:SDL_RELEASED;check(SDL_PushEvent(&e)==1,"Queue held interaction key");
}
void trigger(Uint32 window,Uint8 button,bool down) {
  SDL_Event e{};e.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;e.button.windowID=window;
  e.button.button=button;e.button.state=down?SDL_PRESSED:SDL_RELEASED;
  check(SDL_PushEvent(&e)==1,"Queue weapon trigger");
}
void configure_test_display(DesktopSession &desktop,nlohmann::json &report,bool gpu=false) {
  const auto *requested=std::getenv("HALO2_TEST_RENDER_MODE");
  const std::string mode=requested?requested:"";
  check(mode.empty() || mode=="original" || mode=="pstencil","Unsupported private test render mode");
  if(!gpu && mode.empty())return;
  check(!gpu || mode.empty() || mode=="original","GPU verification requires the Original test render mode");
  const bool pstencil_mode=!gpu && mode=="pstencil";
  desktop.display_settings.graphics_mode=pstencil_mode?pstencil::GraphicsMode::pStencil:pstencil::GraphicsMode::Original;
  desktop.display_settings.upscaler=Upscaler::Off;
  desktop.upscaling_preferences_initialized=true;
  report["declared_display"]={{"graphics_mode",pstencil_mode?"pStencil":"Original"},{"upscaler","Off"},
      {"gpu_verification",gpu},{"private_session_only",true},{"preferences_saved",false}};
}
nlohmann::json retained_body_manager(MultiplayerObjectRuntime &source,std::uint32_t unit) {
  const auto body=source.source_object_body(unit);const auto &memory=source.memory();
  const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
  const auto offset=[&](std::uint32_t at){return std::bit_cast<std::int16_t>(Bytes(memory.view(at,2)).u16(0));};
  const auto manager_offset=offset(body+0x12au),extension_offset=offset(body+0x33eu);
  nlohmann::json result{{"unit",unit},{"source_body",body},{"manager_offset",manager_offset},
      {"extension_offset",extension_offset}};
  if(manager_offset>0) {
    const auto manager=body+std::uint32_t(manager_offset);
    result["manager"]=manager;result["mode_70"]=word(manager+0x70u);
    result["class_74"]=word(manager+0x74u);result["type_78"]=word(manager+0x78u);
    result["activity_7c"]=word(manager+0x7cu);
  }
  if(extension_offset>0) {
    const auto extension=body+std::uint32_t(extension_offset);
    result["primary_context_94"]=word(extension+0x94u);result["secondary_context_98"]=word(extension+0x98u);
    result["arm_104"]=word(extension+0x104u);result["arm_108"]=word(extension+0x108u);
    result["grip_10c"]=word(extension+0x10cu);result["grip_110"]=word(extension+0x110u);
  }
  return result;
}
void record_body_manager(nlohmann::json &trace,std::optional<nlohmann::json> &previous,
    nlohmann::json context,unsigned tick) {
  if(previous && *previous==context)return;
  previous=context;
  if(trace.size()<64){context["tick"]=tick;trace.push_back(std::move(context));}
}
// Exercise the joining client's ordinary render path. The offline dual test
// below obtains its matrices from168DED; a joining client instead consumes
// the authority's two clock channels and its own authored model bindings.
// Keeping this case separate prevents the offline source draw from proving
// coverage of a path it never enters.
template<class Channel>
bool check_authored_dual_channel(const Channel &channel,const test::AuthoredFirstPersonDualVariants &variants) {
  if(!variants.has_activity(channel.activity))return false;
  check(variants.contains(channel.graph,channel.activity,channel.animation_identifier),
      "Native dual channel left its actual authored dual-class variant chain or graph");
  return true;
}
int network_dual_runtime(const MultiplayerOptions &request,unsigned representation,nlohmann::json &report) {
  report["scenario"]="native-authority-to-joining-client-dual";
  struct Host {
    std::string address;
    std::exception_ptr error;
    std::mutex mutex;
    nlohmann::json body_manager_context=nlohmann::json::array();
    std::jthread worker;
    FirstPersonBindingDefinition dual_binding;
    std::optional<test::AuthoredFirstPersonDualVariants> dual_variants;
    Host(MultiplayerOptions options,unsigned representation) {
      options.host_port=1; // Select host preparation; LanHost owns an ephemeral socket below.
      if(options.starting_weapon)options.variant.values.set(89,int(options.starting_weapon));
      std::promise<std::string> ready;auto startup=ready.get_future();
      worker=std::jthread([this,options,representation,ready=std::move(ready)](std::stop_token stop)mutable {
        bool published{};
        try {
          MultiplayerPreparedWorld world(options);world.bind_source_local_views(world.local_slots);
          check(world.cache->is_mcc(),"Network dual runtime requires prepared MCC assets");
          check(world.source_objects && world.source_objects->owns_physics(),
              "Network dual host has no native TU1.5 world");
          dual_binding=world.cache->load_first_person_binding(world.rules.weapon,representation);
          dual_variants.emplace(*world.cache,dual_binding);
          const NetgameEquipment *pickup=nullptr;
          for(const auto &row:world.content.netgame_equipment)
            if(!row.classification && source_netgame_equipment_selector(row,true,world.rules.source_engine)>=0 &&
                row.items.size()==1 && row.items.front().definition==world.rules.weapon) {
              pickup=&row;break;
            }
          check(pickup,"Network dual host has no authored starting-weapon pickup");
          const auto fingerprint=multiplayer_fingerprint(options.map,world.cache->shared_directory(),
              unsigned(options.bsp),world.network_starting_weapon_identity());
          LanHost host(world.match,0,fingerprint,options.variant);
          ready.set_value(host.join_address());published=true;
          std::array<Controls,multiplayer_capacity> commands{};
          std::optional<nlohmann::json> previous_body_manager;
          auto next=std::chrono::steady_clock::now();
          while(!stop.stop_requested()) {
            host.receive(commands);
            const auto state=world.match.state();
            for(unsigned id=0;id<state.players.size();++id)
              if(state.players[id].connected && id!=world.local_slots.front() &&
                  state.players[id].source_unit!=null_datum && !state.players[id].dead) {
                // Declared diagnostic placement only. The original native
                // pickup/action/clock owners still perform the transfer.
                auto position=pickup->position;position.x-=.2f;
                world.source_objects->tu15_world().set_object_position(state.players[id].source_unit,position);
              }
            world.match.step(commands);
            const auto completed=world.match.state();
            for(unsigned id=0;id<completed.players.size();++id)
              if(completed.players[id].connected && id!=world.local_slots.front() &&
                  completed.players[id].source_unit!=null_datum) {
                auto context=retained_body_manager(*world.source_objects,completed.players[id].source_unit);
                std::lock_guard lock(mutex);
                record_body_manager(body_manager_context,previous_body_manager,std::move(context),completed.tick);
              }
            host.broadcast();
            next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
          }
        }catch(...) {
          if(!published)ready.set_exception(std::current_exception());
          std::lock_guard lock(mutex);error=std::current_exception();
        }
      });
      address=startup.get();
    }
    void finish() {
      worker.request_stop();if(worker.joinable())worker.join();
      std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);
    }
  };
  Host host(request,representation);report["native_world_constructor_completed"]=true;report["runtime_cache_source"]="MCC";
  auto options=request;options.join=host.address;
  options.frames=0;options.width=800;options.height=600;options.mute=true;
  options.desktop=std::make_shared<DesktopSession>();
  configure_test_display(*options.desktop,report);
  auto *window=options.desktop->window(options.width,options.height,false);
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Network dual could not capture its actual SDL mouse");
  enum class Stage {pickup,ready,offhand,primary,reload,done};Stage stage=Stage::pickup;
  std::optional<unsigned> slot;std::uint32_t stage_tick{};
  std::array<std::uint32_t,2> weapons{null_datum,null_datum};
  std::array<unsigned,2> draws{};
  std::array<std::set<std::vector<std::uint32_t>>,2> poses;
  std::array<std::set<std::pair<std::uint32_t,std::uint32_t>>,2> channels;
  std::array<std::set<std::uint32_t>,2> hand_clips;
  bool offhand_fired{},primary_fired{},reloaded{},quit{};
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(180);
  options.input_observer=[&](unsigned,std::span<const Controls>,const MultiplayerState &state) {
    check(std::chrono::steady_clock::now()<deadline,"Network dual SDL sequence timed out");
    if(SDL_GetKeyboardFocus()!=window){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);}
    if(!slot)for(unsigned id=1;id<state.players.size();++id)
      if(state.players[id].connected && state.players[id].source_unit!=null_datum){slot=id;break;}
    if(!slot)return;
    const auto &player=state.players[*slot];
    check(!player.dead,"Network dual participant died before its pose assertions");
    weapons={player.source_weapon,player.secondary_weapon.datum};
    report["network_dual"]={{"tick",state.tick},{"slot",*slot},{"stage",unsigned(stage)},
        {"weapons",weapons},{"draws",draws},{"primary_clock",player.first_person_clock.present},
        {"offhand_clock",player.secondary_weapon.clock.present},{"primary_loaded",player.loaded},
        {"offhand_loaded",player.secondary_weapon.loaded},
        {"channel_samples",{channels[0].size(),channels[1].size()}},
        {"clips_per_hand",{hand_clips[0].size(),hand_clips[1].size()}}};
    {std::lock_guard lock(host.mutex);report["authority_body_manager_context"]=host.body_manager_context;}
    const auto advance=[&](Stage next){stage=next;stage_tick=state.tick;};
    if(stage==Stage::pickup) {
      hold_key(window_id,SDL_SCANCODE_X,true);
      if(weapons[1]!=null_datum){hold_key(window_id,SDL_SCANCODE_X,false);advance(Stage::ready);}
    }else if(stage==Stage::ready && state.tick-stage_tick>=20) {
      check(player.first_person_clock.present && player.secondary_weapon.clock.present,
          "Joining-client dual authority did not publish both native pose channels");
      check(check_authored_dual_channel(player.first_person_clock.current,*host.dual_variants) &&
          check_authored_dual_channel(player.secondary_weapon.clock.current,*host.dual_variants),
          "Joining-client dual authority did not select an authored dual-class pose");
      check(draws[0] && draws[1],"Joining-client dual pickup did not render both authoritative weapon owners");
      trigger(window_id,SDL_BUTTON_RIGHT,true);advance(Stage::offhand);
    }else if(stage==Stage::offhand) {
      offhand_fired|=player.secondary_weapon.loaded<60;
      check(player.loaded==60,"Joining-client offhand trigger consumed primary ammunition");
      if(state.tick-stage_tick>=10) {
        trigger(window_id,SDL_BUTTON_RIGHT,false);trigger(window_id,SDL_BUTTON_LEFT,true);advance(Stage::primary);
      }
    }else if(stage==Stage::primary) {
      primary_fired|=player.loaded<60;
      if(state.tick-stage_tick>=10) {
        trigger(window_id,SDL_BUTTON_LEFT,false);key(window_id,SDL_SCANCODE_R);advance(Stage::reload);
      }
    }else if(stage==Stage::reload) {
      reloaded|=offhand_fired && primary_fired && player.loaded==60 && player.secondary_weapon.loaded==60;
      if(reloaded && state.tick-stage_tick>=10)advance(Stage::done);
    }
    if(stage!=Stage::pickup)check(weapons[0]!=null_datum && weapons[1]!=null_datum,
        "Joining-client dual sequence lost an authoritative hand owner");
    if(stage==Stage::done && !quit && draws[0]>10 && draws[1]>10 && poses[0].size()>3 && poses[1].size()>3 &&
        channels[0].size()>3 && channels[1].size()>3 && hand_clips[0].size()>=3 && hand_clips[1].size()>=3) {
      SDL_Event event{};event.type=SDL_QUIT;check(SDL_PushEvent(&event)==1,"Exit network dual runtime");quit=true;
    }
  };
  options.animation_observer=[&](unsigned view,unsigned player,const AnimationRenderObservation &draw) {
    if(view || !slot || player!=*slot || !draw.first_person || weapons[1]==null_datum)return;
    for(unsigned hand=0;hand<2;++hand)if(draw.weapon==weapons[hand]) {
      check(!draw.vertices.empty() && !draw.weapon_nodes.empty(),"Joining-client dual hand submitted no geometry/nodes");
      ++draws[hand];std::vector<std::uint32_t> words;words.reserve(draw.weapon_nodes.size()*13);
      channels[hand].insert({draw.animation,std::bit_cast<std::uint32_t>(draw.frame)});
      hand_clips[hand].insert(draw.animation);
      for(const auto &node:draw.weapon_nodes) {
        const auto values=std::bit_cast<std::array<std::uint32_t,13>>(node);
        words.insert(words.end(),values.begin(),values.end());
      }
      poses[hand].insert(std::move(words));
    }
  };
  const auto result=run_multiplayer(options);host.finish();
  check(result==0 && quit && reloaded,"Joining-client dual fire/reload sequence did not complete");
  for(unsigned hand=0;hand<2;++hand)
    check(draws[hand]>10 && poses[hand].size()>3 && channels[hand].size()>3 && hand_clips[hand].size()>=3,
        "Joining-client dual weapon was absent or its own animation channel stayed frozen");
  report["render_loop_completed"]=true;report["network_dual"]["passed"]=true;
  report["network_dual"]["draws"]=draws;
  report["network_dual"]["distinct_node_banks"]={poses[0].size(),poses[1].size()};
  report["network_dual"]["distinct_channel_samples"]={channels[0].size(),channels[1].size()};
  report["network_dual"]["clips_per_hand"]={hand_clips[0].size(),hand_clips[1].size()};
  std::cout<<"PASS native MCC authority -> authenticated localhost UDP -> joining-client SDL dual pickup, "
      "independent fire/reload, actual hand draws="<<draws[0]<<','<<draws[1]
      <<" distinct node banks="<<poses[0].size()<<','<<poses[1].size()<<'\n';
  std::cout<<"LIMIT localhost transport; no WAN/DTLS, framebuffer or original-hardware parity claim\n";
  return 0;
}
}
int main(int argc,char **argv){HistoryReport history;try {
  check(argc==4 || argc==5,"Supply MAP SHARED XBE [CAPTURE]");
  const bool mcc=std::string(argv[3])=="native-mcc";
  history.data["runtime_cache_source"]=mcc?"prepared MCC requested":"offline reference requested";
  history.data["runtime_cache_path"]=argv[1];
  history.data["native_world_constructor_completed"]=false;
  if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]) || (!mcc && !std::filesystem::exists(argv[3])))return 77;
  SDL_SetHintWithPriority(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0xffff/0xffff",SDL_HINT_OVERRIDE);
  SDL_SetMainReady();MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  if(!mcc)options.gameplay_xbe=argv[3];
  options.width=800;options.height=600;options.local_players=1;options.starting_weapon=5;options.mute=false;options.input_instances={-1};
  const auto *representation_option=std::getenv("HALO2_TEST_REPRESENTATION");
  const unsigned representation=representation_option?unsigned(std::stoul(representation_option)):2u;
  check(representation==2 || representation==3,"Unsupported actual multiplayer representation");
  if(representation==3){auto profile=source_default_player_configuration();profile[0x44]=1;options.selected_profiles={profile};}
  history.data["representation"]=representation;
  options.validation=std::getenv("HALO2_ANIMATION_VALIDATION");
  if(const auto *artwork=std::getenv("HALO2_TEST_HUD_UI_ASSETS"))options.ui_assets=artwork;
  if(std::getenv("HALO2_TEST_NETWORK_DUAL")) {
    check(mcc && argc==4,"Network dual validation requires prepared MCC and no capture argument");
    return network_dual_runtime(options,representation,history.data);
  }
  options.frames=argc==5?0u:320u;options.reference_tick_cadence=true;options.reference_render_fraction=.5f;
  const bool verify_world_poses=std::getenv("HALO2_TEST_DUAL_WORLD_POSES")!=nullptr;
  const bool verify_dual_gpu=std::getenv("HALO2_TEST_DUAL_GPU")!=nullptr;
  if(verify_dual_gpu) {
    check(mcc,"Dual GPU validation requires prepared MCC assets");
    check(std::getenv("HALO2_VERIFY_GPU_FIRST_PERSON_INPUTS") && std::getenv("HALO2_VERIFY_GPU_SKINNING"),
        "Dual GPU validation requires source-bank and actual GPU readback verification");
    check(!verify_world_poses,"Dual GPU validation uses passive nodes rather than posed WORLD draw observers");
  }
  if(verify_world_poses)options.reference_render_fraction=1.f; // Exact completed-tick original palette comparison.
  options.prepared_world=std::make_shared<MultiplayerPreparedWorld>(options);
  const auto &world=*options.prepared_world;
  check(!mcc || world.cache->is_mcc(),"Weapon runtime testing requires an MCC cache");
  history.data["representation"]=representation;
  history.data["native_world_constructor_completed"]=true;
  history.data["runtime_cache_source"]=mcc?"MCC":"offline reference";
  history.data["runtime_cache_path"]=argv[1];
  history.data["expected_unit_definition"]=world.content.player_representations.at(representation).biped.datum;
  const NetgameEquipment *row=nullptr,*primary_row=nullptr;
  for(const auto &r:world.content.netgame_equipment)
    if(!r.classification && source_netgame_equipment_selector(r,true,world.rules.source_engine)>=0 &&
        r.items.size()==1) {
      if(!row && r.items[0].definition==world.rules.weapon)row=&r;
      const auto w=world.content.weapons.find(r.items[0].definition);
      if(!primary_row && w!=world.content.weapons.end() && w->second.name.find("battle_rifle")!=std::string::npos)
        primary_row=&r;
    }
  check(row && primary_row,"Actual map has no SMG/battle-rifle pickup rows");
  const auto primary_row_index=unsigned(primary_row-world.content.netgame_equipment.data());
  const auto smg_row_index=unsigned(row-world.content.netgame_equipment.data());
  history.data["authored_pickup_rows"]={{"primary",primary_row_index},{"smg",smg_row_index},
      {"primary_definition",primary_row->items[0].definition},{"smg_definition",row->items[0].definition}};
  // Only initial diagnostic placement is supplied. Item allocation, transfer,
  // clocks, trigger/reload/grenade outcomes and poses come from the live loop.
  const auto location=row->position;const auto slot=world.local_slots.at(0);
  options.desktop=std::make_shared<DesktopSession>();
  configure_test_display(*options.desktop,history.data,verify_dual_gpu);
  auto *window=options.desktop->window(800,600,false);
  SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);
  std::cout<<"Native SDL video driver: "<<SDL_GetCurrentVideoDriver()<<'\n';
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  std::optional<unsigned> start,carried_at,dual_at,grenade_at;MultiplayerState current;unsigned step{},draws{};
  std::uint32_t primary_target=null_datum,starting_datum=null_datum;
  bool map_pickup{},switched{},dual_requested{};
  bool dual{},right_fired{},left_fired{},reloaded{},thrown{},dropped{};
  bool offhand_radar{},primary_radar{},grenade_radar_phase{};
  bool reload_received{},lost_focus{};
  unsigned firing_particles{},firing_sounds{},fired_rounds{};
  std::optional<std::array<int,2>> previous_ammunition;
  const auto firing_sound=world.content.weapons.at(world.rules.weapon).sound;
  const auto dual_binding=verify_dual_gpu?world.cache->load_first_person_binding(world.rules.weapon,representation):
      FirstPersonBindingDefinition{};
  std::optional<test::AuthoredFirstPersonDualVariants> dual_variants;
  if(verify_dual_gpu)dual_variants.emplace(*world.cache,dual_binding);
  std::set<std::uint32_t> posed_weapons;std::set<unsigned> clips;
  std::array<std::set<std::uint64_t>,2> gpu_hand_nodes;
  std::array<std::set<std::pair<std::uint32_t,std::uint32_t>>,2> gpu_hand_channels;
  std::array<std::set<std::uint32_t>,2> gpu_hand_activities;
  std::array<std::optional<std::uint64_t>,2> gpu_latest_nodes;
  unsigned gpu_dual_frames{},gpu_distinct_hand_frames{};
  std::uint64_t gpu_verified_vertices{},gpu_foreground_vertices{},gpu_deformed_vertices{};
  unsigned history_checks{},history_mismatches{};
  std::optional<nlohmann::json> previous_body_manager;
  history.data["body_manager_context"]=nlohmann::json::array();
  unsigned world_pose_publications{},world_pose_draws{},dropped_pose_draws{},world_pose_mismatches{},world_palette_matrices{};
  history.data["world_pose_metrics"]={{"enabled",verify_world_poses},{"render_fraction",options.reference_render_fraction}};
  const bool follow_drop=std::getenv("HALO2_TEST_DUAL_DROP_LIFETIME")!=nullptr;
  const bool trace_dual_selection=std::getenv("HALO2_TEST_TRACE_DUAL_SELECTION")!=nullptr;
  std::uint32_t offhand_datum=null_datum;
  unsigned drop_checks{},drop_mismatches{},drop_preview_differences{};
  std::optional<unsigned> drop_tick,retired_tick;
  const auto dropped_model=follow_drop?std::optional{world.cache->load_model(world.rules.weapon,false)}:std::nullopt;
  const auto dropped_idle=follow_drop?world.cache->load_initialized_object_idle(world.rules.weapon):std::nullopt;
  const auto observe_drop=[&](const MultiplayerState &state,unsigned tick,bool rendered) {
    if(!follow_drop || offhand_datum==null_datum || state.players[slot].secondary_weapon.datum!=null_datum)return;
    auto *source=options.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Drop lifetime requires the original world owner");
    const auto objects=source->live_objects();
    const auto object=std::find_if(objects.begin(),objects.end(),[&](const auto &o){return o.object==offhand_datum;});
    const auto count=std::count_if(state.world_weapons.begin(),state.world_weapons.end(),
        [&](const auto &w){return w.handle==offhand_datum;});
    const auto &inventory=options.prepared_world->match.unit(slot).source_inventory.slots;
    const bool owned=std::find(inventory.begin(),inventory.end(),offhand_datum)!=inventory.end();
    const auto &memory=source->memory();
    const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
    if(!drop_tick)drop_tick=tick;
    ++drop_checks;
    if(object==objects.end()) {
      if(!retired_tick)retired_tick=tick;
      drop_mismatches+=count!=0 || owned;
      history.data["drop_retired_tick"]=*retired_tick;
      return;
    }
    const auto body=source->object_body(offhand_datum);
    const auto root=source->object_transform(offhand_datum);
    const auto item=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),
        [&](const auto &w){return w.handle==offhand_datum;});
    const auto loaded=std::bit_cast<std::int16_t>(Bytes(memory.view(body+0x22cu,2)).u16(0));
    const auto reserve=std::bit_cast<std::int16_t>(Bytes(memory.view(body+0x22au,2)).u16(0));
    const bool matches=!owned && object->parent==null_datum && object->type==2 &&
        object->definition==world.rules.weapon && count==1 && item!=state.world_weapons.end() &&
        item->definition==object->definition && item->loaded==loaded && item->reserve==reserve &&
        !std::memcmp(&item->position,&root.position,sizeof(Vec3));
    drop_mismatches+=!matches;
    const auto nodes=source->world_nodes(offhand_datum);
    const auto preview=pose_model_nodes_world(*dropped_model,
        source_object_transform(root.position,root.basis[0],root.basis[2],1,false),
        dropped_idle?&*dropped_idle:nullptr,float(tick)/30.f);
    const bool same=nodes.size()==preview.size() &&
        (nodes.empty() || !std::memcmp(nodes.data(),preview.data(),nodes.size()*sizeof(Transform)));
    drop_preview_differences+=!same;
    // Source and preview are compared at one completed simulation tick here,
    // not claimed as an interpolated GPU/framebuffer comparison.
    if(rendered || tick-*drop_tick<4 || (tick-*drop_tick)%30==0) {
      std::array<std::uint32_t,19> object_words{};std::array<std::uint32_t,57> weapon_words{};
      auto bytes=memory.view(body+0x120u,sizeof(object_words));std::memcpy(object_words.data(),bytes.data(),bytes.size());
      bytes=memory.view(body+0x16cu,sizeof(weapon_words));std::memcpy(weapon_words.data(),bytes.data(),bytes.size());
      nlohmann::json row{{"tick",tick},{"source_tick",word(word(0x51df54u)+8u)},
          {"object",offhand_datum},{"definition",object->definition},{"parent",object->parent},
          {"inventory_owned",owned},{"world_publications",count},{"publication_matches",matches},
          {"loaded",loaded},{"reserve",reserve},{"position",{root.position.x,root.position.y,root.position.z}},
          {"published_loaded",item==state.world_weapons.end()?-1:item->loaded},
          {"published_reserve",item==state.world_weapons.end()?-1:item->reserve},
          {"object_words_120",object_words},{"weapon_words_16c",weapon_words},
          {"source_nodes",nodes.size()},{"preview_nodes",preview.size()},
          {"completed_source_and_preview_nodes_equal",same},{"render_loop_sample",rendered}};
      if(tick==*drop_tick || (tick-*drop_tick)%120==0) {
        std::vector<std::array<std::uint32_t,13>> bank(nodes.size());
        static_assert(sizeof(Transform)==52);
        if(!nodes.empty())std::memcpy(bank.data(),nodes.data(),nodes.size()*sizeof(Transform));
        row["source_world_node_words"]=bank;
      }
      history.data["drop_samples"].push_back(std::move(row));
    }
  };
  std::optional<SourcePhysicsEntryObserverBinding> retirement_observer;
  std::vector<std::size_t> retirement_pending;
  struct SelectionFrame {std::uint32_t entry;std::size_t row;bool active;};
  std::vector<SelectionFrame> selection_pending;
  if(follow_drop || trace_dual_selection) {
    auto *source=options.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Drop retirement trace requires original storage");
    const auto &memory=source->memory();
    retirement_observer.emplace(memory,[&](bool before,std::uint32_t entry,const auto &abi) {
      const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
      if(trace_dual_selection && (entry==0xeaf00u || entry==0xe9530u || entry==0xeaa60u ||
          entry==0xeac70u || entry==0xcfb80u || entry==0x113190u || entry==0x1123b0u)) {
        auto &rows=history.data["dual_selection_calls"];
        if(rows.is_null())rows=nlohmann::json::array();
        if(before) {
          const auto tick=word(word(0x51df54u)+8u);
          const bool active=(!selection_pending.empty() && selection_pending.back().active) ||
              (entry==0xeaf00u && tick>=32 && tick<=65 && word(word(abi.gp[4]+8u))==18u);
          std::size_t row=SIZE_MAX;
          if(active && rows.size()<192) {
            row=rows.size();std::array<std::uint32_t,6> stack;
            for(unsigned i=0;i<stack.size();++i)stack[i]=word(abi.gp[4]+i*4u);
            rows.push_back({{"entry",entry},{"source_tick",tick},{"before_gp",abi.gp},
                {"stack_words",stack},{"completed",false}});
          }
          selection_pending.push_back({entry,row,active});
        }else {
          check(!selection_pending.empty() && selection_pending.back().entry==entry,
              "Unpaired original dual equip observation");
          const auto row=selection_pending.back().row;selection_pending.pop_back();
          if(row!=SIZE_MAX) {
            rows.at(row)["after_gp"]=abi.gp;rows.at(row)["returned_value"]=abi.gp[0];
            rows.at(row)["completed"]=true;
          }
        }
        return;
      }
      if(entry!=0xba490u || !follow_drop)return;
      if(before) {
        const auto object=word(abi.gp[4]+4u);
        if(object!=offhand_datum || object==null_datum){retirement_pending.push_back(SIZE_MAX);return;}
        auto &rows=history.data["source_retirement_requests"];
        if(rows.is_null())rows=nlohmann::json::array();retirement_pending.push_back(rows.size());
        rows.push_back({{"object",object},{"source_tick",word(word(0x51df54u)+8u)},
            {"caller_return",word(abi.gp[4])},{"completed",false}});
      }else {
        check(!retirement_pending.empty(),"Unpaired original drop retirement observation");
        const auto index=retirement_pending.back();retirement_pending.pop_back();
        if(index!=SIZE_MAX)history.data["source_retirement_requests"].at(index)["completed"]=true;
      }
    });
  }
  const auto report_observations=[&] {
    history.data["observations"]={{"map_pickup",map_pickup},{"switched",switched},{"dual",dual},
        {"right_fired",right_fired},{"left_fired",left_fired},{"reloaded",reloaded},
        {"thrown",thrown},{"dropped",dropped},{"offhand_radar",offhand_radar},{"primary_radar",primary_radar},
        {"fired_rounds",fired_rounds},{"firing_sounds",firing_sounds},{"firing_particles",firing_particles},
        {"posed_weapons",posed_weapons},{"clips",clips},{"posed_draws",draws},
        {"start_tick",start.value_or(0)},{"dual_tick",dual_at.value_or(0)},{"grenade_tick",grenade_at.value_or(0)}};
  };
  std::optional<Spawn> diagnostic_pose;
  const auto place=[&](const Spawn &pose) {
    test::place_player(options.prepared_world->match,slot,pose);
    auto *source=options.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"Dual placement requires the actual source pose owner");
    check(current.players[slot].source_unit!=null_datum,"Dual placement has no actual source unit");
    source->tu15_world().set_object_position(current.players[slot].source_unit,pose.position);
  };
  options.input_observer=[&](unsigned tick,std::span<const Controls> controls,const MultiplayerState &state){
    step=tick;current=state;const auto &p=state.players[slot];
    if(history.path && p.source_unit!=null_datum)
      record_body_manager(history.data["body_manager_context"],previous_body_manager,
          retained_body_manager(*options.prepared_world->source_objects,p.source_unit),state.tick);
    if(verify_world_poses) {
      auto *source=options.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"World pose comparison requires original source objects");
      for(const auto &weapon:state.world_weapons) {
        if(weapon.handle==null_datum)continue;
        const auto nodes=source->world_nodes(weapon.handle);if(nodes.empty())continue;
        const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
            [&](const auto &p){return p.object==weapon.handle;});
        ++world_pose_publications;
        world_pose_mismatches+=pose==state.source_body_poses.end() || pose->nodes.size()!=nodes.size() ||
            (pose->nodes.size()==nodes.size() && std::memcmp(pose->nodes.data(),nodes.data(),nodes.size()*sizeof(Transform)));
      }
      history.data["world_pose_metrics"]["publication_checks"]=world_pose_publications;
      if(dropped && offhand_datum!=null_datum) {
        const auto root=source->object_transform(offhand_datum);
        const auto delta=root.position-(p.position+Vec3{0,0,world.content.player.camera_height});
        const float desired_yaw=std::atan2(delta.y,delta.x);
        const float desired_pitch=std::atan2(delta.z,std::sqrt(delta.x*delta.x+delta.y*delta.y));
        const auto &settings=options.desktop->keyboard_settings;
        const float gain=.0025f*float(settings.sensitivity)/3.f; // The existing SDL runtime mapping.
        SDL_Event motion{};motion.type=SDL_MOUSEMOTION;motion.motion.windowID=window_id;
        motion.motion.xrel=int(std::lround(-std::remainder(desired_yaw-p.yaw,6.283185307179586f)/gain));
        motion.motion.yrel=int(std::lround(-(desired_pitch-p.pitch)/gain*(settings.invert?-1.f:1.f)));
        check(SDL_PushEvent(&motion)==1,"Queue actual SDL look toward the detached source root");
      }
    }
    if(history.path) {
      auto *source=options.prepared_world->match.source_object_runtime();
      check(source && source->owns_physics(),"Dual history requires actual original source storage");
      const auto &memory=source->memory();
      const auto word=[&](std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);};
      nlohmann::json sample{{"tick",tick},{"source_tick",word(word(0x51df54u)+8u)},
          {"fire",controls[0].fire},{"fire_secondary",controls[0].fire_secondary},
          {"reload",controls[0].reload},{"source_unit",p.source_unit},
          {"pickup",controls[0].pickup},{"pickup_milliseconds",controls[0].pickup_milliseconds},
          {"dual_wield",controls[0].dual_wield},{"dual_wield_milliseconds",controls[0].dual_wield_milliseconds},
          {"switch_weapon",controls[0].switch_weapon},{"source_action_word",p.source_action_word},
          {"motion_sensor_control",p.motion_sensor_control},{"hands",nlohmann::json::array()}};
      sample["actual_keyboard_focus"]=SDL_GetKeyboardFocus()==window;
      sample["actual_window_flags"]=SDL_GetWindowFlags(window);
      sample["published_world_weapon_count"]=state.world_weapons.size();
      if(p.source_player!=null_datum) {
        const auto command=0x6002800cu+(p.source_player&65535u)*0x5cu;
        sample["original_action_result"]={word(0x60041000u),word(0x60041004u),word(0x60041008u)};
        sample["delivered_action_result"]={word(command+0x28u),word(command+0x2cu),word(command+0x30u)};
      }
      if(!start&&(tick<4||tick==120))try {
        sample["primary_netgame_object"]=source->netgame_object(primary_row_index);
        sample["smg_netgame_object"]=source->netgame_object(smg_row_index);
        auto& actual_items=sample["actual_pickup_source_objects"]=nlohmann::json::array();
        for(const auto& object:source->live_objects())
          if((object.definition==primary_row->items[0].definition||object.definition==row->items[0].definition)&&actual_items.size()<12) {
            const auto at=source->object_transform(object.object).position;
            actual_items.push_back({{"object",object.object},{"definition",object.definition},
                {"parent",object.parent},{"type",object.type},{"position",{at.x,at.y,at.z}}});
          }
      }catch(const std::exception& error){sample["pickup_observation_error"]=error.what();}
      if(!start&&(tick<4||tick==120)) {
        auto& primary_candidates=sample["primary_pickup_candidates"]=nlohmann::json::array();
        for(const auto& item:state.world_weapons)
          if(item.definition==primary_row->items[0].definition&&primary_candidates.size()<8) {
            const auto delta=item.position-primary_row->position;
            primary_candidates.push_back({{"object",item.handle},{"definition",item.definition},
                {"position",{item.position.x,item.position.y,item.position.z}},
                {"authored_position_distance_squared",delta.x*delta.x+delta.y*delta.y+delta.z*delta.z}});
          }
      }
      sample["hud_storage"]=word(0x50eeccu);sample["hud_definition_address"]=word(0x51df94u);
      sample["unit_definition"]=p.source_unit==null_datum?null_datum:source->object_definition(p.source_unit);
      check(p.source_unit==null_datum || sample["unit_definition"]==history.data["expected_unit_definition"],
          "Actual dual source unit lost its selected authored species");
      sample["action_prompt"]={{"action",p.action_prompt.target.action},
          {"index",p.action_prompt.target.index},{"object",p.action_prompt.target.object},
          {"definition",p.action_prompt.definition},{"message",p.action_prompt.message}};
      if(p.source_unit!=null_datum) {
        const auto root=source->object_transform(p.source_unit).position;
        const auto equipment=source->tu15_world().player_equipment(p.source_unit);
        const auto view=source->tu15_world().first_person_view(0);
        sample["source_position"]={root.x,root.y,root.z};
        sample["source_equipment"]={{"weapons",equipment.weapons},{"definitions",equipment.definitions},
            {"current",equipment.current},{"requested",equipment.requested},{"ready_ticks",equipment.ready_ticks}};
        sample["source_first_person"]={{"unit",view.unit},{"weapon",view.hands[0].weapon},
            {"graph",view.hands[0].graph},{"activity",view.hands[0].activity},
            {"packed_clip",view.hands[0].animation_identifier},{"frame",view.hands[0].frame}};
        auto &items=sample["actual_smg_items"]=nlohmann::json::array();
        for(const auto &item:state.world_weapons)
          if(item.definition==world.rules.weapon && items.size()<8) {
            const auto at=source->object_transform(item.handle).position;
            items.push_back({{"object",item.handle},{"definition",item.definition},
                {"source_position",{at.x,at.y,at.z}},{"published_position",{item.position.x,item.position.y,item.position.z}}});
          }
      }
      for(unsigned hand=0;hand<2;++hand) {
        const auto datum=hand?p.secondary_weapon.datum:p.source_weapon;if(datum==null_datum)continue;
        const auto body=source->object_body(datum);
        std::array<std::uint32_t,57> raw{};
        const auto bytes=memory.view(body+0x16cu,sizeof(raw));std::memcpy(raw.data(),bytes.data(),bytes.size());
        const auto published_tick=hand?p.secondary_weapon.shot_tick:p.shot_tick;
        const auto definition=hand?p.secondary_weapon.definition:p.weapon;
        ++history_checks;
        history_mismatches+=published_tick!=raw[55] || source->object_definition(datum)!=definition;
        sample["hands"].push_back({{"hand",hand},{"datum",datum},{"definition",definition},
            {"source_definition",source->object_definition(datum)},{"source_words_16c",raw},
            {"published_shot_tick",published_tick},
            {"published_reload_tick",hand?p.secondary_weapon.reload_tick:p.reload_tick},
            {"published_loaded",hand?p.secondary_weapon.loaded:p.loaded},
            {"published_reserve",hand?p.secondary_weapon.reserve:p.reserve}});
      }
      history.data["samples"].push_back(std::move(sample));
      history.data["publication_checks"]=history_checks;history.data["publication_mismatches"]=history_mismatches;
    }
    // Request real X11/SDL focus independently of whether the first animation
    // draw has occurred. No synthetic focus event or gameplay input is used.
    if(SDL_GetKeyboardFocus()!=window){SDL_RaiseWindow(window);SDL_SetWindowInputFocus(window);}
    reload_received|=controls[0].reload;
    lost_focus|=start.has_value() && SDL_GetKeyboardFocus()!=window;
    const auto primary_item=std::find_if(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &item) {
      const auto d=item.position-primary_row->position;
      return item.definition==primary_row->items[0].definition && d.x*d.x+d.y*d.y+d.z*d.z<1e-8f;
    });
    if(!start && primary_item!=state.world_weapons.end() && SDL_GetKeyboardFocus()==window) {
      start=tick;primary_target=primary_item->handle;starting_datum=p.source_weapon;
      Spawn at;at.position=primary_row->position;at.position.x-=.2f;at.bsp=0;at.facing=0;
      diagnostic_pose=at;place(at);hold_key(window_id,SDL_SCANCODE_E,true);
      std::cout<<"Actual map battle rifle available at tick="<<tick<<'\n';
    }
    if(tick>=120&&!start) {
      check(SDL_GetKeyboardFocus()==window,"Weapon window never received actual keyboard focus");
      check(primary_item!=state.world_weapons.end(),"Authored map primary pickup was not published at its observed start location");
    }
    if(start && !map_pickup && p.source_weapon==primary_target) {
      check(p.weapon==primary_row->items[0].definition,"Map pickup changed its authored definition");
      check(std::none_of(state.world_weapons.begin(),state.world_weapons.end(),[&](const auto &item){return item.handle==primary_target;}),
          "Held map pickup still has a world owner");
      map_pickup=true;carried_at=tick;hold_key(window_id,SDL_SCANCODE_E,false);
    }
    if(carried_at && tick==*carried_at+10)key(window_id,SDL_SCANCODE_TAB);
    if(carried_at && tick>*carried_at+10 && !dual_requested && p.source_weapon==starting_datum) {
      switched=true;dual_requested=true;
      Spawn at;at.position=location;at.position.x-=.2f;at.bsp=0;at.facing=0;
      diagnostic_pose=at;place(at);hold_key(window_id,SDL_SCANCODE_X,true);
    }
    if(diagnostic_pose)place(*diagnostic_pose);
    if(p.secondary_weapon.datum!=null_datum) {
      if(offhand_datum==null_datum)offhand_datum=p.secondary_weapon.datum;
      check(offhand_datum==p.secondary_weapon.datum,"Dual history changed its full offhand datum");
      dual=true;if(!dual_at){dual_at=tick;hold_key(window_id,SDL_SCANCODE_X,false);}
      if(previous_ammunition) {
        fired_rounds+=unsigned(std::max(0,(*previous_ammunition)[0]-p.loaded));
        fired_rounds+=unsigned(std::max(0,(*previous_ammunition)[1]-p.secondary_weapon.loaded));
      }
      previous_ammunition=std::array{p.loaded,p.secondary_weapon.loaded};
      right_fired|=p.loaded<60;left_fired|=p.secondary_weapon.loaded<60;
      reloaded|=right_fired && left_fired && p.loaded==60 && p.secondary_weapon.loaded==60;
    }
    if(dual_at) {
      const auto elapsed=tick-*dual_at;
      if(elapsed==20)trigger(window_id,SDL_BUTTON_RIGHT,true);
      if(elapsed>20 && elapsed<26) {
        check(p.loaded==60,"Offhand-only SDL trigger consumed primary ammunition");
        offhand_radar|=p.motion_sensor_control==0x20;
      }
      if(elapsed==26){trigger(window_id,SDL_BUTTON_RIGHT,false);trigger(window_id,SDL_BUTTON_LEFT,true);}
      if(elapsed>26 && elapsed<32)primary_radar|=p.motion_sensor_control==1;
      if(elapsed==32){trigger(window_id,SDL_BUTTON_LEFT,false);trigger(window_id,SDL_BUTTON_RIGHT,false);}
      if(elapsed==40)key(window_id,SDL_SCANCODE_R);
      if(reloaded && !(p.source_action_word&1u) && !grenade_at && argc!=5)grenade_at=tick;
      if(grenade_at && tick==*grenade_at)key(window_id,SDL_SCANCODE_Q);
      if(grenade_at && tick>*grenade_at && p.grenade_action.animation_active)thrown=true;
      if(grenade_at && tick>*grenade_at && (p.grenade_action.phase()==1 || p.grenade_action.phase()==2)) {
        check(!p.motion_sensor_control,"SDL grenade request published a weapon trigger bit");
        grenade_radar_phase=true;
      }
      if(grenade_at && tick>*grenade_at && p.secondary_weapon.datum==null_datum)dropped=true;
      if(grenade_at && tick==*grenade_at+40){SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"Exit weapon probe");}
    }
    observe_drop(state,tick,true);
    report_observations();
    check(!start || tick<*start+250,"Live dual pickup/animation sequence timed out");
  };
  if(!verify_dual_gpu)options.animation_observer=[&](unsigned,unsigned,const AnimationRenderObservation &sample){
    if(!sample.first_person)return;
    for(const auto &v:sample.vertices)check(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&std::isfinite(v.position.z),"Nonfinite posed weapon vertex");
    if(current.players[slot].secondary_weapon.datum!=null_datum) {
      posed_weapons.insert(sample.weapon);clips.insert(sample.animation);++draws;
    }
  };
  options.audio_observer=[&](const MultiplayerAudioSubmission &s) {
    if(s.sound==firing_sound){check(s.frames>0,"SMG firing sound submitted empty PCM");++firing_sounds;}
  };
  if(!verify_dual_gpu)options.effect_observer=[&](unsigned,unsigned,std::span<const ParticleDraw> particles) {
    if(dual_at)firing_particles+=unsigned(particles.size());
  };
  if(verify_dual_gpu) {
    // These observers do not request posed CPU vertices and therefore keep
    // the ordinary GPU admission path active. Source-bank verification checks
    // each actual hand before submission; renderer verification compares its
    // completed GPU bytes with the original CPU deformation and placement.
    options.node_observer=[&](unsigned view,unsigned,const MultiplayerNodeObservation &sample) {
      if(view || sample.stage!=MultiplayerNodeStage::first_person_interpolated || sample.mode!=1 ||
          current.players[slot].secondary_weapon.datum==null_datum)return;
      const auto &player=current.players[slot];
      for(unsigned hand=0;hand<2;++hand)
        if(sample.owner==(hand?player.secondary_weapon.datum:player.source_weapon)) {
          check(!sample.nodes.empty(),"Ordinary dual GPU hand has no source node bank");
          std::uint64_t hash=1469598103934665603ull;
          for(auto byte:std::as_bytes(sample.nodes))hash=(hash^std::to_integer<unsigned char>(byte))*1099511628211ull;
          gpu_latest_nodes[hand]=hash;gpu_hand_nodes[hand].insert(hash);
          posed_weapons.insert(sample.owner);++draws;
          const auto binding=options.prepared_world->source_objects->first_person_view(view);
          check(binding.hands[hand].weapon==sample.owner,"Ordinary dual GPU node owner differs from its native registration");
          const auto &channel=binding.hands[hand];clips.insert(channel.animation_identifier>>16);
          if(dual_at && current.tick>=*dual_at+20)
            check_authored_dual_channel(channel,*dual_variants);
          gpu_hand_channels[hand].insert({channel.animation_identifier,std::bit_cast<std::uint32_t>(channel.frame)});
          gpu_hand_activities[hand].insert(channel.activity);
        }
    };
    options.frame_observer=[&](const MultiplayerFrameTiming &frame) {
      if(!frame.submitted || current.players[slot].secondary_weapon.datum==null_datum)return;
      const auto &renderer=frame.renderer;
      if(renderer.model_skin_gpu_post_identity_vertices) {
        ++gpu_dual_frames;gpu_foreground_vertices+=renderer.model_skin_gpu_post_identity_vertices;
        gpu_verified_vertices+=renderer.model_skin_gpu_verified_vertices;
        gpu_deformed_vertices+=renderer.model_skin_gpu_deformed_vertices;
        check(renderer.model_skin_gpu_verified_vertices>=renderer.model_skin_gpu_post_identity_vertices,
            "Ordinary dual foreground GPU vertices escaped actual readback verification");
        if(gpu_latest_nodes[0] && gpu_latest_nodes[1] && *gpu_latest_nodes[0]!=*gpu_latest_nodes[1])++gpu_distinct_hand_frames;
      }
      history.data["dual_gpu"]={{"frames",gpu_dual_frames},{"distinct_hand_frames",gpu_distinct_hand_frames},
          {"foreground_vertices",gpu_foreground_vertices},{"verified_vertices",gpu_verified_vertices},
          {"deformed_vertices",gpu_deformed_vertices},{"primary_node_banks",gpu_hand_nodes[0].size()},
          {"offhand_node_banks",gpu_hand_nodes[1].size()},
          {"channel_samples",{gpu_hand_channels[0].size(),gpu_hand_channels[1].size()}},
          {"activities",{gpu_hand_activities[0],gpu_hand_activities[1]}},
          {"animation_observer",false},{"effect_observer",false},{"render_overlap",false}};
    };
  }
  std::map<std::uint32_t,SourceModelPaletteDefinition> palette_definitions;
  if(verify_world_poses)options.world_weapon_source_draw_observer=[&](unsigned,unsigned tick,
      std::uint32_t owner,std::uint32_t definition,std::span<const Transform> nodes,
      std::span<const Vertex> vertices,std::span<const SourceLiveModelBaseDrawTu15> draws) {
    ++world_pose_draws;if(dropped && owner==offhand_datum)++dropped_pose_draws;
    auto *source=options.prepared_world->match.source_object_runtime();
    check(source && source->owns_physics(),"World draw lost its original owner");
    const auto raw=source->rendered_world_nodes(owner);
    bool matches=raw && raw->size()==nodes.size() && !nodes.empty() && !vertices.empty() && !draws.empty();
    if(matches)matches=!std::memcmp(raw->data(),nodes.data(),nodes.size()*sizeof(Transform));
    const auto &memory=source->memory();
    const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
    for(const auto &draw:draws) {
      check(draw.object==owner && draw.definition==definition && draw.lod<6,"World draw changed original selection identity");
      const auto model=draw.unit.words[1];
      auto found=palette_definitions.find(model);
      if(found==palette_definitions.end())found=palette_definitions.emplace(model,world.cache->load_source_model_palette_definition(model)).first;
      auto expected=source_model_skin_palette(nodes,found->second.inverse_binds);
      append_source_model_compounds(expected,found->second.compounds[draw.lod]);
      const auto region=draw.unit.words[3]&15u,section=(draw.unit.words[3]>>9)&511u;
      check(region<draw.geometry.palette.regions.size(),"World draw has no original region palette");
      const auto range=draw.geometry.palette.regions[region];
      const auto metadata=word(word(0x4f0414u)+(model&65535u)*16u+8u);
      const auto resident=word(word(metadata+0x28u)+section*92u+0x34u);
      const auto mapped=word(resident+0x64u),mapping=word(resident+0x68u);
      for(unsigned n=0;n<range.count;++n) {
        const auto node=mapped?std::to_integer<unsigned>(memory.view(mapping+n,1)[0]):n;
        check(node<expected.size() && unsigned(range.first)+n<draw.geometry.palette.matrices.size(),"Original region map escapes its WORLD palette");
        ++world_palette_matrices;
        matches&=!std::memcmp(expected[node].data(),draw.geometry.palette.matrices[range.first+n].data(),48);
      }
    }
    world_pose_mismatches+=!matches;
    auto &report=history.data["world_pose_metrics"];
    report["draws"]=world_pose_draws;report["dropped_draws"]=dropped_pose_draws;
    report["original_palette_matrices"]=world_palette_matrices;report["mismatches"]=world_pose_mismatches;
    if(!matches && !report.contains("first_mismatch"))report["first_mismatch"]={{"tick",tick},{"object",owner},{"source_nodes",raw?raw->size():0},{"submitted_nodes",nodes.size()}};
  };
  if(argc==5){options.screenshot=argv[4];options.capture_tick=240;}
  check(run_multiplayer(options)==0,"Weapon runtime failed");
  history.data["render_loop_completed"]=true;
  history.data["pending_dual_selection_calls"]=selection_pending.size();
  if(verify_world_poses) {
    check(world_pose_publications && world_pose_draws && dropped_pose_draws && world_palette_matrices && !world_pose_mismatches,
        "Loose weapon source publication/draw/palette comparison failed");
    const auto original=options.prepared_world->match.state();
    for(const auto &event:original.grenade_detonations)for(const auto &location:event.source_locations)
      history.data["world_pose_metrics"]["original_explosion_locations"].push_back({
          {"serial",event.serial},{"name",location.name},
          {"transform_words",std::bit_cast<std::array<std::uint32_t,13>>(location.frame)}});
    LanHost host(options.prepared_world->match,0,0xd770u);
    LanClient client(host.join_address(),0xd770u,1);
    std::array<Controls,multiplayer_capacity> incoming{};std::array<Controls,1> neutral{};
    for(unsigned n=0;n<128 && (!client.ready() || !client.state().source_body_poses_authoritative);++n) {
      client.send(neutral);host.receive(incoming);host.broadcast();client.receive();
      if(!client.ready() || !client.state().source_body_poses_authoritative)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    history.data["world_pose_metrics"]["lan_admission"]={{"ready",client.ready()},
        {"error",client.error()},{"rejected_packets",client.rejected_packet_count()},
        {"last_packet_rejection",client.last_packet_rejection()},{"host_peers",host.peers()}};
    check(client.ready() && client.error().empty(),"Loose weapon native LAN admission failed");
    const auto sent=options.prepared_world->match.state();const auto &received=client.state();
    unsigned transported{};
    for(const auto &weapon:sent.world_weapons) {
      const auto a=std::find_if(sent.source_body_poses.begin(),sent.source_body_poses.end(),[&](const auto &p){return p.object==weapon.handle;});
      if(a==sent.source_body_poses.end())continue;
      const auto b=std::find_if(received.source_body_poses.begin(),received.source_body_poses.end(),[&](const auto &p){return p.object==weapon.handle;});
      check(b!=received.source_body_poses.end() && a->nodes.size()==b->nodes.size() &&
          !std::memcmp(a->nodes.data(),b->nodes.data(),a->nodes.size()*sizeof(Transform)),"Native LAN changed a loose weapon WORLD bank");
      ++transported;
    }
    check(transported>0,"Native LAN omitted every loose weapon pose");
    unsigned explosion_locations{};
    for(const auto &event:sent.grenade_detonations) {
      const auto remote=std::find_if(received.grenade_detonations.begin(),received.grenade_detonations.end(),
          [&](const auto &e){return e.serial==event.serial;});
      check(remote!=received.grenade_detonations.end() && remote->source_locations.size()==event.source_locations.size(),
          "Native LAN omitted original explosion locations");
      check(remote->source_effect_instance==null_datum,"Remote effect render provenance authenticated a local simulation pool");
      check(remote->replica_render.has_value()==event.replica_render.has_value(),"Native LAN omitted original effect render provenance");
      if(event.replica_render) {
        const auto &a=*event.replica_render,&b=*remote->replica_render;
        check(a.identity==b.identity&&a.attached_owner==b.attached_owner&&a.local_view==b.local_view&&a.flags==b.flags&&
            a.root_owner==b.root_owner&&a.root_type==b.root_type&&
            !std::memcmp(&a.color,&b.color,sizeof(Vec3))&&!std::memcmp(&a.velocity,&b.velocity,sizeof(Vec3)),
            "Native LAN changed completed original effect render inputs");
      }
      for(unsigned n=0;n<event.source_locations.size();++n) {
        const auto &a=event.source_locations[n],&b=remote->source_locations[n];
        check(a.name==b.name && !std::memcmp(&a.frame,&b.frame,sizeof(Transform)),
            "Native LAN changed an original explosion transform");++explosion_locations;
      }
    }
    history.data["world_pose_metrics"]["native_lan_explosion_locations"]=explosion_locations;
    history.data["world_pose_metrics"]["native_lan_poses"]=transported;
    history.data["world_pose_metrics"]["native_lan_protocol"]=native_multiplayer_protocol;
  }
  if(follow_drop) {
    check(drop_tick.has_value(),"Actual offhand never entered the detached lifetime probe");
    std::array<Controls,multiplayer_capacity> neutral{};
    unsigned continued{};
    for(;continued<1024 && !retired_tick;++continued) {
      options.prepared_world->match.step(neutral);
      const auto state=options.prepared_world->match.state();
      observe_drop(state,state.tick,false);
    }
    history.data["drop_lifetime"]={{"object",offhand_datum},{"first_drop_tick",*drop_tick},
        {"headless_neutral_ticks",continued},{"publication_checks",drop_checks},
        {"publication_mismatches",drop_mismatches},{"completed_source_preview_differences",drop_preview_differences},
        {"source_retirement_observed",retired_tick.has_value()},
        {"original_retirement_time_not_assumed",true},{"gpu_or_framebuffer_comparison",false}};
    check(drop_checks>0 && drop_mismatches==0,"Detached source identity/ammunition/root differs from world publication");
  }
  report_observations();
  check(!history.path || (history_checks>0 && history_mismatches==0),"Independent hand publication differs from actual source storage");
  check(map_pickup && switched,"Actual SDL map pickup and carried-weapon switch failed");
  if(!(dual && right_fired && left_fired && reloaded)) {
    const auto &p=current.players[slot];
    std::cerr<<"Weapon runtime state: dual="<<dual<<" right_fired="<<right_fired
        <<" left_fired="<<left_fired<<" reloaded="<<reloaded<<" dead="<<p.dead
        <<" reload_received="<<reload_received<<" lost_focus="<<lost_focus
        <<" primary_ammo="<<p.loaded<<'/'<<p.reserve<<" secondary_ammo="<<p.secondary_weapon.loaded
        <<'/'<<p.secondary_weapon.reserve<<" reload_seconds="<<p.reload_seconds
        <<'/'<<p.secondary_weapon.reload_seconds<<'\n';
  }
  check(dual && right_fired && left_fired && reloaded,"Live dual pickup, independent triggers or reload failed");
  check(offhand_radar && primary_radar,"Actual independent SDL triggers did not retain their original radar bits");
  check(posed_weapons.size()==2 && clips.size()>=3 && draws>20,"Both real weapon datums did not submit animated poses");
  std::cout<<"SMG firing observations: sounds="<<firing_sounds<<" rounds="<<fired_rounds<<'\n';
  check(firing_sounds==fired_rounds && fired_rounds>1 && (verify_dual_gpu || firing_particles>0),
      "Live dual SMGs did not submit exactly one authored sound per fired round");
  if(verify_dual_gpu) {
    check(gpu_dual_frames>10 && gpu_distinct_hand_frames>10 && gpu_verified_vertices && gpu_deformed_vertices &&
        gpu_hand_nodes[0].size()>3 && gpu_hand_nodes[1].size()>3 &&
        gpu_hand_channels[0].size()>3 && gpu_hand_channels[1].size()>3,
        "Ordinary dual weapon path did not deform and verify two independently moving hands on the GPU");
    for(unsigned hand=0;hand<2;++hand)
      check(gpu_hand_activities[hand].contains(0x0400000cu) && gpu_hand_activities[hand].contains(0x06000006u) &&
          (gpu_hand_activities[hand].contains(0x0b000065u) || gpu_hand_activities[hand].contains(0x0c000064u)),
          "Ordinary dual GPU hand did not play its own native idle/fire/reload activities");
    history.data["dual_gpu"]["passed"]=true;
    std::cout<<"PASS ordinary dual GPU path: "<<gpu_dual_frames<<" frames, "<<gpu_foreground_vertices
        <<" foreground vertices, "<<gpu_verified_vertices<<" actual GPU vertices matched original CPU output\n";
  }
  if(argc!=5) {
    std::cout<<"SMG grenade observations: request_tick="<<grenade_at.value_or(0)<<" thrown="<<thrown
        <<" dropped="<<dropped<<" radar_phase="<<grenade_radar_phase<<'\n';
    check(thrown && dropped && grenade_radar_phase,"Grenade animation/phase did not release the actual offhand");
  }
  std::cout<<"PASS actual SDL map pickup and switch; dual pickup, two independently firing/reloading weapon datums, "<<draws<<" posed draws, "<<clips.size()<<" clips, "<<firing_particles<<" particle draws, "<<firing_sounds<<" firing sounds";
  if(argc!=5)std::cout<<", grenade animation and offhand drop";
  std::cout<<'\n';
}catch(const std::exception &e){history.data["runtime_error"]=e.what();std::cerr<<e.what()<<'\n';return 1;}}
