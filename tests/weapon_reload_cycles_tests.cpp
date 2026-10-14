#include "halo2/multiplayer.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5 && argc!=6)throw FormatError("Supply MAP SHARED XBE ORIGINAL_RELOAD_FIXTURE [REPRESENTATION]");
  const unsigned representation=argc==6?unsigned(std::stoul(argv[5])):2u;
  if(representation!=2 && representation!=3)throw FormatError("Unsupported original multiplayer representation");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto content=cache.load_content(0,true);
  std::ifstream input(argv[4]);if(!input)return 77;
  std::unique_ptr<MultiplayerMatch> match;std::vector<WeaponEffectInput> calls;
  std::string prior,line;unsigned player{},cases{},ticks{},requests{};
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::string name,pattern;unsigned local;
    row>>name>>local>>pattern;std::array<std::uint32_t,21> expected;
    for(auto &word:expected)if(!(row>>std::hex>>word))throw FormatError("Truncated original reload row");
    const auto key=name+"/"+std::to_string(local)+"/"+pattern;
    if(key!=prior) {
      const auto weapon=std::find_if(content.weapons.begin(),content.weapons.end(),[&](const auto &entry) {
        return entry.second.name.ends_with("\\"+name);
      });
      if(weapon==content.weapons.end() || !weapon->second.barrel_timing || !weapon->second.magazine)
        throw FormatError("Missing original reload definition: "+name);
      auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=weapon->first;
      rules.score_limit=rules.time_ticks=0;
      match=std::make_unique<MultiplayerMatch>(content,0,cache.spawns(),rules);
      std::optional<SourcePlayerConfiguration> profile;
      if(representation==3){profile=source_default_player_configuration();(*profile)[0x44]=1;}
      player=match->join("Original reload cycle",-1,profile);
      if(match->unit(player).definition.datum!=content.player_representations.at(representation).biped.datum)
        throw FormatError("Reload fixture did not select its actual represented biped");
      match->configure_replicated_first_person(cache.load_first_person_binding(weapon->first,int(representation)));
      if(local) {
        const std::array<unsigned,1> views{player};
        const std::array definitions{cache.load_first_person_binding(weapon->first,int(representation))};
        match->configure_local_views(views,definitions);
      }
      auto &state=const_cast<WeaponState &>(match->unit(player).weapon);
      state.loaded=pattern=="empty_idle"?0:pattern=="empty_auto"?2:pattern=="last_round"?1:pattern=="interrupt"?
          std::max(0,weapon->second.capacity-2):weapon->second.capacity;
      state.reserve=4;
      match->configure_weapon_effect_owner([&](const Unit &unit,const WeaponEffectInput &effect) {
        if(effect.weapon!=unit.weapon.source_datum)throw FormatError("Reload effect lost full weapon identity");
        calls.push_back(effect);return null_datum;
      });
      prior=key;++cases;
    }
    calls.clear();std::array<Controls,16> controls{};
    controls[player].fire=expected[1];controls[player].reload=expected[2];match->step(controls);
    const auto &state=match->unit(player).weapon;
    const auto magazine=std::bit_cast<std::array<std::uint32_t,4>>(state.magazine);
    for(unsigned n=0;n<4;++n)if(magazine[n]!=expected[n+3]) {
      std::cerr<<key<<" tick="<<expected[0]<<" magazine word="<<n<<" expected="<<std::hex<<expected[n+3]
          <<" actual="<<magazine[n]<<std::dec<<" loaded="<<state.loaded<<" reserve="<<state.reserve<<'\n';
      throw FormatError("Live magazine update differs from original complete weapon cycle");
    }
    const auto &barrel=state.smg.barrel;
    const std::array<unsigned,8> words{std::uint8_t(barrel.counter),barrel.mode,std::uint16_t(barrel.ticks),barrel.shot_count,
      std::bit_cast<std::uint32_t>(barrel.rate),std::bit_cast<std::uint32_t>(barrel.recovery),
      std::bit_cast<std::uint32_t>(barrel.error),std::bit_cast<std::uint32_t>(barrel.angle_change)};
    const auto raw=std::bit_cast<std::array<std::uint8_t,52>>(std::array<std::uint32_t,13>{
        expected[7],expected[8],expected[9],expected[10],expected[11],expected[12],expected[13],
        expected[14],expected[15],expected[16],expected[17],expected[18],expected[19]});
    const std::array<unsigned,8> wanted{raw[0],raw[1],unsigned(raw[2])|(unsigned(raw[3])<<8),
        unsigned(raw[6])|(unsigned(raw[7])<<8),expected[11],expected[17],expected[14],expected[15]};
    for(unsigned n=0;n<words.size();++n)if(words[n]!=wanted[n]) {
      std::cerr<<key<<" tick="<<expected[0]<<" barrel field="<<n<<" expected="<<std::hex<<wanted[n]<<" actual="<<words[n]<<'\n';
      throw FormatError("Live barrel chronology differs during original reload cycle");
    }
    if(state.loaded!=state.magazine.loaded || state.reserve!=state.magazine.reserve)
      throw FormatError("Live reload lost authoritative ammo publication");
    if(calls.size()!=expected[20]) {
      std::cerr<<key<<" tick="<<expected[0]<<" effect requests expected="<<expected[20]<<" actual="<<calls.size()<<'\n';
      throw FormatError("Reload/firing effect count differs from original");
    }
    for(const auto &effect:calls) {
      std::uint32_t tag,a,b;
      if(!(row>>tag>>a>>b) || tag!=effect.effect || a!=effect.a_bits || b!=effect.b_bits)
        throw FormatError("Reload/firing effect tag/scalars differ from original");
      ++requests;
    }
    ++ticks;
  }
  if(cases!=96 || ticks!=24512 || requests!=(representation==2?344u:346u))throw FormatError("Incomplete original reload coverage");
  std::cout<<"PASS "<<cases<<" original reload scenarios, "<<ticks<<" ticks, "<<requests
      <<" effect requests: all16 magazine bytes, barrel chronology and published ammo exact\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
