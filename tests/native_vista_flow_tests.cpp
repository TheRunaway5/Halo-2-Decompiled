#include "halo2/native_vista_flow.hpp"
#include "halo2/full_scoreboard_painter.hpp"
#include "halo2/frontend_lobby.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/native_vista_loading_audio.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/original_data.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
void choose(MenuState& state,unsigned row,unsigned screen) {
  state.select(row);state.accept();check(state.frame().screen==screen,"Actual native menu route changed");
}
}
int main(int argc,char** argv)try {
  const bool two_players=argc>1 && std::string_view(argv[argc-1])=="--two-players";
  if(two_players)--argc;
  const unsigned player_count=two_players?2:1;
  const bool visible=std::getenv("H2_TEST_VISIBLE")!=nullptr;
  std::array<std::int32_t,2> inputs{100,101};
  std::array<SDL_Joystick*,2> virtual_inputs{};
  if(two_players && argc==5) {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,"SDL controller initialization failed");
    for(unsigned i=0;i<2;++i) {
      const auto index=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,6,SDL_CONTROLLER_BUTTON_MAX,0);
      check(index>=0 && SDL_IsGameController(index),"Actual virtual SDL controller did not attach");
      virtual_inputs[i]=SDL_JoystickOpen(index);check(virtual_inputs[i],"Virtual input device did not open");
      inputs[i]=SDL_JoystickInstanceID(virtual_inputs[i]);
    }
    check(SDL_JoystickSetVirtualAxis(virtual_inputs[0],SDL_CONTROLLER_AXIS_LEFTY,-24576)==0 &&
        SDL_JoystickSetVirtualAxis(virtual_inputs[1],SDL_CONTROLLER_AXIS_LEFTX,-16384)==0,
        "Independent virtual input values were rejected");
    SDL_JoystickUpdate();
  }
  if((argc!=4 && argc!=5) || !std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"mainmenu.map") ||
      !std::filesystem::is_regular_file(argv[2]) || !std::filesystem::is_regular_file(argv[3]))return 77;
  const std::filesystem::path maps=argv[1];
  FrontendSession frontend;
  {Cache menu(maps/"mainmenu.map");frontend.retain_content(menu.load_frontend());}
  frontend.content().vista=load_vista_settings(argv[2]);
  auto& state=frontend.create_state();
  if(two_players)check(state.controller_slot(inputs[0],true)==0,"Host controller binding missing");
  state.accept();state.accept();state.text("Roundtrip Pilot");state.commit_text();state.back();
  check(state.frame().screen==6,"Actual selected profile did not reach main menu");
  const auto initial_mainmenu=state.frame();
  choose(state,2,30);
  if(two_players) {
    check(state.controller_slot(inputs[0],false)==0,"Host controller binding changed");
    check(state.controller_slot(inputs[1],true)==1 && state.frame().screen==10 &&
        state.joining_controller()==1,"Second controller did not open its actual profile picker");
    // Original profile-create/appearance return, with separate input ownership.
    state.accept();state.text("Second Pilot");state.commit_text();state.back();
    check(state.frame().screen==30 && !state.joining_controller() &&
        state.signed_in_profile()=="Roundtrip Pilot" && state.local_players().size()==2 &&
        state.local_players()[1].name=="Second Pilot","Second profile replaced the host or failed to join");
    // Cancel an additional controller's picker without removing existing users.
    check(state.controller_slot(102,true)==2 && state.frame().screen==10,"Third picker did not open");
    state.back();check(state.frame().screen==30 && state.local_players().size()==2 &&
        !state.joining_controller(),"Profile cancellation changed the retained two-player roster");
  }
  choose(state,0,14);
  check(state.lobby_view()->input.player_count==int(player_count),"Actual lobby lost its local roster");
  const auto name=state.signed_in_profile();
  MultiplayerOptions defaults;defaults.gameplay_xbe=argv[3];
  defaults.mute=std::getenv("H2_TEST_DAMAGE_AUDIO")==nullptr;
  if(argc==5) {
    defaults.desktop=std::make_shared<DesktopSession>();
    const std::filesystem::path loading=std::getenv("H2_TEST_LOADING_ASSETS")?
        std::getenv("H2_TEST_LOADING_ASSETS"):"assets/menu-ui/vista/loading";
    defaults.desktop->configure_loading(loading);
    // Exercise the application's actual loading-audio adapter lifetime. This
    // loading-tone capture stays muted; H2_TEST_DAMAGE_AUDIO enables world
    // audio so the absent-unit HUD interval is exercised after the fade.
    auto loading_audio=std::make_shared<NativeVistaLoadingAudio>(
        loading/"loading-tone.xbox-adpcm");
    defaults.desktop->set_loading_audio_services({
        [loading_audio]{loading_audio->begin(true);},
        [loading_audio](std::uint32_t gain){loading_audio->gain(gain);},
        [loading_audio]{loading_audio->end();}});
    FrontendOptions capture;capture.maps=maps;capture.vista_settings=argv[2];
    capture.session=std::shared_ptr<FrontendSession>(&frontend,[](FrontendSession*){});
    capture.desktop=defaults.desktop;capture.entry=FrontendOptions::Entry::fresh_lobby;
    capture.hidden=!visible;capture.mute=true;capture.skip_intro=true;capture.validation=true;
    capture.width=1280;capture.height=720;capture.frames=8;
    capture.screenshot=std::filesystem::path(argv[4])/"native-initial-lobby.ppm";
    check(!run_frontend(capture),"Actual initial lobby did not present before loading");
  }
  unsigned resource_begins{};
  NativeVistaOfflineFlow* active_flow{};
  NativeVistaOfflineFlow flow(frontend,maps,defaults,[&] {
    check(active_flow && !active_flow->launch_ready(),
        "Loading presentation started after native world readiness publication");
    bool absent{};
    try{(void)active_flow->prepared_world();}catch(const FormatError&){absent=true;}
    check(absent,"Loading presentation started after native world construction");
    ++resource_begins;
    if(argc==5) {
      defaults.desktop->begin_loading();
      defaults.desktop->capture_loading(std::filesystem::path(argv[4])/
          ("native-loading-world-"+std::to_string(resource_begins)+".ppm"));
    }
  });
  active_flow=&flow;
  const auto silent_stage=[](FrontendUiSound){throw FormatError("Early countdown unexpectedly emitted a sound");};
  state.select(0);state.accept();flow.update_frontend(state,0,silent_stage);
  check(flow.countdown()==10,"Initial countdown did not publish source stage10");
  const auto initial_countdown_kind=flow.lifecycle_kind();
  const auto countdown_lobby=state.lobby_view();
  check(countdown_lobby && countdown_lobby->input.countdown &&
      frontend_lobby_presentation(countdown_lobby->input).status_string==0x200001f0 &&
      state.expand(frontend.content().localized(frontend.content().screen(14),0x200001f0))==
          "Game start countdown in progress  10\r\nPress \xee\x84\x82 to cancel",
      "Actual solo countdown shows a party requirement instead of its original countdown status");
  state.accept();
  check(flow.countdown()==9 && !state.launch,"Second Start did not quick-advance the retained countdown");
  state.back();
  check(flow.countdown()==-1 && flow.lifecycle_kind()==1 && state.frame().screen==14 &&
      !state.launch && !flow.launch_ready(),"Back did not cancel the countdown before ordinary Leave");
  state.accept();flow.update_frontend(state,0,silent_stage);
  state.select(1);state.accept();
  check(state.frame().screen==23 && flow.countdown()==-1 && flow.lifecycle_kind()==1 &&
      !state.launch && !flow.launch_ready() && !resource_begins,
      "Game Setup left a stale source launch or loading presentation request");
  state.back();check(state.frame().screen==14,"Game Setup did not return to its actual lobby");
  std::cout<<"PASS actual lobby quick-advance/Back cancellation/Setup cancellation without stale launch\n";
  std::optional<FrontendLaunch> first;
  for(unsigned round=0;round<2;++round) {
    const auto lobby_instance=state.frame().instance;
    state.select(0);state.accept();check(bool(state.launch),"Actual Start did not produce its request");
    check(NativeVistaOfflineFlow::supports(*state.launch),"Authored default Slayer did not enter admitted live flow");
    check(state.launch->local_players==player_count && state.launch->selected_profiles.size()==player_count,
        "Actual Start dropped a selected local player");
    if(two_players)check(state.launch->input_instances==std::vector<std::int32_t>{inputs[0],inputs[1]},
        "Actual Start swapped the two controller bindings");
    if(!first)first=*state.launch;
    else check(state.launch->map==first->map && state.launch->variant.values==first->variant.values &&
        state.launch->selected_profiles==first->selected_profiles,"Second Start lost the selected configuration");
    unsigned beeps{};
    const auto sound=[&](FrontendUiSound slot) {
      check(slot==FrontendUiSound::PregameCountdown,"Actual countdown emitted an unexpected sound slot");++beeps;
    };
    const auto start=std::chrono::steady_clock::now();
    do {
      const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
      check(elapsed<16000,"Real source countdown did not reach its actual native world request");
      flow.update_frontend(state,std::uint32_t(elapsed),sound);
      if(!flow.launch_ready())std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }while(!flow.launch_ready());
    check(beeps==4 && flow.lifecycle_kind()==3 && !state.launch,"Source countdown did not emit exactly its four final-stage beeps");
    check(resource_begins==round+1,"Each real native map load must start presentation exactly once");
    const auto request=flow.take_launch();
    auto options=defaults;options.map=request.map;options.local_players=request.local_players;
    options.variant=request.variant;options.selected_profiles=request.selected_profiles;options.selected_teams=request.selected_teams;
    options.input_instances=request.input_instances;
    options.prepared_world=flow.prepared_world();options.session_random_seed=options.prepared_world->rules.random_seed;
    options.ticks=options.prepared_world->rules.time_ticks+600;
    std::weak_ptr<MultiplayerPreparedWorld> world=options.prepared_world;
    const MultiplayerMatch* initialized=&options.prepared_world->match;
    auto& prepared=*options.prepared_world;
    const auto local=prepared.local_slots.front();
    const auto representation=multiplayer_representation(prepared.match.player(local).source_configuration);
    std::vector<FirstPersonBindingDefinition> local_definitions;
    for(const auto& [datum,weapon]:prepared.content.weapons)
      if(!weapon.first_person_models.empty() && weapon.first_person_models.front().model!=null_datum &&
          weapon.first_person_models.front().animation!=null_datum)
        local_definitions.push_back(prepared.cache->load_first_person_binding(datum,representation));
    prepared.match.configure_local_views(prepared.local_slots,local_definitions);
    check(prepared.local_slots.size()==player_count,"Initialized world dropped a local slot");
    for(unsigned view=0;view<player_count;++view) {
      const auto slot=prepared.local_slots[view];const auto& absent_view=prepared.match.first_person_view(view);
      check(prepared.match.player(slot).source_flags==8 &&
          prepared.match.player(slot).source_current_object==null_datum && flow.player_activation_count()==0 &&
          absent_view.unit==null_datum && absent_view.representation==-1 &&
          absent_view.hands[0].weapon==null_datum && absent_view.hands[1].weapon==null_datum,
          "Local observer creation activated or bound the genuine absent-unit birth");
    }
    bool first_view_bound{};
    const auto observe_initial_view=[&](const MultiplayerMatch& actual) {
      if(first_view_bound)return;
      check(actual.state().tick==1 && flow.player_activation_count()==player_count,
          "First genuine source tick did not activate every local player exactly once");
      for(unsigned view_index=0;view_index<player_count;++view_index) {
        const auto slot=prepared.local_slots[view_index];
        const auto& player=actual.player(slot);const auto& unit=actual.unit(slot);
        const auto& view=actual.first_person_view(view_index);
        check(player.source_current_object==unit.source_datum && player.source_current_object!=null_datum &&
            view.unit==unit.source_datum && view.representation==int(representation) &&
            view.hands[0].weapon==unit.weapon.source_datum && view.hands[0].weapon!=null_datum,
            "First genuine source tick failed to bind a local unit and actual weapon");
      }
      first_view_bound=true;
    };
    unsigned loaded{},updates{};
    options.session_world_loaded=[&](const auto& match,auto slots) {
      check(&match==initialized && slots.size()==player_count && match.player(slots[0]).source_flags==8 &&
          match.player(slots[0]).source_current_object==null_datum && flow.player_activation_count()==0,
          "Runtime rebuilt the match or lost the unactivated real player birth");
      ++loaded;flow.world_loaded(match,slots);
    };
    options.session_world_update=[&](const auto& match) {
      ++updates;const auto close=flow.world_update(match);observe_initial_view(match);return close;
    };
    options.session_audio_update=[&](float volume,const auto& gains){flow.world_audio_update(volume,gains);};
    MultiplayerRuntimeOutcome outcome;
    if(argc==5) {
      const auto output=std::filesystem::path(argv[4]);std::filesystem::create_directories(output);
      auto rendered=options;rendered.ticks=0;rendered.frames=round?0:8;
      rendered.width=1280;rendered.height=720;rendered.hidden=!visible;rendered.validation=true;
      rendered.screenshot=output/(round?"native-world-darkening-2.ppm":"native-world-start-1.ppm");
      unsigned black_draws{};
      unsigned partial_draws{};
      bool accelerated{};
      bool checked_scoreboard{};
      std::optional<std::uint32_t> captured_black;
      rendered.session_view_black=[&](unsigned view) {
        const auto packed=flow.world_view_black(view);
        if(packed)++black_draws;
        if(packed && (*packed>>24)>0 && (*packed>>24)<255)++partial_draws;
        if(round && options.prepared_world->match.state().tick==rendered.capture_tick)
          captured_black=packed;
        return packed;
      };
      bool independent_input{};
      std::array<bool,4> presented_views{};
      rendered.view_observer=[&](unsigned view,const auto&,auto,auto){presented_views.at(view)=true;};
      if(two_players && !round) {
        rendered.frames=0;rendered.capture_tick=90;
        rendered.input_observer=[&](auto tick,auto controls,const auto&) {
          if(tick>1 && tick<60) {
            check(controls.size()==2 && controls[0].forward>.7f && controls[0].right==0 &&
                controls[1].forward==0 && controls[1].right<-.45f,
                "Two actual SDL controllers crossed local player input bindings");
            independent_input=true;
          }
        };
      }
      if(round) {
        const auto original=prepared.cache->original_data();
        const bool source_scoreboard=original && original->full_scoreboard_constants &&
            original->full_scoreboard_name_mappings && original->full_scoreboard_palette_fallback &&
            original->full_scoreboard_paint_constants;
        rendered.capture_tick=options.prepared_world->rules.time_ticks+135;
        rendered.input_observer=[](auto,auto,const auto&){};
        rendered.scoreboard_observer=[&,source_scoreboard](unsigned view,bool normal_hud,const auto& layout,const auto& paint) {
          if(options.prepared_world->match.state().tick==rendered.capture_tick) {
            check(view<player_count,"Scoreboard callback has no selected local view");
            if(source_scoreboard)
              check(!normal_hud && layout.status==FullScoreboardStatus::Ready && !paint.draw.vertices.empty(),
                  "Actual game-over scoreboard did not remain above the world fade");
            else
              check(normal_hud && paint.draw.vertices.empty(),
                  "Game-over scoreboard invented missing original executable operands");
            checked_scoreboard=true;
          }
        };
        if(!source_scoreboard)
          std::cout<<"LIMIT selected executable has no verified full-scoreboard operands; checking its absent-owner path\n";
        rendered.session_world_update=[&](const auto& actual) {
          ++updates;
          const auto close=flow.world_update(actual);observe_initial_view(actual);
          if(close)return true;
          if(accelerated || actual.state().tick<60)return false;
          check(!flow.world_view_black(0),"Source opening fade did not clear before diagnostic acceleration");
          accelerated=true;
          // Diagnostic CPU acceleration within this same initialized GPU
          // session. Every actual Match/lifecycle tick still executes; no
          // source phase, opacity, timer or report is substituted. Intermediate
          // GPU/audio frames are deliberately absent, so this is no AV test.
          std::array<Controls,multiplayer_capacity> controls{};
          for(const auto slot:options.prepared_world->local_slots)
            controls[slot].yaw=actual.unit(slot).yaw;
          auto& match=options.prepared_world->match;
          // Resume ordinary presentation 45 ticks before capture, allowing
          // the actual scoreboard's one-second inactivity delay and held
          // counter to settle. CPU acceleration does not update that owner.
          while(match.state().tick<options.prepared_world->rules.time_ticks+90) {
            match.step(controls);
            check(!flow.world_update(match),"Diagnostic acceleration passed the actual world-close request");
          }
          return false;
        };
      }
      check(run_multiplayer(rendered,outcome)==0 && black_draws && loaded==1 &&
          outcome.reason==((round || two_players)?MultiplayerExitReason::captured_tick:MultiplayerExitReason::frame_limit) &&
          (!round || (checked_scoreboard && partial_draws && captured_black && (*captured_black>>24)>0 &&
              (*captured_black>>24)<255)),"Actual viewport did not render its source fade");
      for(unsigned view=0;view<player_count;++view)check(presented_views[view],"GPU runtime omitted a selected local view");
      check(!two_players || round || independent_input,"GPU round missed independent two-controller input");
      // The GPU runtime's borrowed effect/audio consumers have ended. CPU
      // continuation has no muzzle consumer and an explicit silent animation
      // event consumer, so it cannot call destroyed rendering/audio resources.
      options.prepared_world->match.configure_weapon_effect_owner({});
      options.prepared_world->match.configure_first_person_event_owner(
          [](auto,auto,const auto&,const auto&){});
      options.session_world_loaded={};
      options.ticks=options.prepared_world->rules.time_ticks+600-outcome.state->tick;
    }
    check(run_multiplayer(options,outcome)==0 && loaded==1 && updates>1 && outcome.state &&
        outcome.authoritative_banks && outcome.reason==MultiplayerExitReason::session_world_close &&
        outcome.state->results_finalized && outcome.state->game_over_complete() && flow.lifecycle_kind()==4 &&
        flow.has_report() && flow.player_activation_count()==player_count && first_view_bound,
        "Actual normal expiry did not reach its source-owned postgame handoff with one initial activation");
    if(argc==5) {
      // CPU acceleration ends at the same real world-close result. Explicitly
      // invoke the production resource boundary omitted by the headless branch.
      defaults.desktop->begin_loading();
      defaults.desktop->capture_loading(std::filesystem::path(argv[4])/
          ("native-loading-menu-"+std::to_string(round+1)+".ppm"));
    }
    options.prepared_world.reset();flow.world_unloaded();
    check(world.expired(),"World teardown retained the actual native match");
    if(argc==5) {
      // Optional actual application resource/render boundary. The normal CTest
      // remains CPU-only; neither path asserts original framebuffer equality.
      FrontendOptions capture;
      capture.maps=maps;capture.vista_settings=argv[2];
      capture.session=std::shared_ptr<FrontendSession>(&frontend,[](FrontendSession*){});
      capture.entry=FrontendOptions::Entry::session_return;
      capture.session_mainmenu_ready=[&](MenuState&){flow.mainmenu_loaded();};
      capture.hidden=!visible;capture.mute=true;capture.skip_intro=true;capture.validation=true;
      capture.width=1280;capture.height=720;capture.frames=8;
      const auto output=std::filesystem::path(argv[4]);
      std::filesystem::create_directories(output);
      capture.screenshot=output/("native-completed-round-"+std::to_string(round+1)+".ppm");
      bool waited{};
      capture.session_update=[&](MenuState& actual,std::uint32_t clock,const auto& play) {
        if(!waited) {
          // Let the actual network clock pass the source PCR intro. No clock
          // override or fixture result is used for this diagnostic capture.
          std::this_thread::sleep_for(std::chrono::milliseconds(4100));waited=true;
        }
        flow.update_frontend(actual,clock,play);return false;
      };
      check(!run_frontend(capture),"Actual report capture unexpectedly launched a world");
      check(std::filesystem::is_regular_file(capture.screenshot),"Actual completed report capture missing");
    } else {
      // CPU resource boundary: no GPU framebuffer or original loading/fade
      // parity is asserted by this headless round-trip integration test.
      Cache menu(maps/"mainmenu.map");(void)menu.load_bsp(0);flow.mainmenu_loaded();
    }
    check(state.frame().screen==vista_menu(16) && state.frame().instance!=lobby_instance &&
        state.frame().vista_pcr->results->player_count==player_count && state.frame().vista_pcr->results->team_count==0 &&
        state.signed_in_profile()==name,"Actual report factory lost real results or selected profile");
    const auto report=state.frame().vista_pcr;
    check(report->pane==1 && report->selected_row==0 && bool(report->voice) &&
        report->voice(0).visible && report->voice(0).frame==2,
        "Actual PC report focus or supplied offline voice presentation differs");
    const auto report_instance=state.frame().instance;
    const FrontendPointerTarget report_done{FrontendPointerAction::Back,0,state.revision};
    if(argc==5) {
      // Exercise actual SDL input, hit testing, lifecycle updates and rendered
      // frames together. A pointer target supplied directly to MenuState does
      // not cover SDL recomputing that target after the report is replaced.
      if(two_players) {
        // The earlier world capture deliberately holds separate stick axes.
        // Release them before navigating the actual postgame/lobby controls.
        for(auto* input:virtual_inputs)for(int axis=0;axis<SDL_CONTROLLER_AXIS_MAX;++axis)
          check(SDL_JoystickSetVirtualAxis(input,axis,0)==0,"Virtual controller release failed");
        SDL_JoystickUpdate();
      }
      FrontendOptions capture;capture.maps=maps;capture.vista_settings=argv[2];
      capture.session=std::shared_ptr<FrontendSession>(&frontend,[](FrontendSession*){});
      capture.desktop=defaults.desktop;capture.entry=FrontendOptions::Entry::postgame_report;
      capture.postgame_report=report;capture.hidden=!visible;capture.mute=true;
      capture.skip_intro=true;capture.validation=true;capture.width=1280;capture.height=720;
      capture.frames=150;capture.screenshot=std::filesystem::path(argv[4])/
          ("native-lobby-after-done-"+std::to_string(round+1)+".ppm");
      Mesh hit_mesh;FrontendPainter hit_painter(frontend.content(),maps/"fonts",hit_mesh);
      unsigned frames{},ready_frames{};int click_x{},click_y{};
      const bool mouse_done=round || std::getenv("H2_TEST_MOUSE_DONE")!=nullptr;
      const auto key=[&](SDL_Keycode code) {
        SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=code;
        event.key.keysym.scancode=SDL_GetScancodeFromKey(code);
        event.key.windowID=SDL_GetWindowID(defaults.desktop->window(1280,720,!visible));
        check(SDL_PushEvent(&event)==1,"Actual SDL report key was rejected");
      };
      const auto click=[&](unsigned clicks) {
        SDL_Event event{};event.type=SDL_MOUSEBUTTONDOWN;event.button.button=SDL_BUTTON_LEFT;
        event.button.windowID=SDL_GetWindowID(defaults.desktop->window(1280,720,!visible));
        event.button.state=SDL_PRESSED;event.button.clicks=Uint8(clicks);
        event.button.x=click_x;event.button.y=click_y;
        check(SDL_PushEvent(&event)==1,"Actual SDL Done click was rejected");
        event.type=SDL_MOUSEBUTTONUP;event.button.state=SDL_RELEASED;
        check(SDL_PushEvent(&event)==1,"Actual SDL Done release was rejected");
      };
      capture.session_update=[&](MenuState& actual,std::uint32_t clock,const auto& play) {
        flow.update_frontend(actual,clock,play);
        if(!frames)std::this_thread::sleep_for(std::chrono::milliseconds(4100));
        if(frames==5) {
          check(actual.frame().vista_pcr!=nullptr,"Rendered report disappeared before Done input");
          if(!mouse_done)key(SDLK_ESCAPE);
          else {
            hit_painter.draw(actual,5,-1,clock,{1280,720});
            bool found{};const auto canvas=frontend_canvas(1280,720);
            for(int y=4;y<480 && !found;++y)for(int x=4;x<640;++x) {
              const auto target=hit_painter.pointer_hit(float(x),float(y));
              if(target && target->action==FrontendPointerAction::Back &&
                  hit_painter.pointer_hit(float(x-3),float(y))==target &&
                  hit_painter.pointer_hit(float(x+3),float(y))==target &&
                  hit_painter.pointer_hit(float(x),float(y-3))==target &&
                  hit_painter.pointer_hit(float(x),float(y+3))==target) {
                click_x=int(canvas.left+x*canvas.scale);click_y=int(canvas.top+y*canvas.scale);
                found=true;break;
              }
            }
            check(found,"Rendered PCR has no actual Done mouse target");
            std::cout<<"Actual SDL Done click: "<<click_x<<','<<click_y<<std::endl;click(1);
          }
        }
        if(frames>=6) {
          check(actual.frame().screen==14 && actual.stack().size()==1 &&
              flow.lifecycle_kind()==1 && flow.countdown()==-1 && !actual.launch,
              "Actual SDL Done left the ready multiplayer lobby for Leave or the Vista Network browser");
          ++ready_frames;
          if(mouse_done && frames==6)click(2); // Second press of the same double click.
        }
        ++frames;return false;
      };
      check(!run_frontend(capture) && ready_frames>=120,
          "Actual SDL Done did not retain120 rendered ready-lobby frames");
      std::cout<<"PASS actual SDL Done/rendered lobby/120 later frames round "<<round+1<<std::endl;
      capture.entry=FrontendOptions::Entry::fresh_lobby;capture.postgame_report.reset();
      capture.frames=8;capture.screenshot=std::filesystem::path(argv[4])/
          ("native-next-countdown-"+std::to_string(round+1)+".ppm");
      frames=0;
      capture.session_update=[&](MenuState& actual,std::uint32_t clock,const auto& play) {
        flow.update_frontend(actual,clock,play);
        if(!frames++)key(SDLK_RETURN);
        else check(actual.frame().screen==14 && flow.countdown()==10 &&
            !actual.launch && !flow.launch_ready(),"Actual SDL Start failed in the returned lobby");
        return false;
      };
      check(!run_frontend(capture) && flow.countdown()==10,
          "Actual rendered lobby could not begin the next match countdown");
      state.back();
      check(state.frame().screen==14 && flow.countdown()==-1 && flow.lifecycle_kind()==1,
          "Cancelling the rendered next-match countdown left the lobby");
      std::cout<<"PASS actual SDL Start from returned multiplayer lobby round "<<round+1<<std::endl;
    }
    else if(!round)state.back(); // Keyboard Escape / controller B.
    else if(two_players)check(state.vista_pcr_event(13),"Actual alternate Done component was not consumed");
    else check(state.activate_pointer(report_done),"Actual report Done prompt was not consumed");
    flow.update_frontend(state,20000,sound);
    check(state.frame().screen==14 && state.frame().instance!=report_instance && state.stack().size()==1 &&
        flow.lifecycle_kind()==1 && state.signed_in_profile()==name && !state.launch,
        "Actual B Done did not enter a newly allocated lobby");
    check(state.local_players().size()==player_count && state.lobby_view()->input.player_count==int(player_count),
        "Return to lobby lost a signed-in local player");
    const auto ready_lobby=state.frame().instance;
    // A queued click still carrying the report's geometry must not activate
    // the new lobby's Leave action and dispose the session into PC browser13.
    check(!state.activate_pointer(report_done),"Stale report Done click reached the returned lobby");
    for(unsigned frame=0;frame<120;++frame) {
      flow.update_frontend(state,20000+frame*16,silent_stage);
      check(state.frame().screen==14 && state.frame().instance==ready_lobby &&
          state.stack().size()==1 && !state.frame().vista_pcr && !state.frame().vista_leave_task &&
          flow.lifecycle_kind()==1 && flow.countdown()==-1 && !state.launch,
          "Carnage report return left the ready lobby or opened the Vista Network browser");
    }
    state.select(0);state.accept();
    check(state.launch && state.launch->map==first->map &&
        state.launch->variant.values==first->variant.values &&
        state.launch->selected_profiles==first->selected_profiles &&
        state.launch->input_instances==first->input_instances,
        "Post-report lobby is not ready to start another match with its retained selections");
    flow.update_frontend(state,22000,silent_stage);
    check(flow.countdown()==10 && !state.launch && !flow.launch_ready(),
        "Post-report Start did not begin its actual session countdown");
    state.back();
    check(state.frame().instance==ready_lobby && flow.countdown()==-1 &&
        flow.lifecycle_kind()==1 && !state.launch && !flow.launch_ready(),
        "Post-report countdown cancellation left the returned lobby");
    std::cout<<"PASS carnage Done/stale prompt/idle updates/next Start stay in multiplayer lobby\n";
    std::cout<<"PASS "<<player_count<<"-player real-clock countdown/native authored expiry/report/Done/fresh lobby round "<<round+1<<std::endl;
  }
  const auto returned_lobby=state.frame().instance;
  std::shared_ptr<VistaLeaveTaskScreenOwner> task;
  const auto leave=[&] {
    state.back();
    check(state.frame().screen==vista_menu(182) && bool(state.frame().vista_leave_task) &&
        state.vista_session_screen()==14 && state.stack().size()==2,
        "Actual Leave did not allocate its independent progress task");
    task=state.frame().vista_leave_task;
  };
  unsigned leave_updates{};
  const auto update_leave=[&](std::uint32_t clock,const auto& sound) {
    flow.update_frontend(state,clock,sound);
    if(!leave_updates++)
      check(flow.lifecycle_kind()==0 && state.frame().instance==task->instance &&
          task->status_string==0x0d000441 && state.vista_session_screen()==vista_menu(13) &&
          state.stack().size()==2 && state.stack().front().instance!=returned_lobby,
          "Original Leave update did not dispose the session and replace only its lobby channel");
  };
  if(argc==5) {
    FrontendOptions capture;
    capture.maps=maps;capture.vista_settings=argv[2];
    capture.session=std::shared_ptr<FrontendSession>(&frontend,[](FrontendSession*){});
    capture.entry=FrontendOptions::Entry::session_return;
    // Resource reentry is followed by an actual user Leave action/factory,
    // then ordinary task/observer updates; no browser/status preview is used.
    capture.session_mainmenu_ready=[&](MenuState& actual) {
      check(&actual==&state,"Leave capture lost its actual retained menu owner");leave();
    };
    capture.session_update=[&](MenuState& actual,std::uint32_t clock,const auto& sound) {
      check(&actual==&state,"Leave capture updated another menu owner");update_leave(clock,sound);return false;
    };
    capture.hidden=!visible;capture.mute=true;capture.skip_intro=true;capture.validation=true;
    capture.width=1280;capture.height=720;capture.frames=8;
    capture.screenshot=std::filesystem::path(argv[4])/"native-browser-after-leave.ppm";
    check(!run_frontend(capture) && leave_updates>=2 && std::filesystem::is_regular_file(capture.screenshot),
        "Actual Leave/browser GPU capture did not complete its retained task updates");
  } else {
    leave();update_leave(21000,silent_stage);update_leave(21016,silent_stage);
  }
  check(state.frame().screen==vista_menu(13) && state.stack().size()==1 && !state.launch &&
      state.signed_in_profile()==name && flow.lifecycle_kind()==0,
      "Original Leave task did not close independently over the new PC browser");
  const auto browser_instance=state.frame().instance;
  state.select(0);state.accept();
  check(state.frame().instance==browser_instance && !state.launch && flow.lifecycle_kind()==1 &&
      !flow.has_report(),"Actual Create failed to birth the local session before the lobby observer");
  flow.update_frontend(state,22000,silent_stage);
  check(state.frame().screen==14 && state.frame().instance!=returned_lobby &&
      state.frame().instance!=browser_instance && state.stack().size()==1 && !state.launch &&
      state.signed_in_profile()==name && flow.countdown()==-1,
      "Actual Create did not enter a new lobby without starting its countdown");
  state.select(0);state.accept();
  check(bool(state.launch) && state.launch->map==first->map &&
      state.launch->variant.values==first->variant.values &&
      state.launch->selected_profiles==first->selected_profiles,
      "Create lost the actual profile/map/variant used by the completed matches");
  flow.update_frontend(state,22000,silent_stage);
  check(flow.countdown()==10 && !state.launch && !flow.launch_ready(),
      "Freshly created session did not start the original countdown");
  state.back();
  check(flow.countdown()==-1 && flow.lifecycle_kind()==1 && state.frame().screen==14 && !state.launch,
      "Freshly created session did not cancel its countdown normally");
  std::cout<<"PASS actual Leave/task/PC browser/Create/fresh lobby/third Start cancellation\n";
  // Leave the newly created genuine session a second time. The following
  // Back must allocate the application's original main-menu presentation,
  // without reviving an old lobby/report frame from a previous lifetime.
  state.back();
  check(state.frame().screen==vista_menu(182),"Second actual Leave lost its independent task");
  flow.update_frontend(state,23000,silent_stage);flow.update_frontend(state,23016,silent_stage);
  check(state.frame().screen==vista_menu(13) && state.stack().size()==1 &&
      flow.lifecycle_kind()==0 && state.signed_in_profile()==name,
      "Second actual Leave did not reach its new nonmodal browser");
  const auto final_browser=state.frame().instance;
  const auto browser_back=[&] {
    state.back();
    check(state.frame().screen==(two_players?9:initial_mainmenu.screen) && state.frame().instance!=final_browser &&
        state.frame().instance!=initial_mainmenu.instance &&
        state.stack().size()==1 && (two_players || state.frame().entries.size()==initial_mainmenu.entries.size()) &&
        state.signed_in_profile()==name && flow.lifecycle_kind()==0 && !state.launch,
        "Postgame browser Back switched to a different main menu instead of the original native menu");
    if(!two_players)for(unsigned i=0;i<initial_mainmenu.entries.size();++i) {
      const auto& actual=state.frame().entries[i];const auto& expected=initial_mainmenu.entries[i];
      check(actual.label==expected.label && actual.destination==expected.destination && actual.value==expected.value,
          "Postgame main-menu labels or actions differ from the original native menu");
    }
  };
  if(argc==5) {
    FrontendOptions capture;
    capture.maps=maps;capture.vista_settings=argv[2];
    capture.session=std::shared_ptr<FrontendSession>(&frontend,[](FrontendSession*){});
    capture.entry=FrontendOptions::Entry::session_return;
    capture.session_mainmenu_ready=[&](MenuState& actual) {
      check(&actual==&state,"Main-menu capture lost its actual retained menu owner");browser_back();
    };
    capture.session_update=[&](MenuState& actual,std::uint32_t clock,const auto& sound) {
      check(&actual==&state,"Main-menu capture updated another menu owner");
      flow.update_frontend(state,clock,sound);return false;
    };
    capture.hidden=!visible;capture.mute=true;capture.skip_intro=true;capture.validation=true;
    capture.width=1280;capture.height=720;capture.frames=8;
    capture.screenshot=std::filesystem::path(argv[4])/"native-mainmenu-after-browser-back.ppm";
    check(!run_frontend(capture) && std::filesystem::is_regular_file(capture.screenshot),
        "Actual browser Back/PC6 GPU capture did not complete");
  } else browser_back();
  const auto mainmenu_instance=state.frame().instance;
  flow.update_frontend(state,24000,silent_stage);
  check(state.frame().instance==mainmenu_instance && state.frame().screen==(two_players?9:initial_mainmenu.screen),
      "Ordinary native lifecycle update replaced the completed main-menu Back factory");
  if(two_players) {
    check(state.local_players().size()==2,"Multi-user browser Back lost its selected profiles");
    std::cout<<"PASS actual second Leave/browser Back/fresh local title9/both profiles retained\n";
  } else {
    choose(state,4,vista_menu(19));state.back();
    check(state.frame().instance==mainmenu_instance,"Settings failed to retain its original native main-menu parent");
    std::cout<<"PASS actual second Leave/browser Back/fresh native mainmenu6/PC Settings return\n";
    choose(state,2,30);choose(state,0,14);
    state.select(0);state.accept();
    check(state.launch && state.launch->map==first->map &&
        state.launch->selected_profiles==first->selected_profiles,
        "Starting again from the returned main menu did not produce a fresh local match request");
    flow.update_frontend(state,25000,silent_stage);
    check(flow.lifecycle_kind()==initial_countdown_kind && flow.countdown()==10 && !state.launch && !flow.launch_ready(),
        "Starting again from the returned main menu reused the disposed session");
    state.back();
    check(flow.lifecycle_kind()==1 && flow.countdown()==-1 && state.frame().screen==14,
        "New match countdown from the returned main menu did not cancel normally");
    std::cout<<"PASS returned native main menu can start and cancel a new local match\n";
  }
  std::cout<<"PASS two actual native rounds retain the genuine offline identity and profile; no rendered Vista parity claim\n";
  for(auto* joystick:virtual_inputs)if(joystick)SDL_JoystickClose(joystick);
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
