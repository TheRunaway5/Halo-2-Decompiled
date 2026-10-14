#include "halo2/weapon_energy.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=3)return 2;
  std::ifstream input(argv[1]);if(!input)return 77;
  std::array<unsigned,2> cases{};std::string line;
  while(std::getline(input,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,26> words;
    for(auto &word:words)if(!(row>>std::hex>>word))throw std::runtime_error("Truncated original energy input");
    const auto f=[&](unsigned n){return std::bit_cast<float>(words[n]);};
    const auto short_=[](std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));};
    WeaponEnergyState state{words[10],words[11],words[12],f(13),f(14),f(15),words[16],short_(words[17])};
    const WeaponHeatDefinition heat{f(1),f(2),f(3),f(4),f(5),short_(words[6]),words[7]};
    const WeaponAgeDefinition age{words[8],words[9]};
    std::vector<std::uint32_t> events;
    const auto event=[&](unsigned kind,std::uint32_t a=0,std::uint32_t b=0) {
      events.insert(events.end(),{kind,a,b});
    };
    WeaponHeatServices services;services.has_unit=words[21]!=0;
    services.reset_zoom=[&]{event(0);};services.action=[&](unsigned action){event(1,action);};
    services.vent_timing_ticks=[&]{event(2);return short_(words[22]);};
    services.create_overheat=[&](std::uint32_t effect){event(3,effect);return words[23];};
    services.stop_overheat=[&](std::uint32_t handle,bool holstered){event(4,handle,holstered);};
    if(words[0]==0)weapon_heat_step(state,heat,f(19),f(20),services);
    else if(words[0]==1)weapon_age(state,f(24),f(20),
        words[21] && (state.item_flags&1u) && words[25],age,[&](std::uint32_t effect){event(5,effect);});
    else throw std::runtime_error("Unknown energy operation");
    std::vector<std::uint32_t> actual{state.definition,state.item_flags,state.weapon_flags,
      std::bit_cast<std::uint32_t>(state.heat),std::bit_cast<std::uint32_t>(state.spent),
      std::bit_cast<std::uint32_t>(state.cooling_inhibitor),state.overheat_handle,
      std::uint16_t(state.blocking_ticks),0,std::uint32_t(events.size()/3)};
    actual.insert(actual.end(),events.begin(),events.end());
    for(unsigned n=0;n<actual.size();++n) {
      std::uint32_t expected;
      if(!(row>>expected))throw std::runtime_error("Truncated original energy output");
      if(actual[n]!=expected) {
        std::cerr<<"operation="<<words[0]<<" case="<<cases[words[0]]<<" output="<<n
            <<" expected="<<std::hex<<expected<<" actual="<<actual[n]<<'\n';
        throw std::runtime_error("Native energy differs from original TU1.5/TU1.1");
      }
    }
    std::string extra;if(row>>extra)throw std::runtime_error("Trailing original energy outputs");
    ++cases[words[0]];
  }
  if(cases[0]<100 || cases[1]<100)throw std::runtime_error("Original energy coverage missing");
  std::ifstream timing(argv[2]);if(!timing)return 77;
  unsigned timings{};
  while(std::getline(timing,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,6> words;
    for(auto &word:words)if(!(row>>std::hex>>word))throw std::runtime_error("Truncated original vent timing");
    const auto short_=[](std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));};
    const auto result=words[0]?weapon_vent_timing(short_(words[1]),short_(words[2]),short_(words[3]),
        std::bit_cast<float>(words[4])):0;
    if(std::uint16_t(result)!=words[5])throw std::runtime_error("Native vent timing differs from original executable");
    ++timings;
  }
  if(timings<100)throw std::runtime_error("Original vent timing coverage missing");
  std::cout<<"PASS original TU1.5/TU1.1 heat="<<cases[0]<<" battery="<<cases[1]
      <<" vent timing="<<timings<<"; retained fields and ordered service requests exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
