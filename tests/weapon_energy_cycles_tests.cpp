#include "helpers/gameplay_fixture.hpp"
#include "halo2/multiplayer.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5 && argc!=6)throw FormatError("Supply MAP SHARED XBE ORIGINAL_FIXTURE [REPRESENTATION]");
  const unsigned representation=argc==6?unsigned(std::stoul(argv[5])):2u;
  if(representation!=2 && representation!=3)throw FormatError("Unsupported original multiplayer representation");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]))return 77;
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto authored=cache.load_content(0,true);const auto spawns=cache.spawns();
  std::ifstream input(argv[4]);if(!input)return 77;
  std::optional<LevelContent> level;
  std::unique_ptr<MultiplayerMatch> match;std::vector<WeaponEffectInput> calls;
  std::string prior,line;unsigned player{},cases{},ticks{},requests{};
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::string name,pattern;unsigned local;
    row>>name>>local>>pattern;std::array<std::uint32_t,15> expected;
    for(auto &value:expected)if(!(row>>std::hex>>value))throw FormatError("Truncated original energy cycle");
    const auto key=name+"/"+std::to_string(local)+"/"+pattern;
    if(key!=prior) {
      const auto weapon=std::find_if(authored.weapons.begin(),authored.weapons.end(),[&](const auto &entry) {
        return entry.second.name.ends_with("\\"+name);
      });
      if(weapon==authored.weapons.end() || !weapon->second.energy || !weapon->second.barrel_timing || !weapon->second.firing_presentation)
        throw FormatError("Actual energy definition has no recovered timing/effect bank: "+name);
      match.reset();level=authored;auto &content=*level;content.netgame_equipment.clear();
      // Declared placement isolates the same actual pickup/attachment owner
      // for all three resident definitions, independently of map spawn rules.
      Spawn placement=spawns.at(0);
      NetgameEquipment pickup;pickup.modes={2,0,0,0};pickup.position=placement.position+Vec3{.2f,0,.4f};
      pickup.items.push_back({1,0x77656170u,weapon->first,0});content.netgame_equipment.push_back(pickup);
      auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.score_limit=rules.time_ticks=0;
      const auto smg=std::find_if(authored.weapons.begin(),authored.weapons.end(),[](const auto &entry) {
        return entry.second.smg.has_value();
      });
      if(smg==authored.weapons.end())throw FormatError("Actual initial SMG is unavailable");
      rules.weapon=smg->first;rules.random_seed=0x12345678u;
      match=std::make_unique<MultiplayerMatch>(content,0,spawns,rules);match->configure_weapon_pickups();
      std::optional<SourcePlayerConfiguration> profile;
      if(representation==3){profile=source_default_player_configuration();(*profile)[0x44]=1;}
      player=match->join("Original energy cycle",-1,profile);
      if(match->unit(player).definition.datum!=content.player_representations.at(representation).biped.datum)
        throw FormatError("Energy fixture did not select its actual represented biped");
      match->configure_replicated_first_person(cache.load_first_person_binding(weapon->first,int(representation)));
      if(local) {
        const std::array<unsigned,1> views{player};
        const std::array definitions{cache.load_first_person_binding(rules.weapon,int(representation)),
                                    cache.load_first_person_binding(weapon->first,int(representation))};
        match->configure_local_views(views,definitions);
      }
      std::array<Controls,16> idle{};
      test::place_player(*match,player,placement);match->step(idle);
      test::place_player(*match,player,placement);
      idle[player].pickup=true;idle[player].pickup_milliseconds=233;match->step(idle);
      if(match->unit(player).weapon.definition!=weapon->first)throw FormatError("Declared energy pickup did not attach: "+name);
      idle[player]={};
      for(unsigned n=0;n<120 && !(match->unit(player).weapon.shader.weapon_flags&0x20u);++n)match->step(idle);
      if(!(match->unit(player).weapon.shader.weapon_flags&0x20u))throw FormatError("Energy pickup ready owner did not complete");
      // Same declared post-constructor spent control as the original fixture.
      auto &state=const_cast<WeaponState &>(match->unit(player).weapon);
      state.shader.power_fraction=pattern=="exhaustion"?0.99f:0.f;
      std::uint32_t auxiliary{};
      match->source_effect_random(auxiliary).primary=0x12345678u;
      if(state.shader.heat!=0.f || !(state.shader.item_flags&8u))throw FormatError("Energy pickup lost cold/player-owned state");
      match->configure_weapon_effect_owner([&](const Unit &unit,const WeaponEffectInput &effect) {
        if(effect.weapon!=unit.weapon.source_datum)throw FormatError("Energy request lost its full weapon identity");
        if(effect.effect!=null_datum)calls.push_back(effect);
        return null_datum; // Same declared unavailable generic effect allocation.
      });
      prior=key;++cases;
    }
    calls.clear();std::array<Controls,16> controls{};controls[player].fire=expected[1]!=0;match->step(controls);
    const auto &state=match->unit(player).weapon;const auto &barrel=state.smg.barrel;
    std::uint32_t auxiliary{};
    const std::array<std::uint32_t,12> actual{std::bit_cast<std::uint32_t>(state.shader.heat),
      std::bit_cast<std::uint32_t>(state.shader.power_fraction),state.shader.weapon_flags&6u,
      std::uint8_t(barrel.counter),barrel.mode,std::uint16_t(barrel.ticks),barrel.shot_count,
      std::bit_cast<std::uint32_t>(barrel.rate),std::bit_cast<std::uint32_t>(barrel.recovery),
      std::bit_cast<std::uint32_t>(barrel.error),std::bit_cast<std::uint32_t>(barrel.angle_change),
      match->source_effect_random(auxiliary).primary};
    for(unsigned n=0;n<actual.size();++n)if(actual[n]!=expected[n+2]) {
      std::cerr<<key<<" tick="<<expected[0]<<" field="<<n<<" expected="<<std::hex<<expected[n+2]<<" actual="<<actual[n]<<'\n';
      throw FormatError("Native energy cycle differs from original source timeline");
    }
    if(calls.size()!=expected[14])throw FormatError("Energy effect count differs at "+key+" tick "+std::to_string(expected[0]));
    for(const auto &effect:calls) {
      std::uint32_t tag,a,b;if(!(row>>tag>>a>>b) || effect.effect!=tag || effect.a_bits!=a || effect.b_bits!=b)
        throw FormatError("Energy firing effect/scalars differ at "+key+" tick "+std::to_string(expected[0]));
      ++requests;
    }
    ++ticks;
  }
  if(cases!=18 || ticks!=8130 || requests!=450)throw FormatError("Original energy scenario coverage incomplete");
  std::cout<<"PASS "<<cases<<" actual-asset energy scenarios, "<<ticks<<" ticks, "<<requests
      <<" prepared requests: heat, spent, overheat/vent flags, barrel timing and actual effect/scalars exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
