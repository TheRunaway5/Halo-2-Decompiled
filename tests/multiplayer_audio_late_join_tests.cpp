#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/desktop_session.hpp"
#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Host {
  Cache cache;
  LevelContent content;
  std::uint32_t sound;
  MultiplayerMatch match;
  unsigned owner;
  LanHost host;
  struct Impulse {std::uint32_t tick;};
  std::vector<Impulse> impulses; // Worker-owned until finish() joins it.
  std::atomic<bool> postjoin_requested{};
  std::atomic<std::uint32_t> postjoin_finished{};
  std::mutex mutex;
  std::condition_variable ready;
  bool prejoin_ready{};
  std::exception_ptr error;
  std::jthread worker;
  static SlayerRules rules(const Cache &c){SlayerRules r;r.weapon=c.slayer_starting_weapon(5);r.time_ticks=r.score_limit=0;return r;}
  Host(const MultiplayerOptions &o):cache(o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe)),
    content(cache.load_content(0,true)),sound(content.weapons.at(cache.slayer_starting_weapon(5)).sound),
    match(content,0,cache.spawns(),rules(cache)),owner(match.join("Existing host player")),
    host(match,0,multiplayer_fingerprint(o.map,cache.shared_directory(),0,cache.slayer_starting_weapon(5),o.gameplay_xbe)) {
    check(sound!=null_datum,"Authored starting SMG sound is unavailable");
    match.configure_replicated_first_person(cache.load_first_person_binding(cache.slayer_starting_weapon(5),2));
    worker=std::jthread([this](std::stop_token stop){try {
      std::array<Controls,multiplayer_capacity> controls{};
      std::optional<std::uint32_t> prejoin_last,postjoin_start,release_tick;
      auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        host.receive(controls);
        const auto before=match.state().tick;
        if(!prejoin_last)controls[owner].fire=true;
        else {
          controls[owner].fire=false;
          if(postjoin_requested.load(std::memory_order_acquire)) {
            if(!postjoin_start)postjoin_start=before;
            // This is an ordinary held command window, not an expected shot cadence.
            controls[owner].fire=before-*postjoin_start<18;
            if(!controls[owner].fire&&!release_tick)release_tick=before;
          }
        }
        match.step(controls);
        const auto after=match.state().tick;
        for(const auto &event:match.take_sounds())
          if(event.action==SoundEvent::Impulse&&event.datum==sound) {
            impulses.push_back({after});
            if(!postjoin_start)prejoin_last=after;
          }
        if(prejoin_last&&!postjoin_start&&after-*prejoin_last>=6) {
          std::lock_guard lock(mutex);prejoin_ready=true;ready.notify_all();
        }
        if(release_tick&&after-*release_tick>=10&&!postjoin_finished.load(std::memory_order_relaxed))
          postjoin_finished.store(after,std::memory_order_release);
        host.broadcast();
        next+=std::chrono::microseconds(33333);
        std::this_thread::sleep_until(next);
      }
    } catch(...) {std::lock_guard lock(mutex);error=std::current_exception();ready.notify_all();}});
  }
  void wait_prejoin() {
    std::unique_lock lock(mutex);
    check(ready.wait_for(lock,std::chrono::seconds(10),[&]{return prejoin_ready||bool(error);}),
        "Existing host never emitted its real prejoin shot");
    if(error)std::rethrow_exception(error);
  }
  void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv){try {
  check(argc==4,"Supply MAP SHARED_MAPS UPDATE_XBE");
  if(!std::getenv("DISPLAY")||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.width=640;options.height=480;options.starting_weapon=5;options.mute=false;
  Host host(options);host.wait_prejoin();
  options.join="127.0.0.1:"+std::to_string(host.host.port());
  options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(640,480,false);
  SDL_RaiseWindow(window);check(SDL_SetWindowInputFocus(window)==0,"Could not focus late-join test window");
  SDL_PumpEvents();check(SDL_GetKeyboardFocus()==window,"Late-join window lacks actual OS focus");
  for(int i=0;i<SDL_NumJoysticks();++i)check(!SDL_IsGameController(i),"Physical controller contaminated late-join test");
  SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  std::optional<std::uint32_t> baseline;
  MultiplayerState observed;
  unsigned initial_submissions=0,postjoin_submissions=0;
  std::set<std::uint64_t> voices;
  bool quit_queued=false;
  options.input_observer=[&](unsigned tick,std::span<const Controls>,const MultiplayerState &snapshot) {
    check(tick<600,"Late-join audio scenario timed out");
    observed=snapshot;
    if(snapshot.players[host.owner].connected&&!baseline) {
      baseline=snapshot.tick;
      check(snapshot.players[host.owner].shot_tick!=0,"Join baseline did not retain the actual earlier shot stamp");
    }
    // Hold the host neutral through several observed authority ticks. Stale
    // audio must already have been submitted before postjoin commands begin.
    if(baseline&&snapshot.tick-*baseline>=3)
      host.postjoin_requested.store(true,std::memory_order_release);
    const auto finished=host.postjoin_finished.load(std::memory_order_acquire);
    if(finished&&snapshot.tick>=finished&&!quit_queued) {
      SDL_Event event{};event.type=SDL_QUIT;
      check(SDL_PushEvent(&event)==1,"Could not queue late-join test exit");quit_queued=true;
    }
  };
  options.audio_observer=[&](const MultiplayerAudioSubmission &submission) {
    if(submission.kind!=MultiplayerAudioKind::Impulse||submission.sound!=host.sound)return;
    check(submission.frames>0&&submission.voice!=0,"Late-join sound lost actual SDL voice/resource");
    check(voices.insert(submission.voice).second,"A firing submission reused an already observed SDL voice");
    if(host.postjoin_requested.load(std::memory_order_acquire))++postjoin_submissions;
    else ++initial_submissions;
  };
  const auto result=run_multiplayer(options);host.finish();
  check(bool(baseline)&&quit_queued,"Late-join scenario did not observe its complete real authority window");
  unsigned prejoin_source=0,postjoin_source=0,unobserved_tail=0;
  std::cout<<"actual late-join authority impulse ticks=";
  for(const auto &event:host.impulses) {
    std::cout<<event.tick<<',';
    if(event.tick<=*baseline)++prejoin_source;
    else if(event.tick<=observed.tick)++postjoin_source;
    else ++unobserved_tail;
  }
  std::cout<<"\nlate-join audio: baseline="<<*baseline<<" final="<<observed.tick
      <<" source_prejoin="<<prejoin_source<<" initial_submissions="<<initial_submissions
      <<" source_postjoin="<<postjoin_source<<" postjoin_submissions="<<postjoin_submissions
      <<" unobserved_tail="<<unobserved_tail<<'\n';
  check(result==0&&prejoin_source>0&&postjoin_source>0&&unobserved_tail==0,"Actual late-join source window was not established");
  check(initial_submissions==0,"LAN late join replayed an already completed remote firing sound");
  check(postjoin_submissions==postjoin_source,"LAN late join lost or duplicated real postjoin firing sounds");
  std::cout<<"PASS actual prejoin shot stamp baselined; all observed postjoin source impulses submitted once to SDL; physical speakers unverified\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
