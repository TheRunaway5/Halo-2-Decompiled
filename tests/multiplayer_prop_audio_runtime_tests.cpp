#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/math.hpp"
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
struct Host {
  Cache cache;LevelContent content;MultiplayerMatch match;LanHost host;
  std::uint32_t module=null_datum;
  Controls shooter;
  std::atomic<bool> fire{},removed{};
  std::exception_ptr error;std::mutex mutex;std::jthread worker;
  static SlayerRules rules(const Cache &c){SlayerRules r;r.weapon=c.slayer_starting_weapon(5);r.time_ticks=r.score_limit=0;return r;}
  Host(const MultiplayerOptions &o):cache(o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe)),
    content(cache.load_content(0,true)),match(content,0,cache.spawns(),rules(cache)),host(match,0,
      multiplayer_fingerprint(o.map,cache.shared_directory(),0,cache.slayer_starting_weapon(5),o.gameplay_xbe)) {
    match.join("Native collision shooter");
    match.configure_replicated_first_person(cache.load_first_person_binding(cache.slayer_starting_weapon(5),2));
    const auto &props=match.simulation().props();
    for(const auto &p:props.states())if(p.definition==0xe6f80583u){module=p.handle;break;}
    check(module!=null_datum,"Actual Lockout power module missing");
    const auto placement=props.state(module).placement;
    CollisionWorld terrain(content.collision);bool vantage{};Vec3 from{},to{};
    for(const auto &t:props.triangles()) {
      if(t.source.instance!=int(placement))continue;
      const auto n=normalized(t.source_normal);if(std::abs(n.z)>.25f)continue;
      const auto center=(t.a+t.b+t.c)/3.f,origin=center+n*.8f;
      const auto hit=props.raycast(origin,center-origin);
      if(!hit || hit->handle!=module || terrain.raycast(origin,normalized(center-origin),.79f))continue;
      from=origin;to=center;vantage=true;break;
    }
    check(vantage,"Actual module has no unobstructed firing face");
    const auto aim=normalized(to-from);
    Spawn spawn;spawn.bsp=0;spawn.position=from-Vec3{0,0,content.player.camera_height};
    spawn.facing=std::atan2(aim.y,aim.x);
    // Only the test's location is supplied. Actual Match controls still run
    // the recovered SMG, collision, health, response delay and blast owners.
    DebugGameSettings settings;settings.players[0].noclip=true;match.debug_settings(settings);
    match.debug_request({DebugGameRequest::Teleport,0},&spawn);
    shooter.yaw=spawn.facing;shooter.pitch=std::atan2(aim.z,std::hypot(aim.x,aim.y));
    worker=std::jthread([this](std::stop_token stop){try {
      std::array<Controls,16> commands{};auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        host.receive(commands);commands[0]=shooter;commands[0].fire=fire.load()&&!removed.load();
        match.step(commands);
        for(const auto &event:match.take_prop_events())if(event.handle==module) {
          check(event.effect==0xe70a0595u&&event.remove,"Actual power response lost its authored effect");
          removed=true;
        }
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...){std::lock_guard lock(mutex);error=std::current_exception();}});
  }
  void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv){try {
  check(argc==4,"Supply actual MAP SHARED_MAPS UPDATE_XBE");
  if(!std::getenv("DISPLAY")||!std::filesystem::exists(argv[1]))return 77;
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions o;o.map=argv[1];o.shared_maps=argv[2];o.gameplay_xbe=argv[3];
  o.starting_weapon=5;o.width=640;o.height=480;
  Host host(o);o.join="127.0.0.1:"+std::to_string(host.host.port());
  o.desktop=std::make_shared<DesktopSession>();o.desktop->window(o.width,o.height,false);
  unsigned sounds{};bool replicated_removal{};
  o.input_observer=[&](unsigned tick,std::span<const Controls>,const MultiplayerState &s) {
    if(tick==25)host.fire=true;
    replicated_removal|=std::any_of(s.prop_removals.begin(),s.prop_removals.end(),
        [&](const auto &p){return p.handle==host.module&&p.effect==0xe70a0595u;});
    if(tick==125){SDL_Event e{};e.type=SDL_QUIT;check(SDL_PushEvent(&e)==1,"SDL quit queue failed");}
  };
  o.audio_observer=[&](const MultiplayerAudioSubmission &s) {
    if(s.kind==MultiplayerAudioKind::Impulse&&s.sound==0xe70f059au) {
      check(s.owner==host.module&&s.frames>0&&s.voice!=0,"Explosion sound lost its actual emitted object owner");
      check(s.position.has_value(),"LAN explosion sound discarded its actual emitted world position");++sounds;
    }
  };
  const auto result=run_multiplayer(o);host.finish();
  check(result==0&&host.removed&&replicated_removal,"Native shot/explosion/removal did not reach the client");
  check(sounds==1,"Actual authored LAN barrel explosion sound was absent or duplicated");
  std::cout<<"PASS actual SMG -> power-module blast/removal -> native UDP request -> exactly one authored SDL explosion sound\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
