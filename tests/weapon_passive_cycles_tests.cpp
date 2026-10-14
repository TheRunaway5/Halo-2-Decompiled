#include "halo2/multiplayer.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5)throw FormatError("Supply MAP SHARED XBE ORIGINAL_PASSIVE_FIXTURE");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto authored=cache.load_content(0,true);const auto spawns=cache.spawns();
  std::ifstream input(argv[4]);if(!input)return 77;
  std::optional<LevelContent> level;std::unique_ptr<MultiplayerMatch> match;
  unsigned player{},cases{},ticks{};std::uint32_t handle=null_datum;
  std::vector<WeaponEffectInput> calls;
  const auto definition=[&](const std::string &name) {
    const auto found=std::find_if(authored.weapons.begin(),authored.weapons.end(),[&](const auto &w){return w.second.name.ends_with("\\"+name);});
    if(found==authored.weapons.end())throw FormatError("Missing passive actual definition: "+name);
    return found->first;
  };
  const auto state=[&]() -> WeaponState & {
    if(const auto *w=match->weapon_from_datum(handle))return const_cast<WeaponState &>(*w);
    for(const auto &item:match->weapon_pickups())if(item.handle==handle && item.weapon)
      return const_cast<WeaponState &>(*item.weapon);
    throw FormatError("Passive fixture lost its retained full weapon identity");
  };
  for(std::string line;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);char kind;std::string name,context,pattern;row>>kind>>name>>context>>pattern;
    const auto key=name+"/"+context+"/"+pattern;
    if(kind=='I') {
      const auto target=definition(name),smg=definition("smg");
      const auto first=name=="smg"?definition("battle_rifle"):target;
      const auto replacement=first==definition("battle_rifle")?definition("magnum"):definition("battle_rifle");
      match.reset();level=authored;level->netgame_equipment.clear();const auto spawn=spawns.at(0);
      NetgameEquipment pickup;pickup.modes={2,0,0,0};pickup.respawn_seconds=1000;
      pickup.position=spawn.position+Vec3{.2f,0,.4f};pickup.items.push_back({1,0x77656170u,first,0});
      level->netgame_equipment.push_back(pickup);pickup.position=spawn.position+Vec3{.4f,0,.4f};
      pickup.items[0].definition=replacement;level->netgame_equipment.push_back(pickup);
      auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=smg;rules.score_limit=rules.time_ticks=0;
      match=std::make_unique<MultiplayerMatch>(*level,0,spawns,rules);match->configure_weapon_pickups();player=match->join("Passive source clocks");
      for(const auto datum:std::set{smg,first,replacement})match->configure_replicated_first_person(cache.load_first_person_binding(datum,2));
      const auto initial=match->unit(player).weapon.source_datum;
      std::array<Controls,16> controls{};
      const auto teleport=[&]{match->debug_request({DebugGameRequest::Teleport,player},&spawn);};
      const auto warm=[&]{controls[player]={};for(unsigned n=0;n<120;++n)match->step(controls);};
      teleport();match->step(controls);teleport();controls[player].pickup=true;controls[player].pickup_milliseconds=233;match->step(controls);warm();
      if(match->unit(player).weapon.definition!=first)throw FormatError("Actual first passive pickup did not attach");
      handle=name=="smg"?initial:match->unit(player).weapon.source_datum;
      if((context=="holstered" && name!="smg") || (context=="dropped" && name=="smg")) {
        controls[player].switch_weapon=true;match->step(controls);warm();
      }
      if(context=="dropped") {
        teleport();controls[player].pickup=true;controls[player].pickup_milliseconds=233;match->step(controls);warm();
        bool dropped=false;for(const auto &item:match->weapon_pickups())dropped|=item.handle==handle && item.weapon.has_value();
        if(!dropped)throw FormatError("Actual replacement did not drop the passive target");
      } else if(context!="holstered")throw FormatError("Unknown passive fixture ownership");
      if(match->unit(player).weapon.source_datum==handle)throw FormatError("Passive target is still selected");
      std::array<std::uint32_t,24> words;for(auto &word:words)if(!(row>>std::hex>>word))throw FormatError("Truncated original passive initial state");
      auto &w=state();w.ready_countdown=std::int16_t(words[0]);w.ready_clock=true;
      w.shader.heat=std::bit_cast<float>(words[1]);w.shader.power_fraction=std::bit_cast<float>(words[2]);
      w.cooling_inhibitor=std::bit_cast<float>(words[3]);w.shader.weapon_flags=words[4];w.energy_blocking_ticks=std::int16_t(words[5]);
      w.shader.item_flags=context=="holstered"?11u:0u;
      w.magazine=std::bit_cast<WeaponMagazineState>(std::array{words[6],words[7],words[8],words[9]});
      w.loaded=w.magazine.loaded;w.reserve=w.magazine.reserve;w.smg={};w.smg.attached=false;w.smg.hand_timer_registered=false;
      w.smg.loaded=w.magazine.loaded;w.smg.reserve=w.magazine.reserve;w.smg.magazine_mode=w.magazine.mode;w.smg.magazine_ticks=w.magazine.ticks;
      w.smg.transfer_ticks=w.magazine.transfer_ticks;w.smg.magazine_other_ticks=w.magazine.other_ticks;
      w.smg.weapon_flags=w.shader.weapon_flags;w.smg.blocking_ticks=w.energy_blocking_ticks;
      std::array<std::uint32_t,13> barrel;std::copy_n(words.begin()+10,13,barrel.begin());w.smg.barrel=std::bit_cast<SmgBarrelState>(barrel);
      std::uint32_t auxiliary{};match->source_effect_random(auxiliary).primary=words[23];
      match->configure_weapon_effect_owner([&](const Unit &,const WeaponEffectInput &effect){calls.push_back(effect);return null_datum;});
      match->configure_world_weapon_effect_owner([&](const WeaponEffectInput &effect){calls.push_back(effect);return null_datum;});
      ++cases;continue;
    }
    if(kind!='T')throw FormatError("Unknown original passive fixture row");
    std::array<std::uint32_t,26> expected;for(auto &word:expected)if(!(row>>std::hex>>word))throw FormatError("Truncated original passive clock row");
    calls.clear();match->step(std::array<Controls,16>{});const auto &w=state();
    const auto &d=authored.weapons.at(w.definition);
    const std::array<std::uint32_t,6> clocks{std::uint16_t(w.ready_countdown),std::bit_cast<std::uint32_t>(w.shader.heat),
        std::bit_cast<std::uint32_t>(w.shader.power_fraction),std::bit_cast<std::uint32_t>(w.cooling_inhibitor),w.shader.weapon_flags,
        std::uint16_t(d.smg?w.smg.blocking_ticks:w.energy_blocking_ticks)};
    for(unsigned n=0;n<clocks.size();++n)if(clocks[n]!=expected[n+1]) {
      std::cerr<<key<<" tick="<<expected[0]<<" clock="<<n<<" expected="<<std::hex<<expected[n+1]<<" actual="<<clocks[n]<<'\n';
      throw FormatError("Retained passive weapon clock differs from original complete update");
    }
    auto magazine=w.magazine;if(d.smg)magazine={w.smg.magazine_mode,w.smg.magazine_ticks,0,w.smg.reserve,w.smg.loaded,0,w.smg.transfer_ticks,w.smg.magazine_other_ticks};
    const auto raw=std::bit_cast<std::array<std::uint32_t,4>>(magazine);
    for(unsigned n=0;n<4;++n)if(raw[n]!=expected[n+7])throw FormatError("Passive magazine differs at "+key+" tick "+std::to_string(expected[0]));
    const auto barrel=std::bit_cast<std::array<std::uint32_t,13>>(w.smg.barrel);
    for(unsigned n=0;n<13;++n)if(barrel[n]!=expected[n+11]) {
      std::cerr<<key<<" tick="<<expected[0]<<" barrel word="<<n<<" expected="<<std::hex<<expected[n+11]<<" actual="<<barrel[n]<<'\n';
      throw FormatError("Passive complete barrel differs from original");
    }
    std::uint32_t auxiliary{};if(match->source_effect_random(auxiliary).primary!=expected[24])throw FormatError("Passive clocks changed shared RNG");
    if(calls.size()!=expected[25])throw FormatError("Passive effect count differs at "+key+" tick "+std::to_string(expected[0]));
    for(const auto &effect:calls) {
      std::uint32_t tag,previous,a,b;if(!(row>>tag>>previous>>a>>b) || effect.effect!=tag || effect.previous_handle!=previous || effect.a_bits!=a || effect.b_bits!=b)
        throw FormatError("Passive effect ABI differs from original");
    }
    ++ticks;
  }
  if(cases!=90 || ticks!=16200)throw FormatError("Incomplete passive weapon coverage");
  std::cout<<"PASS "<<cases<<" original passive scenarios, "<<ticks<<" ticks: ready/heat/blocking clocks, magazines, all52 barrel bytes and effect ABI exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
