#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/animation_presentation.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
struct Host {
  std::shared_ptr<const Cache> cache;LevelContent content;MultiplayerMatch match;LanHost host;
  std::exception_ptr error;std::mutex mutex;std::jthread worker;
  static SlayerRules rules(const Cache &c){auto r=slayer_rules(UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}});r.weapon=c.slayer_starting_weapon(5);return r;}
  Host(const MultiplayerOptions &o):cache(std::make_shared<const Cache>(o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe))),
    content(cache->load_content(0,true)),match(content,0,cache->spawns(),rules(*cache)),host(match,0,
      multiplayer_fingerprint(o.map,cache->shared_directory(),0,cache->slayer_starting_weapon(5),o.gameplay_xbe)) {
    check(bool(cache->original_data()->grenade_constants),"Verified selected grenade operands absent");
    match.configure_grenades(cache,*cache->original_data()->grenade_constants);
    match.configure_replicated_first_person(cache->load_first_person_binding(cache->slayer_starting_weapon(5),2));
    worker=std::jthread([this](std::stop_token stop){try {
      std::array<Controls,16> commands{};auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        host.receive(commands);
        const auto roster=match.state();
        if(std::any_of(roster.players.begin(),roster.players.end(),[](const auto&p){return p.connected;}))match.step(commands);
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){std::lock_guard lock(mutex);error=std::current_exception();}});
  }
  void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv){try {
  check(argc>=4&&argc<=8,"Supply MAP SHARED_MAPS UPDATE_XBE [--split4] [--lan] [--capture FILE]");
  if(!std::getenv("DISPLAY")||!std::filesystem::exists(argv[1]))return 77;
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];o.gameplay_xbe=argv[3];
  o.width=640;o.height=480;o.starting_weapon=5;o.mute=true;bool lan{},capture{};
  for(int n=4;n<argc;++n)if(std::string(argv[n])=="--lan")lan=true;
    else if(std::string(argv[n])=="--split4")o.local_players=4;
    else if(std::string(argv[n])=="--capture"){
      check(++n<argc,"Capture path absent");o.screenshot=argv[n];o.capture_tick=32;capture=true;
    }else throw FormatError("Unknown grenade runtime option");
  check(!capture||!lan,"Tick capture is an isolated local diagnostic");
  Cache cache(o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe));
  const auto weapon=cache.load_content(0,true).weapons.at(cache.slayer_starting_weapon(5));
  const auto throw_clip=cache.load_unit_weapon_action(weapon.first_person_animation,
      0x030000d9,0x030000d9,0x030000d9,0x0d000021);
  std::unique_ptr<Host> host;if(lan){host=std::make_unique<Host>(o);o.join=host->host.join_address();}
  o.desktop=std::make_shared<DesktopSession>();auto *window=o.desktop->window(o.width,o.height,false);
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  unsigned pulses{},fp_throws{},body_throws{};bool prepared{},released{},detonated{},debit{};
  int grenade_type=-1;
  std::array<unsigned,4> grenade_draws{},fractional_draws{};
  struct DrawSample {std::uint32_t tick{};std::vector<Vec3> positions;};
  std::map<std::pair<unsigned,std::uint32_t>,DrawSample> last_draws;
  std::map<std::uint32_t,std::pair<std::uint32_t,std::uint32_t>> body_clips;
  o.input_observer=[&](unsigned tick,std::span<const Controls> commands,const MultiplayerState &s) {
    if(grenade_type<0 && s.players[0].connected){
      const auto &bank=s.players[0].grenades;
      check(bank==std::array<std::uint8_t,2>{2,0} || bank==std::array<std::uint8_t,2>{0,2},
          "Grenade runtime test needs an actual two-frag or two-plasma spawn");
      grenade_type=bank[0]?0:1;
    }
    body_clips.clear();
    for(const auto &p:s.players)if(p.connected && (p.source_action_word&(1u<<22)) && p.grenade_action.animation_active)
      body_clips.emplace(p.source_unit,std::pair{p.grenade_action.graph,p.grenade_action.clip});
    pulses+=commands[0].grenade;
    if(tick==18) {
      SDL_RaiseWindow(window);
      SDL_SetWindowInputFocus(window);
      check(SDL_SetWindowInputFocus(window)==0,"Could not focus the actual grenade test window");
    }
    if(tick==20) {
      // A slow four-view frame can consume ticks18..20 before another SDL
      // poll. Complete the actual focus delivery before checking/sending F.
      SDL_RaiseWindow(window);
      check(SDL_SetWindowInputFocus(window)==0,"Could not refocus the actual grenade test window");
      SDL_PumpEvents();
      check(SDL_GetKeyboardFocus()==window,"Grenade test lost actual OS keyboard focus before F");
      std::cout<<"actual SDL F delivery: focused_window="<<SDL_GetWindowID(SDL_GetKeyboardFocus())<<'\n';
      if(grenade_type==1){
        check(SDL_SetRelativeMouseMode(SDL_TRUE)==0,"Could not enable actual relative mouse look");
        const auto &settings=o.desktop->keyboard_settings;
        const float gain=.0025f*float(settings.sensitivity)/3.f;
        check(gain>0,"Actual look sensitivity must be positive");
        SDL_Event look{};look.type=SDL_MOUSEMOTION;look.motion.windowID=window_id;
        look.motion.yrel=int(std::lround(1.2f/gain))*(settings.invert?-1:1);
        check(SDL_PushEvent(&look)==1,"SDL downward plasma look queue failed");
      }
      SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=window_id;e.key.state=SDL_PRESSED;
      e.key.keysym.scancode=SDL_SCANCODE_F;e.key.keysym.sym=SDLK_f;
      check(SDL_PushEvent(&e)==1,"SDL F down queue failed");e.type=SDL_KEYUP;e.key.state=SDL_RELEASED;
      check(SDL_PushEvent(&e)==1,"SDL F up queue failed");
    }
    for(const auto &g:s.grenade_projectiles)if(g.unit==s.players[0].source_unit) {
      check(g.type==grenade_type,"Actual projectile differs from authored spawn grenade type");
      prepared|=g.parent!=null_datum;released|=g.parent==null_datum;
      check((g.datum>>16)!=0 && std::isfinite(g.position.x),"Actual projectile lost salt/current pose");
    }
    if(grenade_type>=0)debit|=s.players[0].grenades[unsigned(grenade_type)]==1;
    detonated|=!s.grenade_detonations.empty();
    if(tick==130){SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"SDL quit queue failed");}
  };
  o.animation_observer=[&](unsigned view,unsigned,const AnimationRenderObservation &p) {
    if(p.first_person&&p.graph==throw_clip.source_graph&&p.animation==throw_clip.source_index) {
      check(view==0&&!p.vertices.empty(),"Authored throw escaped its local view or has no posed geometry");++fp_throws;
    }
    if(!p.first_person)if(const auto clip=body_clips.find(p.unit);clip!=body_clips.end() &&
        clip->second==std::pair{p.graph,p.animation} && !p.vertices.empty())++body_throws;
  };
  o.grenade_draw_observer=[&](unsigned view,std::uint32_t tick,std::uint32_t owner,std::uint32_t definition,
                             std::span<const Vertex> vertices) {
    check(view<o.local_players && owner!=null_datum && definition!=null_datum,
        "Grenade world draw lost its real owner or local view");
    check(!vertices.empty() && vertices.size()%3==0,"Authored grenade has no complete world triangles");
    for(const auto&vertex:vertices)
      check(std::isfinite(vertex.position.x)&&std::isfinite(vertex.position.y)&&std::isfinite(vertex.position.z),
          "Grenade draw contains an invalid world pose");
    auto&last=last_draws[{view,owner}];
    if(last.tick==tick && last.positions.size()==vertices.size()) {
      const auto movement=vertices.front().position-last.positions.front();
      if(length(movement)>1e-5f) {
        // An unchanged tick can only acquire movement from presentation.
        // Require the whole selected mesh to translate together, so a LOD
        // change cannot count as a fractional position sample.
        bool translated=true;
        for(unsigned n=0;n<vertices.size();++n)
          translated&=length((vertices[n].position-last.positions[n])-movement)<1e-4f;
        if(translated)++fractional_draws[view];
      }
    }
    last.tick=tick;last.positions.clear();
    for(const auto&vertex:vertices)last.positions.push_back(vertex.position);
    ++grenade_draws[view];
  };
  const auto result=run_multiplayer(o);if(host)host->finish();
  std::cout<<"actual window grenade: pulses="<<pulses<<" debit="<<debit<<" prepared="<<prepared
      <<" released="<<released<<" detonated="<<detonated<<" fp_throw="<<fp_throws<<" body_throw="<<body_throws<<'\n';
  check(result==0&&pulses==1&&debit&&prepared&&released&&(capture||detonated),"Actual window F lifecycle/inventory failed");
  check(fp_throws>0,"Actual window never rendered the registered source first-person throw");
  if(o.local_players>1)check(body_throws>0,"Four-view match submitted no actual body poses");
  for(unsigned view=0;view<o.local_players;++view) {
    check(grenade_draws[view]>0,"Local view received no authored grenade world draw");
    check(fractional_draws[view]>0,"Grenade world positions never moved between renders of the same simulation tick");
    std::cout<<"view="<<view<<" grenade_draws="<<grenade_draws[view]
        <<" fractional_position_draws="<<fractional_draws[view]<<'\n';
  }
  std::cout<<"PASS actual SDL F -> source throw pose/inventory -> salted attachment/release"
      <<(capture?" -> finite released capture":" -> detonation")<<", views="
      <<o.local_players<<" nativeLAN="<<lan<<" grenade_type="<<grenade_type<<" first_person_throw_frames="<<fp_throws<<'\n';
  std::cout<<"PASS authored grenade world geometry submitted independently to every local view\n";
  std::cout<<"LIMIT grenade particle effects, angular physics and original Havok remain separate owners\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
