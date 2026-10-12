#include "halo2/native_unit_interaction.hpp"
#include "halo2/content.hpp"
#include "halo2/hud.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try{Cache cache(argv[1]);auto c=cache.load_content(0);auto hud=cache.load_hud(c.player.datum,c.starting_weapon);for(const auto &[id,d]:c.unit_interactions->units){for(const auto &s:d.seats)for(const auto msg:{s.entry_message,s.grenade_message,s.melee_message})if(msg)std::cout<<cache.string_id(s.mode)<<" msg="<<std::hex<<msg<<std::dec<<" "<<(hud.strings.contains(msg)?hud.strings.at(msg):"MISSING")<<'\n';}return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
