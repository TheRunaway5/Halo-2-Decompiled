#define SDL_MAIN_HANDLED
#include "halo2/multiplayer_audio.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/muzzle_effects.hpp"
#include "halo2/desktop_session.hpp"
#include <iostream>
#include <future>
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
std::shared_ptr<const OriginalData> original(const MultiplayerOptions &options) {
  return options.gameplay_xbe.empty()?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(options.gameplay_xbe);
}
struct Host {
  Cache cache;LevelContent content;std::uint32_t starting_sound,starting_effect;unsigned port{};std::string join_address;
  std::uint32_t effect_primary{0x12345678u},effect_auxiliary{0x12345678u};
  NativeMuzzleEffects effects;
  struct SourceImpulse {std::uint32_t tick;SoundEvent event;};
  // Written only by the real authority worker; inspected only after join.
  // These are cues emitted by the authority's authored effect owner; neither
  // generic shot notifications nor ammo-derived cadence supplies expectations.
  std::vector<SourceImpulse> starting_impulses;
  std::vector<NativeWeaponEffectEvent> scheduled_effects;
  std::uint32_t scheduled_effect_serial{};
  unsigned fallback_effect_calls{};
  struct NativeSoundRequest {std::uint32_t tick,sound,flags,owner,type;};
  std::vector<NativeSoundRequest> native_sound_requests;
  struct FiringSample {std::uint32_t tick,unit,weapon,definition,shot_tick;int loaded;bool fire,reload;};
  std::vector<FiringSample> firing_samples;
  std::exception_ptr error;std::mutex mutex;std::jthread worker;
  Host(const MultiplayerOptions &o):cache(o.map,o.shared_maps,original(o)),
    content(cache.load_content(0,true)),starting_sound(content.weapons.at(cache.slayer_starting_weapon(5)).sound),
    starting_effect(content.weapons.at(cache.slayer_starting_weapon(5)).firing_effects.front().effects[0]),
    effects(cache,SourceEffectRandom{effect_primary,effect_auxiliary}){
    check(starting_sound!=null_datum,"Authored starting SMG firing sound is unavailable");
    std::promise<void> ready;auto prepared=ready.get_future();
    worker=std::jthread([this,o,&ready](std::stop_token stop){bool announced=false;try{
      // A current MCC client requires the real completed source model/prop
      // replica. Construct and service the authority on its retained executor,
      // just as the dedicated runtime does; a legacy match is insufficient.
      auto server=o;server.local_players=0;server.dedicated=true;server.mute=true;
      server.variant=UiVariant{"Slayer",0,{{2,0},{10,0},{89,5}}};server.starting_weapon=5;
      server.source_object_world=true;
      server.source_object_savegame_directory=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"lan-source-host";
      MultiplayerPreparedWorld world(server);auto &match=world.match;
      check(world.source_objects && world.source_objects->owns_physics(),"LAN audio authority lacks its original source world");
      check(bool(match.session_configuration()),"Audio authority has no admitted session configuration");
      match.configure_replicated_first_person(cache.load_first_person_binding(cache.slayer_starting_weapon(5),2));
      LanHost host(match,0,custom_slayer_fingerprint(multiplayer_fingerprint(server.map,world.cache->shared_directory(),
          0,world.network_starting_weapon_identity(),server.gameplay_xbe),*match.session_configuration()),*match.session_configuration());
      port=host.port();join_address=host.join_address();
      match.configure_weapon_effect_owner([&](const Unit &,const WeaponEffectInput &input){
      ++fallback_effect_calls;
      if(input.effect==null_datum)return input.previous_handle;
      const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto &t){return t.datum==input.effect;});
      check(tag!=cache.tags().end(),"Authority weapon effect lost its authored tag");
      if(tag->group=="effe")return effects.request(input,std::uint32_t(match.simulation().ticks()+1));
      if(tag->group=="snd!"&&input.effect==starting_sound)
        starting_impulses.push_back({std::uint32_t(match.simulation().ticks()+1),{input.effect,{}}});
      return null_datum;
      });
      world.source_objects->observe_sound_requests([&](std::uint32_t sound,std::span<const std::byte,160> raw) {
        if(native_sound_requests.size()>=64)return;
        const Bytes request(raw);
        native_sound_requests.push_back({std::uint32_t(match.simulation().ticks()+1),sound,
            request.u32(0),request.u32(0x50),std::to_integer<unsigned>(raw[0xf])&15u});
      });
      announced=true;ready.set_value();
      std::array<Controls,16> controls{};auto next=std::chrono::steady_clock::now();
      while(!stop.stop_requested()){host.receive(controls);
        const auto roster=match.state();
        if(std::any_of(roster.players.begin(),roster.players.end(),[](const auto&p){return p.connected;}))match.step(controls);
        const auto completed=match.state();const auto sound_tick=completed.tick;
        const auto &player=completed.players[0];
        if(player.connected && firing_samples.size()<64 &&
            (firing_samples.empty() || firing_samples.back().loaded!=player.loaded ||
             firing_samples.back().shot_tick!=player.shot_tick || firing_samples.back().fire!=controls[0].fire ||
             firing_samples.back().reload!=controls[0].reload || firing_samples.back().weapon!=player.source_weapon))
          firing_samples.push_back({sound_tick,player.source_unit,player.source_weapon,player.weapon,player.shot_tick,
              player.loaded,controls[0].fire,controls[0].reload});
        for(const auto &event:completed.weapon_effects)if(event.serial>scheduled_effect_serial) {
          scheduled_effect_serial=event.serial;
          if(scheduled_effects.size()<64)scheduled_effects.push_back(event);
          // Canonical native firing bypasses the software callback above.
          // Independently schedule the actual authored completed birth,
          // including births retired by their original initial update.
          if(event.source_scheduled && event.effect==starting_effect) {
            check(event.source_presentation && event.source_instance!=null_datum,
                "Authority firing event lost its actual native effect birth");
            const WeaponEffectInput input{event.effect,event.weapon,null_datum,event.a_bits,event.b_bits};
            effects.request(input,event.tick,event.colors,event.velocity,false,true);
          }
        }
        // Retire unrelated simulation notifications. The actual authored
        // effect owner supplies the independently observed firing cues.
        (void)match.take_sounds();effects.advance(sound_tick,tick_seconds);
        for(const auto &sound:effects.take_sounds())if(sound.cue.sound==starting_sound)
          starting_impulses.push_back({sound_tick,{sound.cue.sound,{},sound.cue.gain}});
        host.broadcast();
        next+=std::chrono::microseconds(33333);std::this_thread::sleep_until(next);}}
      catch(...){const auto failure=std::current_exception();{std::lock_guard lock(mutex);error=failure;}
        if(!announced)ready.set_exception(failure);}});
    prepared.get();
  }
  void finish(){worker.request_stop();worker.join();std::lock_guard lock(mutex);if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv){try{
  check(argc>=4,"Supply MAP SHARED_MAPS UPDATE_XBE|native-mcc [--split2|--split4] [--lan] [--slow-client] [--announcer] [--shaders PATH]");
  const bool native_mcc=std::string_view(argv[3])=="native-mcc";
  if(!std::getenv("DISPLAY")||!std::filesystem::exists(argv[1])||(!native_mcc&&!std::filesystem::exists(argv[3])))return 77;
  const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
  check(scratch && std::filesystem::path(scratch).is_absolute(),"Run live audio validation through tools/run_test.py");
  const std::filesystem::path test_scratch(scratch);
  for(const auto &[key,folder]:std::array<std::pair<const char*,const char*>,3>{{
      {"HALO2_PIPELINE_CACHE_DIRECTORY","vulkan-cache"},
      {"MESA_SHADER_CACHE_DIR","mesa-cache"},
      {"__GL_SHADER_DISK_CACHE_PATH","nvidia-cache"}}})
    check(SDL_setenv(key,(test_scratch/folder).string().c_str(),1)==0,"Could not isolate the live audio GPU cache");
  SDL_SetMainReady();SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT,"0x0000/0x0000");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];if(!native_mcc)options.gameplay_xbe=argv[3];
  options.width=640;options.height=480;options.starting_weapon=5;options.mute=false;bool lan=false,slow_client=false,weapons=false,announcer=false;unsigned cycles=1;
  for(int at=4;at<argc;++at){const std::string arg=argv[at];if(arg=="--split4")options.local_players=4;
    else if(arg=="--split2")options.local_players=2;
    else if(arg=="--weapons")weapons=true;
    else if(arg=="--lan")lan=true;else if(arg=="--slow-client")slow_client=true;
    else if(arg=="--announcer")announcer=true;
    else if(arg=="--shaders" && at+1<argc){
      options.capture_shader_directory=argv[++at];options.capture_tick=10000;
      options.reference_tick_cadence=true;
      options.screenshot=test_scratch/"last.ppm";
    }
    else if(arg=="--cycles" && at+1<argc){cycles=unsigned(std::stoul(argv[++at]));check(cycles>=1 && cycles<=10,"Audio cycles must be 1..10");}
    else throw FormatError("Unknown live audio option");}
  check(!slow_client||lan,"Slow-client delivery check requires a real independently ticking authority");
  if(lan){Cache admitted(options.map,options.shared_maps);if(admitted.is_mcc())options.gameplay_xbe.clear();}
  Cache cache(options.map,options.shared_maps,original(options));
  check(!native_mcc||cache.is_mcc(),"native-mcc requires the prepared imported MCC cache");
  const auto fp=cache.load_first_person_binding(cache.slayer_starting_weapon(5),2);std::set<std::uint32_t> reload_tags;
  for(const auto &clip:fp.clock_clips)if(clip.activity==0x0b000065||clip.activity==0x0c000064)
    for(const auto &e:clip.events)if(e.type==1&&e.tag!=null_datum)reload_tags.insert(e.tag);
  std::unique_ptr<Host> host;if(lan){host=std::make_unique<Host>(options);options.join=host->join_address;}
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
  std::optional<unsigned> start;unsigned step=0,ambient=0,reloads=0,impulses=0,footsteps=0,walking_commands=0;bool fired=false,requested=false;
  std::optional<std::uint32_t> client_baseline;
  unsigned client_starting_impulses=0,client_initial_impulses=0,local_effect_requests=0;
  std::uint32_t local_effect_serial{};
  std::set<std::uint64_t> starting_voices;
  std::map<std::uint32_t,NativeWeaponEffectEvent> received_effects;
  std::map<std::uint32_t,unsigned> submitted_sounds;
  std::set<std::uint32_t> source_announcer_events,consumed_announcer_events;
  std::set<std::uint32_t> source_startup_events;
  std::set<std::uint32_t> announcer_sounds;
  std::set<std::uint64_t> announcer_voices;
  unsigned selected_announcer_cues{};
  if(announcer)options.objective_audio_observer=[&](const auto &event,std::span<const std::uint32_t> sounds){
    consumed_announcer_events.insert(event.serial);selected_announcer_cues+=sounds.size();
    announcer_sounds.insert(sounds.begin(),sounds.end());
  };
  MultiplayerState state;
  options.frame_observer=[&](const auto &){focus_window();if(!start)start=step;
    if(slow_client)std::this_thread::sleep_for(std::chrono::milliseconds(85));};
  options.input_observer=[&](unsigned tick,std::span<const Controls> commands,const MultiplayerState &snapshot){
    state=snapshot;step=tick;check(tick<100||start,"SDL audio test never received focus");
    for(const auto &event:snapshot.weapon_effects)if(received_effects.size()<64)
      received_effects.try_emplace(event.serial,event);
    if(announcer)for(const auto &event:snapshot.source_engine_events) {
      source_announcer_events.insert(event.serial);
      // Actual TU1.5 Slayer vtable slot6/2C5D70 emits category2/subtype0
      // immediately at14FDC0 activation, targeted at this full player datum.
      if(event.words[0]==2 && event.words[1]==0 && event.words[2]!=null_datum &&
          std::any_of(snapshot.players.begin(),snapshot.players.end(),[&](const auto& player){
            return player.connected && player.source_player==event.words[2];}))
        source_startup_events.insert(event.serial);
    }
    const auto &player=state.players[0];const auto &effect=player.weapon_effect;
    if(!lan&&effect.serial&&effect.serial!=local_effect_serial&&effect.effect==starting_effect&&
        effect.unit==player.source_unit&&effect.weapon==player.source_weapon){
      local_effect_serial=effect.serial;++local_effect_requests;
    }
    if(lan&&!client_baseline&&std::any_of(state.players.begin(),state.players.end(),[](const auto&p){return p.connected;}))
      client_baseline=state.tick;
    const auto phase=start?(tick-*start)%180:0;
    if(start&&(phase==6||phase==20||phase==28||phase==55||phase==125))
      focus_window();
    if(start&&phase==6)fire(window_id,true);
    if(start&&phase==20)fire(window_id,false);
    if(start&&phase==28)reload(window_id);
    if(start&&!weapons&&phase==55)walk(window_id,true);
    if(start&&!weapons&&phase==125)walk(window_id,false);
    requested|=commands[0].reload;fired|=state.players[0].shot_tick&&state.players[0].loaded<60;
    walking_commands+=commands[0].forward!=0;
    if(start&&tick==*start+cycles*180-10){SDL_Event e{};e.type=SDL_QUIT;push(e);}
  };
  options.audio_observer=[&](const auto &s){check(s.frames>0&&s.voice!=0,"Real SDL voice packet lost resource/handle");
    if(submitted_sounds.size()<64 || submitted_sounds.contains(s.sound))++submitted_sounds[s.sound];
    if(announcer_sounds.contains(s.sound))announcer_voices.insert(s.voice);
    if(cache.is_mcc()) {
      const auto tag=cache.query_tag({.handle=TagHandle{s.sound},.group=TagGroup::from_string("snd!"),
          .require_address=true,.minimum_size=28});
      check(tag && !(Bytes(cache.read_tag_bytes(tag->address,28)).u16(0)&0x800u),
          "Classic runtime submitted a remastered-only MCC sound layer");
    }
    if(s.kind==MultiplayerAudioKind::Ambient)++ambient;
    if(s.kind==MultiplayerAudioKind::Impulse)++impulses;
    if(s.sound==starting_sound){
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
    std::cout<<"actual authority effect publication: fallback_calls="<<host->fallback_effect_calls
      <<" native_events="<<host->scheduled_effects.size()<<'\n';
    for(const auto &request:host->native_sound_requests)std::cout<<"native sound request tick="<<request.tick
      <<" sound="<<request.sound<<" flags="<<request.flags<<" owner="<<request.owner<<" type="<<request.type<<'\n';
    for(const auto &sample:host->firing_samples)std::cout<<"authority firing state tick="<<sample.tick
      <<" unit="<<sample.unit<<" weapon="<<sample.weapon<<" definition="<<sample.definition
      <<" shot_tick="<<sample.shot_tick<<" loaded="<<sample.loaded<<" fire="<<sample.fire<<" reload="<<sample.reload<<'\n';
    for(const auto &event:host->scheduled_effects)std::cout<<"authority effect serial="<<event.serial
      <<" tick="<<event.tick<<" definition="<<event.effect<<" unit="<<event.unit<<" weapon="<<event.weapon
      <<" instance="<<event.source_instance<<" native="<<event.source_scheduled<<'\n';
    for(const auto &[serial,event]:received_effects)std::cout<<"received effect serial="<<serial
      <<" tick="<<event.tick<<" definition="<<event.effect<<" unit="<<event.unit<<" weapon="<<event.weapon
      <<" instance="<<event.source_instance<<" native="<<event.source_scheduled<<'\n';
    for(const auto &[sound,count]:submitted_sounds)std::cout<<"actual SDL sound="<<sound<<" submissions="<<count<<'\n';
  }
  std::cout<<"actual SDL audio: views="<<options.local_players<<" nativeLAN="<<lan<<" ticks="<<state.tick
    <<" fired="<<fired<<" requested_reload="<<requested<<" reload_packets="<<reloads<<" ambient_packets="<<ambient
    <<" impulse_packets="<<impulses<<" footstep_packets="<<footsteps
    <<" walking_commands="<<walking_commands<<" walking_check="<<!weapons
    <<" source_smg_impulses="<<(lan?source_starting_impulses:local_effect_requests)<<" client_smg_submissions="<<client_starting_impulses
    <<" source_initial_smg="<<source_initial_impulses<<" client_initial_smg="<<client_initial_impulses
    <<" source_unobserved_tail_smg="<<source_unobserved_tail<<'\n';
  if(announcer) {
    std::cout<<"actual source announcer: engine="<<state.source_engine<<" events="<<source_announcer_events.size()
      <<" startup_events="<<source_startup_events.size()
      <<" consumed="<<consumed_announcer_events.size()<<" selected_cues="<<selected_announcer_cues
      <<" SDL_voices="<<announcer_voices.size()<<'\n';
    check(!source_announcer_events.empty(),"Original engine produced no announcer event in live activation scenario");
    check(state.source_engine!=2 || !source_startup_events.empty(),
      "Actual native Slayer activation never reached its targeted category2/subtype0 journal");
    check(!consumed_announcer_events.empty() && selected_announcer_cues,
      "Live runtime discarded the original engine's authored announcer events");
    check(!announcer_voices.empty(),"Original announcer responses never reached actual native SDL voice submission");
  }
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
  // The independently counted firing-sound identity above covers both native
  // effect impulses and centered source-owned first-person submissions.
  check(result==0&&fired&&requested&&reloads==cycles&&ambient>=2,"Live shot/reload/ambient SDL symptom regression failed");
  if(!weapons)check(footsteps>0,"Live walking footstep SDL symptom regression failed");
  std::cout<<"PASS actual authored shots, reload and BSP wind reached real SDL device submission; walking="
    <<(weapons?"separate":"passed")<<"; physical speakers unverified\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
