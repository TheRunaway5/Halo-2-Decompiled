#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/source_effect_location_admission.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <future>
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
  unsigned port{},owner{};std::uint32_t sound{};
  bool observed_location_split{}; // Worker-owned until finish() joins it.
  struct Impulse {std::uint32_t tick;};
  std::vector<Impulse> impulses; // Worker-owned until finish() joins it.
  std::atomic<bool> postjoin_requested{};
  std::atomic<std::uint32_t> postjoin_finished{};
  std::mutex mutex;std::condition_variable ready;
  bool prejoin_ready{};std::exception_ptr error;std::jthread worker;
  Host(const MultiplayerOptions &o) {
    MultiplayerOptions server;server.map=o.map;server.shared_maps=o.shared_maps;server.gameplay_xbe=o.gameplay_xbe;
    server.starting_weapon=o.starting_weapon;server.local_players=1;server.source_object_world=true;
    std::promise<std::array<unsigned,3>> loaded;auto future=loaded.get_future();
    worker=std::jthread([this,server,loaded=std::move(loaded)](std::stop_token stop)mutable {bool published=false;try {
      MultiplayerPreparedWorld world(server);world.bind_source_local_views(world.local_slots);
      auto &match=world.match;const auto &cache=*world.cache;const auto local=world.local_slots.front();
      const auto firing_sound=world.content.weapons.at(world.rules.weapon).sound;
      check(firing_sound!=null_datum,"Authored starting SMG sound is unavailable");
      match.configure_replicated_first_person(cache.load_first_person_binding(world.rules.weapon,2));
      LanHost host(match,0,multiplayer_fingerprint(server.map,cache.shared_directory(),0,world.network_starting_weapon_identity(),server.gameplay_xbe));
      std::uint32_t effect_primary=0x12345678u,effect_auxiliary=0x12345678u;
      NativeMuzzleEffects effects(cache,SourceEffectRandom{effect_primary,effect_auxiliary});
      std::uint64_t effect_serial{};
      loaded.set_value({host.port(),local,firing_sound});published=true;
      std::array<Controls,multiplayer_capacity> controls{};
      std::optional<std::uint32_t> prejoin_last,postjoin_start,release_tick;
      auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()) {
        host.receive(controls);const auto before=match.state().tick;
        if(!prejoin_last)controls[local].fire=true;
        else {
          controls[local].fire=false;
          if(postjoin_requested.load(std::memory_order_acquire)) {
            if(!postjoin_start)postjoin_start=before;
            controls[local].fire=before-*postjoin_start<18;
            if(!controls[local].fire&&!release_tick)release_tick=before;
          }
        }
        match.step(controls);const auto state=match.state();const auto after=state.tick;
        // The original scheduler's completed effect journal supplies the
        // independent authority cues; generic sound/shot notifications do not.
        for(const auto &event:state.weapon_effects)if(event.serial>effect_serial) {
          effect_serial=event.serial;
          if(event.unit!=state.players[local].source_unit || event.source_instance==null_datum)continue;
          if(!observed_location_split) {
            const auto &memory=world.source_objects->memory();
            const auto word=[&](auto at){return Bytes(memory.view(at,4)).u32(0);};
            const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return memory.view(at,n);}};
            const auto body=source_raw_datum_lookup(word(0x4f720c),event.source_instance,storage);
            check(body && word(body+4)==event.effect,"Authority lost its original effect allocation");
            const auto metadata=cache.load_source_tag_metadata(event.effect);
            const auto count=Bytes(metadata.bytes_at(metadata.source_address(),0x14)).u32(0xc);
            check(count<=32,"Authority effect has an invalid location array");
            unsigned world_nodes{},fp_nodes{};std::set<std::uint32_t> visited;
            for(unsigned n=0;n<count;++n)for(auto id=word(body+0x7c+n*4);id!=null_datum;) {
              check(visited.insert(id).second && visited.size()<=512,"Authority source location chain is invalid");
              const auto location=source_raw_datum_lookup(word(0x4f7208),id,storage);
              check(bool(location),"Authority source location lost its full generation");
              const auto node=Bytes(memory.view(location+2,2)).u16(0);
              if(source_effect_world_location_admitted_tu15(node))++world_nodes;else ++fp_nodes;
              id=word(location+4);
            }
            check(world_nodes && fp_nodes && event.source_locations.size()==world_nodes,
                "WORLD projection retained a first-person location or fixture lacks the original split");
            observed_location_split=true;
            std::cout<<"Original TU1.5 effect location split: WORLD="<<world_nodes<<" FP="<<fp_nodes
                <<" published_WORLD="<<event.source_locations.size()<<'\n';
          }
          const WeaponEffectInput input{event.effect,event.weapon,null_datum,event.a_bits,event.b_bits};
          effects.request(input,event.tick,event.colors,event.velocity,false,true);
        }
        (void)match.take_sounds();effects.advance(after,tick_seconds);
        for(const auto &event:effects.take_sounds())if(event.cue.sound==firing_sound) {
          impulses.push_back({after});if(!postjoin_start)prejoin_last=after;
        }
        if(prejoin_last&&!postjoin_start&&after-*prejoin_last>=6) {
          std::lock_guard lock(mutex);prejoin_ready=true;ready.notify_all();
        }
        if(release_tick&&after-*release_tick>=10&&!postjoin_finished.load(std::memory_order_relaxed))
          postjoin_finished.store(after,std::memory_order_release);
        host.broadcast();next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);
      }
    }catch(...) {if(!published)loaded.set_exception(std::current_exception());
      std::lock_guard lock(mutex);error=std::current_exception();ready.notify_all();}});
    const auto values=future.get();port=values[0];owner=values[1];sound=values[2];
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
  options.join="127.0.0.1:"+std::to_string(host.port);
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
      check(!snapshot.weapon_effects.empty(),"Join baseline did not retain the original completed effect journal");
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
  check(result==0&&host.observed_location_split&&prejoin_source>0&&postjoin_source>0&&unobserved_tail==0,
      "Actual late-join source window/location admission was not established");
  check(initial_submissions==0,"LAN late join replayed an already completed remote firing sound");
  check(postjoin_submissions==postjoin_source,"LAN late join lost or duplicated real postjoin firing sounds");
  std::cout<<"PASS actual prejoin effect journal baselined; all observed postjoin source impulses submitted once to SDL; physical speakers unverified\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
