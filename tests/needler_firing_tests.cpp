#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {void check(bool b,const char *why){if(!b)throw FormatError(why);}}
int main(int argc,char **argv)try {
  check(argc==5,"Supply MAP SHARED XBE ORIGINAL_FIXTURE");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));const auto content=cache.load_content(0,true);
  const auto weapon=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &p){return p.second.weapon_type==2;});
  check(weapon!=content.weapons.end() && weapon->second.firing_presentation.has_value(),"Actual Needler firing branch absent");
  auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=weapon->first;rules.score_limit=rules.time_ticks=0;
  MultiplayerMatch match(content,0,cache.spawns(),rules);const auto id=match.join("Needler original effect comparison");
  std::vector<WeaponEffectInput> calls;
  match.configure_weapon_effect_owner([&](const Unit &u,const WeaponEffectInput &i){
    check(u.source_datum==match.unit(id).source_datum && i.weapon==u.weapon.source_datum,"Needler effect lost actual Unit/weapon identity");
    calls.push_back(i);return null_datum; // Explicit unavailable allocation, matching independent original boundary.
  });
  std::ifstream fixture(argv[4]);check(bool(fixture),"Missing independent Needler firing fixture");
  std::array<Controls,16> input{};unsigned rows{},requests{};
  for(std::string line;std::getline(fixture,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);row>>std::hex;
    std::uint32_t tick,fire,loaded,rate,count;row>>tick>>fire>>loaded>>rate>>count;
    calls.clear();input[id].fire=fire;match.step(input);
    const auto &state=match.unit(id).weapon;
    if(state.loaded!=int(loaded) || std::bit_cast<std::uint32_t>(state.smg.barrel.rate)!=rate || calls.size()!=count)
      throw FormatError("Needler prepared effect state differs from original at tick "+std::to_string(tick));
    for(const auto &call:calls) {
      std::uint32_t effect,a,b;row>>effect>>a>>b;
      check(call.effect==effect && call.a_bits==a && call.b_bits==b && call.previous_handle==null_datum,
          "Needler prepared-effect identity/scalars differ from original107160");++requests;
    }
    check(bool(row),"Truncated original Needler fixture");++rows;
  }
  check(rows==100 && requests==14,"Independent Needler firing cases omitted");
  std::cout<<"PASS "<<rows<<" actual Needler input/tick states and "<<requests<<" prepared effect requests/scalars match unchanged original1021D0/107160\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
