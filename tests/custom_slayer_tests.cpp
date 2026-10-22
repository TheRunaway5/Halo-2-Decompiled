#include "halo2/custom_slayer.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include "halo2/vista_pcr_publication.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <set>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
void kernel(const std::filesystem::path &fixture) {
  std::ifstream input(fixture);check(bool(input),"Missing unchanged-original weapon controls");
  std::array<std::uint32_t,18> bank{};unsigned direct=0,random=0;
  for(std::string line;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);char kind;row>>kind;
    if(kind=='B'){row>>std::hex;for(auto &ref:bank)row>>ref;}
    if(kind=='D') {
      unsigned option;std::uint32_t expected,seed=0x12345678;row>>option>>std::hex>>expected;
      check(source_starting_weapon(bank[1],option,null_datum,bank,seed)==expected && seed==0x12345678,"Original direct choice or RNG retention differs");++direct;
    }
    if(kind=='R') {
      std::uint32_t seed,excluded,expected,after;row>>std::hex>>seed>>excluded>>expected>>after;
      check(source_starting_weapon(bank[1],2,excluded,bank,seed)==expected && seed==after,"Original random selection/retry/RNG differs");++random;
    }
  }
  check(direct==21 && random==768,"Original control corpus is incomplete");
}
void pc_variant(const std::filesystem::path &fixture) {
  std::ifstream input(fixture);check(bool(input),"Missing original PC secondary controls");unsigned count=0;
  for(std::string line;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);int parameter,value,expected;std::string raw;row>>parameter>>value>>raw>>expected;
    auto variant=source_game_variant_default(0);source_game_variant_set(variant,parameter,value);
    const auto pc=vista_pcr_slayer_variant(variant);
    check(source_game_variant_get(variant,parameter)==expected,"PC secondary getter differs");
    for(unsigned i=0;i<pc.bytes.size();++i)check(std::to_integer<unsigned>(pc.bytes[i])==std::stoul(raw.substr(i*2,2),nullptr,16),"Complete PC secondary variant differs");
    ++count;
  }
  check(count==12,"PC original secondary corpus incomplete");
}
void gameplay(const std::filesystem::path &root,const std::filesystem::path &xbe,const std::string &map) {
  const auto maps=root/"assets/disc/maps";
  const Cache cache(maps/(map+".map"),maps,OriginalData::load_xbe(xbe));
  auto content=cache.load_content(0,true);const auto bank=cache.multiplayer_weapon_choices();
  const auto variants=load_custom_slayer_variants(root/"data/tags/variants");check(variants.size()==3,"Missing bundled modes");
  const auto fiesta=*std::find_if(variants.begin(),variants.end(),[](const auto &v){return custom_slayer(v)==CustomSlayer::fiesta;});
  const auto infection=*std::find_if(variants.begin(),variants.end(),[](const auto &v){return custom_slayer(v)==CustomSlayer::infection;});
  check(custom_slayer_fingerprint(7,fiesta)!=custom_slayer_fingerprint(7,infection) && custom_slayer_fingerprint(7,fiesta)!=7,"Custom mode admission not fenced");
  std::set<std::uint32_t> spawned;
  for(unsigned option=3;option<=18;++option) {
    std::uint32_t seed=0;const auto definition=source_starting_weapon(bank[1],option,null_datum,bank,seed);
    if(definition==null_datum)continue;
    auto variant=fiesta;variant.values.set(89,int(option));auto rules=slayer_rules(variant);
    rules.weapon=bank[1];rules.weapon_choices=bank;
    MultiplayerMatch match(content,0,cache.spawns(),rules);match.join("Fiesta");
    const auto &unit=match.unit(0);check(unit.weapon.definition==definition,"Authored Fiesta spawn definition differs");
    check(unit.weapon.loaded>=0 && unit.weapon.reserve>=0,"Energy/melee loadout has negative ammunition");
    spawned.insert(definition);
  }
  check(spawned.size()==14,"Fiesta did not spawn every original random weapon");
  auto rules=slayer_rules(infection);rules.weapon=bank[1];rules.weapon_choices=bank;
  rules.respawn_ticks=1;rules.time_ticks=60;
  MultiplayerMatch match(content,0,cache.spawns(),rules);match.configure_weapon_pickups();
  for(unsigned i=0;i<4;++i)match.join("Survivor "+std::to_string(i));
  std::array<Controls,16> controls{};match.step(controls);
  unsigned zombies=0,human=16;
  for(unsigned i=0;i<4;++i) {
    const auto state=match.state().players[i];
    if((*state.source_configuration)[0x40]==6){++zombies;check(state.weapon==bank[14] && state.shield==0,"Initial zombie lacks sword/no shields");}
    else {human=i;check((*state.source_configuration)[0x40]==2 && state.weapon==bank[2],"Human color/shotgun differs");}
  }
  check(zombies==1 && human<4,"Infection initial selection differs");
  // Explicit fatal-body boundary isolates conversion/respawn from weapon aiming.
  auto &victim=const_cast<Unit&>(match.unit(human));victim.vitality.dead=true;victim.vitality.body=0;
  match.step(controls);
  check((*match.state().players[human].source_configuration)[0x40]==6,"Human fatality did not convert to green zombie");
  for(unsigned tick=0;tick<40 && match.state().players[human].dead;++tick)match.step(controls);
  check(!match.state().players[human].dead && match.state().players[human].weapon==bank[14],"Converted zombie failed sword respawn");
  while(!match.state().finished && match.state().tick<100)match.step(controls);
  check(match.state().finished && match.state().results_finalized,"Infection survival timer failed to finalize PCR");
  for(unsigned i=0;i<4;++i)if((*match.state().players[i].source_configuration)[0x40]!=6)
    check(match.state().players[i].score==2,"Survival bonus bypassed by ordinary Slayer timer");
  MultiplayerMatch waiting(content,0,cache.spawns(),rules);waiting.configure_weapon_pickups();
  waiting.join("Waiting");for(unsigned i=0;i<90;++i)waiting.step(controls);
  check(!waiting.state().finished,"Infection expired before a second player arrived");
  waiting.join("Arrival");waiting.step(controls);
  check(!waiting.state().finished,"Infection did not start a fresh survival timer");
  for(unsigned i=0;i<60;++i)waiting.step(controls);
  check(waiting.state().finished,"Delayed-start Infection never completed");
  // Full infection, independently of the time-limit outcome.
  rules.time_ticks=0;MultiplayerMatch all(content,0,cache.spawns(),rules);all.configure_weapon_pickups();
  for(unsigned i=0;i<4;++i)all.join("Player "+std::to_string(i));all.step(controls);
  for(unsigned i=0;i<4;++i)if((*all.state().players[i].source_configuration)[0x40]!=6) {
    auto &unit=const_cast<Unit&>(all.unit(i));unit.vitality.dead=true;unit.vitality.body=0;
  }
  all.step(controls);check(all.state().finished && all.state().results_finalized,"Infecting all humans failed to end match");
  if(map=="lockout") {
    auto arena=content;
    arena.collision={{{-100,-100,10},{100,-100,10},{100,100,10},0,0},{{-100,-100,10},{100,100,10},{-100,100,10},0,0}};
    Spawn start;start.position={0,0,10};std::array<Spawn,1> starts{start};
    MultiplayerMatch combat(arena,0,starts,rules);combat.configure_weapon_pickups();
    auto owner=std::make_shared<Cache>(maps/(map+".map"),maps,OriginalData::load_xbe(xbe));
    combat.configure_melee_animation(owner,*owner->original_data()->melee_constants);
    for(unsigned i=0;i<3;++i)combat.join("Combat "+std::to_string(i));
    std::array<Controls,16> input{};for(unsigned tick=0;tick<90;++tick)combat.step(input);
    unsigned zombie=16;std::vector<unsigned> humans;
    for(unsigned i=0;i<3;++i)if((*combat.state().players[i].source_configuration)[0x40]==6)zombie=i;else humans.push_back(i);
    check(zombie<3 && humans.size()==2,"Combat initial factions differ");
    const auto place=[&](unsigned id,Vec3 position){auto &u=const_cast<Unit&>(combat.unit(id));u.body.position=position;u.body.velocity={};};
    place(humans[0],{0,0,10});place(humans[1],{.65f,0,10});place(zombie,{20,20,10});
    const auto health=combat.unit(humans[1]).vitality.body+combat.unit(humans[1]).vitality.shield;
    const auto ammo=combat.unit(humans[0]).weapon.loaded;
    input[humans[0]].fire=true;combat.step(input);input[humans[0]].fire=false;
    for(unsigned tick=0;tick<20;++tick)combat.step(input);
    check(combat.unit(humans[0]).weapon.loaded<ammo,"Friendly-fire stimulus never fired the actual shotgun");
    check(combat.unit(humans[1]).vitality.body+combat.unit(humans[1]).vitality.shield==health,"Faction friendly fire damaged a survivor");
    place(zombie,{0,0,10});place(humans[0],{.65f,0,10});place(humans[1],{20,20,10});
    input[zombie].fire=true;combat.step(input);input[zombie].fire=false;
    for(unsigned tick=0;tick<120 && (*combat.state().players[humans[0]].source_configuration)[0x40]!=6;++tick)
      combat.step(input);
    check((*combat.state().players[humans[0]].source_configuration)[0x40]==6,"Actual sword fire failed to kill and infect an in-range human");
    check(combat.state().players[zombie].kills==1,"Actual sword kill was not credited once");
    for(unsigned tick=0;tick<120 && combat.state().players[humans[0]].dead;++tick)combat.step(input);
    check(!combat.state().players[humans[0]].dead && combat.state().players[humans[0]].weapon==bank[14],"Sword victim failed to respawn as a sword zombie");
    // Let the original thirty-tick spawn protection expire before shooting.
    for(unsigned tick=0;tick<60;++tick)combat.step(input);
    place(humans[1],{0,0,10});place(humans[0],{.65f,0,10});place(zombie,{20,20,10});
    input[humans[1]].fire=true;
    for(unsigned tick=0;tick<180 && !combat.state().players[humans[1]].kills;++tick)combat.step(input);
    input[humans[1]].fire=false;
    check(combat.state().players[humans[1]].kills==1,"Survivor shotgun failed to kill an in-range zombie");
    check((*combat.state().players[humans[0]].source_configuration)[0x40]==6,"Zombie death lost its green faction");
    std::cout<<"PASS actual shotgun friendly-fire rejection/zombie kill and authored sword-fire kill/conversion/respawn\n";
  }
  std::cout<<"PASS 789 original weapon controls; all14 authored random weapons; initial/converted green zombies, sword respawn, survival/all-infected endings and PCR finalization\n";
}
}
int main(int argc,char **argv)try {
 check(argc==3 || argc==4,"Supply ROOT XBE [MAP]");const std::filesystem::path root=argv[1];
 kernel(root/"tests/fixtures/custom_slayer_random_original.tsv");pc_variant(root/"tests/fixtures/custom_slayer_pc_original.tsv");gameplay(root,argv[2],argc==4?argv[3]:"lockout");return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
