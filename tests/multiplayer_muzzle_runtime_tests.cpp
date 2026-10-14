#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/particle_draw.hpp"
#include "halo2/multiplayer_network.hpp"
#include <SDL.h>
#include <cmath>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

using namespace halo2;
namespace {
void require(bool condition,const char *why){if(!condition)throw FormatError(why);}
void push(SDL_Event event){require(SDL_PushEvent(&event)==1,"Actual SDL event queue rejected input");}
void button(unsigned window,bool held){
  SDL_Event event{};event.type=held?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
  event.button.windowID=window;event.button.button=SDL_BUTTON_LEFT;
  event.button.state=held?SDL_PRESSED:SDL_RELEASED;push(event);
}
struct Host {
  Cache cache;LevelContent content;MultiplayerMatch match;LanHost host;
  std::exception_ptr error;std::mutex mutex;std::jthread worker;
  static SlayerRules rules(const Cache &c){auto r=slayer_rules(UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}});r.weapon=c.slayer_starting_weapon(5);return r;}
  Host(const MultiplayerOptions &o):cache(o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe)),
    content(cache.load_content(0,true)),match(content,0,cache.spawns(),rules(cache)),host(match,0,
      multiplayer_fingerprint(o.map,cache.shared_directory(),0,cache.slayer_starting_weapon(5),o.gameplay_xbe)){
    match.configure_replicated_first_person(cache.load_first_person_binding(cache.slayer_starting_weapon(5),2));
    worker=std::jthread([this](std::stop_token stop){try{std::array<Controls,16> controls{};auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()){host.receive(controls);
        const auto roster=match.state();
        if(std::any_of(roster.players.begin(),roster.players.end(),[](const auto&p){return p.connected;}))match.step(controls);
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);}}
      catch(...){std::lock_guard lock(mutex);error=std::current_exception();}});
  }
  void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv) {
  try {
    if(argc<4 || argc>7)throw FormatError("Supply MAP SHARED_MAPS UPDATE_XBE [PLAYERS] [SCREENSHOT] [--lan]");
    if(!std::getenv("DISPLAY") || !std::filesystem::exists(argv[1]))return 77;
    SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
    MultiplayerOptions options;
    options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
    options.local_players=argc>=5?unsigned(std::stoul(argv[4])):1;
    options.width=960;options.height=720;options.starting_weapon=5;options.mute=true;
    options.validation=std::getenv("HALO2_MUZZLE_VALIDATION");
    bool lan{};
    for(int at=5;at<argc;++at)if(std::string(argv[at])=="--lan")lan=true;
      else {options.screenshot=argv[at];options.capture_tick=36;}
    require(!lan || options.screenshot.empty(),"LAN muzzle regression uses live expiry rather than a local capture");
    std::unique_ptr<Host> host;if(lan){host=std::make_unique<Host>(options);options.join="127.0.0.1:"+std::to_string(host->host.port());}
    options.desktop=std::make_shared<DesktopSession>();
    auto *window=options.desktop->window(options.width,options.height,false);
    const auto window_id=SDL_GetWindowID(window);
    SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    SDL_Event focus{};focus.type=SDL_WINDOWEVENT;focus.window.windowID=window_id;
    focus.window.event=SDL_WINDOWEVENT_FOCUS_GAINED;push(focus);
    require(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Actual relative mouse capture failed");
    unsigned positive{},transparent{},empty_after{},submitted{},input_tick{};
    std::set<std::uint32_t> effect_ticks;bool fired{};
    options.input_observer=[&](std::uint32_t tick,std::span<const Controls> controls,const MultiplayerState &state){
      input_tick=tick;
      if(tick==18)button(window_id,true);
      if(tick==40)button(window_id,false);
      if(tick==60){SDL_Event quit{};quit.type=SDL_QUIT;push(quit);}
      fired|=state.players[0].loaded<60;
      for(unsigned i=1;i<controls.size();++i)require(!controls[i].fire,"Mouse fire leaked into another local view");
    };
    options.effect_observer=[&](unsigned view,std::uint32_t tick,std::span<const ParticleDraw> draws){
      ++submitted;
      if(view!=0)require(draws.empty(),"Another local player's foreground received this muzzle flash");
      if(view==0 && !draws.empty()) {
        bool visible=false;
        for(const auto &draw:draws) {
          require(draw.material && draw.material->vertex.classification==7,"Actual authored muzzle material missing");
          for(float value:draw.vertex.attributes[4])require(std::isfinite(value),"Actual muzzle marker is not finite");
          // Original224550/2265C0 quantizes the authored age fade to zero
          // before and at age1. A retained endpoint is a valid submission.
          const auto alpha=draw.vertex.attributes[8][3];
          require(std::isfinite(alpha)&&alpha>=0&&alpha<=1,"Actual muzzle sprite has invalid packed alpha");
          visible|=alpha>0;transparent+=alpha==0;
        }
        if(visible){++positive;effect_ticks.insert(tick);}
      }
      if(view==0 && input_tick>48 && draws.empty())++empty_after;
    };
    const auto result=run_multiplayer(options);if(host)host->finish();
    require(result==0 && fired,"Actual mouse fire did not reach the stock SMG");
    require(submitted>10 && positive>0 && effect_ticks.size()>2,"Actual fire never reached native particle rendering");
    if(options.screenshot.empty())require(empty_after>0,"Authored muzzle particles did not expire after firing");
    std::cout<<"PASS actual SDL SMG fire: "<<positive<<" visible particle submissions on "<<effect_ticks.size()
        <<" ticks; "<<transparent<<" authored zero-alpha endpoints; "<<options.local_players
        <<" independent local views; native LAN="<<lan<<"\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
