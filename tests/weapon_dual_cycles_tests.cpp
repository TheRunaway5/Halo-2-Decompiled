#include "halo2/multiplayer.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5 && argc!=6)throw FormatError("Supply MAP SHARED XBE ORIGINAL_DUAL_FIXTURE [smg]");
  const bool smg_suite=argc==6 && std::string(argv[5])=="smg";
  const bool trigger_suite=argc==6 && std::string(argv[5])=="registered-trigger";
  const bool registered=trigger_suite || (argc==6 && std::string(argv[5])=="registered");
  if(argc==6 && !smg_suite && !registered)throw FormatError("Unknown dual fixture family");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto authored=cache.load_content(0,true);const auto spawns=cache.spawns();
  std::ifstream input(argv[4]);if(!input)return 77;
  std::optional<LevelContent> level;std::unique_ptr<MultiplayerMatch> match;
  std::string prior,line;unsigned player{},cases{},ticks{},requests{};
  std::vector<WeaponEffectInput> calls;
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::string name,pattern;unsigned representation;
    row>>name>>representation>>pattern;
    if(smg_suite && name!="smg")throw FormatError("SMG fixture contains another definition");
    std::vector<std::uint32_t> expected(trigger_suite?54u:registered?48u:46u);
    for(auto &value:expected)if(!(row>>std::hex>>value))throw FormatError("Truncated original dual record");
    const auto key=name+"/"+std::to_string(representation)+"/"+pattern;
    if(key!=prior) {
      const auto weapon=std::find_if(authored.weapons.begin(),authored.weapons.end(),[&](const auto &w) {
        return w.second.name.ends_with("\\"+name);
      });
      if(weapon==authored.weapons.end() || !weapon->second.dual_wieldable)
        throw FormatError("Missing actual dual definition: "+name);
      match.reset();level=authored;level->netgame_equipment.clear();
      const auto spawn=spawns.at(0);NetgameEquipment pickup;
      pickup.modes={2,0,0,0};pickup.respawn_seconds=1000;pickup.position=spawn.position+Vec3{.2f,0,.4f};
      pickup.items.push_back({1,0x77656170u,weapon->first,0});level->netgame_equipment.push_back(pickup);
      auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.weapon=weapon->first;rules.score_limit=rules.time_ticks=0;
      const bool primary_pickup=weapon->second.capacity<=0;
      if(primary_pickup) {
        const auto smg=std::find_if(authored.weapons.begin(),authored.weapons.end(),[](const auto &w){return w.second.smg.has_value();});
        if(smg==authored.weapons.end())throw FormatError("Missing actual initial SMG");
        rules.weapon=smg->first;pickup.position=spawn.position+Vec3{.4f,0,.4f};level->netgame_equipment.push_back(pickup);
      }
      match=std::make_unique<MultiplayerMatch>(*level,0,spawns,rules);match->configure_weapon_pickups();
      std::optional<SourcePlayerConfiguration> profile;
      if(representation==3){profile=source_default_player_configuration();(*profile)[0x44]=1;}
      player=match->join("Original dual phase",-1,profile);
      if(match->unit(player).definition.datum!=authored.player_representations.at(representation).biped.datum)
        throw FormatError("Dual fixture lost its actual represented biped");
      match->configure_replicated_first_person(cache.load_first_person_binding(weapon->first,int(representation)));
      std::array<Controls,16> controls{};
      match->debug_request({DebugGameRequest::Teleport,player},&spawn);match->step(controls);
      match->debug_request({DebugGameRequest::Teleport,player},&spawn);
      if(primary_pickup) {
        controls[player].pickup=true;controls[player].pickup_milliseconds=233;match->step(controls);
        controls[player]={};for(unsigned n=0;n<120;++n)match->step(controls);
        match->debug_request({DebugGameRequest::Teleport,player},&spawn);
      }
      controls[player].dual_wield=true;controls[player].dual_wield_milliseconds=233;match->step(controls);
      controls[player]={};
      for(unsigned n=0;n<120;++n)match->step(controls);
      auto &unit=const_cast<Unit &>(match->unit(player));
      for(unsigned hand=0;hand<2;++hand) {
        auto *state=native_hand_weapon(unit,hand);
        if(!state || state->definition!=weapon->first)throw FormatError("Actual dual pickup did not retain both authored items");
        // Match the original fixture's declared cold, ready raw storage.
        // Pickup/attachment is exercised above, but not compared by this suite.
        state->loaded=pattern=="shared" && hand==0?1:weapon->second.capacity;
        state->reserve=pattern=="shared" && hand==0?0:4;
        state->magazine={};state->smg.barrel={};state->alternate_barrel={};
        if(weapon->second.smg) {
          state->smg={};state->smg.loaded=std::int16_t(state->loaded);state->smg.reserve=std::int16_t(state->reserve);
          state->smg.hand_timer_registered=false;
        }
        state->smg.held=false;state->trigger_armed=true;
        state->shader.heat=state->shader.power_fraction=0;
        state->shader.weapon_flags&=~6u;state->energy_blocking_ticks=0;state->cooling_inhibitor=0;
      }
      if(registered) {
        unit.source_weapon_hand_feedback={};
        const std::array<unsigned,1> views{player};
        const std::array definitions{cache.load_first_person_binding(weapon->first,int(representation))};
        match->configure_local_views(views,definitions);
      }
      std::uint32_t auxiliary{};match->source_effect_random(auxiliary).primary=0x12345678;
      match->configure_weapon_effect_owner([&](const Unit &,const WeaponEffectInput &effect) {
        calls.push_back(effect);return null_datum;
      });
      prior=key;++cases;
    }
    calls.clear();std::array<Controls,16> controls{};
    controls[player].fire=expected[1];controls[player].fire_secondary=expected[2];controls[player].reload=expected[3];
    match->step(controls);
    const auto &unit=match->unit(player);
    std::uint32_t auxiliary{};
    if(match->source_effect_random(auxiliary).primary!=expected[4]) {
      std::cerr<<key<<" tick="<<expected[0]<<std::hex<<" expected_seed="<<expected[4]
          <<" actual_seed="<<match->source_effect_random(auxiliary).primary<<'\n';
      throw FormatError("Dual shared random stream differs at "+key+" tick "+std::to_string(expected[0]));
    }
    for(unsigned hand=0;hand<2;++hand) {
      const auto &state=*native_hand_weapon(unit,hand);const auto offset=5+hand*20;
      const auto &definition=authored.weapons.at(state.definition);
      auto native_magazine=state.magazine;
      if(definition.smg)native_magazine={state.smg.magazine_mode,state.smg.magazine_ticks,0,
          state.smg.reserve,state.smg.loaded,0,state.smg.transfer_ticks,state.smg.magazine_other_ticks};
      const auto magazine=std::bit_cast<std::array<std::uint32_t,4>>(native_magazine);
      const bool source_magazine=definition.magazine &&
          !(definition.inventory_flags&((1u<<11)|(1u<<19)|(1u<<25)));
      for(unsigned n=0;n<4;++n)if(source_magazine && magazine[n]!=expected[offset+n]) {
        std::cerr<<key<<" tick="<<expected[0]<<" hand="<<hand<<" magazine word="<<n<<" expected="
            <<std::hex<<expected[offset+n]<<" actual="<<magazine[n]<<'\n';
        throw FormatError("Dual magazine/shared reserve phase differs from original");
      }
      const auto &b=state.smg.barrel;
      const std::array<std::uint32_t,8> actual{std::uint8_t(b.counter),b.mode,std::uint16_t(b.ticks),b.shot_count,
        std::bit_cast<std::uint32_t>(b.rate),std::bit_cast<std::uint32_t>(b.recovery),
        std::bit_cast<std::uint32_t>(b.error),std::bit_cast<std::uint32_t>(b.angle_change)};
      const auto raw=std::bit_cast<std::array<std::uint8_t,52>>(std::array<std::uint32_t,13>{
        expected[offset+4],expected[offset+5],expected[offset+6],expected[offset+7],expected[offset+8],
        expected[offset+9],expected[offset+10],expected[offset+11],expected[offset+12],expected[offset+13],
        expected[offset+14],expected[offset+15],expected[offset+16]});
      const std::array<std::uint32_t,8> wanted{raw[0],raw[1],unsigned(raw[2])|(unsigned(raw[3])<<8),
        unsigned(raw[6])|(unsigned(raw[7])<<8),expected[offset+8],expected[offset+14],expected[offset+11],expected[offset+12]};
      for(unsigned n=0;n<actual.size();++n)if(actual[n]!=wanted[n]) {
        std::cerr<<key<<" tick="<<expected[0]<<" hand="<<hand<<" barrel field="<<n<<" expected="
            <<std::hex<<wanted[n]<<" actual="<<actual[n]<<'\n';
        throw FormatError("Dual barrel phase differs from original");
      }
      if(definition.smg && std::bit_cast<std::array<std::uint8_t,52>>(b)!=raw)
        throw FormatError("Complete52-byte dual SMG barrel differs at "+key+" tick "+std::to_string(expected[0]));
      if(std::bit_cast<std::uint32_t>(state.shader.heat)!=expected[offset+17] ||
          std::bit_cast<std::uint32_t>(state.shader.power_fraction)!=expected[offset+18] ||
          (state.shader.weapon_flags&6u)!=expected[offset+19])
        throw FormatError("Dual energy state differs from original");
    }
    if(!registered && (unit.source_weapon_hand_feedback[0] || unit.source_weapon_hand_feedback[1]))
      throw FormatError("Unregistered original view produced native hand feedback at "+key+" tick "+std::to_string(expected[0]));
    if(registered && (unit.source_weapon_hand_feedback[0]!=expected[45] || unit.source_weapon_hand_feedback[1]!=expected[46])) {
      std::cerr<<key<<" tick="<<expected[0]<<" expected shared timers="<<expected[45]<<','<<expected[46]
          <<" actual="<<unsigned(unit.source_weapon_hand_feedback[0])<<','<<unsigned(unit.source_weapon_hand_feedback[1])<<'\n';
      throw FormatError("Registered dual unit hand feedback differs from original");
    }
    if(trigger_suite)for(unsigned hand=0;hand<2;++hand) {
      const auto &trigger=native_hand_weapon(unit,hand)->smg;
      const std::array<std::uint32_t,3> actual{unsigned(trigger.trigger_state)|(unsigned(std::uint16_t(trigger.trigger_ticks))<<16),
          trigger.trigger_flags,trigger.trigger_effect_handle};
      for(unsigned word=0;word<3;++word)if(actual[word]!=expected[47+hand*3+word]) {
        std::cerr<<key<<" tick="<<expected[0]<<" hand="<<hand<<" trigger word="<<word<<std::hex
            <<" expected="<<expected[47+hand*3+word]<<" actual="<<actual[word]<<'\n';
        throw FormatError("Complete native trigger record differs from original registered cycle");
      }
    }
    if(calls.size()!=expected[trigger_suite?53:registered?47:45])throw FormatError("Dual effect count differs at "+key+" tick "+std::to_string(expected[0]));
    for(const auto &effect:calls) {
      std::uint32_t source,tag,a,b;
      if(!(row>>source>>tag>>a>>b))throw FormatError("Truncated dual effect request");
      const auto hand=source==0x10000?0u:source==0x10002?1u:2u;
      if(hand>1 || native_hand_weapon(unit,hand)->source_datum!=effect.weapon || effect.effect!=tag || effect.a_bits!=a || effect.b_bits!=b) {
        std::cerr<<key<<" tick="<<expected[0]<<std::hex<<" expected="<<source<<','<<tag<<','<<a<<','<<b
            <<" actual="<<effect.weapon<<','<<effect.effect<<','<<effect.a_bits<<','<<effect.b_bits
            <<" hands="<<unit.weapon.source_datum<<','<<native_hand_weapon(unit,1)->source_datum<<'\n';
        throw FormatError("Dual effect identity/scalars differ from original");
      }
      ++requests;
    }
    ++ticks;
  }
  if(registered) {
    const std::array<unsigned,0> no_players{};const std::array<FirstPersonBindingDefinition,0> no_definitions{};
    const auto timers=match->unit(player).source_weapon_hand_feedback;
    match->configure_local_views(no_players,no_definitions);
    for(unsigned hand=0;hand<2;++hand)if(native_hand_weapon(match->unit(player),hand)->smg.hand_timer_registered)
      throw FormatError("Removed local view retained weapon feedback registration");
    if(match->unit(player).source_weapon_hand_feedback!=timers)throw FormatError("View removal mutated unit-owned feedback clocks");
  }
  if(cases!=(registered?32u:smg_suite?8u:24u) || ticks!=(registered?8320u:smg_suite?2080u:6240u) || requests!=(registered?1378u:smg_suite?548u:830u))
    throw FormatError("Incomplete dual weapon coverage");
  std::cout<<"PASS "<<cases<<" original dual phase scenarios, "<<ticks<<" ticks, "<<requests
      <<" requests: both magazines/barrels, shared reserves/seed, heat and effect identities exact\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
