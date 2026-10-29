#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/animation_presentation.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Preferences {
  std::filesystem::path path=std::filesystem::temp_directory_path()/
      ("halo2-species-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".cfg");
  ~Preferences(){std::error_code e;std::filesystem::remove(path,e);std::filesystem::remove(path.string()+".tmp",e);}
};
struct Host {
  std::shared_ptr<const Cache> cache;
  LevelContent content;
  MultiplayerMatch match;
  LanHost host;
  std::exception_ptr error;
  std::mutex mutex;
  std::atomic<bool> advance{};
  std::jthread worker;
  static SlayerRules rules(const Cache &cache){auto rules=slayer_rules(UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}});
    rules.weapon=cache.slayer_starting_weapon(5);return rules;}
  explicit Host(const MultiplayerOptions &o):cache(std::make_shared<const Cache>(
      o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe))),content(cache->load_content(0,true)),
      match(content,0,cache->spawns(),rules(*cache)),host(match,0,
          multiplayer_fingerprint(o.map,cache->shared_directory(),0,cache->slayer_starting_weapon(5),o.gameplay_xbe)) {
    match.configure_grenades(cache,*cache->original_data()->grenade_constants);
    for(unsigned representation:{2u,3u})
      match.configure_replicated_first_person(cache->load_first_person_binding(cache->slayer_starting_weapon(5),representation));
    worker=std::jthread([this](std::stop_token stop){try {
      std::array<Controls,16> commands{};auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        host.receive(commands);
        const auto roster=match.state();
        if(advance && std::any_of(roster.players.begin(),roster.players.end(),[](const auto&p){return p.connected;}))match.step(commands);
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){std::lock_guard lock(mutex);error=std::current_exception();}});
  }
  void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv)try {
  check(argc==4 || argc==5,"Supply repository root, output directory, single/mixed/lan and optional gameplay XBE");
  if(!std::getenv("DISPLAY"))return 77;
  const std::filesystem::path root=argv[1],output=argv[2];
  const std::string mode=argv[3];check(mode=="single"||mode=="mixed"||mode=="lan","Unknown species test mode");
  const auto gameplay_xbe=argc==5?std::filesystem::path(argv[4]):
      root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03";
  if(!std::filesystem::exists(root/"assets/disc/maps/mainmenu.map") ||
      !std::filesystem::exists(root/"assets/disc/maps/lockout.map") || !std::filesystem::exists(gameplay_xbe))return 77;
  std::filesystem::create_directories(output);
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  Cache front(root/"assets/disc/maps/mainmenu.map");
  Preferences preferences;
  {std::ofstream file(preferences.path);file<<"halo2-native-menu-v1\nprofile "<<std::quoted("Elite")
      <<"\nactive "<<std::quoted("Elite")<<"\noption "<<std::quoted("Elite")<<" 52 1\n";}
  const auto frontend=front.load_frontend();
  MenuState menu(frontend,preferences.path);menu.accept();menu.accept();
  const auto choose=[&](unsigned entry,unsigned screen){menu.select(entry);menu.accept();
    check(menu.frame().screen==screen,"Saved Elite profile menu route changed");};
  choose(2,30);choose(0,14);menu.select(0);menu.accept();
  check(menu.launch && menu.launch->selected_profiles.size()==1 &&
      menu.launch->selected_profiles[0][0x44]==1,"Normal Start lost the saved Elite selection");
  MultiplayerOptions options;
  options.map=root/"assets/disc/maps/lockout.map";
  options.shared_maps=root/"assets/disc/maps";
  options.gameplay_xbe=gameplay_xbe;
  options.variant=menu.launch->variant;options.starting_weapon=5;
  options.selected_profiles=menu.launch->selected_profiles;
  options.local_players=mode=="single"?1:4;
  for(unsigned i=1;i<options.local_players;++i) {
    std::array<std::uint8_t,9> appearance{10,0,0,10,std::uint8_t(i%2?0:1),0,0,0,0};
    options.selected_profiles.push_back(native_selected_player_configuration("View "+std::to_string(i),appearance,-1));
  }
  options.width=640;options.height=480;options.hidden=true;options.mute=true;
  options.validation=std::getenv("HALO2_TEST_VALIDATION")!=nullptr;
  options.capture_tick=100;options.screenshot=output/(mode+".ppm");
  if(mode=="lan"){options.capture_tick=0;options.frames=6000;options.screenshot.clear();}
  Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
  const auto content=cache.load_content(0,true);const auto weapon=cache.slayer_starting_weapon(5);
  std::array<FirstPersonBindingDefinition,4> expected;
  std::array<std::set<std::uint32_t>,4> body_graphs;
  std::array<unsigned,4> fp_frames{},body_frames{},camera_frames{};
  for(unsigned i=0;i<options.local_players;++i) {
    const auto representation=i%2?2u:3u;
    expected[i]=cache.load_first_person_binding(weapon,representation);
    const auto graph=cache.load_model(content.player_representations[representation].biped.datum,false).animation;
    body_graphs[i].insert(graph);
    const auto ids=cache.load_weapon_animation_identifiers(weapon);
    for(const auto action:{0x0400000cu,0x0a000017u,0x09000016u,0x0a000014u,0x09000015u})
      body_graphs[i].insert(cache.load_unit_weapon_action(graph,0x06000086,ids[0],ids[1],action).source_graph);
    body_graphs[i].insert(cache.load_unit_weapon_action(graph,0x06000086,ids[0],ids[1],0x0d000021,true).source_graph);
    try{body_graphs[i].insert(cache.load_unit_weapon_action(graph,0x06000086,ids[0],ids[1],0x1000006c,true).source_graph);}
    catch(const FormatError&){} // The preferred throw action is optional in the authored bank.
  }
  std::unique_ptr<Host> host;
  if(mode=="lan"){host=std::make_unique<Host>(options);options.join=host->host.join_address();}
  options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(640,480,true);
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=window_id;
  focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;check(SDL_PushEvent(&focus)==1,"SDL focus failed");
  bool prepared{},released{},fire_down{},fire_up{},reload_sent{},grenade_sent{};unsigned last_tick{};
  unsigned delivered_fire{},delivered_reload{},delivered_grenade{};
  options.input_observer=[&](unsigned tick,std::span<const Controls> commands,const MultiplayerState &state) {
    if(!commands.empty()) {
      delivered_fire+=commands[0].fire;delivered_reload+=commands[0].reload;delivered_grenade+=commands[0].grenade;
    }
    if(host)host->advance=true; // Shader/GPU setup must finish before the test's source clock starts.
    const auto phase=mode=="lan"?state.tick:tick;
    last_tick=std::max(last_tick,phase);
    if((phase>=6 && !fire_down) || (phase>=12 && !fire_up)) {
      const bool down=!fire_down;
      SDL_Event event{};event.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
      event.button.windowID=window_id;event.button.button=SDL_BUTTON_LEFT;
      event.button.state=down?SDL_PRESSED:SDL_RELEASED;
      check(SDL_PushEvent(&event)==1,"Elite fire input failed");
      if(down)fire_down=true;else fire_up=true;
    }
    if((phase>=18 && !reload_sent) || (phase>=75 && !grenade_sent)) {
      const bool reload=!reload_sent;
      SDL_Event event{};event.type=SDL_KEYDOWN;event.key.windowID=window_id;event.key.state=SDL_PRESSED;
      event.key.keysym.scancode=reload?SDL_SCANCODE_R:SDL_SCANCODE_F;
      event.key.keysym.sym=reload?SDLK_r:SDLK_f;
      check(SDL_PushEvent(&event)==1,"Elite grenade key down failed");
      event.type=SDL_KEYUP;event.key.state=SDL_RELEASED;check(SDL_PushEvent(&event)==1,"Elite grenade key up failed");
      if(reload)reload_sent=true;else grenade_sent=true;
    }
    for(unsigned i=0;i<options.local_players;++i)if(state.players[i].connected)
      check(multiplayer_representation(state.players[i].source_configuration)==unsigned(expected[i].representation),
          "Actual native runtime lost the selected species");
    for(const auto &g:state.grenade_projectiles)if(g.unit==state.players[0].source_unit) {
      prepared|=g.parent!=null_datum;released|=g.parent==null_datum;
    }
    if(mode=="lan" && state.tick>=120) {
      SDL_Event event{};event.type=SDL_QUIT;check(SDL_PushEvent(&event)==1,"Elite LAN quit failed");
    }
  };
  options.animation_observer=[&](unsigned view,unsigned id,const AnimationRenderObservation &observation) {
    check(view<options.local_players && id<options.local_players,"Rendered species has an unexpected owner");
    check(!observation.vertices.empty(),"Selected species produced no posed vertices");
    for(const auto &vertex:observation.vertices)
      check(std::isfinite(vertex.position.x)&&std::isfinite(vertex.position.y)&&std::isfinite(vertex.position.z),
          "Selected species produced invalid posed geometry");
    if(observation.first_person) {
      check(observation.graph==expected[id].graph,"Rendered first-person view uses the other species' weapon graph");++fp_frames[id];
    }else {
      check(body_graphs[id].contains(observation.graph),"Rendered body uses the other species' animation graph");++body_frames[id];
    }
  };
  options.view_observer=[&](unsigned view,const Camera &,float fov,std::int16_t) {
    const auto &biped=content.player_representations[expected[view].representation].biped;
    check(std::abs(fov-biped.camera_field_of_view)<.00001f,"Selected species camera uses another biped's field of view");
    ++camera_frames[view];
  };
  const auto result=run_multiplayer(options);if(host)host->finish();
  check(result==0 && last_tick>=99,"Elite normal Start did not complete its actual gameplay capture");
  for(unsigned i=0;i<options.local_players;++i) {
    check(fp_frames[i] && camera_frames[i],"Selected local species was never rendered");
    if(options.local_players>1)check(body_frames[i],"Mixed species body was never rendered in another viewport");
  }
  if(std::getenv("HALO2_TU15_SPECIES_TRACE"))std::cout<<"Delivered species controls: fire="<<delivered_fire
      <<" reload="<<delivered_reload<<" grenade="<<delivered_grenade<<" prepared="<<prepared<<" released="<<released
      <<" OS_focus="<<bool(SDL_GetWindowFlags(window)&SDL_WINDOW_INPUT_FOCUS)<<'\n';
  check(prepared&&released,"Actual Elite grenade input did not prepare and release its projectile");
  std::cout<<"PASS saved Elite normal Start: "<<mode<<", actual Vulkan bodies/arms/cameras and original Elite grenade attachment/release\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
