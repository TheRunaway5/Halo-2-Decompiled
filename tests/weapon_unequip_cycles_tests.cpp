#include "helpers/gameplay_fixture.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5)throw FormatError("Supply MAP SHARED XBE ORIGINAL_UNEQUIP_FIXTURE");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto authored=cache.load_content(0,true);const auto spawns=cache.spawns();
  std::ifstream input(argv[4]);if(!input)return 77;
  std::optional<LevelContent> level;std::unique_ptr<MultiplayerMatch> match;
  unsigned player{},cases{},ticks{},case_tick{};std::uint32_t handle=null_datum;
  const auto definition=[&](const std::string &name) {
    const auto found=std::find_if(authored.weapons.begin(),authored.weapons.end(),[&](const auto &w){return w.second.name.ends_with("\\"+name);});
    if(found==authored.weapons.end())throw FormatError("Missing unequip actual definition: "+name);
    return found->first;
  };
  const auto state=[&]() -> WeaponState & {
    if(const auto *w=match->weapon_from_datum(handle))return const_cast<WeaponState &>(*w);
    for(const auto &item:match->weapon_pickups())if(item.handle==handle && item.weapon)return const_cast<WeaponState &>(*item.weapon);
    throw FormatError("Unequip lost the actual retained full weapon identity");
  };
  for(std::string line;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);char kind;std::string name,context,pattern;unsigned representation;
    row>>kind>>name>>representation>>context>>pattern;
    const auto key=name+"/"+std::to_string(representation)+"/"+context+"/"+pattern;
    std::array<std::uint32_t,42> words;for(auto &word:words)if(!(row>>std::hex>>word))throw FormatError("Truncated original unequip state");
    if(kind=='I') {
      const auto target=definition(name),smg=definition("smg");
      const auto first=name=="smg"?definition("battle_rifle"):target;
      match.reset();level=authored;level->netgame_equipment.clear();const auto spawn=spawns.at(0);
      NetgameEquipment pickup;pickup.modes={2,0,0,0};pickup.respawn_seconds=1000;
      pickup.position=spawn.position+Vec3{.2f,0,.4f};pickup.items.push_back({1,0x77656170u,first,0});level->netgame_equipment.push_back(pickup);
      pickup.position=spawn.position+Vec3{.4f,0,.4f};pickup.items[0].definition=first==definition("battle_rifle")?definition("magnum"):definition("battle_rifle");
      level->netgame_equipment.push_back(pickup);
      auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=smg;rules.score_limit=rules.time_ticks=0;
      match=std::make_unique<MultiplayerMatch>(*level,0,spawns,rules);match->configure_weapon_pickups();
      std::optional<SourcePlayerConfiguration> profile;
      if(representation==3){profile=source_default_player_configuration();(*profile)[0x44]=1;}
      player=match->join("Original clear caller",-1,profile);
      for(const auto datum:std::set{smg,first,pickup.items[0].definition})match->configure_replicated_first_person(cache.load_first_person_binding(datum,int(representation)));
      std::array<Controls,16> controls{};
      test::place_player(*match,player,spawn);match->step(controls);
      test::place_player(*match,player,spawn);controls[player].pickup=true;controls[player].pickup_milliseconds=233;match->step(controls);
      controls[player]={};for(unsigned n=0;n<120;++n)match->step(controls);
      if(name=="smg"){controls[player].switch_weapon=true;match->step(controls);controls[player]={};for(unsigned n=0;n<120;++n)match->step(controls);}
      if(match->unit(player).weapon.definition!=target)throw FormatError("Original clear fixture target did not equip");
      if(match->unit(player).definition.datum!=authored.player_representations.at(representation).biped.datum)throw FormatError("Unequip lost its represented biped");
      handle=match->unit(player).weapon.source_datum;auto &w=state();w.ready_countdown=std::int16_t(words[0]);w.ready_clock=true;
      w.shader.heat=std::bit_cast<float>(words[1]);w.shader.power_fraction=std::bit_cast<float>(words[2]);w.cooling_inhibitor=std::bit_cast<float>(words[3]);
      w.shader.weapon_flags=words[4];w.energy_blocking_ticks=std::int16_t(words[5]);w.shader.item_flags=9;
      w.magazine=std::bit_cast<WeaponMagazineState>(std::array{words[6],words[7],words[8],words[9]});w.loaded=w.magazine.loaded;w.reserve=w.magazine.reserve;
      w.smg={};w.smg.attached=true;w.smg.hand_timer_registered=false;w.smg.loaded=w.magazine.loaded;w.smg.reserve=w.magazine.reserve;
      w.smg.magazine_mode=w.magazine.mode;w.smg.magazine_ticks=w.magazine.ticks;w.smg.transfer_ticks=w.magazine.transfer_ticks;w.smg.magazine_other_ticks=w.magazine.other_ticks;
      w.smg.weapon_flags=w.shader.weapon_flags;w.smg.blocking_ticks=w.energy_blocking_ticks;
      std::array<std::uint32_t,13> barrel;std::copy_n(words.begin()+10,13,barrel.begin());w.smg.barrel=std::bit_cast<SmgBarrelState>(barrel);
      std::copy_n(words.begin()+23,13,barrel.begin());w.alternate_barrel=std::bit_cast<SmgBarrelState>(barrel);
      w.smg.trigger_state=std::uint8_t(words[36]);w.smg.trigger_ticks=std::int16_t(words[37]);w.smg.trigger_flags=words[38];
      w.smg.input_flags=std::uint16_t(words[39]);w.smg.weapon_state=words[40];
      std::uint32_t auxiliary{};match->source_effect_random(auxiliary).primary=words[41];case_tick=0;++cases;continue;
    }
    if(kind!='T')throw FormatError("Unknown original unequip fixture row");
    std::array<Controls,16> controls{};
    if(case_tick==0) {
      if(context=="holstered")controls[player].switch_weapon=true;
      else if(context=="dropped") {
        test::place_player(*match,player,spawns.at(0));
        controls[player].pickup=true;controls[player].pickup_milliseconds=233;
      } else throw FormatError("Unknown unequip ownership context");
    }
    match->step(controls);const auto &w=state();const auto &d=authored.weapons.at(w.definition);
    if(match->unit(player).weapon.source_datum==handle || bool(w.shader.item_flags&2u)!=(context=="holstered"))throw FormatError("Actual clear did not holster its target");
    std::array<std::uint32_t,42> actual{std::uint16_t(w.ready_countdown),std::bit_cast<std::uint32_t>(w.shader.heat),std::bit_cast<std::uint32_t>(w.shader.power_fraction),
      std::bit_cast<std::uint32_t>(w.cooling_inhibitor),w.shader.weapon_flags,std::uint16_t(d.smg?w.smg.blocking_ticks:w.energy_blocking_ticks)};
    const auto magazine=std::bit_cast<std::array<std::uint32_t,4>>(w.magazine);std::copy(magazine.begin(),magazine.end(),actual.begin()+6);
    auto barrel=std::bit_cast<std::array<std::uint32_t,13>>(w.smg.barrel);std::copy(barrel.begin(),barrel.end(),actual.begin()+10);
    if(d.barrel_ammunition.size()==2){barrel=std::bit_cast<std::array<std::uint32_t,13>>(w.alternate_barrel);std::copy(barrel.begin(),barrel.end(),actual.begin()+23);}
    actual[36]=w.smg.trigger_state;actual[37]=std::uint16_t(w.smg.trigger_ticks);actual[38]=w.smg.trigger_flags;actual[39]=w.smg.input_flags;actual[40]=w.smg.weapon_state;
    std::uint32_t auxiliary{};actual[41]=match->source_effect_random(auxiliary).primary;
    for(unsigned n=0;n<words.size();++n)if(words[n]!=actual[n]) {
      std::cerr<<key<<" tick="<<case_tick<<" word="<<n<<" expected="<<std::hex<<words[n]<<" actual="<<actual[n]<<'\n';throw FormatError("Native clear and subsequent clocks differ from original E9A90");
    }
    ++ticks;++case_tick;
  }
  if(cases!=180 || ticks!=10800)throw FormatError("Incomplete original clear caller coverage");
  std::cout<<"PASS "<<cases<<" original single-hand clear/post-detach scenarios, "<<ticks<<" ticks, both representations; magazine, all retained barrel bytes, input/trigger and clock words exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
