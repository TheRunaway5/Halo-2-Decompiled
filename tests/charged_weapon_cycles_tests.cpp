#include "halo2/multiplayer.hpp"
#include "halo2/hud.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace halo2;
static std::array<std::uint32_t,43> projected_state(MultiplayerMatch &match,unsigned player) {
  const auto &unit=match.unit(player);const auto &state=unit.weapon;
  std::uint32_t auxiliary{};std::array<std::uint32_t,43> actual{};
    actual[2]=std::bit_cast<std::uint32_t>(state.shader.heat);
    actual[3]=std::bit_cast<std::uint32_t>(state.shader.power_fraction);actual[4]=state.shader.weapon_flags&15u;
    actual[5]=match.source_effect_random(auxiliary).primary;
    actual[6]=unit.source_weapon_hand_feedback[0];actual[7]=unit.source_weapon_hand_feedback[1];
    actual[8]=std::uint16_t(state.energy_blocking_ticks);actual[9]=std::bit_cast<std::uint32_t>(state.cooling_inhibitor);
    const auto primary=std::bit_cast<std::array<std::uint32_t,13>>(state.smg.barrel);
    const auto alternate=std::bit_cast<std::array<std::uint32_t,13>>(state.alternate_barrel);
    std::copy(primary.begin(),primary.end(),actual.begin()+10);std::copy(alternate.begin(),alternate.end(),actual.begin()+23);
    const auto trigger=std::bit_cast<std::array<std::uint32_t,3>>(WeaponTriggerState{
      state.smg.trigger_state,0,state.smg.trigger_ticks,state.smg.trigger_flags,state.smg.trigger_effect_handle});
    std::copy(trigger.begin(),trigger.end(),actual.begin()+36);
    const auto &a=state.trigger_input;
    actual[39]=a.current|(std::uint32_t(a.previous)<<8)|(std::uint32_t(a.anchor)<<16)|(std::uint32_t(a.direction)<<24);
    actual[40]=a.ticks|(std::uint32_t(state.charge_fraction)<<8);
  return actual;
}
int main(int argc,char **argv)try {
  if(argc!=6)throw FormatError("Supply MAP SHARED XBE ORIGINAL_CHARGED_FIXTURE ORIGINAL_EFFECT_STATE_FIXTURE");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  const auto authored=cache.load_content(0,true);const auto spawns=cache.spawns();
  const auto weapon=std::find_if(authored.weapons.begin(),authored.weapons.end(),[](const auto &w) {
    return w.second.name.ends_with("\\plasma_pistol");
  });
  if(weapon==authored.weapons.end() || !weapon->second.charged_cycle || weapon->second.barrel_views.size()!=2)
    throw FormatError("Actual plasma pistol has no recovered two-barrel charge owner");
  std::ifstream input(argv[4]),effect_states(argv[5]);if(!input || !effect_states)return 77;
  std::optional<LevelContent> level;std::unique_ptr<MultiplayerMatch> match;
  std::string prior,line;unsigned player{},cases{},ticks{},requests{},births{};
  std::vector<WeaponEffectInput> calls;unsigned current_tick{};
  std::array<HudContent,2> hud{cache.load_hud(authored.player_representations.at(2).biped.datum,weapon->first),
                             cache.load_hud(authored.player_representations.at(3).biped.datum,weapon->first)};
  for(const auto &h:hud)if(h.weapon_trigger_count!=1 || h.weapon_trigger_behavior!=2 || h.weapon_charging_type!=1)
    throw FormatError("Charged HUD lost its actual trigger operands");
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);unsigned representation,local;std::string pattern;
    row>>representation>>local>>pattern;std::array<std::uint32_t,43> expected{};
    for(auto &v:expected)if(!(row>>std::hex>>v))throw FormatError("Truncated original charged cycle");
    const auto key=std::to_string(representation)+"/"+std::to_string(local)+"/"+pattern;
    if(key!=prior) {
      match.reset();level=authored;level->netgame_equipment.clear();
      const auto spawn=spawns.at(0);NetgameEquipment pickup;pickup.modes={2,0,0,0};
      pickup.position=spawn.position+Vec3{.2f,0,.4f};pickup.items.push_back({1,0x77656170u,weapon->first,0});
      level->netgame_equipment.push_back(pickup);
      auto rules=slayer_rules({"Slayer",0,{{0x59,5}}});rules.score_limit=rules.time_ticks=0;
      const auto smg=std::find_if(authored.weapons.begin(),authored.weapons.end(),[](const auto &w){return w.second.smg.has_value();});
      if(smg==authored.weapons.end())throw FormatError("Actual initial SMG is unavailable");
      rules.weapon=smg->first;rules.random_seed=0x12345678;
      match=std::make_unique<MultiplayerMatch>(*level,0,spawns,rules);match->configure_weapon_pickups();
      std::optional<SourcePlayerConfiguration> profile;
      if(representation==3){profile=source_default_player_configuration();(*profile)[0x44]=1;}
      player=match->join("Original charged cycle",-1,profile);
      if(match->unit(player).definition.datum!=authored.player_representations.at(representation).biped.datum)
        throw FormatError("Charged fixture lost its actual represented biped");
      match->configure_replicated_first_person(cache.load_first_person_binding(weapon->first,int(representation)));
      if(local) {
        const std::array<unsigned,1> views{player};
        const std::array definitions{cache.load_first_person_binding(rules.weapon,int(representation)),
                                    cache.load_first_person_binding(weapon->first,int(representation))};
        match->configure_local_views(views,definitions);
      }
      std::array<Controls,16> idle{};
      match->debug_request({DebugGameRequest::Teleport,player},&spawn);match->step(idle);
      match->debug_request({DebugGameRequest::Teleport,player},&spawn);
      idle[player].pickup=true;idle[player].pickup_milliseconds=233;match->step(idle);idle[player]={};
      if(match->unit(player).weapon.definition!=weapon->first)throw FormatError("Actual charged pickup did not attach");
      for(unsigned n=0;n<120;++n)match->step(idle);
      auto &unit=const_cast<Unit &>(match->unit(player));auto &state=unit.weapon;
      // Actual pickup above; the compared phase starts at the original fixture's
      // declared cold constructor storage, unavailable own model/effect allocator.
      state.smg={};state.smg.hand_timer_registered=local;state.alternate_barrel={};
      state.trigger_input={};state.trigger_target={};state.charge_fraction=0;
      state.shader.heat=0;state.shader.power_fraction=pattern=="spent"?.99f:0;
      state.shader.weapon_flags=0x20;state.cooling_inhibitor=0;state.energy_blocking_ticks=0;
      state.overheat_handle=null_datum;state.ready_clock=false;state.ready_countdown=0;
      unit.source_weapon_hand_feedback={};
      std::uint32_t auxiliary{};match->source_effect_random(auxiliary).primary=0x12345678;
      match->configure_weapon_effect_owner([&,representation,local,pattern,key](const Unit &u,const WeaponEffectInput &effect) {
        if(effect.weapon!=u.weapon.source_datum)throw FormatError("Charged effect lost full weapon datum");
        unsigned rep,view,tick,ordinal;std::string name;
        if(!(effect_states>>std::dec>>rep>>view>>name>>tick>>ordinal) || rep!=representation || view!=local ||
            name!=pattern || tick!=current_tick || ordinal!=calls.size())throw FormatError("Charged effect state fixture order differs");
        const auto actual=projected_state(*match,player);
        for(unsigned n=2;n<=40;++n) {
          std::uint32_t expected;if(!(effect_states>>std::hex>>expected))throw FormatError("Truncated charged callback state");
          if(actual[n]!=expected) {
            std::cerr<<key<<" tick="<<tick<<" effect="<<ordinal<<" callback word="<<n<<" expected="<<std::hex<<expected
                <<" actual="<<actual[n]<<'\n';throw FormatError("Charged stores at original effect boundary differ");
          }
        }
        calls.push_back(effect);return null_datum;
      });
      prior=key;++cases;
    }
    current_tick=expected[0];calls.clear();const auto prior_shots=match->simulation().shots();
    std::set<std::uint32_t> old_projectiles;
    for(const auto &p:match->simulation().presentation_projectiles())old_projectiles.insert(p.datum);
    // The original request fixture declares stationary forward +X. Keep birth
    // above the map to separate creation direction from native world contact.
    auto &controlled=const_cast<Unit &>(match->unit(player));
    controlled.body.position.z=100;controlled.body.velocity={};controlled.yaw=controlled.pitch=0;
    std::array<Controls,16> controls{};controls[player].fire=expected[1];match->step(controls);
    const auto &unit=match->unit(player);const auto &state=unit.weapon;
    auto actual=projected_state(*match,player);
    actual[0]=expected[0];actual[1]=expected[1];actual[41]=unsigned(calls.size());
    actual[42]=unsigned(match->simulation().shots()-prior_shots);
    for(unsigned n=2;n<actual.size();++n)if(actual[n]!=expected[n]) {
      std::cerr<<key<<" tick="<<expected[0]<<" word="<<n<<" expected="<<std::hex<<expected[n]<<" actual="<<actual[n]<<'\n';
      throw FormatError("Native charged cycle differs from original source timeline");
    }
    const auto snapshot=match->state();const auto &published=snapshot.players[player];
    const auto flight=match->simulation().presentation_projectiles();
    if(flight.size()!=snapshot.weapon_projectiles.size())throw FormatError("Charged snapshot lost live projectiles");
    for(unsigned n=0;n<flight.size();++n)if(flight[n].datum!=snapshot.weapon_projectiles[n].datum ||
        flight[n].definition!=snapshot.weapon_projectiles[n].definition)
      throw FormatError("Charged snapshot replaced the actual selected barrel's projectile");
    const auto projected=multiplayer_hud_weapon_state(published,hud[representation-2],0);
    if(published.weapon_trigger.mode!=(expected[36]&255u) ||
        std::uint16_t(published.weapon_trigger.ticks)!=(expected[36]>>16) ||
        published.weapon_trigger.charge!=std::uint8_t(expected[40]>>8) ||
        projected.input_held!=expected[1] || projected.charging!=((expected[36]&255u)==2u) ||
        std::bit_cast<std::uint32_t>(projected.charge)!=std::bit_cast<std::uint32_t>(float(std::uint8_t(expected[40]>>8))*std::bit_cast<float>(0x3b808081u)))
      throw FormatError("Actual charged HUD publication differs from original trigger/charge state");
    for(const auto &effect:calls) {
      std::array<std::uint32_t,4> e{};for(auto &v:e)if(!(row>>v))throw FormatError("Truncated charged effect request");
      if(e!=std::array{effect.effect,effect.previous_handle,effect.a_bits,effect.b_bits})
        throw FormatError("Charged effect ABI differs at "+key+" tick "+std::to_string(expected[0]));
      ++requests;
    }
    std::vector<NativeWeaponProjectile> created;
    for(const auto &p:match->simulation().presentation_projectiles())if(!old_projectiles.contains(p.datum))created.push_back(p);
    if(created.size()!=expected[42])throw FormatError("Charged birth disappeared in declared clear-space fixture");
    for(const auto &p:created) {
      std::array<std::uint32_t,4> e{};for(auto &v:e)if(!(row>>v))throw FormatError("Truncated charged projectile request");
      if(e!=std::array{p.definition,std::bit_cast<std::uint32_t>(p.forward.x),
          std::bit_cast<std::uint32_t>(p.forward.y),std::bit_cast<std::uint32_t>(p.forward.z)})
        throw FormatError("Charged projectile definition/direction differs from original request");
      ++births;
    }
    ++ticks;
  }
  if(cases!=24 || ticks!=5164 || requests!=136 || births!=44)throw FormatError("Original charged scenario coverage incomplete");
  effect_states>>std::ws;if(!effect_states.eof())throw FormatError("Unconsumed original effect states");
  std::cout<<"PASS "<<cases<<" actual charged scenarios, "<<ticks<<" ticks, "<<requests<<" effect ABI requests, "<<births
      <<" projectile requests: both52-byte barrels, trigger, analog/charge bytes, heat/age, hand timers and RNG exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
