#define SDL_MAIN_HANDLED
#include "halo2/multiplayer.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/frontend_emblem.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/object_lighting.hpp"
#include <SDL.h>
#include <algorithm>
#include <bit>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
}
int main(int argc,char **argv)try {
  check(argc==3,"Supply repository and output directories");
  if(!std::getenv("DISPLAY"))return 77;
  SDL_SetMainReady();
  const std::filesystem::path root=argv[1],output=argv[2];
  std::filesystem::create_directories(output);
  MultiplayerOptions options;
  options.map=root/"assets/disc/maps/lockout.map";
  options.shared_maps=options.map.parent_path();
  options.gameplay_xbe=root/"assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03";
  options.width=640;options.height=480;options.frames=4;options.local_players=4;
  options.starting_weapon=5;options.hidden=true;options.mute=true;
  options.validation=true;options.screenshot=output/"four-player-armor.ppm";
  const std::array<std::array<std::uint8_t,9>,4> appearances{{
    {8,12,9,1,0,26,9,0,0}, {3,4,3,4,1,5,31,0,0},
    {8,12,9,1,0,63,0,1,0}, {3,4,5,6,1,0,12,0,0}}};
  for(unsigned i=0;i<appearances.size();++i)
    options.selected_profiles.push_back(native_selected_player_configuration(
        "Armor "+std::to_string(i),appearances[i],-1));
  Cache cache(options.map,options.shared_maps,OriginalData::load_xbe(options.gameplay_xbe));
  const auto palette=cache.load_full_scoreboard_assets().palette;
  std::set<unsigned> players;unsigned programs=0;
  options.player_armor_observer=[&](unsigned player,const Material &actual) {
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
    check(actual.texture_frames[0]==bytes[5],"Actual multiplayer body lost its selected player's foreground page");
    check(actual.forward_program.words==expected.forward_program.words,
        "Actual multiplayer body lost its selected player's emblem colors/toggle");
    players.insert(player);++programs;
  };
  check(run_multiplayer(options)==0,"Actual multiplayer renderer failed");
  check(players==std::set<unsigned>{0,1,2,3} && programs>=4,
      "Actual four-view gameplay did not animate all selected player armor banks");
  check(std::filesystem::file_size(options.screenshot)>640u*480u*3u,"Actual multiplayer screenshot missing");
  std::cout<<programs<<" actual multiplayer armor banks: four profiles, both species, foreground/colors/toggle and Vulkan upload pass\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
