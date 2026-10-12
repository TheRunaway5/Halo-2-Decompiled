#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/desktop_session.hpp"
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void push(SDL_Event e){check(SDL_PushEvent(&e)==1,"Queue actual SDL audio input");}
void fire(Uint32 window,bool down){SDL_Event e{};e.type=down?SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
  e.button.windowID=window;e.button.button=SDL_BUTTON_LEFT;e.button.state=down?SDL_PRESSED:SDL_RELEASED;push(e);}
void reload(Uint32 window){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=window;
  e.key.keysym.scancode=SDL_SCANCODE_R;e.key.keysym.sym=SDLK_r;e.key.state=SDL_PRESSED;push(e);
  e.type=SDL_KEYUP;e.key.state=SDL_RELEASED;push(e);}
void walk(Uint32 window,bool down){SDL_Event e{};e.type=down?SDL_KEYDOWN:SDL_KEYUP;e.key.windowID=window;
  e.key.keysym.scancode=SDL_SCANCODE_W;e.key.keysym.sym=SDLK_w;e.key.state=down?SDL_PRESSED:SDL_RELEASED;push(e);}
struct Host {
  Cache cache;LevelContent content;std::uint32_t starting_sound;MultiplayerMatch match;LanHost host;
  std::uint32_t effect_primary{0x12345678u},effect_auxiliary{0x12345678u};
  NativeMuzzleEffects effects;
  struct SourceImpulse {std::uint32_t tick;SoundEvent event;};
  // Written only by the real authority worker; inspected only after join.
  // These are cues emitted by the authority's authored effect owner; neither
  // generic shot notifications nor ammo-derived cadence supplies expectations.
  std::vector<SourceImpulse> starting_impulses;
  std::exception_ptr error;std::mutex mutex;std::jthread worker;
  static SlayerRules rules(const Cache &c){SlayerRules r;r.weapon=c.slayer_starting_weapon(5);r.time_ticks=r.score_limit=0;return r;}
  Host(const MultiplayerOptions &o):cache(o.map,o.shared_maps,OriginalData::load_xbe(o.gameplay_xbe)),
    content(cache.load_content(0,true)),starting_sound(content.weapons.at(cache.slayer_starting_weapon(5)).sound),
    match(content,0,cache.spawns(),rules(cache)),host(match,0,
      multiplayer_fingerprint(o.map,cache.shared_directory(),0,cache.slayer_starting_weapon(5),o.gameplay_xbe)),effects(cache,SourceEffectRandom{effect_primary,effect_auxiliary}){
    check(starting_sound!=null_datum,"Authored starting SMG firing sound is unavailable");
    match.configure_replicated_first_person(cache.load_first_person_binding(cache.slayer_starting_weapon(5),2));
    match.configure_weapon_effect_owner([&](const Unit &,const WeaponEffectInput &input){
      if(input.effect==null_datum)return input.previous_handle;
      const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.datum==input.effect;});
      check(tag!=cache.tags().end(),"Authority weapon effect lost its authored tag");
      if(tag->group=="effe")return effects.request(input,std::uint32_t(match.simulation().ticks()+1));
      if(tag->group=="snd!"&&input.effect==starting_sound)
        starting_impulses.push_back({std::uint32_t(match.simulation().ticks()+1),{input.effect,{}}});
      return null_datum;
    });
    worker=std::jthread([this](std::stop_token stop){try{std::array<Controls,16> controls{};auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()){host.receive(controls);
        const auto roster=match.state();
        if(std::any_of(roster.players.begin(),roster.players.end(),[](const auto&p){return p.connected;}))match.step(controls);
        const auto sound_tick=match.state().tick;
        // Retire unrelated simulation notifications. The actual authored
        // effect owner supplies the independently observed firing cues.
        (void)match.take_sounds();effects.advance(sound_tick,tick_seconds);
        for(const auto &sound:effects.take_sounds())if(sound.cue.sound==starting_sound)
          starting_impulses.push_back({sound_tick,{sound.cue.sound,{},sound.cue.gain}});
        host.broadcast();
        next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);}}
      catch(...){std::lock_guard lock(mutex);error=std::current_exception();}});
  }
  void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv){try{
  check(argc>=4&&argc<=7,"Supply MAP SHARED_MAPS UPDATE_XBE [--split4] [--lan] [--slow-client]");
  if(!std::getenv("DISPLAY")||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[3]))return 77;
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];options.gameplay_xbe=argv[3];
  options.width=640;options.height=480;options.starting_weapon=5;options.mute=false;bool lan=false,slow_client=false;
  for(int at=4;at<argc;++at){const std::string arg=argv[at];if(arg=="--split4")options.local_players=4;
    else if(arg=="--lan")lan=true;else if(arg=="--slow-client")slow_client=true;
    else throw FormatError("Unknown live audio option");}
  check(!slow_client||lan,"Slow-client delivery check requires a real independently ticking authority");
  Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
  const auto fp=cache.load_first_person_binding(cache.slayer_starting_weapon(5),2);std::set<std::uint32_t> reload_tags;
  for(const auto &clip:fp.clock_clips)if(clip.activity==0x0b000065||clip.activity==0x0c000064)
    for(const auto &e:clip.events)if(e.type==1&&e.tag!=null_datum)reload_tags.insert(e.tag);
  std::unique_ptr<Host> host;if(lan){host=std::make_unique<Host>(options);options.join="127.0.0.1:"+std::to_string(host->host.port());}
  const auto [starting_sound,starting_effect]=[&]{
    const auto selected=cache.slayer_starting_weapon(5);
    const auto pick=[&](const LevelContent &content){const auto &w=content.weapons.at(selected);
      check(w.smg&& !w.firing_effects.empty(),"Starting SMG authored effect is unavailable");
      return std::pair{w.sound,w.firing_effects.front().effects[0]};};
    return host?pick(host->content):pick(cache.load_content(0,true));
  }();
  options.desktop=std::make_shared<DesktopSession>();auto *window=options.desktop->window(640,480,false);
  const auto focus_window=[&] {
    SDL_RaiseWindow(window);
    check(SDL_SetWindowInputFocus(window)==0,"Could not focus the actual audio test window");
    // X11 delivers FocusIn asynchronously after the actual focus request.
    // Wait for SDL to observe that OS event before admitting test input.
    // Keep the focus assertion: a synthetic SDL event is insufficient here.
    const auto focus_deadline=SDL_GetTicks64()+1000;
    do {SDL_PumpEvents();if(SDL_GetKeyboardFocus()==window)break;SDL_Delay(1);}
    while(SDL_GetTicks64()<focus_deadline);
    check(SDL_GetKeyboardFocus()==window,"Audio test lost actual OS keyboard focus before its input");
  };
  for(int i=0;i<SDL_NumJoysticks();++i)check(!SDL_IsGameController(i),"Physical controller contaminated isolated audio test");
  const auto window_id=SDL_GetWindowID(window);SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
  std::optional<unsigned> start;unsigned step=0,ambient=0,reloads=0,impulses=0,footsteps=0;bool fired=false,requested=false;
  std::optional<std::uint32_t> client_baseline;
  unsigned client_starting_impulses=0,client_initial_impulses=0,local_effect_requests=0;
  std::uint32_t local_effect_serial{};
  std::set<std::uint64_t> starting_voices;
  MultiplayerState state;
  options.frame_observer=[&](const auto &){focus_window();if(!start)start=step;
    if(slow_client)std::this_thread::sleep_for(std::chrono::milliseconds(85));};
  options.input_observer=[&](unsigned tick,std::span<const Controls> commands,const MultiplayerState &snapshot){
    state=snapshot;step=tick;check(tick<100||start,"SDL audio test never received focus");
    const auto &player=state.players[0];const auto &effect=player.weapon_effect;
    if(!lan&&effect.serial&&effect.serial!=local_effect_serial&&effect.effect==starting_effect&&
        effect.unit==player.source_unit&&effect.weapon==player.source_weapon){
      local_effect_serial=effect.serial;++local_effect_requests;
    }
    if(lan&&!client_baseline&&std::any_of(state.players.begin(),state.players.end(),[](const auto&p){return p.connected;}))
      client_baseline=state.tick;
    if(start&&(tick==*start+6||tick==*start+20||tick==*start+28||tick==*start+55||tick==*start+125))
      focus_window();
    if(start&&tick==*start+6)fire(window_id,true);
    if(start&&tick==*start+20)fire(window_id,false);
    if(start&&tick==*start+28)reload(window_id);
    if(start&&tick==*start+55)walk(window_id,true);
    if(start&&tick==*start+125)walk(window_id,false);
    requested|=commands[0].reload;fired|=state.players[0].shot_tick&&state.players[0].loaded<60;
    if(start&&tick==*start+170){SDL_Event e{};e.type=SDL_QUIT;push(e);}
  };
  options.audio_observer=[&](const auto &s){check(s.frames>0&&s.voice!=0,"Real SDL voice packet lost resource/handle");
    if(s.kind==MultiplayerAudioKind::Ambient)++ambient;
    if(s.kind==MultiplayerAudioKind::Impulse)++impulses;
    if(s.kind==MultiplayerAudioKind::Impulse&&s.sound==starting_sound){
      check(starting_voices.insert(s.voice).second,"Starting SMG impulse submitted twice on one SDL voice");
      if(!lan||client_baseline)++client_starting_impulses;else ++client_initial_impulses;
    }
    if(s.kind==MultiplayerAudioKind::Footstep)++footsteps;
    if(s.kind==MultiplayerAudioKind::FirstPerson&&reload_tags.contains(s.sound)){
      check(s.view==0&&s.owner!=null_datum,"Live reload sound escaped its actual local weapon");++reloads;}
  };
  const auto result=run_multiplayer(options);if(host)host->finish();
  unsigned source_starting_impulses=0,source_initial_impulses=0,source_unobserved_tail=0;
  if(host){
    check(bool(client_baseline),"LAN audio test never observed an authoritative roster");
    for(const auto &sound:host->starting_impulses)
      if(sound.tick<=*client_baseline)++source_initial_impulses;
      else if(sound.tick<=state.tick)++source_starting_impulses;
      else ++source_unobserved_tail;
    std::cout<<"actual authority authored SMG cues: sound="<<host->starting_sound
      <<" baseline_tick="<<*client_baseline<<" final_observed_tick="<<state.tick<<" ticks=";
    for(const auto &sound:host->starting_impulses)std::cout<<sound.tick<<',';
    std::cout<<'\n';
  }
  std::cout<<"actual SDL audio: views="<<options.local_players<<" nativeLAN="<<lan<<" ticks="<<state.tick
    <<" fired="<<fired<<" requested_reload="<<requested<<" reload_packets="<<reloads<<" ambient_packets="<<ambient
    <<" impulse_packets="<<impulses<<" footstep_packets="<<footsteps
    <<" source_smg_impulses="<<(lan?source_starting_impulses:local_effect_requests)<<" client_smg_submissions="<<client_starting_impulses
    <<" source_initial_smg="<<source_initial_impulses<<" client_initial_smg="<<client_initial_impulses
    <<" source_unobserved_tail_smg="<<source_unobserved_tail<<'\n';
  if(host){
    // The unchanged SDL scenario fires only after joining, then stops well
    // before quitting. Do not silently count pre-join or unseen tail events as
    // a client obligation, or let either boundary hide this regression.
    check(source_initial_impulses==0&&client_initial_impulses==0&&source_unobserved_tail==0,
      "SMG delivery comparison crossed its explicit join/final-snapshot baseline");
    check(source_starting_impulses>0&&client_starting_impulses==source_starting_impulses,
      "LAN client firing sound count differs from actual authority effect cues");
  }
  if(!lan)check(local_effect_requests>0&&client_starting_impulses==local_effect_requests,
      "Local firing sound count differs from actual authority weapon effects");
  check(result==0&&fired&&requested&&reloads==1&&ambient>=2&&impulses>0&&footsteps>0,"Live reload/ambient/footstep SDL symptom regression failed");
  std::cout<<"PASS actual authored reload, BSP wind and walking footsteps reached real SDL device submission; physical speakers unverified\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
