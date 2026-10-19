#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include "helpers/campaign_vehicle_observer_inputs_tu15.hpp"
#include <SDL_syswm.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
using namespace halo2;using Json=nlohmann::json;
namespace {
void require(bool yes,const char *why){if(!yes)throw FormatError(why);}
Json xyz(Vec3 value){return Json::array({value.x,value.y,value.z});}
void key(SDL_Window *window,SDL_Scancode scan,bool down) {
 SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;
 event.key.windowID=SDL_GetWindowID(window);event.key.state=down?SDL_PRESSED:SDL_RELEASED;
 event.key.keysym.scancode=scan;event.key.keysym.sym=SDL_GetKeyFromScancode(scan);
 require(SDL_PushEvent(&event)==1,"SDL rejected the ordinary player input");
}
bool same(const MultiplayerState &a,const MultiplayerState &b) {
 if(a.tick!=b.tick || a.source_game_type!=b.source_game_type || a.world_vehicles.size()!=b.world_vehicles.size() || a.source_body_poses.size()!=b.source_body_poses.size())return false;
 for(unsigned n=0;n<a.players.size();++n) {
  const auto &x=a.players[n],&y=b.players[n];
  if(x.connected!=y.connected || x.source_player!=y.source_player || x.source_unit!=y.source_unit || x.source_parent!=y.source_parent || x.source_seat!=y.source_seat || x.source_configuration!=y.source_configuration || std::memcmp(&x.position,&y.position,sizeof(Vec3)) || std::memcmp(&x.velocity,&y.velocity,sizeof(Vec3)))return false;
 }
 for(unsigned n=0;n<a.world_vehicles.size();++n) {
  const auto &x=a.world_vehicles[n],&y=b.world_vehicles[n];
  if(x.handle!=y.handle || x.placement!=y.placement || std::memcmp(&x.transform,&y.transform,sizeof(Transform)) || std::memcmp(&x.lighting_velocity,&y.lighting_velocity,sizeof(Vec3)))return false;
 }
 if(a.prop_regions.size()!=b.prop_regions.size())return false;
 for(unsigned n=0;n<a.prop_regions.size();++n) {
  const auto &x=a.prop_regions[n],&y=b.prop_regions[n];
  if(x.handle!=y.handle || x.placement!=y.placement || x.definition!=y.definition ||
      x.regions.selection.variant_index!=y.regions.selection.variant_index ||
      x.regions.selection.permutations!=y.regions.selection.permutations ||
      x.regions.selection.states!=y.regions.selection.states ||
      x.regions.selection.unresolved_rng_regions!=y.regions.selection.unresolved_rng_regions ||
      x.regions.looping_effects!=y.regions.looping_effects)return false;
 }
 if(a.source_scene_roots.size()!=b.source_scene_roots.size())return false;
 for(unsigned n=0;n<a.source_scene_roots.size();++n) {
  const auto &x=a.source_scene_roots[n],&y=b.source_scene_roots[n];
  if(x.model_regions.has_value()!=y.model_regions.has_value())return false;
  if(x.model_regions && (x.model_regions->selection.variant_index!=y.model_regions->selection.variant_index ||
      x.model_regions->selection.permutations!=y.model_regions->selection.permutations ||
      x.model_regions->selection.states!=y.model_regions->selection.states ||
      x.model_regions->looping_effects!=y.model_regions->looping_effects))return false;
  if(x.object!=y.object || x.definition!=y.definition || x.placement!=y.placement || std::memcmp(&x.transform,&y.transform,sizeof(Transform)) || std::memcmp(&x.velocity,&y.velocity,sizeof(Vec3)))return false;
 }
 for(unsigned n=0;n<a.world_vehicles.size();++n) {
  const auto &x=a.world_vehicles[n].regions,&y=b.world_vehicles[n].regions;
  if(x.has_value()!=y.has_value())return false;
  if(x && (x->selection.variant_index!=y->selection.variant_index || x->selection.permutations!=y->selection.permutations || x->selection.states!=y->selection.states || x->looping_effects!=y->looping_effects))return false;
 }
 for(unsigned n=0;n<a.source_body_poses.size();++n) {
  const auto &x=a.source_body_poses[n],&y=b.source_body_poses[n];
  if(x.object!=y.object || x.nodes.size()!=y.nodes.size() || std::memcmp(x.nodes.data(),y.nodes.data(),x.nodes.size()*sizeof(Transform)))return false;
 }
 return true;
}
struct Authority {
 std::atomic_bool ready{};std::atomic_uint32_t vehicle{null_datum};
 std::mutex mutex;std::vector<MultiplayerState> history;std::exception_ptr error;
 unsigned port{},neutral_prefix_ticks{};float causal_response{};std::jthread worker;
 Authority(MultiplayerOptions options,std::size_t placement,std::filesystem::path output) {
  std::promise<void> startup;auto future=startup.get_future();
  worker=std::jthread([this,options,placement,output,startup=std::move(startup)](std::stop_token stop)mutable {
   bool started=false;
   try {
    MultiplayerPreparedWorld world(options,ScriptProgram{});
    require(world.source_objects && world.match.source_game_type()==1,"Campaign authority lost its original source world");
    const auto object=world.match.create_world_object(placement);vehicle.store(object);
    require(object!=null_datum,"Authored class3 source birth returned NONE");
    require(world.source_objects->object_definition(object)==world.content.objects.at(placement).definition,"Actual source vehicle birth changed its authored definition");
    world.source_objects->set_unit_controller_active(object,true);
    auto neutral_options=options;neutral_options.source_object_savegame_directory=output/"neutral-source-storage";
    MultiplayerPreparedWorld neutral(neutral_options,ScriptProgram{});
    const auto neutral_object=neutral.match.create_world_object(placement);
    require(neutral_object!=null_datum,"Independent neutral source vehicle birth failed");
    neutral.source_objects->set_unit_controller_active(neutral_object,true);
    for(const auto &[datum,weapon]:world.content.weapons)if(!weapon.first_person_models.empty())
      for(auto representation:source_session_player_representations(world.content,1)) {
        const auto definition=world.cache->load_first_person_binding(datum,int(representation));
        if(!definition.record_present || definition.weapon_model==null_datum || definition.graph==null_datum)continue;
        world.match.configure_replicated_first_person(definition);
        neutral.match.configure_replicated_first_person(definition);
      }
    LanHost transport(world.match,0,multiplayer_fingerprint(options.map,world.cache->shared_directory(),options.bsp,world.rules.weapon,options.gameplay_xbe),options.variant);
    port=transport.port();
    test::LivePhysicsEntryCaptureTu15 controller(output/"original-live-unit-input-capture"),object_frame(output/"original-live-object-capture"),solve(output/"original-live-world-capture"),look_animation(output/"original-live-look-animation-capture"),look_rotation(output/"original-live-look-rotation-capture"),body_phase(output/"original-live-body-capture"),unit_view_reset(output/"original-unit-view-reset-capture");
    SourcePhysicsEntryObserverBinding observation(world.source_objects->memory(),[&](bool entering,std::uint32_t entry,const auto &abi) {
      if(entry==0x158690u && std::getenv("HALO2_TU15_UNIT_VIEW_RESET_CAPTURE")) {unit_view_reset.observe(entering,abi,world.source_objects->memory(),entry);return;}
      if(entry==0x290030u) {look_animation.observe(entering,abi,world.source_objects->memory(),entry);return;}
      if(entry==0x28c2c0u) {look_rotation.observe(entering,abi,world.source_objects->memory(),entry);return;}
      if(world.match.simulation().ticks()<35)return;
      if(entry==0xc92c0u)controller.observe(entering,abi,world.source_objects->memory(),entry);
      else if(entry==0xb8f40u)object_frame.observe(entering,abi,world.source_objects->memory(),entry);
      else if(entry==0x1cc8d0u)solve.observe(entering,abi,world.source_objects->memory(),entry);
      else if(entry==0xb9090u)body_phase.observe(entering,abi,world.source_objects->memory(),entry);
    });
    std::ofstream trace(output/"authority.jsonl");
    started=true;startup.set_value();
    std::array<Controls,multiplayer_capacity> controls{};
    auto next=std::chrono::steady_clock::now();
    while(!stop.stop_requested()) {
      transport.receive(controls);
      // Mirror actual admitted birth inputs, never either world's body state.
      for(unsigned slot=0;slot<multiplayer_capacity;++slot) {
        const auto &player=world.match.player(slot);
        if(player.connected && !neutral.match.player(slot).connected)
          require(neutral.match.join(player.name,-1,player.source_profile_request,slot)==slot,"Neutral source roster changed an admitted slot");
      }
      if(ready.load()) {
        const auto tick=world.match.simulation().ticks();
        const Vec3 throttle=tick>=30 && tick<90?Vec3{1,0,.5f}:Vec3{};
        const auto facing=world.source_objects->object_transform(object).basis[0];
        world.source_objects->deliver_unit_input(object,throttle,facing);
        neutral.source_objects->deliver_unit_input(neutral_object,{},neutral.source_objects->object_transform(neutral_object).basis[0]);
        neutral.match.step(controls);world.match.step(controls);
        const auto actual=world.source_objects->object_transform(object),baseline=neutral.source_objects->object_transform(neutral_object);
        const auto actual_velocity=world.source_objects->object_linear_velocity(object),baseline_velocity=neutral.source_objects->object_linear_velocity(neutral_object);
        if(tick<30) {
          require(!std::memcmp(&actual,&baseline,sizeof(Transform)) && !std::memcmp(&actual_velocity,&baseline_velocity,sizeof(Vec3)),"Independent neutral/control source worlds diverged before NPC throttle changed");
          ++neutral_prefix_ticks;
        } else causal_response=std::max(causal_response,length(actual.position-baseline.position));
      }
      const auto state=world.match.state();
      {std::lock_guard lock(mutex);history.push_back(state);}
      trace<<Json{{"tick",state.tick},{"kind",state.source_game_type},{"vehicle",object},
        {"position",xyz(world.source_objects->object_transform(object).position)},
        {"velocity",xyz(world.source_objects->object_linear_velocity(object))},
        {"neutral_position",xyz(neutral.source_objects->object_transform(neutral_object).position)},
        {"causal_response",causal_response}}.dump()<<'\n';trace.flush();
      transport.broadcast();
      next+=std::chrono::milliseconds(33);std::this_thread::sleep_until(next);
    }
   } catch(...) {
    {std::lock_guard lock(mutex);error=std::current_exception();}
    if(!started)startup.set_exception(std::current_exception());
   }
  });future.get();
 }
 void finish(){worker.request_stop();if(worker.joinable())worker.join();if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv) {
 Json report{{"passed",false},{"acceptance_passed",false},{"reference","English TU1.5 only"},{"runtime_cache_source","MCC"},{"full_physics_parity",false},
  {"scenario","lan-campaign-class3"},{"visible",true},{"stimulus","Declared cached-BSP initial starts, actual SDL player input, original C93D0/C92C0 NPC unit input; no supplied physical output"}};
 std::filesystem::path output;
 try {
  require(argc==7,"Supply MCC_MAP MCC_MAPS native-mcc MCC_ROOT OUTPUT SCENARIO");output=argv[5];std::filesystem::create_directories(output);
  require(std::string_view(argv[3])=="native-mcc","Campaign physics acceptance requires prepared MCC runtime inputs");
  const auto mcc_maps=std::filesystem::canonical(std::filesystem::path(argv[4])/"halo2/h2_maps_win64_dx11");
  require(std::filesystem::canonical(argv[2])==mcc_maps && std::filesystem::canonical(argv[1]).parent_path()==mcc_maps,
      "Campaign physics map is outside the selected prepared MCC cache");
  const auto original=OriginalData::native_tu15();require(original->image==OriginalData::Image::TitleUpdate15,"Campaign visible world requires native TU1.5");
  Cache cache(argv[1],argv[2],original);
  require(cache.is_mcc(),"Campaign physics acceptance requires an actual MCC cache");
  const auto bsp=std::getenv("HALO2_TU15_CAMPAIGN_BSP")?std::stoul(std::getenv("HALO2_TU15_CAMPAIGN_BSP")):0u;
  const auto selected=std::getenv("HALO2_TU15_CAMPAIGN_PLACEMENT")?std::stol(std::getenv("HALO2_TU15_CAMPAIGN_PLACEMENT")):68;
  const auto content=cache.load_content(bsp,false);
  const auto placement=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &p){return p.type==1 && p.placement==selected && !p.model_attachment;});
  require(placement!=content.objects.end(),"Authored campaign vehicle placement absent");
  const auto index=std::size_t(placement-content.objects.begin());
  const auto target_tag=cache.query_tag({.handle=TagHandle{placement->definition}});
  require(target_tag!=nullptr,"Authored class3 definition is missing from the original cache");
  report["map"]=std::filesystem::path(argv[1]).filename().string();report["bsp"]=bsp;
  report["authored_vehicle_placement"]=selected;report["content_placement_index"]=index;
  report["selected_vehicle_definition"]=placement->definition;report["selected_vehicle_name"]=target_tag->name;
  const auto starts=test::campaign_vehicle_observer_starts<3>(cache,content,bsp,*placement);
  MultiplayerOptions host_options;host_options.map=argv[1];host_options.shared_maps=argv[2];host_options.bsp=bsp;
  host_options.campaign_difficulty=1;host_options.local_players=1;host_options.source_object_world=true;
  host_options.source_object_savegame_directory=output/"authority-storage";
  host_options.native_spawn_inputs.assign(starts.begin(),starts.end());
  host_options.variant.values.set(2,0);host_options.variant.values.set(10,0);
  host_options.campaign_spawn_input=[starts](unsigned slot,bool initial)->std::optional<Spawn>{if(initial && slot<starts.size())return starts[slot];return {};};
  report["stage"]="construct actual source authority, roster and authored vehicle";
  Authority authority(host_options,index,output);
  MultiplayerOptions client=host_options;client.source_object_world=false;client.source_object_savegame_directory.clear();client.native_spawn_inputs.clear();client.campaign_spawn_input={};
  client.join="127.0.0.1:"+std::to_string(authority.port);client.local_players=2;client.width=960;client.height=540;client.mute=true;
  client.prepared_world=std::make_shared<MultiplayerPreparedWorld>(client);
  SDL_SetMainReady();client.desktop=std::make_shared<DesktopSession>();auto *window=client.desktop->window(960,540,false);
  SDL_SetWindowTitle(window,"TU1.5 class3 physics: two live multiplayer views");
  SDL_SysWMinfo wm{};SDL_VERSION(&wm.version);
#if defined(SDL_VIDEO_DRIVER_X11)
  if(SDL_GetWindowWMInfo(window,&wm) && wm.subsystem==SDL_SYSWM_X11)std::cout<<"acceptance_x11_window="<<wm.info.x11.window<<std::endl;
#endif
  SDL_RaiseWindow(window);require(SDL_SetWindowInputFocus(window)==0,"Actual campaign window cannot acquire focus");SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  unsigned frames{},commands{},forward_commands{},scene_frames{},view_frames[2]{},vehicle_draws[2]{};
  std::optional<SourceModelRegionBank> rendered_vehicle_regions;
  const auto vehicle_initial=cache.load_initial_model_definition(placement->definition);float travel{};std::optional<Vec3> first;
  const int look_pixels=std::getenv("HALO2_TU15_CAMPAIGN_LOOK_PIXELS")?std::stoi(std::getenv("HALO2_TU15_CAMPAIGN_LOOK_PIXELS")):0;
  bool pressed{},released{},look_sent{};float look_delta{};std::optional<float> initial_pitch;
  std::vector<MultiplayerState> received;
  client.frame_observer=[&](const auto &frame){if(!frames++)std::cout<<"acceptance_first_frame=1"<<std::endl;require(frame.submitted,"Campaign renderer submitted no frame");};
  client.source_scene_observer=[&](const MultiplayerState &state,const SceneObjects &scene) {
    // SourceSceneRoot intentionally excludes type1 vehicles. Vehicles have a
    // separate authored placement/transform/region bank and completed WORLD
    // nodes; validate those owners before their actual per-view draw callback.
    for(const auto &root:state.source_scene_roots) {
      const auto object=std::find_if(scene.objects().begin(),scene.objects().end(),[&](const auto &p){return p.handle==root.object;});
      require(object!=scene.objects().end() && object->solved_transform && !std::memcmp(&*object->solved_transform,&root.transform,sizeof(Transform)),"Campaign renderer changed a received scene root transform");
    }
    const auto vehicle=std::find_if(state.world_vehicles.begin(),state.world_vehicles.end(),[&](const auto &p){return p.handle==authority.vehicle.load();});
    require(vehicle!=state.world_vehicles.end() && vehicle->placement==index && vehicle->regions.has_value(),"Vehicle snapshot lost its actual placement or original model region bank");
    const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),[&](const auto &p){return p.object==vehicle->handle;});
    require(pose!=state.source_body_poses.end() && !pose->nodes.empty(),"Vehicle snapshot lost its completed source WORLD hierarchy");
    rendered_vehicle_regions=vehicle->regions;++scene_frames;
  };
  client.vehicle_draw_observer=[&](unsigned view,std::uint32_t object,std::uint32_t definition,const ObjectLightingState&,bool,unsigned lod,std::span<const Vertex> vertices) {
    if(object!=authority.vehicle.load() || vertices.empty())return;
    require(definition==placement->definition,"Actual GPU vehicle draw changed its authored definition");
    require(view<2 && rendered_vehicle_regions.has_value(),"Visible vehicle draw has no received original region choices");
    const auto sections=source_initial_model_sections(vehicle_initial,rendered_vehicle_regions->selection,lod);
    for(const auto &vertex:vertices)require(std::find(sections.begin(),sections.end(),vertex.source_section)!=sections.end(),"Vehicle draw changed an original selected model section");
    ++vehicle_draws[view];
  };
  client.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &camera){
    require(view<2 && state.source_game_type==1,"Rendered view lost original campaign world kind");
    require(std::isfinite(camera.position.x) && std::isfinite(camera.position.y) && std::isfinite(camera.position.z),"Campaign camera is nonfinite");++view_frames[view];
    if(view==0) {if(!initial_pitch)initial_pitch=camera.pitch;look_delta=std::max(look_delta,std::abs(camera.pitch-*initial_pitch));}
  };
  client.input_observer=[&](unsigned,std::span<const Controls> input,const MultiplayerState &state) {
    {std::lock_guard lock(authority.mutex);if(authority.error)std::rethrow_exception(authority.error);}
    require(state.source_game_type==1,"UDP snapshot changed original campaign world kind");
    // Count only the actual admitted source roster, never the local pre-join
    // display state. Begin the declared NPC stimulus after both views have
    // drawn its received geometry, so the motion is visible from its start.
    if(std::count_if(state.players.begin(),state.players.end(),[](const auto &p){return p.connected;})!=3)return;
    if(frames && vehicle_draws[0] && vehicle_draws[1])authority.ready.store(true);
    if(frames && look_pixels && !look_sent) {
      require(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Campaign demonstration cannot acquire relative mouse input");
      SDL_Event motion{};motion.type=SDL_MOUSEMOTION;motion.motion.windowID=SDL_GetWindowID(window);motion.motion.yrel=look_pixels;
      require(SDL_PushEvent(&motion)==1,"SDL rejected the ordinary campaign mouse look");look_sent=true;
    }
    received.push_back(state);++commands;
    for(const auto &p:state.players)if(p.connected)require(p.source_configuration && (*p.source_configuration)[0x44]==content.player_start_representations.front() && (*p.source_configuration)[0x7c]==1,"UDP campaign profile/team was remapped");
    for(const auto &vehicle:state.world_vehicles)if(vehicle.handle==authority.vehicle.load()) {
      if(!first)first=vehicle.transform.position;travel=std::max(travel,length(vehicle.transform.position-*first));
    }
    if(!pressed && state.tick>=35){key(window,SDL_SCANCODE_W,true);pressed=true;}
    if(!released && state.tick>=50){key(window,SDL_SCANCODE_W,false);released=true;}
    if(!input.empty())forward_commands+=input.front().forward>0;
    if(state.tick>=180 && frames>=90){SDL_Event quit{};quit.type=SDL_QUIT;require(SDL_PushEvent(&quit)==1,"SDL rejected the finite completion event");}
  };
  report["stage"]="render actual UDP source world through two local views";
  const auto result=run_multiplayer(client);authority.finish();
  unsigned exact{};Json comparisons=Json::array();
  for(const auto &state:received) {
    const auto match=std::any_of(authority.history.begin(),authority.history.end(),[&](const auto &a){return same(a,state);});
    exact+=match;comparisons.push_back({{"tick",state.tick},{"exact",match}});
  }
  std::ofstream(output/"authority-comparisons.json")<<comparisons.dump(2)<<'\n';
  report["neutral_prefix_ticks"]=authority.neutral_prefix_ticks;report["causal_controller_response"]=authority.causal_response;
  report["runtime_exit"]=result;report["submitted_frames"]=frames;report["view_frames"]={view_frames[0],view_frames[1]};
  report["declared_mouse_look_pixels"]=look_pixels;report["mouse_look_sent"]=look_sent;report["observed_camera_look_delta"]=look_delta;
  require(!look_pixels || (look_sent && look_delta>.2f),"Actual campaign mouse input did not turn the rendered camera");
  report["observed_input_ticks"]=commands;report["forward_commands"]=forward_commands;report["exact_lan_snapshot_comparisons"]=exact;
  report["source_vehicle_render_frames"]=scene_frames;report["vehicle_draws"]={vehicle_draws[0],vehicle_draws[1]};report["maximum_vehicle_distance"]=travel;report["object"]=authority.vehicle.load();
  require(result==0 && frames>=90 && view_frames[0]>0 && view_frames[1]>0 && commands>0 && exact==commands && forward_commands>0 && scene_frames>0 && vehicle_draws[0]>0 && vehicle_draws[1]>0 && travel>content.player.radius && authority.neutral_prefix_ticks==30 && authority.causal_response>content.player.radius,"Visible campaign class3 integration did not complete all live gates");
  report["passed"]=true;report["acceptance_passed"]=true;
  report["boundary"]="Actual shared kind1 source authority, original campaign representations, UDP state comparison, ordinary SDL player input, actual class3 controller motion and two rendered views. NPC inputs and the three-player native stress roster are declared; autonomous campaign AI/scripts, retail cooperative-session admission, player seating, Xbox hardware trajectories and full parity remain unproven.";
 }catch(const std::exception &e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(!output.empty())std::ofstream(output/"report.json")<<report.dump(2)<<'\n';
 std::cout<<(report["passed"].get<bool>()?"PASS":"RED")<<" visible lan-campaign: "<<report.dump()<<'\n';
 return report["passed"].get<bool>()?0:1;
}
