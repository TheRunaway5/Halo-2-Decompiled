#include "halo2/frontend.hpp"
#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

using namespace halo2;
namespace {
void check(bool value,const char *message) {
  if(!value)throw FormatError(message);
}
std::string read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary);
  check(bool(file),"could not read saved-profile fixture");
  return {std::istreambuf_iterator<char>(file),{}};
}
void choose(MenuState &menu,unsigned index,unsigned destination) {
  menu.select(index);menu.accept();
  check(menu.frame().screen==destination,"saved-profile menu path changed");
}
void lobby(MenuState &menu) {
  check(menu.frame().screen==9,"saved-profile fixture did not start at title");
  menu.accept(); // Original title -> Choose Profile.
  check(menu.frame().screen==10,"title did not enter Choose Profile");
  const auto &entries=menu.frame().entries;
  const auto profile=std::find_if(entries.begin(),entries.end(),
      [&](const auto &entry){return entry.label==menu.profile_name();});
  check(profile!=entries.end(),"saved active profile absent from profile picker");
  choose(menu,unsigned(profile-entries.begin()),6);
  check(menu.signed_in_profile()==menu.profile_name(),"saved profile did not sign in");
  choose(menu,2,30);choose(menu,0,14);
}
}
int main(int argc,char **argv) try {
  if(argc<2 || argc>3 || !std::filesystem::exists(std::filesystem::path(argv[1])/"mainmenu.map"))return 77;
  const std::filesystem::path maps=argv[1];
  const auto temporary=std::filesystem::temp_directory_path()/
      ("halo2-slayer-vehicle-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(temporary);
  struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove_all(path,error);}} cleanup{temporary};
  const auto preferences=temporary/"menu.cfg";
  // Observed failing saved variant. A caller may instead supply its own file;
  // it is read and copied, never handed to the saving MenuState directly.
  const std::string input=argc==3?read(argv[2]):
      "halo2-native-menu-v1\nactive \"EmkMage\"\nvariant_name \"Slayer\" 0\n"
      "profile \"EmkMage\"\noption \"EmkMage\" 256 0\nvariant 2 10\n"
      "variant 10 120\nvariant 20 1\nvariant 28 0\nvariant 76 1\nvariant 83 0\nvariant 89 0\n";
  {std::ofstream file(preferences);file<<input;check(bool(file),"could not copy saved-profile fixture");}
  Cache menu_cache(maps/"mainmenu.map"),lockout(maps/"lockout.map"),coagulation(maps/"coagulation.map");
  const auto frontend=menu_cache.load_frontend();
  const auto level=lockout.load_content(0,true),vehicles=coagulation.load_content(0,true);
  check(std::none_of(level.netgame_equipment.begin(),level.netgame_equipment.end(),
      [](const auto &row){return row.classification==1;}),"original Lockout row classification changed");
  check(std::any_of(vehicles.netgame_equipment.begin(),vehicles.netgame_equipment.end(),
      [](const auto &row){return row.classification==1;}),"positive vehicle-map control has no class1 rows");
  unsigned validations=0;
  MenuState menu(frontend,preferences);
  menu.set_multiplayer_map_validator([&](const FrontendLaunch &launch){
    ++validations;check(launch.map=="lockout.map","selected saved-profile map changed");
    validate_slayer_map_options(level,slayer_rules(launch.variant));
  });
  lobby(menu);menu.select(0);menu.accept();
  check(menu.launch && validations==1,"saved Warthog override still blocks Lockout Start Game");
  const auto &launch=*menu.launch;
  auto override=slayer_rules(launch.variant);
  check(launch.multiplayer && !launch.campaign && launch.local_players==1 && !launch.host_port,
      "one signed-in profile did not retain one local player");
  check(override.score_limit==10 && override.time_ticks==120*30 && override.primary_light_vehicle==1,
      "saved score/time/vehicle option was reset to make the match launch");
  const auto parameter=std::find_if(frontend.parameters.begin(),frontend.parameters.end(),
      [](const auto &item){return item.second.parameter==76;});
  check(parameter!=frontend.parameters.end(),"original Primary Light Vehicle menu is absent");
  MenuState saved(frontend,preferences);saved.open(223,parameter->first);
  check(saved.frame().selected==1,"saved vehicle override was rewritten");
  auto defaults=override;defaults.primary_light_vehicle=0;
  defaults.weapon=override.weapon=lockout.slayer_starting_weapon(override.starting_weapon);
  MultiplayerMatch a(level,0,lockout.spawns(),defaults),b(level,0,lockout.spawns(),override);
  const auto slot_a=a.join("Saved profile"),slot_b=b.join("Saved profile");
  check(slot_a==slot_b,"inert vehicle override changed player allocation");
  std::array<Controls,multiplayer_capacity> controls{};
  controls[slot_a].yaw=a.unit(slot_a).yaw;
  controls[slot_a].fire=true;
  for(unsigned tick=0;tick<180;++tick) {
    a.step(controls);b.step(controls);
    const auto &x=a.unit(slot_a),&y=b.unit(slot_b);
    check(x.source_datum==y.source_datum && x.weapon.source_datum==y.weapon.source_datum &&
        x.weapon.loaded==y.weapon.loaded && x.weapon.reserve==y.weapon.reserve &&
        !std::memcmp(&x.weapon.smg.barrel,&y.weapon.smg.barrel,sizeof(SmgBarrelState)) &&
        x.body.position.x==y.body.position.x && x.body.position.y==y.body.position.y && x.body.position.z==y.body.position.z &&
        a.state().tick==b.state().tick && a.player(slot_a).score==b.player(slot_b).score,
        "inert Lockout override changed actual match firing/state");
  }
  bool rejected=false;
  try {validate_slayer_map_options(vehicles,override);}catch(const FormatError &e){
    rejected=std::string(e.what()).find("Primary Light Vehicle")!=std::string::npos;
  }
  check(rejected,"affected vehicle map silently admitted unfinished substitution");
  validate_slayer_map_options(vehicles,defaults);
  auto invalid=launch.variant;invalid.values.set(76,4);rejected=false;
  try {(void)slayer_rules(invalid);}catch(const FormatError &){rejected=true;}
  check(rejected,"absent original vehicle choice4 was accepted");
  MenuState unavailable(frontend,preferences);
  unavailable.set_multiplayer_map_validator([&](const FrontendLaunch &launch){
    validate_slayer_map_options(vehicles,slayer_rules(launch.variant));
  });
  lobby(unavailable);unavailable.select(0);unavailable.accept();
  check(!unavailable.launch && unavailable.frame().screen!=14 &&
      unavailable.frame().description.find("Primary Light Vehicle")!=std::string::npos,
      "failed map validation escaped the menu instead of showing its named option");
  if(argc==3)check(read(argv[2])==input,"original saved preferences were modified");
  std::cout<<"Saved profile title/sign-in/lobby -> one-player Lockout launch passed; score10/time120/vehicle1 retained.\n"
      <<"180 actual firing ticks match default vehicle state; affected-map and invalid-choice controls rejected.\n";
  return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
