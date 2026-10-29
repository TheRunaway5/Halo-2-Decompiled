#include "halo2/native_vista_flow.hpp"
#include "halo2/full_scoreboard_painter.hpp"
#include "halo2/frontend_lobby.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/native_vista_loading_audio.hpp"
#include "halo2/mcc_loading_presentation.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/original_data.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "../src/ui/mcc_frontend_string_ids.hpp"
#include <bit>
#include <chrono>
#include <cstdlib>
#include <fstream>
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
  bool imported{};
  {Cache menu(maps/"mainmenu.map");imported=menu.is_mcc();frontend.retain_content(menu.load_frontend());}
  frontend.content().vista=load_vista_settings(argv[2]);
  std::filesystem::path preferences;
  if(imported && argc==5) {
    // Use the real persisted one-minute Slayer option, so the native fade
    // journal and render owner see every tick of an ordinary timed match.
    // CPU acceleration would omit the presentation's bounded source history.
    preferences=std::filesystem::path(argv[4])/"flow-menu.cfg";
    std::filesystem::create_directories(preferences.parent_path());
    std::ofstream(preferences)<<"halo2-native-menu-v1\nvariant 10 60\n";
  }
  auto& state=frontend.create_state(preferences);
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
  MultiplayerOptions defaults;
  if(imported) {
    // Match the installed menu's actual native object-world path. The TU1.5
    // argument remains a reference input, never a runtime executable provider.
    defaults.source_object_world=true;
  } else defaults.gameplay_xbe=argv[3];
  defaults.mute=std::getenv("H2_TEST_DAMAGE_AUDIO")==nullptr;
  if(argc==5) {
    defaults.desktop=std::make_shared<DesktopSession>();
    const std::filesystem::path loading=std::getenv("H2_TEST_LOADING_ASSETS")?
        std::getenv("H2_TEST_LOADING_ASSETS"):"assets/menu-ui/vista/loading";
    if(imported)check(configure_h2ek_original_loading_presentation(*defaults.desktop,maps.parent_path().parent_path(),true),
        "Imported roundtrip requires the onboarded H2EK campaign loading bank");
    else defaults.desktop->configure_loading(loading);
    // Exercise the application's actual loading-audio adapter lifetime. This
    // loading-tone capture stays muted; H2_TEST_DAMAGE_AUDIO enables world
    // audio so the absent-unit HUD interval is exercised after the fade.
    if(!imported) {
    auto loading_audio=std::make_shared<NativeVistaLoadingAudio>(
        loading/"loading-tone.xbox-adpcm");
    defaults.desktop->set_loading_audio_services({
        [loading_audio]{loading_audio->begin(true);},
        [loading_audio](std::uint32_t gain){loading_audio->gain(gain);},
        [loading_audio]{loading_audio->end();}});
    }
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
  const auto make_flow=[&] {
    auto fresh=std::make_unique<NativeVistaOfflineFlow>(frontend,maps,defaults,[&] {
    check(active_flow && !active_flow->launch_ready(),
        "Loading presentation started after native world readiness publication");
    bool absent{};
    try{(void)active_flow->prepared_world();}catch(const FormatError&){absent=true;}
    check(absent,"Loading presentation started after native world construction");
    ++resource_begins;
    if(argc==5) {
      if(imported)defaults.desktop->begin_multiplayer_loading();
      else defaults.desktop->begin_loading();
      if(imported)
      check(defaults.desktop->loading_snapshot()->network_transition==FrontendNetworkTransitionStatus::SettingUpGame,
          "Actual local Start did not select the multiplayer setup dialog");
      defaults.desktop->capture_loading(std::filesystem::path(argv[4])/
          ("native-loading-world-"+std::to_string(resource_begins)+".ppm"));
    }
  });
    active_flow=fresh.get();return fresh;
  };
  auto flow=make_flow();
  const auto silent_stage=[](FrontendUiSound){throw FormatError("Early countdown unexpectedly emitted a sound");};
  state.select(0);state.accept();flow->update_frontend(state,0,silent_stage);
  check(flow->countdown()==10,"Initial countdown did not publish source stage10");
  const auto countdown_lobby=state.lobby_view();
  check(countdown_lobby && countdown_lobby->input.countdown &&
      frontend_lobby_presentation(countdown_lobby->input).status_string==0x200001f0,
      "Actual countdown did not select its original status owner");
  const auto countdown_text=state.expand(detail::frontend_compiled_localized(
      frontend.content(),frontend.content().screen(14),0x200001f0));
  check(countdown_text=="Game start countdown in progress  10\r\nPress \xee\x84\x82 to cancel",
      ("Actual countdown caption differs: "+countdown_text).c_str());
  state.accept();
  check(flow->countdown()==9 && !state.launch,"Second Start did not quick-advance the retained countdown");
  state.back();
  check(flow->countdown()==-1 && flow->lifecycle_kind()==1 && state.frame().screen==14 &&
      !state.launch && !flow->launch_ready(),"Back did not cancel the countdown before ordinary Leave");
  state.accept();flow->update_frontend(state,0,silent_stage);
  state.select(1);state.accept();
  check(state.frame().screen==23 && flow->countdown()==-1 && flow->lifecycle_kind()==1 &&
      !state.launch && !flow->launch_ready() && !resource_begins,
      "Game Setup left a stale source launch or loading presentation request");
  state.back();check(state.frame().screen==14,"Game Setup did not return to its actual lobby");
  std::cout<<"PASS actual lobby quick-advance/Back cancellation/Setup cancellation without stale launch\n";
  unsigned title_returns{};
  const auto leave_to_menu=[&] {
    state.back();
    check(state.frame().vista_leave_task!=nullptr,"Leave did not create its actual disposal task");
    flow->update_frontend(state,23000,silent_stage);
    check(state.frame().vista_leave_task!=nullptr && flow->lifecycle_kind()==0,
        "Leave did not dispose the session before completing its progress task");
    flow->update_frontend(state,23016,silent_stage);
    check(state.frame().screen==6 && state.stack().size()==1 && flow->finished() &&
        !flow->has_report() && !state.launch,"Completed Leave did not return directly to the native main menu");
    check(state.frame().entries.size()==initial_mainmenu.entries.size(),"Leave selected a different main-menu layout");
    for(unsigned i=0;i<initial_mainmenu.entries.size();++i) {
      const auto& a=state.frame().entries[i];const auto& b=initial_mainmenu.entries[i];
      check(a.label==b.label && a.destination==b.destination && a.value==b.value,
          "Returned main menu changed its native labels or actions");
    }
    const auto instance=state.frame().instance;
    for(unsigned i=0;i<4;++i)flow->update_frontend(state,23032+i*16,silent_stage);
    check(state.frame().screen==6 && state.frame().instance==instance,"Idle update replaced the returned menu");
    // Match main.cpp: a completed disposal detaches this owner's input hook.
    flow.reset();active_flow=nullptr;
  };
  const auto return_through_title=[&] {
    ++title_returns;
    state.back();state.select(0);state.accept();
    check(state.frame().screen==9 && state.stack().size()==1 && state.local_players().empty(),
        "Main menu Back did not create a fresh signed-out title");
    state.accept();check(state.frame().screen==10,"Title did not open its actual profile picker");
    choose(state,1,6);choose(state,2,30);
    if(two_players) {
      check(state.controller_slot(inputs[0],true)==0,"Title reentry did not bind the host controller");
      check(state.controller_slot(inputs[1],true)==1,"Title reentry did not add the second controller");
      const auto found=std::find_if(state.frame().entries.begin(),state.frame().entries.end(),
          [](const auto& entry){return entry.label=="Second Pilot";});
      check(found!=state.frame().entries.end(),"The second profile disappeared at title");
      state.select(unsigned(found-state.frame().entries.begin()));state.accept();
      check(state.frame().screen==30 && state.local_players().size()==2,
          "Title reentry failed to restore the actual second profile");
    }
    choose(state,0,14);
    check(state.local_players().size()==player_count,"Title reentry lost selected local players");
    flow=make_flow();
    flow->update_frontend(state,24000,silent_stage);
    check(state.frame().screen==14 && !state.launch,"Fresh session owner changed the selected lobby");
    if(argc==5) {
      FrontendOptions capture;capture.maps=maps;capture.vista_settings=argv[2];
      capture.session=std::shared_ptr<FrontendSession>(&frontend,[](FrontendSession*){});
      capture.desktop=defaults.desktop;capture.entry=FrontendOptions::Entry::fresh_lobby;
      capture.hidden=!visible;capture.mute=true;capture.skip_intro=true;capture.validation=true;
      capture.width=1280;capture.height=720;capture.frames=8;
      capture.screenshot=std::filesystem::path(argv[4])/("native-lobby-after-title-"+std::to_string(title_returns)+".ppm");
      capture.session_update=[&](auto& actual,auto clock,const auto& sound) {
        flow->update_frontend(actual,clock,sound);return false;
      };
      check(!run_frontend(capture),"Title reentry capture launched before Start");
    }
  };
  if(std::getenv("H2_TEST_NAV_ONLY")) {
    leave_to_menu();return_through_title();
    std::cout<<"PASS disposed session/title/profile/Split Screen navigation\n";return 0;
  }
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
      flow->update_frontend(state,std::uint32_t(elapsed),sound);
      if(!flow->launch_ready())std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }while(!flow->launch_ready());
    check(beeps==4 && flow->lifecycle_kind()==3 && !state.launch,"Source countdown did not emit exactly its four final-stage beeps");
    check(resource_begins==round+1,"Each real native map load must start presentation exactly once");
    const auto request=flow->take_launch();
    auto options=defaults;options.map=request.map;options.local_players=request.local_players;
    options.variant=request.variant;options.selected_profiles=request.selected_profiles;options.selected_teams=request.selected_teams;
    options.input_instances=request.input_instances;
    options.prepared_world=flow->prepared_world();options.session_random_seed=options.prepared_world->rules.random_seed;
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
          prepared.match.player(slot).source_current_object==null_datum && flow->player_activation_count()==0 &&
          absent_view.unit==null_datum && absent_view.representation==-1 &&
          absent_view.hands[0].weapon==null_datum && absent_view.hands[1].weapon==null_datum,
          "Local observer creation activated or bound the genuine absent-unit birth");
    }
    bool first_view_bound{};
    const auto observe_initial_view=[&](const MultiplayerMatch& actual) {
      if(first_view_bound)return;
      check(actual.state().tick==1 && flow->player_activation_count()==player_count,
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
          match.player(slots[0]).source_current_object==null_datum && flow->player_activation_count()==0,
          "Runtime rebuilt the match or lost the unactivated real player birth");
      ++loaded;flow->world_loaded(match,slots);
    };
    options.session_world_update=[&](const auto& match) {
      ++updates;const auto close=flow->world_update(match);observe_initial_view(match);return close;
    };
    options.session_audio_update=[&](float volume,const auto& gains){flow->world_audio_update(volume,gains);};
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
      std::optional<float> captured_black;
      bool native_opening_clear{};
      if(imported)rendered.gameplay_fade_observer=[&](unsigned view,auto tick,auto alpha,const auto& fade) {
        check(view<player_count,"Native fade used an unselected viewport");
        if(alpha) {
          check(std::bit_cast<std::uint32_t>(*alpha)==fade.opacity_bits[view],
              "Native viewport changed the source binary32 fade opacity");
          ++black_draws;
          if(*alpha>0 && *alpha<1)++partial_draws;
        } else if(view==0 && tick>=45)native_opening_clear=true;
        if(round && tick==rendered.capture_tick)captured_black=alpha;
      };
      rendered.session_view_black=[&](unsigned view) {
        check(!imported,"Imported world called the legacy PC viewport fade");
        const auto packed=flow->world_view_black(view);
        if(packed)++black_draws;
        if(packed && (*packed>>24)>0 && (*packed>>24)<255)++partial_draws;
        if(round && options.prepared_world->match.state().tick==rendered.capture_tick)
          captured_black=packed?std::optional<float>{float(*packed>>24)/255.f}:std::nullopt;
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
          const auto close=flow->world_update(actual);observe_initial_view(actual);
          if(close)return true;
          if(imported)return false;
          if(accelerated || actual.state().tick<60)return false;
          check(!flow->world_view_black(0),"Source opening fade did not clear before diagnostic acceleration");
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
            check(!flow->world_update(match),"Diagnostic acceleration passed the actual world-close request");
          }
          return false;
        };
      }
      check(run_multiplayer(rendered,outcome)==0 && black_draws && loaded==1 &&
          outcome.reason==((round || two_players)?MultiplayerExitReason::captured_tick:MultiplayerExitReason::frame_limit) &&
          (!round || (checked_scoreboard && partial_draws && captured_black && *captured_black>0 &&
              *captured_black<1 && (!imported || native_opening_clear))),
          "Actual viewport did not render its source fade");
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
        outcome.state->results_finalized && outcome.state->game_over_complete() && flow->lifecycle_kind()==4 &&
        flow->has_report() && flow->player_activation_count()==player_count && first_view_bound,
        "Actual normal expiry did not reach its source-owned postgame handoff with one initial activation");
    if(argc==5) {
      // CPU acceleration ends at the same real world-close result. Explicitly
      // invoke the production resource boundary omitted by the headless branch.
      if(imported)defaults.desktop->begin_multiplayer_loading(FrontendNetworkTransitionStatus::LeavingGame);
      else defaults.desktop->begin_loading();
      defaults.desktop->capture_loading(std::filesystem::path(argv[4])/
          ("native-loading-menu-"+std::to_string(round+1)+".ppm"));
    }
    options.prepared_world.reset();flow->world_unloaded();
    check(world.expired(),"World teardown retained the actual native match");
    if(argc==5) {
      // Optional actual application resource/render boundary. The normal CTest
      // remains CPU-only; neither path asserts original framebuffer equality.
      FrontendOptions capture;
      capture.maps=maps;capture.vista_settings=argv[2];
      capture.session=std::shared_ptr<FrontendSession>(&frontend,[](FrontendSession*){});
      capture.entry=FrontendOptions::Entry::session_return;
      capture.session_mainmenu_ready=[&](MenuState&){flow->mainmenu_loaded();};
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
        flow->update_frontend(actual,clock,play);return false;
      };
      check(!run_frontend(capture),"Actual report capture unexpectedly launched a world");
      check(std::filesystem::is_regular_file(capture.screenshot),"Actual completed report capture missing");
    } else {
      // CPU resource boundary: no GPU framebuffer or original loading/fade
      // parity is asserted by this headless round-trip integration test.
      Cache menu(maps/"mainmenu.map");(void)menu.load_bsp(0);flow->mainmenu_loaded();
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
      Cache selected_menu(maps/"mainmenu.map");
      Mesh hit_mesh;FrontendPainter hit_painter(frontend.content(),gameplay_font_directory(selected_menu),hit_mesh);
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
        flow->update_frontend(actual,clock,play);
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
              flow->lifecycle_kind()==1 && flow->countdown()==-1 && !actual.launch,
              "Actual SDL Done left the ready multiplayer lobby for Leave or a retired screen");
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
        flow->update_frontend(actual,clock,play);
        if(!frames++)key(SDLK_RETURN);
        else check(actual.frame().screen==14 && flow->countdown()==10 &&
            !actual.launch && !flow->launch_ready(),"Actual SDL Start failed in the returned lobby");
        return false;
      };
      check(!run_frontend(capture) && flow->countdown()==10,
          "Actual rendered lobby could not begin the next match countdown");
      state.back();
      check(state.frame().screen==14 && flow->countdown()==-1 && flow->lifecycle_kind()==1,
          "Cancelling the rendered next-match countdown left the lobby");
      std::cout<<"PASS actual SDL Start from returned multiplayer lobby round "<<round+1<<std::endl;
    }
    else if(!round)state.back(); // Keyboard Escape / controller B.
    else if(two_players)check(state.vista_pcr_event(13),"Actual alternate Done component was not consumed");
    else check(state.activate_pointer(report_done),"Actual report Done prompt was not consumed");
    flow->update_frontend(state,20000,sound);
    check(state.frame().screen==14 && state.frame().instance!=report_instance && state.stack().size()==1 &&
        flow->lifecycle_kind()==1 && state.signed_in_profile()==name && !state.launch,
        "Actual B Done did not enter a newly allocated lobby");
    check(state.local_players().size()==player_count && state.lobby_view()->input.player_count==int(player_count),
        "Return to lobby lost a signed-in local player");
    const auto ready_lobby=state.frame().instance;
    // A queued click still carrying the report's geometry must not activate
    // the new lobby's Leave action and dispose its retained session.
    check(!state.activate_pointer(report_done),"Stale report Done click reached the returned lobby");
    for(unsigned frame=0;frame<120;++frame) {
      flow->update_frontend(state,20000+frame*16,silent_stage);
      check(state.frame().screen==14 && state.frame().instance==ready_lobby &&
          state.stack().size()==1 && !state.frame().vista_pcr && !state.frame().vista_leave_task &&
          flow->lifecycle_kind()==1 && flow->countdown()==-1 && !state.launch,
          "Carnage report return left the ready lobby or opened a retired screen");
    }
    state.select(0);state.accept();
    check(state.launch && state.launch->map==first->map &&
        state.launch->variant.values==first->variant.values &&
        state.launch->selected_profiles==first->selected_profiles &&
        state.launch->input_instances==first->input_instances,
        "Post-report lobby is not ready to start another match with its retained selections");
    flow->update_frontend(state,22000,silent_stage);
    check(flow->countdown()==10 && !state.launch && !flow->launch_ready(),
        "Post-report Start did not begin its actual session countdown");
    state.back();
    check(state.frame().instance==ready_lobby && flow->countdown()==-1 &&
        flow->lifecycle_kind()==1 && !state.launch && !flow->launch_ready(),
        "Post-report countdown cancellation left the returned lobby");
    std::cout<<"PASS carnage Done/stale prompt/idle updates/next Start stay in multiplayer lobby\n";
    std::cout<<"PASS "<<player_count<<"-player real-clock countdown/native authored expiry/report/Done/fresh lobby round "<<round+1<<std::endl;
    if(!round) {
      leave_to_menu();return_through_title();
      std::cout<<"PASS post-carnage/title/profile/Split Screen reentry; next round must fully load\n";
    }
  }
  leave_to_menu();
  const auto mainmenu_instance=state.frame().instance;
  choose(state,4,vista_menu(19));state.back();
  check(state.frame().instance==mainmenu_instance,"Settings lost the returned native menu");
  return_through_title();
  state.select(0);state.accept();flow->update_frontend(state,25000,silent_stage);
  check(flow->countdown()==10 && !state.launch,"Title reentry could not start another local countdown");
  state.back();
  check(flow->countdown()==-1 && state.frame().screen==14,"Title reentry could not cancel its countdown");
  std::cout<<"PASS post-carnage Leave/native menu/Settings/title/Split Screen without retired screens\n";
  std::cout<<"PASS two actual native rounds cover report/title/new-session restart; no rendered Vista parity claim\n";
  for(auto* joystick:virtual_inputs)if(joystick)SDL_JoystickClose(joystick);
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
