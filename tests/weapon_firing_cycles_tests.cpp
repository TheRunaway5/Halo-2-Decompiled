#include "halo2/multiplayer.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5)throw FormatError("Supply MAP SHARED XBE ORIGINAL_FIXTURE");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto content=cache.load_content(0,true);
  std::ifstream input(argv[4]);if(!input)return 77;
  std::unique_ptr<MultiplayerMatch> match;
  std::vector<WeaponEffectInput> calls;
  std::string prior,line;unsigned player{},cases{},ticks{},requests{};
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::string name,pattern;row>>name>>pattern;
    std::array<std::uint32_t,9> expected;
    for(auto &v:expected)if(!(row>>std::hex>>v))throw FormatError("Truncated original firing row");
    const auto key=name+"/"+pattern;
    if(key!=prior) {
      const auto weapon=std::find_if(content.weapons.begin(),content.weapons.end(),[&](const auto &entry) {
        return entry.second.name.substr(entry.second.name.find_last_of('\\')+1)==name;
      });
      if(weapon==content.weapons.end() || !weapon->second.barrel_timing)
        throw FormatError("Missing original ordinary barrel timing: "+name);
      auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=weapon->first;
      rules.score_limit=rules.time_ticks=0;
      match=std::make_unique<MultiplayerMatch>(content,0,cache.spawns(),rules);
      player=match->join("Original firing cycle");prior=key;++cases;
      match->configure_weapon_effect_owner([&](const Unit &unit,const WeaponEffectInput &input) {
        if(input.weapon!=unit.weapon.source_datum)throw FormatError("Prepared effect lost its actual weapon owner");
        calls.push_back(input);return null_datum; // Same explicit unavailable allocation as the original fixture.
      });
    }
    calls.clear();
    std::array<Controls,16> controls{};controls[player].fire=expected[1]!=0;
    match->step(controls);
    const auto &weapon=match->unit(player).weapon;
    const auto &barrel=weapon.smg.barrel;
    const std::array<std::uint32_t,7> actual{std::uint32_t(weapon.loaded),
      std::uint8_t(barrel.counter),barrel.mode,std::uint16_t(barrel.ticks),barrel.shot_count,
      std::bit_cast<std::uint32_t>(barrel.rate),std::bit_cast<std::uint32_t>(barrel.recovery)};
    for(unsigned n=0;n<actual.size();++n)if(actual[n]!=expected[n+2]) {
      std::cerr<<key<<" tick="<<expected[0]<<" field="<<n<<" expected="<<std::hex
          <<expected[n+2]<<" actual="<<actual[n]<<'\n';
      throw FormatError("Native firing cycle differs from original source timeline");
    }
    std::uint32_t count;
    if(!(row>>count) || calls.size()!=count)
      throw FormatError("Prepared effect request count differs at "+key+" tick "+std::to_string(expected[0]));
    for(const auto &call:calls) {
      std::uint32_t effect,a,b;
      if(!(row>>effect>>a>>b) || call.effect!=effect || call.a_bits!=a || call.b_bits!=b ||
          call.previous_handle!=null_datum)
        throw FormatError("Prepared effect identity/scalars differ at "+key+" tick "+std::to_string(expected[0]));
      ++requests;
    }
    ++ticks;
  }
  if(cases!=24)throw FormatError("Original firing scenario coverage incomplete");
  std::cout<<"PASS "<<cases<<" actual-asset firing scenarios, "<<ticks
      <<" ticks, "<<requests<<" prepared requests: ammunition, barrel timing and actual effect/scalars exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
