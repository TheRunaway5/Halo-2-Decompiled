#define SDL_MAIN_HANDLED
#include "halo2/desktop_session.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <future>
#include <thread>
#include <SDL_syswm.h>

// Inputs are ordinary SDL events. The sole location override is the explicitly
// declared authored lift-interior start, before tick zero. Nothing supplies a
// contact, velocity, force, solver result or post-step transform.
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool yes,const char *why){if(!yes)throw FormatError(why);}
Json xyz(Vec3 v){return Json::array({v.x,v.y,v.z});}
void key(Uint32 window,SDL_Scancode scan,bool down) {
  SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
  e.key.state=down?SDL_PRESSED:SDL_RELEASED;e.key.keysym.scancode=scan;
  e.key.keysym.sym=SDL_GetKeyFromScancode(scan);
  check(SDL_PushEvent(&e)==1,"SDL rejected acceptance key input");
}
void focus(SDL_Window *window) {
  SDL_RaiseWindow(window);
  check(SDL_SetWindowInputFocus(window)==0,"Acceptance window could not acquire OS focus");
  SDL_Event e{};e.type=SDL_WINDOWEVENT;e.window.windowID=SDL_GetWindowID(window);
  e.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;
  check(SDL_PushEvent(&e)==1,"SDL rejected acceptance focus event");
}
void quit() {SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"SDL rejected acceptance quit");}
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
  std::mutex mutex;std::vector<MultiplayerState> history;std::exception_ptr error;std::jthread worker;
  Host(MultiplayerOptions options) {
    std::promise<void> startup;auto ready=startup.get_future();
    worker=std::jthread([this,options=std::move(options),startup=std::move(startup)](std::stop_token stop)mutable {
      bool started=false;try {
      // Construction, all source calls and retirement share one executor.
      MultiplayerPreparedWorld world(options);
      LanHost transport(world.match,0,multiplayer_fingerprint(options.map,world.cache->shared_directory(),0,world.rules.weapon,options.gameplay_xbe));
      world.match.configure_replicated_first_person(world.cache->load_first_person_binding(world.rules.weapon,2));
      initial_inventory=inventory(world);port=transport.port();host_slot=world.local_slots.front();
      started=true;startup.set_value();
      std::array<Controls,multiplayer_capacity> controls{};
      auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        transport.receive(controls);world.match.step(controls);transport.broadcast();
        {std::lock_guard lock(mutex);history.push_back(world.match.state());}
        next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){
      if(!started)startup.set_exception(std::current_exception());
      else {std::lock_guard lock(mutex);error=std::current_exception();}
    }});
    ready.get();
  }
  void finish(){worker.request_stop();worker.join();if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv) try {
  check(argc==7,"Supply MAP SHARED_MAPS TU15_XBE VISTA_UI OUTPUT_DIR movement|lift|vehicle|objects|lan-movement");
  const std::string scenario=argv[6];const bool lan=scenario=="lan-movement",lift=scenario=="lift",vehicle=scenario=="vehicle",objects=scenario=="objects";
  check(lan||lift||vehicle||objects||scenario=="movement","Unknown multiplayer physics scenario");
  check(std::getenv("DISPLAY")||std::getenv("WAYLAND_DISPLAY"),"Visible desktop display is required");
  const std::filesystem::path output=argv[5];std::filesystem::create_directories(output);
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];o.gameplay_xbe=argv[3];o.ui_assets=argv[4];
  o.starting_weapon=5;o.width=960;o.height=540;o.local_players=lan?1:2;o.mute=true;
  o.source_object_world=true;o.source_object_savegame_directory=output/"source-savegame";
  o.variant.values.set(2,0);o.variant.values.set(10,0); // Finite test lifecycle owns exit, not game-over.
  Json report{{"scenario",scenario},{"reference","English TU1.5 only"},{"visible",true},
      {"live_physics_parity",false},{"stimulus","SDL keyboard events; no supplied physics results"}};
  std::unique_ptr<Host> host;
  if(lan) {
    auto host_options=o;host_options.local_players=1;
    host=std::make_unique<Host>(host_options);
    report["initial_inventory"]=host->initial_inventory;
    o.join="127.0.0.1:"+std::to_string(host->port);report["udp_port"]=host->port;
    o.source_object_world=false; // Remote authority owns the solved source bank.
  } else {
    if(lift) {
      const Cache cache(o.map,o.shared_maps);const auto content=cache.load_content(o.bsp,true);
      const auto spawn=lift_entrance(cache,content);o.native_spawn_inputs={spawn};
      report["declared_initial_location"]=xyz(spawn.position);
      report["location_authority"]="Authored Lockout phmo lower-polyhedron center and placement transform";
    }
    o.prepared_world=std::make_shared<MultiplayerPreparedWorld>(o);
    report["initial_inventory"]=inventory(*o.prepared_world);
  }
  o.desktop=std::make_shared<DesktopSession>();auto *window=o.desktop->window(o.width,o.height,false);
  SDL_SetWindowTitle(window,("TU1.5 physics acceptance: "+scenario).c_str());
  SDL_SysWMinfo wm{};SDL_VERSION(&wm.version);
#ifdef SDL_VIDEO_DRIVER_X11
  if(SDL_GetWindowWMInfo(window,&wm) && wm.subsystem==SDL_SYSWM_X11)
    std::cout<<"acceptance_x11_window="<<wm.info.x11.window<<std::endl;
#endif
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);focus(window);
  Json samples=Json::array(),frames=Json::array();
  std::ofstream trace(output/"ticks.jsonl");check(bool(trace),"Could not create live tick trace");
  const auto window_id=SDL_GetWindowID(window);unsigned local_slot=~0u;bool started=false;
  std::uint32_t start_step{};float initial_z{},peak_z{},fastest{},largest_crouch{},distance{};
  Vec3 initial{};unsigned jump_commands{},forward_commands{},matched_snapshots{};
  unsigned grenade_commands{};bool attached_grenade{},released_grenade{},falling_grenade{},detonated{};
  std::array<unsigned,4> grenade_draws{};
  o.grenade_draw_observer=[&](unsigned view,std::uint32_t,std::uint32_t,std::uint32_t,std::span<const Vertex> vertices) {
    check(view<o.local_players && !vertices.empty(),"Actual grenade draw lost its view or geometry");++grenade_draws[view];
  };
  o.frame_observer=[&](const MultiplayerFrameTiming &f) {
    if(frames.empty())std::cout<<"acceptance_first_frame=1"<<std::endl;
    frames.push_back({{"frame",f.frame},{"views",f.views},{"wall_ms",f.wall_ms},{"submitted",f.submitted}});
  };
  o.input_observer=[&](unsigned step,std::span<const Controls> commands,const MultiplayerState&s) {
    if(!started) {
      for(unsigned n=0;n<s.players.size();++n)
        if(s.players[n].connected && (!lan || n!=host->host_slot)){local_slot=n;break;}
      if(local_slot==~0u)return;
      const auto &p=s.players[local_slot];started=true;start_step=step;initial=p.position;
      initial_z=peak_z=p.position.z;
    }
    const auto elapsed=step-start_step;const auto &p=s.players[local_slot];const auto &c=commands[0];
    peak_z=std::max(peak_z,p.position.z);fastest=std::max(fastest,p.velocity.z);
    largest_crouch=std::max(largest_crouch,p.crouch);distance=std::max(distance,length(p.position-initial));
    jump_commands+=c.jump;forward_commands+=c.forward!=0;
    Json row{{"input_step",step},{"elapsed",elapsed},{"authority_tick",s.tick},{"slot",local_slot},
      {"player",p.source_player},{"unit",p.source_unit},{"position",xyz(p.position)},
      {"velocity",xyz(p.velocity)},{"crouch",p.crouch},{"grounded",p.native_grounded},
      {"controls",{{"forward",c.forward},{"right",c.right},{"jump",c.jump},{"crouch",c.crouch}}}};
    grenade_commands+=c.grenade;row["grenade_command"]=c.grenade;
    row["grenade_projectiles"]=Json::array();
    for(const auto &g:s.grenade_projectiles) {
      attached_grenade|=g.parent!=null_datum;released_grenade|=g.parent==null_datum;
      falling_grenade|=g.parent==null_datum && g.velocity.z<0;
      row["grenade_projectiles"].push_back({{"datum",g.datum},{"parent",g.parent},{"position",xyz(g.position)},{"velocity",xyz(g.velocity)}});
    }
    detonated|=!s.grenade_detonations.empty();
    if(lan) {
      std::lock_guard lock(host->mutex);
      const auto at=std::find_if(host->history.begin(),host->history.end(),[&](const auto&h){return h.tick==s.tick;});
      if(at!=host->history.end()) {
        const auto &a=at->players[local_slot];
        const bool same=a.source_player==p.source_player && a.source_unit==p.source_unit &&
          a.position.x==p.position.x && a.position.y==p.position.y && a.position.z==p.position.z &&
          a.velocity.x==p.velocity.x && a.velocity.y==p.velocity.y && a.velocity.z==p.velocity.z && a.crouch==p.crouch;
        row["exact_authority_snapshot"]=same;check(same,"LAN snapshot disagrees with the actual same-tick authority");
        ++matched_snapshots;
      }
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
    if(objects && elapsed==35){key(window_id,SDL_SCANCODE_Q,true);key(window_id,SDL_SCANCODE_Q,false);}
    if(!lift&&!vehicle&&!objects) switch(elapsed) {
      case 25:focus(window);key(window_id,SDL_SCANCODE_W,true);break;
      case 40:key(window_id,SDL_SCANCODE_W,false);break;
      case 50:key(window_id,SDL_SCANCODE_LSHIFT,true);break;
      case 65:key(window_id,SDL_SCANCODE_LSHIFT,false);break;
      case 85:key(window_id,SDL_SCANCODE_SPACE,true);key(window_id,SDL_SCANCODE_SPACE,false);break;
    }
    if(elapsed>=180)quit();
  };
  // Finite local screenshots retain all earlier real input delivery. Network
  // exit uses SDL_QUIT because exact tick capture deliberately rejects LAN.
  if(!lan){o.capture_tick=181;o.screenshot=output/"frame.ppm";}
  const int result=run_multiplayer(o);if(host)host->finish();trace.close();
  report["runtime_exit"]=result;report["observed_input_ticks"]=samples.size();report["submitted_frames"]=frames.size();
  report["local_slot"]=local_slot;report["initial_position"]=xyz(initial);report["maximum_distance"]=distance;
  report["initial_z"]=initial_z;report["peak_z"]=peak_z;report["maximum_upward_velocity"]=fastest;
  report["maximum_crouch"]=largest_crouch;report["jump_commands"]=jump_commands;report["forward_commands"]=forward_commands;
  report["exact_lan_snapshot_comparisons"]=matched_snapshots;
  bool passed=result==0 && started && samples.size()>=170 && frames.size()>0;
  if(lift){report["lift_travel_observed"]=peak_z>initial_z && fastest>0;passed&=peak_z>initial_z && fastest>0;}
  else if(vehicle) {
    report["final_inventory"]=inventory(*o.prepared_world);
    passed&=!report["initial_inventory"]["authored_vehicle_candidates"].empty() &&
      report["final_inventory"]["live_scene_vehicle_roots"].get<unsigned>()>0;
    report["driving_and_solved_contacts_verified"]=false;
    passed=false; // Presence alone can never complete driving acceptance.
  } else if(objects) {
    report["grenade_commands"]=grenade_commands;report["attached_grenade"]=attached_grenade;
    report["released_grenade"]=released_grenade;report["falling_grenade"]=falling_grenade;
    report["detonated"]=detonated;report["grenade_draws_by_view"]=grenade_draws;
    report["dynamic_prop_impulses_verified"]=false;
    passed&=grenade_commands==1 && attached_grenade && released_grenade && falling_grenade && detonated;
    for(unsigned view=0;view<o.local_players;++view)passed&=grenade_draws[view]>0;
  } else passed&=distance>0 && fastest>0 && largest_crouch>0 && jump_commands==1 && forward_commands>0 && (!lan||matched_snapshots>0);
  if(!lan)report["final_inventory"]=inventory(*o.prepared_world);
  report["acceptance_passed"]=passed;
  report["limit"]="Live native behavior only. Full retail contact/solver, driving, dynamic-object impulses and complete TU1.5 parity remain required.";
  {std::ofstream file(output/"report.json");file<<report.dump(2)<<'\n';check(bool(file),"Could not write acceptance report");}
  {std::ofstream file(output/"frames.json");file<<frames.dump()<<'\n';}
  std::cout<<(passed?"PASS":"RED")<<" visible "<<scenario<<": ticks="<<samples.size()<<" peak_z="<<peak_z
      <<" upward_velocity="<<fastest<<" crouch="<<largest_crouch<<" LAN_comparisons="<<matched_snapshots<<'\n';
  return passed?0:2;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
