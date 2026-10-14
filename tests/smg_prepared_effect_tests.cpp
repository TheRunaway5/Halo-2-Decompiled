#include "halo2/content.hpp"
#include "halo2/multiplayer_network.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const std::string &why) {if(!value)throw FormatError(why);}
std::uint32_t word(std::istringstream &row) {
  std::string value;check(bool(row>>value),"Missing original preparation field");
  return std::stoul(value,nullptr,16);
}
struct Expected {
  std::array<std::uint32_t,13> fields;
  std::string barrel;
  std::uint32_t seed;
};
Expected read(std::istringstream &row) {
  Expected result;for(auto &field:result.fields)field=word(row);
  check(bool(row>>result.barrel),"Missing original barrel");result.seed=word(row);
  return result;
}
void compare_state(const SmgState &state,const Expected &expected,const std::string &label) {
  const std::array<std::uint32_t,13> actual{
      std::uint32_t(state.loaded),std::uint32_t(state.reserve),std::uint32_t(state.magazine_mode),
      std::uint16_t(state.magazine_ticks),std::uint16_t(state.transfer_ticks),
      state.trigger_state,std::uint16_t(state.trigger_ticks),state.weapon_state,
      state.trigger_flags,state.weapon_flags,state.input_flags,state.hand_timers[0],state.hand_timers[1]};
  check(actual==expected.fields,label+" source preparation state fields");
  constexpr char hex[]="0123456789abcdef";
  const auto *bytes=reinterpret_cast<const unsigned char *>(&state.barrel);std::string raw;
  for(unsigned i=0;i<sizeof(state.barrel);++i) {raw+=hex[bytes[i]>>4];raw+=hex[bytes[i]&15];}
  check(raw==expected.barrel,label+" complete52-byte source barrel");
}
void compare(const SmgState &state,std::uint32_t seed,const Expected &expected,const std::string &label) {
  compare_state(state,expected,label);
  check(seed==expected.seed,label+" preparation touched projectile RNG");
}
struct Effect {
  std::uint32_t tag,previous,a,b,result;
  Expected state;
};
}
int main(int argc,char **argv) {
  try {
    check(argc==4,"Supply original map, prepared-effect and complete-effect fixtures");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]) || !std::filesystem::exists(argv[3]))return 77;
    Cache cache(argv[1]);auto content=cache.load_content(0,true);content.kill_triggers.clear();
    const auto id=cache.slayer_starting_weapon(5);const auto &weapon=content.weapons.at(id);
    check(weapon.smg.has_value() && weapon.firing_effects.size()==1,"Authored stock SMG effect row");
    const auto &firing=weapon.firing_effects[0];
    check(firing.minimum==1 && firing.maximum==1,"Original prepared-entry count bounds");
    const auto &definition=*weapon.smg;
    std::ifstream file(argv[2]);std::string name,line;SmgState state;std::uint32_t seed{};
    unsigned cases=0,ticks=0,effects=0,empty=0,requests=0;
    std::vector<std::string> equipped;
    while(std::getline(file,line)) {
      std::istringstream row(line);char kind;check(bool(row>>kind),"Empty fixture line");
      if(kind=='J' || kind=='P') {equipped.push_back(line);continue;}
      if(kind=='I') {
        row>>name;const auto expected=read(row);state={};
        state.loaded=std::int16_t(expected.fields[0]);state.reserve=std::int16_t(expected.fields[1]);
        state.own_manager=false;state.attached=true;state.hand_timer_registered=true;
        seed=0x12345678;compare(state,seed,expected,name+" initial");++cases;
      } else {
        check(kind=='T',"Unknown preparation fixture record");
        const auto tick=word(row);const bool held=word(row),reload=word(row);
        const auto before=read(row),after=read(row),postpass=read(row);
        const auto requested=word(row),count=word(row);std::vector<Effect> expected;
        for(unsigned i=0;i<count;++i)expected.push_back({word(row),word(row),word(row),word(row),word(row),read(row)});
        const auto label=name+" tick"+std::to_string(tick);
        smg_input(state,held);if(reload)smg_reload(state,definition);
        compare(state,seed,before,label+" before");unsigned cursor=0;
        smg_update(state,definition,tick_seconds,{},[&](const SmgState &prepared,unsigned selection,
              std::uint32_t a,std::uint32_t b) {
          check(cursor<expected.size(),label+" unexpected effect callback");const auto &source=expected[cursor++];
          check(selection<3 && firing.effects[selection]==source.tag &&
              prepared.barrel.effect_handle==source.previous && a==source.a && b==source.b,
              label+" literal107B50 tag/previous/scalar stack words");
          compare(prepared,seed,source.state,label+" callback before projectile pass");
          // Declared failed service result from original fixture; this test
          // does not allocate an effect or claim successful flash admission.
          ++effects;empty+=selection==2;return source.result;
        });
        check(cursor==count,label+" missing original preparation callback");compare(state,seed,after,label+" after");
        const bool emitted=bool(smg_dispatch(state,definition,{1,0,0},seed,content.original_data->weapon_directions));
        check(emitted==bool(requested),label+" source projectile request presence");requests+=emitted;
        compare(state,seed,postpass,label+" postpass");
        const auto stored=state.barrel;const auto before_seed=seed;
        check(!smg_dispatch(state,definition,{1,0,0},seed,content.original_data->weapon_directions) &&
              std::memcmp(&stored,&state.barrel,sizeof stored)==0 && before_seed==seed,
              label+" duplicate postpass changed state");++ticks;
      }
      std::string extra;check(!(row>>extra),"Trailing prepared-effect fixture words");
    }
    check(cases==22 && ticks==2220 && effects>requests && requests==566 && empty>0,
          "Incomplete independently frozen preparation suite");
    std::map<std::pair<std::string,unsigned>,std::vector<Effect>> complete_effects;
    std::ifstream complete_file(argv[3]);
    for(std::string line;std::getline(complete_file,line);) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);std::string name;row>>name;
      const auto tick=word(row),count=word(row);std::vector<Effect> all;
      for(unsigned n=0;n<count;++n)all.push_back({word(row),word(row),word(row),word(row),word(row),{}});
      check(complete_effects.emplace(std::pair{name,tick},std::move(all)).second,"Duplicate original complete effect row");
    }
    check(complete_effects.size()==599,"Missing original complete effect rows");
    unsigned match_ticks=0,udp_ticks=0,match_effects=0,udp_effects=0,match_all_effects=0,udp_all_effects=0;
    for(unsigned mode=0;mode<2;++mode) {
      std::unique_ptr<MultiplayerMatch> match;std::unique_ptr<LanHost> host;std::unique_ptr<LanClient> client;
      for(const auto &line:equipped) {
        std::istringstream row(line);char kind;row>>kind;
        if(kind=='J') {
          row>>name;const auto initial=read(row);client.reset();host.reset();
          SlayerRules rules;rules.weapon=id;rules.random_seed=0x12345678;rules.time_ticks=rules.score_limit=0;
          match=std::make_unique<MultiplayerMatch>(content,0,cache.spawns(),rules);
          if(mode) {
            host=std::make_unique<LanHost>(*match,0,0x107b50);
            client=std::make_unique<LanClient>("127.0.0.1:"+std::to_string(host->port()),0x107b50,1);
            std::array<Controls,multiplayer_capacity> controls{};
            for(unsigned i=0;i<64 && !client->ready();++i) {
              client->send(std::array<Controls,1>{});host->receive(controls);host->broadcast();client->receive();
            }
            check(client->ready() && client->slots()[0]==0,"Prepared-effect actual UDP join");
          } else match->join("Original preparation");
          compare_state(match->unit(0).weapon.smg,initial,name+" actual equipped initial");
        } else {
          const auto tick=word(row);const bool held=word(row),reload=word(row);
          const auto post=read(row);const auto requested=word(row),count=word(row);
          std::vector<Effect> expected;
          for(unsigned i=0;i<count;++i)expected.push_back({word(row),word(row),word(row),word(row),word(row),{}});
          check(match && match->simulation().ticks()==tick,"Actual equipped preparation tick");
          unsigned cursor=0,all_cursor=0;const auto shots=match->simulation().shots();
          const auto &all=complete_effects.at({name,tick});
          match->configure_weapon_effect_owner([&](const Unit &unit,const WeaponEffectInput &request) {
            check(all_cursor<all.size(),name+" extra actual complete effect callback");const auto &source=all[all_cursor++];
            check(unit.source_datum==match->player(0).source_current_object &&
                  request.weapon==unit.weapon.source_datum && match->weapon_from_datum(request.weapon)==&unit.weapon,
                  name+" full live player/unit/carried-weapon identity");
            check(request.effect==source.tag && request.previous_handle==source.previous &&
                  request.a_bits==source.a && request.b_bits==source.b,
                  name+" live imported tag/previous/scalar ABI");
            check(match->simulation().shots()==shots,name+" effect request was deferred after projectile pass");
            // The frozen prepared fixture excludes magazine calls. Compare
            // every call against the separate unfiltered original stream,
            // then retain its original prepared-only assertion as well.
            if(cursor<expected.size() && request.effect==expected[cursor].tag &&
                request.previous_handle==expected[cursor].previous && request.a_bits==expected[cursor].a && request.b_bits==expected[cursor].b) {
              ++cursor;mode?++udp_effects:++match_effects;
            } else check(weapon.magazine && (request.effect==weapon.magazine->reload_effect ||
                request.effect==weapon.magazine->chamber_effect),name+" non-prepared effect lost its actual magazine owner");
            mode?++udp_all_effects:++match_all_effects;return source.result;
          });
          std::array<Controls,multiplayer_capacity> controls{};
          controls[0].fire=held;controls[0].reload=reload;controls[0].yaw=match->unit(0).yaw;
          if(mode) {client->send(std::array<Controls,1>{controls[0]});host->receive(controls);}
          match->step(controls);
          check(cursor==count && all_cursor==all.size() && match->simulation().shots()-shots==requested,
                name+" actual effect/projectile admission and count");
          compare_state(match->unit(0).weapon.smg,post,name+" actual equipped complete postpass");
          // Never retain a callback whose per-tick comparison storage expires.
          match->configure_weapon_effect_owner({});
          if(mode) {host->broadcast();client->receive();++udp_ticks;}else ++match_ticks;
        }
        std::string extra;check(!(row>>extra),"Trailing actual prepared-effect fields");
      }
    }
    check(match_ticks==599 && udp_ticks==599 && match_effects==udp_effects && match_effects>=165,
          "Incomplete actual Match/UDP source preparation integration");
    check(match_all_effects==170 && udp_all_effects==170,"Incomplete original magazine/prepared effect stream");
    std::cout<<"PASS "<<cases<<" original cases, "<<ticks<<" ticks, "<<effects
             <<" prepared-effect callbacks ("<<empty<<" empty), "<<requests<<" projectile requests; "
             <<match_ticks<<" local + "<<udp_ticks<<" UDP ticks, "<<match_effects<<" prepared / "<<match_all_effects<<" complete callbacks each\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
