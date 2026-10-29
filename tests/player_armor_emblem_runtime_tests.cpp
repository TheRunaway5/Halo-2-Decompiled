#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/object_lighting.hpp"
#include <SDL.h>
#include <algorithm>
#include <bit>
#include <iostream>
#include <set>
#include <thread>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct VirtualPads {
  std::vector<SDL_JoystickID> ids;
  std::vector<SDL_Joystick*> joysticks;
  VirtualPads() {
    check(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0,SDL_GetError());
    for(unsigned player=0;player<4;++player) {
      SDL_VirtualJoystickDesc description{};description.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
      description.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;description.naxes=SDL_CONTROLLER_AXIS_MAX;
      description.nbuttons=SDL_CONTROLLER_BUTTON_MAX;description.name="Team switching controller";
      const auto device=SDL_JoystickAttachVirtualEx(&description);
      check(device>=0 && SDL_IsGameController(device),"Actual SDL team controller could not be mapped");
      auto *joystick=SDL_JoystickOpen(device);check(joystick,SDL_GetError());
      joysticks.push_back(joystick);ids.push_back(SDL_JoystickInstanceID(joystick));
      for(int axis=0;axis<SDL_CONTROLLER_AXIS_MAX;++axis)
        check(SDL_JoystickSetVirtualAxis(joystick,axis,axis>=4?-32768:0)==0,SDL_GetError());
    }
    SDL_JoystickUpdate();
  }
  ~VirtualPads() {
    if(!SDL_WasInit(SDL_INIT_JOYSTICK))return;
    for(auto *joystick:joysticks)SDL_JoystickClose(joystick);
    for(auto id:ids)for(int device=0;device<SDL_NumJoysticks();++device)
      if(SDL_JoystickGetDeviceInstanceID(device)==id){SDL_JoystickDetachVirtual(device);break;}
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  }
};
struct Host {
  std::shared_ptr<const Cache> cache;
  LevelContent content;
  MultiplayerMatch match;
  LanHost transport;
  std::exception_ptr error;
  std::jthread worker;
  static SlayerRules rules(const MultiplayerOptions &options,const Cache &cache) {
    auto result=slayer_rules(options.variant,true);
    result.weapon=cache.slayer_starting_weapon(5);
    return result;
  }
  explicit Host(const MultiplayerOptions &options):cache(std::make_shared<const Cache>(
      options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe))),
      content(cache->load_content(0,true)),match(content,0,cache->spawns(),rules(options,*cache)),
      transport(match,0,multiplayer_fingerprint(options.map,options.shared_maps,0,
          cache->slayer_starting_weapon(5),options.gameplay_xbe),options.variant) {
    for(unsigned representation:{2u,3u})match.configure_replicated_first_person(
        cache->load_first_person_binding(cache->slayer_starting_weapon(5),representation));
    worker=std::jthread([this](std::stop_token stop) {
      try {
        std::array<Controls,multiplayer_capacity> commands{};
        while(!stop.stop_requested()) {
          transport.receive(commands);transport.broadcast();
          std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
      }catch(...){error=std::current_exception();}
    });
  }
  void finish(){worker.request_stop();worker.join();if(error)std::rethrow_exception(error);}
};
}
int main(int argc,char **argv)try {
  check(argc==3 || argc==4 || argc==5,"Supply repository and output directories, and optional teams mode");
  const bool switching=argc==5 && std::string_view(argv[3])=="teams-switch";
  const bool lan=argc==4 && std::string_view(argv[3])=="teams-lan";
  const bool teams=switching || (argc==4 && (std::string_view(argv[3])=="teams" || lan));
  check(argc==3 || teams,"Unknown armor runtime mode");
  if(!std::getenv("DISPLAY"))return 77;
  SDL_SetMainReady();
  std::optional<VirtualPads> pads;if(switching)pads.emplace();
  const std::filesystem::path root=argv[1],output=argv[2];
  std::filesystem::create_directories(output);
  MultiplayerOptions options;
  options.map=root/"assets/disc/maps/lockout.map";
  options.shared_maps=options.map.parent_path();
  options.gameplay_xbe=root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03";
  options.width=640;options.height=480;options.frames=4;options.local_players=4;
  options.starting_weapon=5;options.hidden=true;options.mute=true;
  options.validation=true;options.screenshot=output/"four-player-armor.ppm";
  if(teams) {
    options.variant={"Team Slayer",0,{{2,0},{10,0},{89,5},{91,1}}};
    options.selected_teams={0,1,1,0};
  }
  if(switching) {
    options.map=argv[4];options.shared_maps=options.map.parent_path();options.gameplay_xbe.clear();
    if(!std::filesystem::is_regular_file(options.map))return 77;
    options.source_object_world=true;options.frames=0;options.capture_tick=240;
    options.variant.values.set(22,1);options.input_instances.assign(pads->ids.begin(),pads->ids.end());
  }
  const std::array<std::array<std::uint8_t,9>,4> appearances{{
    {8,12,9,1,0,26,9,0,0}, {3,4,3,4,1,5,31,0,0},
    {8,12,9,1,0,63,0,1,0}, {3,4,5,6,1,0,12,0,0}}};
  for(unsigned i=0;i<appearances.size();++i)
    options.selected_profiles.push_back(native_selected_player_configuration(
        "Armor "+std::to_string(i),appearances[i],teams?options.selected_teams[i]:-1));
  std::unique_ptr<Host> host;
  if(lan) {
    host=std::make_unique<Host>(options);
    options.join=host->transport.join_address();options.capture_players=4;
  }
  Cache cache(options.map,options.shared_maps,switching?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(options.gameplay_xbe));
  const auto assets=cache.load_full_scoreboard_assets();
  const auto &palette=assets.palette;
  std::array<std::int8_t,4> accepted_teams{0,1,1,0};bool requested{},respawned{};unsigned requested_views{};
  auto accepted_profiles=options.selected_profiles;
  if(teams)options.input_observer=[&](auto,auto,const MultiplayerState &state) {
    bool all_respawned=true;
    for(unsigned player=0;player<4;++player)if(state.players[player].connected) {
      const auto &actual=state.players[player];accepted_teams[player]=actual.source_membership;
      if(actual.source_configuration)accepted_profiles[player]=*actual.source_configuration;
      check(actual.source_configuration && actual.source_configuration->at(0x7c)==std::uint8_t(actual.source_membership),
          "Team selection did not reach the accepted local/UDP player configuration");
      if(!switching)check(actual.source_membership==options.selected_teams[player],"Initial team selection changed");
      all_respawned&=actual.source_membership==int(player+2) && actual.source_unit!=null_datum;
    } else all_respawned=false;
    if(switching && requested_views<4 && state.tick>=80+20*requested_views) {
      requested=true;
      const auto button=[](int instance,SDL_GameControllerButton code) {SDL_Event event{};event.type=SDL_CONTROLLERBUTTONDOWN;
        event.cbutton.which=instance;event.cbutton.button=code;check(SDL_PushEvent(&event)==1,"Pause team controller event failed");};
      {
        const unsigned view=requested_views++;
        const auto instance=options.input_instances[view];
        button(instance,SDL_CONTROLLER_BUTTON_START);
        const auto down=[&]{button(instance,SDL_CONTROLLER_BUTTON_DPAD_DOWN);};
        const auto accept=[&]{button(instance,SDL_CONTROLLER_BUTTON_A);};
        down();down();accept();
        for(int n=accepted_teams[view];n<int(view+2);++n)down();accept();
      }
    }
    if(switching && requested && all_respawned)respawned=true;
  };
  std::set<unsigned> players;unsigned programs=0;
  options.player_armor_observer=[&](unsigned player,const Material &actual) {
    if(teams && player<4) {
      check(bool(actual.object_render_colors),"Team armor has no object color bank");
      const auto expected=source_object_render_colors({{
          std::bit_cast<Vec3>(assets.team_palette.at(unsigned(accepted_teams[player]))),
          std::bit_cast<Vec3>(palette.at(appearances[player][0])),
          std::bit_cast<Vec3>(palette.at(appearances[player][2])),
          std::bit_cast<Vec3>(palette.at(appearances[player][3]))}});
      for(unsigned channel=0;channel<4;++channel)
        if(length(actual.object_render_colors->at(channel)-expected[channel])>=1e-6f)
          throw FormatError("Team armor ignores accepted team: player="+std::to_string(player)+
              " team="+std::to_string(accepted_teams[player])+" channel="+std::to_string(channel)+
              " material="+actual.name);
    }
    if(player>=4 || (!actual.name.ends_with("\\mc_emblem") && !actual.name.ends_with("\\elite_emblem")))return;
    auto expected=cache.load_material(actual.source_shader_datum);
    expected.selected_shader_lod=actual.selected_shader_lod;
    check(prepare_dynamic_material(cache,expected,{},true),"Expected actual source armor pass unavailable");
    FrontendAppearance appearance;
    const auto &bytes=appearances[player];
    appearance.colors={std::int8_t(bytes[0]),std::int8_t(bytes[1]),std::int8_t(bytes[2]),std::int8_t(bytes[3])};
    appearance.foreground=bytes[5];appearance.background=bytes[6];appearance.flags=bytes[7];
    ObjectColors colors=*actual.object_render_colors;
    colors[2]=std::bit_cast<Vec3>(palette[bytes[2]]);colors[3]=std::bit_cast<Vec3>(palette[bytes[3]]);
    apply_object_colors(expected,colors);
    const auto input=[&](std::uint32_t name){return frontend_model_emblem_input(appearance,name).value_or(0.f);};
    animate_forward_material(expected,0,input);animate_material_bitmaps(expected,0,input);
    if(actual.texture_frames!=expected.texture_frames)throw FormatError("Actual multiplayer body bitmap pages: player="+std::to_string(player)+
        " texture="+std::to_string(actual.texture_frames[0])+" expected="+std::to_string(expected.texture_frames[0])+" accepted="+
        std::to_string(accepted_profiles[player][0x45]));
    check(actual.forward_program.words==expected.forward_program.words,
        "Actual multiplayer body lost its selected player's emblem colors/toggle");
    players.insert(player);++programs;
  };
  check(run_multiplayer(options)==0,"Actual multiplayer renderer failed");
  if(host)host->finish();
  if(switching && !(requested && requested_views==4 && respawned))throw FormatError(
      "Actual pause switch/respawn missing: requests="+std::to_string(requested_views)+" teams="+
      std::to_string(accepted_teams[0])+","+std::to_string(accepted_teams[1])+","+
      std::to_string(accepted_teams[2])+","+std::to_string(accepted_teams[3]));
  check(players==std::set<unsigned>{0,1,2,3} && programs>=4,
      "Actual four-view gameplay did not animate all selected player armor banks");
  check(std::filesystem::file_size(options.screenshot)>640u*480u*3u,"Actual multiplayer screenshot missing");
  std::cout<<programs<<" actual multiplayer armor banks: four profiles, both species, "
      <<(teams?"red/blue teams, ":"")<<"foreground/colors/toggle and Vulkan upload pass\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
