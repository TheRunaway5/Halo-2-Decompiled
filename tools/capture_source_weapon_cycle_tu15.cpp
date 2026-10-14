#include "halo2/multiplayer_world.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char *message) {if(!ok)throw FormatError(message);}
std::uint32_t word(SourceRuntimeMemory &memory,NativeRuntimeAddress address,unsigned count=4) {
  const auto bytes=memory.view(address,count);std::uint32_t value{};
  for(unsigned i=0;i<count;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
  return value;
}
}

// Normal match inputs must reach the retained original weapon. Native shadow
// ammo, native effects or an independently requested source effect cannot pass.
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  check(argc==5,"Supply MAP SHARED TU15 OUTPUT_DIRECTORY");
  MultiplayerOptions options;options.map=argv[1];options.shared_maps=argv[2];
  options.gameplay_xbe=argv[3];options.starting_weapon=5;
  options.session_random_seed=0x12345678u;
  options.source_object_savegame_directory=std::filesystem::absolute(argv[4])/"source-savegames";
  options.defer_initial_player_spawn=true;
  std::vector<std::uint32_t> players;
  options.player_birth_owner=[&](MultiplayerPlayer &p,unsigned) {
    players.push_back(p.source_datum);p.source_flags=8;
  };
  options.pre_player_update_owner=[&](MultiplayerMatch &match) {
    for(const auto datum:players) {
      const auto *p=match.player_from_datum(datum);check(p,"Source weapon player disappeared");
      if(!(p->source_flags&1u))match.apply_source_player_activation(datum,p->source_flags,p->source_flags|1u);
    }
  };
  MultiplayerPreparedWorld world(options);auto &match=world.match;
  check(world.source_objects && world.source_objects->owns_physics(),"TU1.5 source world was not admitted");
  auto &memory=world.source_objects->memory();std::array<Controls,16> controls{};
  unsigned loaded_before{},loaded_after{},retained_trigger_frames{};
  std::uint32_t weapon=null_datum;
  for(unsigned tick=0;tick<120;++tick) {
    controls[0].fire=tick>=80 && tick<92;
    match.step(controls);
    const auto &unit=match.unit(0);
    if(unit.source_datum==null_datum)continue;
    const auto body=world.source_objects->object_body(unit.source_datum);
    if(tick==79) {
      weapon=unit.weapon.source_datum;
      check(weapon!=null_datum,"Source player has no actual weapon");
      const auto weapon_body=world.source_objects->object_body(weapon);
      loaded_before=word(memory,weapon_body+0x22cu,2);
      check(loaded_before>0,"Original SMG has no loaded rounds before input");
    }
    if(tick>=80 && tick<92)
      retained_trigger_frames+=word(memory,body+0x1c0u)==std::bit_cast<std::uint32_t>(1.f);
    if(tick==119) {
      check(unit.weapon.source_datum==weapon,"Source input unexpectedly changed weapon identity");
      loaded_after=word(memory,world.source_objects->object_body(weapon)+0x22cu,2);
    }
  }
  std::cout<<"original_loaded_before="<<loaded_before<<" original_loaded_after="<<loaded_after
    <<" retained_primary_trigger_frames="<<retained_trigger_frames<<'\n';
  check(retained_trigger_frames==12,"Normal primary input did not reach the original unit trigger");
  check(loaded_after<loaded_before,"Normal primary input did not consume original weapon ammunition");
  std::cout<<"PASS normal primary input reaches actual TU1.5 unit and weapon\n";
  return 0;
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
