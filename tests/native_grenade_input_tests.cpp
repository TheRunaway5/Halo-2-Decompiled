#include "halo2/multiplayer.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=4)throw FormatError("Supply MAP SHARED TU15");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto content=cache.load_content(0,true);auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});
  rules.weapon=cache.slayer_starting_weapon(5);rules.score_limit=rules.time_ticks=0;
  MultiplayerMatch match(content,0,cache.spawns(),rules);const auto id=match.join("Grenade selector");
  // Declared native inventory input to the native adapter, not an original
  //world snapshot or a source pickup/allocation claim.
  auto &unit=const_cast<Unit &>(match.unit(id));unit.grenades={2,2};unit.grenade_current=unit.grenade_requested=0;
  std::array<Controls,16> controls{};controls[id].switch_grenade=true;match.step(controls);
  if(match.unit(id).grenade_current!=1)throw FormatError("Switch grenade input never selected plasma");
  controls[id].switch_grenade=false;match.step(controls);
  if(match.unit(id).grenade_current!=1)throw FormatError("Neutral input reset selected plasma");
  controls[id].switch_grenade=true;match.step(controls);
  if(match.unit(id).grenade_current!=0)throw FormatError("Second switch grenade pulse never selected frag");
  std::cout<<"PASS actual native selector input and retained selection; explicit mixed native inventory fixture\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
