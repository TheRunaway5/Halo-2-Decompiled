#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "halo2/player_action_edges.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/particle_draw.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <set>
#include <future>
#include <thread>
#include <SDL_syswm.h>

// Inputs are ordinary SDL events. Location overrides use declared authored
// lift or vehicle-entry frames before tick zero. Nothing supplies a
// contact, velocity, force, solver result or post-step transform.
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
Json xyz(Vec3 v){return Json::array({v.x,v.y,v.z});}
void focus(SDL_Window *window);
void key(Uint32 window,SDL_Scancode scan,bool down) {
  if(down) {
    auto *surface=SDL_GetWindowFromID(window);check(surface,"Acceptance key lost its actual window");
    focus(surface);
  }
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
  e.key.state=down?SDL_PRESSED:SDL_RELEASED;e.key.keysym.scancode=scan;
  e.key.keysym.sym=SDL_GetKeyFromScancode(scan);
  check(SDL_PushEvent(&e)==1,"SDL rejected acceptance key input");
}
void focus(SDL_Window *window) {
#if defined(HALO2_ACCEPTANCE_X11_FOCUS) && defined(SDL_VIDEO_DRIVER_X11)
  SDL_SysWMinfo native{};SDL_VERSION(&native.version);
  if(SDL_GetWindowWMInfo(window,&native) && native.subsystem==SDL_SYSWM_X11) {
    // The finite UI test represents the user's requested interaction. Ask the
    // window manager to activate its real surface, rather than forging SDL
    // focus events or relying on a raise that the compositor may ignore.
    const auto display=native.info.x11.display;
    XEvent event{};event.xclient.type=ClientMessage;event.xclient.display=display;
    event.xclient.window=native.info.x11.window;event.xclient.format=32;
    event.xclient.message_type=XInternAtom(display,"_NET_ACTIVE_WINDOW",False);
    event.xclient.data.l[0]=2;event.xclient.data.l[1]=CurrentTime;
    check(XSendEvent(display,DefaultRootWindow(display),False,
        SubstructureRedirectMask|SubstructureNotifyMask,&event)!=0,"Acceptance WM activation request failed");
    XFlush(display);
  }
#endif
  SDL_RaiseWindow(window);
  check(SDL_SetWindowInputFocus(window)==0,"Acceptance window could not acquire OS focus");
  if(!SDL_GetRelativeMouseMode()) {
    int w{},h{};SDL_GetWindowSize(window,&w,&h);SDL_WarpMouseInWindow(window,w/2,h/2);
    check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Acceptance window could not capture actual mouse input");
  }
  // Deliver actual backend focus transitions before inserting keys. A delayed
  // lost event can otherwise clear the press, and loading releases capture.
  SDL_PumpEvents();
  const auto deadline=SDL_GetTicks()+1000u;
  while(!(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS) && SDL_GetTicks()<deadline) {
    SDL_RaiseWindow(window);
    check(SDL_SetWindowInputFocus(window)==0,"Acceptance focus request failed after window presentation");
    SDL_Delay(10);SDL_PumpEvents();
  }
  check(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS,"Acceptance window did not retain actual OS input focus");
}
void quit() {SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"SDL rejected acceptance quit");}
struct ObserverController {
  SDL_Joystick *joystick{};SDL_JoystickID instance=-1;
  ObserverController() {
    const int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
        SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    check(device>=0 && SDL_IsGameController(device),"Could not attach the visible observer's SDL gamepad");
    joystick=SDL_JoystickOpen(device);check(joystick,"Could not open the actual virtual observer device");
    instance=SDL_JoystickInstanceID(joystick);
    axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT,-1.f);axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT,-1.f);
    axis(SDL_CONTROLLER_AXIS_LEFTY,1.f);
  }
  void axis(SDL_GameControllerAxis axis,float value) {
    check(SDL_JoystickSetVirtualAxis(joystick,int(axis),Sint16(std::lround(
        std::clamp(value,-1.f,1.f)*32767.f)))==0,"SDL rejected observer stick input");
  }
  ~ObserverController() {
    SDL_JoystickClose(joystick);
    for(int n=0;n<SDL_NumJoysticks();++n)if(SDL_JoystickGetDeviceInstanceID(n)==instance) {
      SDL_JoystickDetachVirtual(n);break;
    }
  }
};
Spawn lift_entrance(const Cache &cache,const LevelContent &content) {
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const Tag&t){
    return t.group=="bloc" && t.name=="scenarios\\objects\\multi\\lockout\\lockout_lift_tube\\lockout_lift_tube";
  });
  check(tag!=cache.tags().end(),"Authored Lockout lift definition absent");
  const auto at=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto&o){return o.definition==tag->datum;});
  check(at!=content.objects.end(),"Authored Lockout lift placement absent");
  const auto object=cache.load_source_tag_metadata(at->definition);
  const auto model=cache.load_source_tag_metadata(Bytes(object.bytes_at(object.source_address(),object.size())).u32(0x38));
  const auto physics=cache.load_source_tag_metadata(Bytes(model.bytes_at(model.source_address(),model.size())).u32(0x24));
  const Bytes p(physics.bytes_at(physics.source_address(),physics.size()));
  check(p.u32(0x70)==2 && p.u32(0x108)==2,"Authored Lockout phantom layout changed");
  const Bytes hull(physics.bytes_at(p.u32(0x74),0x100));
  Transform local;local.position={hull.f32(0x40),hull.f32(0x44),hull.f32(0x48)};
  const auto basis=authored_basis(at->rotation);
  const auto root=source_object_transform(at->position,basis.forward,basis.up,at->scale,false);
  Spawn spawn;spawn.position=compose_transform(root,local).position;
  // View direction only; root is the actual authored lower hull center.
  spawn.facing=at->rotation.z;return spawn;
}
struct VehicleEntrance {Spawn spawn;Vec3 target;std::size_t placement;unsigned seat;};
VehicleEntrance vehicle_entrance(const Cache &cache,const LevelContent &content) {
  check(bool(content.unit_interactions),"Vehicle acceptance has no authored unit interactions");
  const auto &assets=*content.unit_interactions;
  const auto actor=assets.units.find(content.player.datum);
  check(actor!=assets.units.end(),"Vehicle acceptance has no authored player seat animations");
  const auto selected=std::getenv("HALO2_TU15_VEHICLE_NAME");
  const std::string vehicle_name=selected?selected:"warthog";
  check(vehicle_name=="warthog" || vehicle_name=="ghost" || vehicle_name=="banshee",
      "Vehicle acceptance selection has no authored Coagulation fixture");
  for(const auto &[row,placement]:assets.netgame_placements) {
    const auto &object=content.objects.at(placement);
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const Tag&t){return t.datum==object.definition;});
    if(tag==cache.tags().end() || tag->name.find("\\"+vehicle_name+"\\")==std::string::npos)continue;
    const auto &definition=assets.units.at(object.definition);
    const auto basis=authored_basis(object.rotation);
    const auto root=source_object_transform(object.position,basis.forward,basis.up,object.scale,false);
    const auto nodes=pose_model_nodes_world(definition.hierarchy,root);
    for(unsigned index=0;index<definition.seats.size();++index) {
      const auto &seat=definition.seats[index];if(!(seat.flags&4u))continue;
      const auto animations=actor->second.seat_animations.find(seat.mode);
      if(animations==actor->second.seat_animations.end() || animations->second.entries.empty())continue;
      const auto marker=definition.markers.find(seat.marker),entry=definition.markers.find(seat.entry_marker);
      if(marker==definition.markers.end() || entry==definition.markers.end())continue;
      const auto seats=pose_model_markers(marker->second,nodes,definition.permutations,{},1);
      const auto entrances=pose_model_markers(entry->second,nodes,definition.permutations,{},1);
      if(seats.empty() || entrances.empty())continue;
      // This is an explicitly declared pre-tick test location, not an
      // inferred retail spawn or an injected post-step contact. Approach the
      // actual entry marker from its facing side, above the cached BSP floor.
      check(seat.radius>0,"Vehicle entry marker has no authored approach radius");
      Spawn spawn;spawn.position=entrances.front().world.position+
          entrances.front().world.basis[0]*(seat.radius*.5f);
      const CollisionWorld terrain(content.collision);
      const auto floor=terrain.raycast(spawn.position+Vec3{0,0,2*content.player.height},
          {0,0,-1},4*content.player.height);
      if(!floor)continue;
      spawn.position.z=floor->position.z+content.player.radius;
      const auto eye=spawn.position+Vec3{0,0,content.player.camera_height};
      const auto look=entrances.front().world.position-eye;
      spawn.facing=std::atan2(look.y,look.x);
      return {spawn,entrances.front().world.position,placement,index};
    }
  }
  throw FormatError("Vehicle acceptance found no authored "+vehicle_name+" driver entrance");
}
Json inventory(const MultiplayerPreparedWorld &world) {
  Json out;out["authored_vehicle_candidates"]=Json::array();out["scene_roots"]=Json::array();
  for(unsigned n=0;n<world.content.netgame_equipment.size();++n) {
    const auto &row=world.content.netgame_equipment[n];
    for(const auto &item:row.items) {
      const auto tag=std::find_if(world.cache->tags().begin(),world.cache->tags().end(),[&](const Tag&t){return t.datum==item.definition;});
      if(tag!=world.cache->tags().end() && tag->group=="vehi")
        out["authored_vehicle_candidates"].push_back({{"row",n},{"classification",row.classification},
            {"definition",tag->datum},{"name",tag->name},{"position",xyz(row.position)}});
    }
  }
  unsigned vehicles=0;
  for(const auto &root:world.match.simulation().scene().objects()) {
    const auto &placement=world.content.objects.at(root.placement);
    const auto tag=std::find_if(world.cache->tags().begin(),world.cache->tags().end(),[&](const Tag&t){return t.datum==placement.definition;});
    if(tag==world.cache->tags().end())continue;
    vehicles+=tag->group=="vehi";
    out["scene_roots"].push_back({{"handle",root.handle},{"placement",root.placement},{"definition",tag->datum},
        {"group",tag->group},{"name",tag->name},{"position",xyz(root.position)}});
  }
  out["live_scene_vehicle_roots"]=vehicles;
  const auto count=world.source_objects ? world.source_objects->physical_body_count() : std::nullopt;
  out["physical_body_count_available"]=bool(count);
  if(count)out["source_physical_body_count"]=*count;
  return out;
}
struct Host {
  unsigned port{},host_slot{};
  Json initial_inventory;
  std::shared_ptr<const LevelContent> content;
  std::mutex mutex;std::vector<MultiplayerState> history;std::exception_ptr error;std::jthread worker;
  Host(MultiplayerOptions options) {
    std::promise<void> startup;auto ready=startup.get_future();
    worker=std::jthread([this,options=std::move(options),startup=std::move(startup)](std::stop_token stop)mutable {
      bool started=false;try {
      // Construction, all source calls and retirement share one executor.
      MultiplayerPreparedWorld world(options);
      LanHost transport(world.match,0,multiplayer_fingerprint(options.map,world.cache->shared_directory(),0,world.rules.weapon,options.gameplay_xbe));
      world.match.configure_replicated_first_person(world.cache->load_first_person_binding(world.rules.weapon,2));
      initial_inventory=inventory(world);content=std::make_shared<LevelContent>(world.content);
      port=transport.port();host_slot=world.local_slots.front();
      started=true;startup.set_value();
      std::array<Controls,multiplayer_capacity> controls{};
      auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        transport.receive(controls);
        // HELLO can publish a new participant at the existing world clock,
        // before the next physics frame. Retain that actual sent-state phase.
        {std::lock_guard lock(mutex);history.push_back(world.match.state());}
        world.match.step(controls);
        {std::lock_guard lock(mutex);history.push_back(world.match.state());}
        transport.broadcast();
        next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){
      try {throw;}catch(const std::exception &e){std::cerr<<"acceptance_host_error="<<e.what()<<std::endl;}
      catch(...){std::cerr<<"acceptance_host_error=non-standard exception"<<std::endl;}
      if(!started)startup.set_exception(std::current_exception());
      else {std::lock_guard lock(mutex);error=std::current_exception();}
    }});
    ready.get();
  }
  void finish(){worker.request_stop();worker.join();if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv) try {
  check(argc==7,"Supply MAP SHARED_MAPS TU15_XBE VISTA_UI OUTPUT_DIR movement|lift|vehicle|objects|lan-movement|lan-lift|lan-vehicle|lan-objects");
  const std::string scenario=argv[6];const bool lan=scenario.starts_with("lan-"),
      lift=scenario=="lift"||scenario=="lan-lift",vehicle=scenario=="vehicle"||scenario=="lan-vehicle",objects=scenario=="objects"||scenario=="lan-objects";
  check(!lan || scenario=="lan-movement" || lift || vehicle || objects,"Unknown LAN physics scenario");
  check(lan||lift||vehicle||objects||scenario=="movement","Unknown multiplayer physics scenario");
  check(std::getenv("DISPLAY")||std::getenv("WAYLAND_DISPLAY"),"Visible desktop display is required");
  const std::filesystem::path output=argv[5];std::filesystem::create_directories(output);
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];o.gameplay_xbe=argv[3];o.ui_assets=argv[4];
  o.starting_weapon=5;o.width=960;o.height=540;o.local_players=lan&&!vehicle&&!objects?1:2;o.mute=true;
  o.source_object_world=true;o.source_object_savegame_directory=output/"source-savegame";
  o.variant.values.set(2,0);o.variant.values.set(10,0); // Finite test lifecycle owns exit, not game-over.
  Json report{{"scenario",scenario},{"reference","English TU1.5 only"},{"visible",true},
      {"live_physics_parity",false},{"stimulus","SDL keyboard events; no supplied physics results"}};
  std::unique_ptr<Host> host;
  std::optional<VehicleEntrance> vehicle_start;
  if(lift || vehicle) {
    const Cache cache(o.map,o.shared_maps);const auto content=cache.load_content(o.bsp,true);
    if(lift) {
      const auto spawn=lift_entrance(cache,content);o.native_spawn_inputs={spawn};
      report["declared_initial_location"]=xyz(spawn.position);
      report["location_authority"]="Authored Lockout phmo lower-polyhedron center and placement transform";
    } else {
      vehicle_start=vehicle_entrance(cache,content);o.native_spawn_inputs={vehicle_start->spawn};
      report["declared_initial_location"]=xyz(vehicle_start->spawn.position);
      report["location_authority"]="Declared half-radius approach to authored vehicle entry marker, above cached BSP floor before tick zero";
      const auto selected=std::getenv("HALO2_TU15_VEHICLE_NAME");
      report["selected_vehicle"]=selected?selected:"warthog";
      report["driver_seat"]=vehicle_start->seat;report["vehicle_placement"]=vehicle_start->placement;
    }
  }
  if(lan) {
    auto host_options=o;host_options.local_players=1;
    host=std::make_unique<Host>(host_options);
    report["initial_inventory"]=host->initial_inventory;
    o.join="127.0.0.1:"+std::to_string(host->port);report["udp_port"]=host->port;
    o.source_object_world=false; // Remote authority owns the solved source bank.
    o.native_spawn_inputs.clear(); // The authority alone owns declared test spawns.
  } else {
    o.prepared_world=std::make_shared<MultiplayerPreparedWorld>(o);
    report["initial_inventory"]=inventory(*o.prepared_world);
  }
  const auto &interaction_content=lan?*host->content:o.prepared_world->content;
  o.desktop=std::make_shared<DesktopSession>();auto *window=o.desktop->window(o.width,o.height,false);
  SDL_SetWindowTitle(window,("TU1.5 physics acceptance: "+scenario).c_str());
  SDL_SysWMinfo wm{};SDL_VERSION(&wm.version);
#ifdef SDL_VIDEO_DRIVER_X11
  if(SDL_GetWindowWMInfo(window,&wm) && wm.subsystem==SDL_SYSWM_X11)
    std::cout<<"acceptance_x11_window="<<wm.info.x11.window<<std::endl;
#endif
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);focus(window);
  std::unique_ptr<ObserverController> observer;
  if(vehicle || objects) {
    observer=std::make_unique<ObserverController>();o.input_instances={-1,observer->instance};
    if(objects)observer->axis(SDL_CONTROLLER_AXIS_LEFTY,0.f);
    report["observer_input"]="Actual SDL virtual gamepad sampled by local view1; movement and look only";
  }
  Json samples=Json::array(),frames=Json::array();
  std::ofstream trace(output/"ticks.jsonl");check(bool(trace),"Could not create live tick trace");
  const auto window_id=SDL_GetWindowID(window);unsigned local_slot=~0u;bool started=false;
  std::uint32_t start_step{};float initial_z{},peak_z{},fastest{},largest_crouch{},distance{};
  Vec3 initial{};unsigned jump_commands{},forward_commands{},matched_snapshots{};
  unsigned grenade_commands{};bool attached_grenade{},released_grenade{},falling_grenade{},detonated{};
  bool grenade_pressed{},grenade_released{};
  struct AuthoritySample {
    unsigned slot;std::uint32_t tick,player,unit,parent;std::int16_t seat;
    Vec3 position,velocity;float crouch;
    std::vector<WorldVehiclePresentation> vehicles;
    std::vector<SourceWorldPose> poses;
    std::vector<SourceSceneRoot> scene_roots;
    std::vector<NativeGrenadeProjectile> grenades;
    std::vector<NativeGrenadeDetonation> detonations;
  };
  std::vector<AuthoritySample> authority_samples;
  std::uint32_t driven_vehicle=null_datum;Vec3 vehicle_initial{};float vehicle_distance{};
  bool vehicle_entered{},vehicle_exited{};unsigned vehicle_drive_commands{};
  bool entry_released{},exit_released{},entry_approaching{};
  std::optional<unsigned> entry_press,entered_tick,drive_start,exit_press;
  std::optional<unsigned> drive_end;bool steering_started{},steering_released{},forward_released{};
  Vec3 drive_initial{};float driven_distance{},vehicle_climb{};
  Vec3 drive_forward{};float steering_angle{};unsigned steering_commands{};
  unsigned observer_slot=~0u,observer_back_commands{};Vec3 observer_initial{};float observer_distance{};
  std::array<unsigned,4> grenade_draws{};
  std::array<unsigned,4> explosion_particle_draws{};
  std::set<std::uint32_t> detonated_projectiles;
  o.effect_observer=[&](unsigned view,std::uint32_t,std::span<const ParticleDraw> draws) {
    check(view<o.local_players,"Effect submission lost its actual local view");
    for(const auto &draw:draws)if(detonated_projectiles.contains(draw.effect_owner))
      ++explosion_particle_draws[view];
  };
  o.grenade_draw_observer=[&](unsigned view,std::uint32_t,std::uint32_t,std::uint32_t,std::span<const Vertex> vertices) {
    check(view<o.local_players && !vertices.empty(),"Actual grenade draw lost its view or geometry");++grenade_draws[view];
  };
  o.frame_observer=[&](const MultiplayerFrameTiming &f) {
    if(frames.empty())std::cout<<"acceptance_first_frame=1"<<std::endl;
    frames.push_back({{"frame",f.frame},{"views",f.views},{"wall_ms",f.wall_ms},{"submitted",f.submitted}});
  };
  unsigned adopted_scene_frames{};
  o.source_scene_observer=[&](const MultiplayerState &state,const SceneObjects &scene) {
    if(!lan)return;
    check(state.source_scene_roots_authoritative,"Replica scene observer has no source authority");
    for(const auto &root:state.source_scene_roots) {
      const auto &object=scene.get(root.object);
      check(object.placement==root.placement && object.solved_transform &&
          !std::memcmp(&*object.solved_transform,&root.transform,sizeof(Transform)) &&
          object.visible==root.visible,"Rendered scene root differs from received source placement/frame");
      const auto pose=std::find_if(state.source_body_poses.begin(),state.source_body_poses.end(),
          [&](const auto &p){return p.object==root.object;});
      check(object.solved_nodes.has_value(),"Rendered scene lost its source WORLD adoption");
      if(root.has_world_nodes)check(pose!=state.source_body_poses.end() &&
          object.solved_nodes->size()==pose->nodes.size() &&
          !std::memcmp(object.solved_nodes->data(),pose->nodes.data(),pose->nodes.size()*sizeof(Transform)),
          "Rendered scene hierarchy differs from received source WORLD bank");
      else check(object.solved_nodes->empty(),"Empty source hierarchy gained invented nodes");
    }
    ++adopted_scene_frames;
  };
  Json camera_frames=Json::array();unsigned seated_camera_frames{},exit_camera_frames{};
  float seated_camera_distance{};
  std::array<float,4> submitted_horizontal_fov{};
  o.view_observer=[&](unsigned view,const Camera &,float horizontal_fov,std::int16_t) {
    check(view<submitted_horizontal_fov.size(),"Camera submission exceeds actual local views");
    submitted_horizontal_fov[view]=horizontal_fov;
  };
  o.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &camera) {
    if(objects && started && view<2) {
      const auto slot=view?observer_slot:local_slot;
      if(slot<state.players.size()) {
        const auto &player=state.players[slot];
        camera_frames.push_back({{"view",view},{"tick",state.tick},{"slot",slot},
            {"unit",player.source_unit},{"dead",player.dead},{"player_position",xyz(player.position)},
            {"position",xyz(camera.position)},{"forward",xyz(camera.forward)},
            {"up",xyz(camera.up)},{"field_of_view",camera.field_of_view},{"horizontal_field_of_view",camera.horizontal_field_of_view}});
      }
    }
    if(!vehicle || !started || view!=0 || local_slot>=state.players.size())return;
    const auto &player=state.players[local_slot];
    const auto eye=ordinary_unit_eye(player.position,interaction_content.player.camera_height,
        interaction_content.player.crouch_camera_height,player.crouch);
    const auto separation=length(camera.position-eye);
    if(player.source_parent==driven_vehicle && player.source_seat==int(vehicle_start->seat)) {
      ++seated_camera_frames;seated_camera_distance=std::max(seated_camera_distance,separation);
      if(lan) {
        check(player.source_vehicle_camera.has_value(),"UDP driver has no actual authority vehicle camera");
        const auto &expected=*player.source_vehicle_camera;
        check(!std::memcmp(&camera.position,&expected.position,sizeof(Vec3)) &&
            !std::memcmp(&camera.forward,&expected.forward,sizeof(Vec3)) &&
            !std::memcmp(&camera.up,&expected.up,sizeof(Vec3)) &&
            std::bit_cast<std::uint32_t>(submitted_horizontal_fov[view])==
                std::bit_cast<std::uint32_t>(expected.horizontal_fov),
            "Submitted UDP driver camera differs from the actual authority vehicle view");
      }
    } else if(vehicle_exited && player.source_parent==null_datum) {
      ++exit_camera_frames;
    }
    camera_frames.push_back({{"view",view},{"tick",state.tick},{"unit",player.source_unit},
        {"parent",player.source_parent},{"seat",player.source_seat},{"position",xyz(camera.position)},
        {"forward",xyz(camera.forward)},{"up",xyz(camera.up)},{"ordinary_eye_distance",separation},
        {"horizontal_fov",submitted_horizontal_fov[view]},
        {"received_source_vehicle_camera",player.source_vehicle_camera.has_value()}});
  };
  o.input_observer=[&](unsigned step,std::span<const Controls> commands,const MultiplayerState&s) {
    if(!started) {
      for(unsigned n=0;n<s.players.size();++n)
        if(s.players[n].connected && (!lan || n!=host->host_slot)){local_slot=n;break;}
      if(local_slot==~0u)return;
      const auto &p=s.players[local_slot];started=true;start_step=step;initial=p.position;
      initial_z=peak_z=p.position.z;
      if(observer) {
        for(unsigned n=0;n<s.players.size();++n)if(s.players[n].connected && n!=local_slot && (!lan || n!=host->host_slot)) {
          observer_slot=n;observer_initial=s.players[n].position;break;
        }
        check(observer_slot!=~0u,"Visible observer did not join the real roster");
      }
      focus(window);
    }
    const auto elapsed=step-start_step;const auto &p=s.players[local_slot];const auto &c=commands[0];
    peak_z=std::max(peak_z,p.position.z);fastest=std::max(fastest,p.velocity.z);
    largest_crouch=std::max(largest_crouch,p.crouch);distance=std::max(distance,length(p.position-initial));
    jump_commands+=c.jump;forward_commands+=c.forward!=0;
    Json row{{"input_step",step},{"elapsed",elapsed},{"authority_tick",s.tick},{"slot",local_slot},
      {"player",p.source_player},{"unit",p.source_unit},{"position",xyz(p.position)},
      {"velocity",xyz(p.velocity)},{"crouch",p.crouch},{"grounded",p.native_grounded},
      {"window_focus",bool(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)},
      {"controls",{{"forward",c.forward},{"right",c.right},{"jump",c.jump},{"crouch",c.crouch},
          {"pickup",c.pickup},{"pickup_milliseconds",c.pickup_milliseconds},
          {"yaw",c.yaw},{"pitch",c.pitch}}}};
    grenade_commands+=c.grenade;row["grenade_command"]=c.grenade;
    row["dead"]=p.dead;row["health"]=p.health;row["shield"]=p.shield;
    row["source_scene_roots"]=s.source_scene_roots.size();
    row["grenade_projectiles"]=Json::array();
    if(vehicle) {
      const auto threshold=source_pickup_hold_threshold(interaction_content.pickup_hold_seconds);
      row["pickup_hold_threshold_ms"]=threshold;
      row["pickup_buttons"]=source_pickup_buttons(c.pickup,c.pickup?c.pickup_milliseconds:0,
          c.dual_wield,c.dual_wield?c.dual_wield_milliseconds:0,threshold);
      row["parent"]=p.source_parent;row["seat"]=p.source_seat;row["unit_action_word"]=p.source_action_word;
      row["action_prompt"]={{"action",p.action_prompt.target.action},
          {"index",p.action_prompt.target.index},{"object",p.action_prompt.target.object}};
      std::optional<Vec3> vehicle_position,vehicle_forward,entry_target;
      for(const auto &v:s.world_vehicles)if(v.placement==vehicle_start->placement) {
        if(driven_vehicle==null_datum){driven_vehicle=v.handle;vehicle_initial=v.transform.position;}
        check(v.handle==driven_vehicle,"Driving acceptance crossed a vehicle generation");
        vehicle_distance=std::max(vehicle_distance,length(v.transform.position-vehicle_initial));
        row["vehicle_root"]=xyz(v.transform.position);row["vehicle_forward"]=xyz(v.transform.basis[0]);row["vehicle"]=v.handle;
        vehicle_position=v.transform.position;
        vehicle_forward=v.transform.basis[0];
        const auto &definition=interaction_content.unit_interactions->units.at(
            interaction_content.objects.at(vehicle_start->placement).definition);
        const auto &seat=definition.seats.at(vehicle_start->seat);
        const auto pose=std::find_if(s.source_body_poses.begin(),s.source_body_poses.end(),[&](const auto &pose){return pose.object==v.handle;});
        check(pose!=s.source_body_poses.end(),"Vehicle snapshot omitted the actual WORLD hierarchy");
        const auto markers=pose_model_markers(definition.markers.at(seat.entry_marker),pose->nodes,definition.permutations,{},1);
        check(!markers.empty(),"Live source vehicle has no actual driver-entry WORLD marker");entry_target=markers.front().world.position;
        const auto entry_delta=*entry_target-(p.position+Vec3{0,0,interaction_content.player.camera_height});
        const auto entry_distance=length(entry_delta);
        row["entry_distance"]=entry_distance;row["entry_radius"]=seat.radius;
        row["entry_maximum_velocity"]=seat.maximum_velocity;row["entry_velocity"]=length(p.velocity);
        row["entry_aim_limit"]=seat.aim;row["entry_facing_limit"]=seat.facing;
        if(entry_distance>0) {
          const auto toward=entry_delta*(1.f/entry_distance);
          row["entry_aim_angle"]=std::acos(std::clamp(dot(toward,direction(c.yaw,c.pitch)),-1.f,1.f));
          row["entry_facing_angle"]=3.1415927f-std::acos(std::clamp(dot(toward,markers.front().world.basis[0]),-1.f,1.f));
        }
        if(!entry_press && report["selected_vehicle"]=="banshee") {
          if(entry_distance>seat.radius*.75f) {
            if(!entry_approaching || !c.forward)key(window_id,SDL_SCANCODE_W,true);
            entry_approaching=true;
          } else if(entry_approaching) {
            key(window_id,SDL_SCANCODE_W,false);entry_approaching=false;
          }
        }
      }
      if(observer) {
        check(commands.size()==2,"Observer gamepad lost its selected local view");
        const auto &watcher=s.players[observer_slot];const auto &input=commands[1];
        check(!input.fire && !input.grenade && !input.melee && !input.pickup,
            "Observer gamepad produced an unrequested gameplay button");
        observer_back_commands+=input.forward<0;
        observer_distance=std::max(observer_distance,length(watcher.position-observer_initial));
        row["observer"]={{"player",watcher.source_player},{"unit",watcher.source_unit},
            {"position",xyz(watcher.position)},{"forward_command",input.forward},
            {"yaw",input.yaw},{"pitch",input.pitch}};
        observer->axis(SDL_CONTROLLER_AXIS_LEFTY,elapsed<45?1.f:0.f);
        if(vehicle_position) {
          const auto look=*vehicle_position+Vec3{0,0,interaction_content.player.camera_height} -
              (watcher.position+Vec3{0,0,interaction_content.player.camera_height});
          const auto yaw=std::atan2(look.y,look.x),pitch=std::atan2(look.z,std::hypot(look.x,look.y));
          observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-std::remainder(yaw-input.yaw,6.283185307f)*8.f);
          observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,-(pitch-input.pitch)*8.f);
        }
      }
      if(p.source_parent==driven_vehicle && driven_vehicle!=null_datum) {
        check(p.source_seat==int(vehicle_start->seat),"Source attached the player to a different vehicle seat");
        vehicle_entered=true;vehicle_drive_commands+=c.forward>0;
        if(!entered_tick)entered_tick=elapsed;
        if(drive_start && vehicle_position) {
          const auto delta=*vehicle_position-drive_initial;
          driven_distance=std::max(driven_distance,std::hypot(delta.x,delta.y));
          vehicle_climb=std::max(vehicle_climb,delta.z);
          if(c.right!=0 && vehicle_forward) {
            ++steering_commands;
            const auto &f=*vehicle_forward;
            steering_angle=std::max(steering_angle,std::abs(std::atan2(
                drive_forward.x*f.y-drive_forward.y*f.x,drive_forward.x*f.x+drive_forward.y*f.y)));
          }
        }
      }else if(vehicle_entered && p.source_parent==null_datum)vehicle_exited=true;
      if(entry_press && entry_released && !vehicle_entered && elapsed>=*entry_press+45) {
        // A hold can expire after real contacts carry the player out of seat
        // range. Re-approach and retry through normal input; source admission
        // and attachment remain the only success signal.
        entry_press.reset();entry_released=false;
      }
      if(entry_target && !entry_press && elapsed%5==0) {
        focus(window);check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Could not enable actual vehicle-test mouse input");
        const auto look=*entry_target-(p.position+Vec3{0,0,interaction_content.player.camera_height});
        const float pitch=std::atan2(look.z,std::hypot(look.x,look.y));
        const float yaw=std::atan2(look.y,look.x);
        const auto &settings=o.desktop->keyboard_settings;const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Invalid actual mouse gain");SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=window_id;
        e.motion.xrel=-int(std::lround(std::remainder(yaw-c.yaw,6.283185307f)/gain));
        e.motion.yrel=-int(std::lround((pitch-c.pitch)/gain))*(settings.invert?-1:1);
        check(SDL_PushEvent(&e)==1,"SDL rejected vehicle-entry look input");
      }
      const int pickup=o.desktop->keyboard_settings.bindings[unsigned(PcAction::Pickup)];
      check(pickup>0 && pickup<SDL_NUM_SCANCODES,"Vehicle acceptance requires a keyboard pickup binding");
      if(!entry_press && driven_vehicle!=null_datum && p.action_prompt.target.object==driven_vehicle &&
          p.action_prompt.target.index==int(vehicle_start->seat) &&
          p.action_prompt.target.action>=4 && p.action_prompt.target.action<=6) {
        if(entry_approaching) {key(window_id,SDL_SCANCODE_W,false);entry_approaching=false;}
        focus(window);entry_press=elapsed;key(window_id,SDL_Scancode(pickup),true);
      }
      if(entry_press && !entry_released && c.pickup && c.pickup_milliseconds>=threshold) {
        key(window_id,SDL_Scancode(pickup),false);entry_released=true;
      }
      if(entered_tick && vehicle_position && vehicle_forward && !drive_start && elapsed>=*entered_tick+30) {
        drive_start=elapsed;drive_initial=*vehicle_position;drive_forward=*vehicle_forward;
        focus(window);key(window_id,SDL_SCANCODE_W,true);
        if(report["selected_vehicle"]=="banshee") {
          const auto &settings=o.desktop->keyboard_settings;
          const float gain=.0025f*float(settings.sensitivity)/3.f;
          check(gain>0,"Invalid actual flight-test mouse gain");
          SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
          const auto yaw=std::atan2(vehicle_forward->y,vehicle_forward->x);
          look.motion.xrel=-int(std::lround(std::remainder(yaw-c.yaw,6.283185307f)/gain));
          look.motion.yrel=-int(std::lround(-c.pitch/gain))*(settings.invert?-1:1);
          check(SDL_PushEvent(&look)==1,"SDL rejected Banshee departure look input");
        }
      }
      if(drive_start) {
        // A real desktop focus loss clears SDL's keys. Reacquire the actual
        // window and deliver the remaining observed input budget, instead of
        // silently shortening the driving sequence while counting wall ticks.
        if(!forward_released && !c.forward) {focus(window);key(window_id,SDL_SCANCODE_W,true);}
        if(vehicle_drive_commands>=(report["selected_vehicle"]=="banshee"?120u:25u) && !steering_started) {
          key(window_id,SDL_SCANCODE_D,true);steering_started=true;
          if(report["selected_vehicle"]!="warthog") {
            // Deliver an ordinary look turn as well as lateral input. The
            // actual authored controller decides how either affects motion.
            const auto &settings=o.desktop->keyboard_settings;
            const float gain=.0025f*float(settings.sensitivity)/3.f;
            check(gain>0,"Invalid actual vehicle-test mouse gain");
            SDL_Event e{};e.type=SDL_MOUSEMOTION;e.motion.windowID=window_id;
            e.motion.xrel=-int(std::lround(.6f/gain));
            if(report["selected_vehicle"]=="banshee")
              e.motion.yrel=-int(std::lround((.6f-c.pitch)/gain))*(settings.invert?-1:1);
            check(SDL_PushEvent(&e)==1,"SDL rejected vehicle driving look input");
          }
        }
        if(steering_started && !steering_released) {
          if(steering_commands>=25) {key(window_id,SDL_SCANCODE_D,false);steering_released=true;}
          else if(!c.right)key(window_id,SDL_SCANCODE_D,true);
        }
        if(vehicle_drive_commands>=(report["selected_vehicle"]=="banshee"?300u:65u) && !forward_released) {
          key(window_id,SDL_SCANCODE_W,false);forward_released=true;drive_end=elapsed;
        }
        if(drive_end && elapsed>=*drive_end+45 && !exit_press) {
          exit_press=elapsed;key(window_id,SDL_Scancode(pickup),true);
        }
      }
      if(exit_press && !exit_released && c.pickup && c.pickup_milliseconds>=threshold) {
        key(window_id,SDL_Scancode(pickup),false);exit_released=true;
      }
    }
    if(objects && observer) {
      check(commands.size()==2,"Object observer lost its actual gamepad view");
      const auto &watcher=s.players[observer_slot];const auto &input=commands[1];
      check(!input.fire && !input.grenade && !input.melee && !input.pickup,
          "Object observer produced an unrequested gameplay button");
      std::optional<Vec3> target;
      for(const auto &projectile:s.grenade_projectiles)
        if(projectile.parent==null_datum){target=projectile.position;break;}
      if(!target && !s.grenade_detonations.empty())target=s.grenade_detonations.back().position;
      if(!target)target=p.position;
      const auto eye=ordinary_unit_eye(watcher.position,interaction_content.player.camera_height,
          interaction_content.player.crouch_camera_height,watcher.crouch);
      const auto delta=*target-eye;
      observer->axis(SDL_CONTROLLER_AXIS_RIGHTX,-std::remainder(std::atan2(delta.y,delta.x)-input.yaw,6.283185307f)*8.f);
      observer->axis(SDL_CONTROLLER_AXIS_RIGHTY,-(std::atan2(delta.z,std::hypot(delta.x,delta.y))-input.pitch)*8.f);
      row["observer"]={{"slot",observer_slot},{"position",xyz(watcher.position)},{"dead",watcher.dead},
          {"yaw",input.yaw},{"pitch",input.pitch}};
    }
    row["source_action_word"]=p.source_action_word;
    row["source_grenade_phase"]=p.grenade_action.phase();
    row["source_grenade_counter"]=p.grenade_action.counter();
    row["source_held_grenade"]=p.grenade_action.held();
    row["source_grenade_current"]=p.grenade_current;
    if(objects && elapsed%5==0) {
      const auto released=std::find_if(s.grenade_projectiles.begin(),s.grenade_projectiles.end(),
          [](const auto &projectile){return projectile.parent==null_datum;});
      if(released!=s.grenade_projectiles.end()) {
        const auto eye=ordinary_unit_eye(p.position,interaction_content.player.camera_height,
            interaction_content.player.crouch_camera_height,p.crouch);
        const auto target=released->position-eye;
        const auto &settings=o.desktop->keyboard_settings;
        const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Invalid actual mouse gain while watching the source grenade");
        SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
        look.motion.xrel=-int(std::lround(std::remainder(std::atan2(target.y,target.x)-c.yaw,6.283185307f)/gain));
        look.motion.yrel=-int(std::lround((std::atan2(target.z,std::hypot(target.x,target.y))-c.pitch)/gain))*(settings.invert?-1:1);
        check(SDL_PushEvent(&look)==1,"SDL rejected actual look input while observing grenade flight");
      }
    }
    for(const auto &g:s.grenade_projectiles) {
      attached_grenade|=g.parent!=null_datum;released_grenade|=g.parent==null_datum;
      falling_grenade|=g.parent==null_datum && g.velocity.z<0;
      row["grenade_projectiles"].push_back({{"datum",g.datum},{"parent",g.parent},{"position",xyz(g.position)},{"velocity",xyz(g.velocity)}});
    }
    detonated|=!s.grenade_detonations.empty();
    for(const auto &event:s.grenade_detonations) {
      detonated_projectiles.insert(event.projectile);
      row["grenade_detonations"].push_back({{"serial",event.serial},{"tick",event.tick},
          {"projectile",event.projectile},{"effect",event.effect},{"position",xyz(event.position)},
          {"forward",xyz(event.forward)},{"up",xyz(event.up)}});
    }
    if(lan) {
      // Compare after the authority worker stops: HELLO's reply can reach this
      // thread before receive() returns and records that same-clock roster.
      // Every observed sample must match; no unavailable comparison is skipped.
      authority_samples.push_back({local_slot,s.tick,p.source_player,p.source_unit,p.source_parent,p.source_seat,
          p.position,p.velocity,p.crouch,s.world_vehicles,s.source_body_poses,s.source_scene_roots,s.grenade_projectiles,s.grenade_detonations});
      row["authority_comparison_deferred"]=true;
    }
    trace<<row.dump()<<'\n';samples.push_back(std::move(row));
    if(objects && elapsed==25) {
      focus(window);check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Could not enable actual object-test mouse input");
      const auto &settings=o.desktop->keyboard_settings;
      const float gain=.0025f*float(settings.sensitivity)/3.f;check(gain>0,"Invalid actual mouse gain");
      SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
      look.motion.yrel=int(std::lround(.8f/gain))*(settings.invert?-1:1);
      check(SDL_PushEvent(&look)==1,"SDL rejected downward object-test look");
    }
    if(objects) {
      const int binding=o.desktop->keyboard_settings.bindings[unsigned(PcAction::Grenade)];
      check(binding>0 && binding<SDL_NUM_SCANCODES,"Object acceptance needs the actual grenade key binding");
      if(!grenade_pressed && elapsed>=35) {
        key(window_id,SDL_Scancode(binding),true);grenade_pressed=true;
      }
      if(grenade_pressed && !grenade_released && c.grenade) {
        key(window_id,SDL_Scancode(binding),false);grenade_released=true;
      }
      if(grenade_pressed && !grenade_released && elapsed>=45 && elapsed%10==0)
        key(window_id,SDL_Scancode(binding),true);
    }
    if(!lift&&!vehicle&&!objects) switch(elapsed) {
      case 25:focus(window);key(window_id,SDL_SCANCODE_W,true);break;
      case 40:key(window_id,SDL_SCANCODE_W,false);break;
      case 50:key(window_id,SDL_SCANCODE_LSHIFT,true);break;
      case 65:key(window_id,SDL_SCANCODE_LSHIFT,false);break;
      case 85:focus(window);key(window_id,SDL_SCANCODE_SPACE,true);key(window_id,SDL_SCANCODE_SPACE,false);break;
    }
    if(elapsed>=(vehicle?(report["selected_vehicle"]=="banshee"?900u:300u):180u))quit();
  };
  // Finite local screenshots retain all earlier real input delivery. Network
  // exit uses SDL_QUIT because exact tick capture deliberately rejects LAN.
  if(!lan){o.capture_tick=vehicle?(report["selected_vehicle"]=="banshee"?901:301):181;o.screenshot=output/"frame.ppm";}
  const int result=run_multiplayer(o);if(host)host->finish();trace.close();
  if(lan) {
    Json comparisons=Json::array();
    for(const auto &p:authority_samples) {
      const bool same=std::any_of(host->history.begin(),host->history.end(),[&](const auto &h) {
        if(h.tick!=p.tick)return false;const auto &a=h.players[p.slot];
        const auto same_poses=[&] {
          if(h.source_body_poses.size()!=p.poses.size() || h.world_vehicles.size()!=p.vehicles.size())return false;
          if(h.source_scene_roots.size()!=p.scene_roots.size())return false;
          for(unsigned n=0;n<p.scene_roots.size();++n) {
            const auto &x=h.source_scene_roots[n],&y=p.scene_roots[n];
            if(x.object!=y.object || x.definition!=y.definition || x.placement!=y.placement ||
                x.visible!=y.visible || x.has_world_nodes!=y.has_world_nodes ||
                std::memcmp(&x.transform,&y.transform,sizeof(Transform)))return false;
          }
          for(unsigned i=0;i<p.poses.size();++i) {
            const auto &x=h.source_body_poses[i],&y=p.poses[i];
            if(x.object!=y.object || x.nodes.size()!=y.nodes.size() ||
                std::memcmp(&x.sphere,&y.sphere,sizeof(x.sphere)) ||
                std::memcmp(x.nodes.data(),y.nodes.data(),x.nodes.size()*sizeof(Transform)))return false;
          }
          for(unsigned i=0;i<p.vehicles.size();++i) {
            const auto &x=h.world_vehicles[i],&y=p.vehicles[i];
            if(x.handle!=y.handle || x.placement!=y.placement || std::memcmp(&x.transform,&y.transform,sizeof(Transform)))return false;
          }
          return true;
        };
        return a.source_player==p.player && a.source_unit==p.unit && a.source_parent==p.parent && a.source_seat==p.seat &&
            a.position.x==p.position.x && a.position.y==p.position.y && a.position.z==p.position.z &&
            a.velocity.x==p.velocity.x && a.velocity.y==p.velocity.y && a.velocity.z==p.velocity.z && a.crouch==p.crouch && same_poses() && [&] {
          if(h.grenade_detonations.size()!=p.detonations.size())return false;
          for(unsigned n=0;n<p.detonations.size();++n) {
            const auto &x=h.grenade_detonations[n],&y=p.detonations[n];
            if(x.source_locations.size()!=y.source_locations.size())return false;
            for(unsigned j=0;j<x.source_locations.size();++j)
              if(x.source_locations[j].name!=y.source_locations[j].name ||
                  std::memcmp(&x.source_locations[j].frame,&y.source_locations[j].frame,sizeof(Transform)))return false;
            if(x.serial!=y.serial || x.tick!=y.tick || x.projectile!=y.projectile ||
                x.definition!=y.definition || x.effect!=y.effect || x.player!=y.player || x.unit!=y.unit ||
                std::memcmp(&x.position,&y.position,sizeof(Vec3)) ||
                std::memcmp(&x.forward,&y.forward,sizeof(Vec3)) ||
                std::memcmp(&x.up,&y.up,sizeof(Vec3)))return false;
          }
          if(h.grenade_projectiles.size()!=p.grenades.size())return false;
          for(unsigned n=0;n<p.grenades.size();++n) {
            const auto &x=h.grenade_projectiles[n],&y=p.grenades[n];
            if(x.datum!=y.datum || x.definition!=y.definition || x.parent!=y.parent ||
                x.player!=y.player || x.unit!=y.unit || x.designator!=y.designator ||
                x.native_owner!=y.native_owner || x.type!=y.type || x.flags!=y.flags ||
                x.state!=y.state || x.age!=y.age ||
                std::memcmp(&x.position,&y.position,sizeof(Vec3)) ||
                std::memcmp(&x.velocity,&y.velocity,sizeof(Vec3)) ||
                std::memcmp(&x.forward,&y.forward,sizeof(Vec3)) ||
                std::memcmp(&x.up,&y.up,sizeof(Vec3)))return false;
            for(const auto pair:{std::pair{x.timer,y.timer},std::pair{x.timer_step,y.timer_step},
                std::pair{x.arming,y.arming},std::pair{x.arming_step,y.arming_step},std::pair{x.traveled,y.traveled}})
              if(std::bit_cast<std::uint32_t>(pair.first)!=std::bit_cast<std::uint32_t>(pair.second))return false;
          }
          return true;
        }();
      });
      comparisons.push_back({{"tick",p.tick},{"player",p.player},{"unit",p.unit},{"exact",same}});
      matched_snapshots+=same;
    }
    std::ofstream file(output/"authority-comparisons.json");file<<comparisons.dump(2)<<'\n';
    check(bool(file),"Could not retain actual authority comparisons");
    check(matched_snapshots==authority_samples.size(),"LAN snapshot disagrees with every actual same-tick authority publication");
  }
  report["runtime_exit"]=result;report["observed_input_ticks"]=samples.size();report["submitted_frames"]=frames.size();
  report["local_slot"]=local_slot;report["initial_position"]=xyz(initial);report["maximum_distance"]=distance;
  report["initial_z"]=initial_z;report["peak_z"]=peak_z;report["maximum_upward_velocity"]=fastest;
  report["maximum_crouch"]=largest_crouch;report["jump_commands"]=jump_commands;report["forward_commands"]=forward_commands;
  report["exact_lan_snapshot_comparisons"]=matched_snapshots;
  report["adopted_source_scene_frames"]=adopted_scene_frames;
  bool passed=result==0 && started && samples.size()>=170 && frames.size()>0;
  if(lan)passed&=adopted_scene_frames>0;
  if(lift){report["lift_travel_observed"]=peak_z>initial_z && fastest>0;passed&=peak_z>initial_z && fastest>0;}
  else if(vehicle) {
    passed&=!report["initial_inventory"]["authored_vehicle_candidates"].empty() &&
      driven_vehicle!=null_datum;
    report["driving_and_solved_contacts_verified"]=false;
    report["source_driver_entered"]=vehicle_entered;report["source_driver_exited"]=vehicle_exited;
    report["source_vehicle_maximum_distance"]=vehicle_distance;report["driver_forward_commands"]=vehicle_drive_commands;
    report["source_driver_horizontal_distance"]=driven_distance;
    report["source_vehicle_climb"]=vehicle_climb;
    if(report["selected_vehicle"]=="banshee")passed&=vehicle_climb>2.f && driven_distance>10.f;
    report["source_driving_observed"]=vehicle_entered && vehicle_drive_commands>0 && driven_distance>1;
    report["driver_steering_commands"]=steering_commands;report["source_driver_steering_radians"]=steering_angle;
    report["observer_backward_commands"]=observer_back_commands;report["observer_maximum_distance"]=observer_distance;
    report["source_driver_controls_completed"]=vehicle_entered && vehicle_exited &&
        vehicle_drive_commands>0 && driven_distance>1 && steering_commands>0 && steering_angle>.05f;
    passed&=report["source_driver_controls_completed"].get<bool>() && (!lan || matched_snapshots==samples.size());
    passed&=observer_back_commands>0 && observer_distance>.5f;
    report["submitted_seated_camera_frames"]=seated_camera_frames;
    report["submitted_camera_distance_from_first_person_eye"]=seated_camera_distance;
    report["submitted_camera_frames_after_exit"]=exit_camera_frames;
    passed&=seated_camera_frames>0 && seated_camera_distance>.5f && exit_camera_frames>0;
  } else if(objects) {
    report["grenade_commands"]=grenade_commands;report["attached_grenade"]=attached_grenade;
    report["released_grenade"]=released_grenade;report["falling_grenade"]=falling_grenade;
    report["detonated"]=detonated;report["grenade_draws_by_view"]=grenade_draws;
    report["explosion_particle_draws_by_view"]=explosion_particle_draws;
    report["dynamic_prop_impulses_verified"]=false;
    passed&=grenade_commands==1 && attached_grenade && released_grenade && falling_grenade && detonated;
    for(unsigned view=0;view<o.local_players;++view)passed&=grenade_draws[view]>0 && explosion_particle_draws[view]>0;
  } else passed&=distance>0 && fastest>0 && largest_crouch>0 && jump_commands==1 && forward_commands>0 && (!lan||matched_snapshots>0);
  if(!lan)report["final_inventory"]=inventory(*o.prepared_world);
  report["acceptance_passed"]=passed;
  report["limit"]="Live native behavior only. Full retail contact/solver, driving, dynamic-object impulses and complete TU1.5 parity remain required.";
  {std::ofstream file(output/"report.json");file<<report.dump(2)<<'\n';check(bool(file),"Could not write acceptance report");}
  {std::ofstream file(output/"frames.json");file<<frames.dump()<<'\n';}
  {std::ofstream file(output/"cameras.json");file<<camera_frames.dump(2)<<'\n';}
  std::cout<<(passed?"PASS":"RED")<<" visible "<<scenario<<": ticks="<<samples.size()<<" peak_z="<<peak_z
      <<" upward_velocity="<<fastest<<" crouch="<<largest_crouch<<" LAN_comparisons="<<matched_snapshots<<'\n';
  return passed?0:2;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
