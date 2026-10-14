#include "halo2/weapon_barrel_timing.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)return 2;
  std::ifstream input(argv[1]);if(!input)return 77;
  std::array<unsigned,4> cases{};std::string line;
  while(std::getline(input,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,45> words;
    for(auto &w:words)if(!(row>>std::hex>>w))throw FormatError("Truncated original barrel timing row");
    std::string extra;if(row>>extra)throw FormatError("Trailing original barrel timing words");
    const auto f=[&](unsigned n){return std::bit_cast<float>(words[n]);};
    const auto short_=[](std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));};
    const WeaponBarrelTimingDefinition definition{words[1],f(2),f(3),f(4),f(5),f(6),short_(words[7]),short_(words[8])};
    const WeaponBarrelTimingConstants constants{f(10),f(11)};
    WeaponBarrelTimingContext context;context.tick_rate=short_(words[9]);context.spent=f(12);
    if(words[13]!=0xffffffffu)context.alternate_mode=std::uint8_t(words[13]);
    if(words[14]!=0xffffffffu)context.other_hand=WeaponBarrelTimingContext::OtherHand{
        std::uint8_t(words[14]),std::bit_cast<std::int8_t>(std::uint8_t(words[15]))};
    std::array<std::uint32_t,13> raw;
    std::copy_n(words.begin()+16,raw.size(),raw.begin());
    auto state=std::bit_cast<SmgBarrelState>(raw);
    WeaponBarrelGate gate;
    switch(words[0]) {
    case 0:gate=weapon_barrel_gate(state,definition,context,constants);break;
    case 1:weapon_barrel_begin_recovery(state,definition,context.tick_rate,constants);break;
    case 2:weapon_barrel_finish_recovery(state,definition,context.tick_rate,constants);break;
    case 3:weapon_barrel_reset(state,definition);break;
    default:throw FormatError("Unknown original barrel operation");
    }
    std::array<std::uint32_t,16> result{gate.admitted,gate.minimum_met,gate.maximum_met};
    raw=std::bit_cast<std::array<std::uint32_t,13>>(state);
    std::copy(raw.begin(),raw.end(),result.begin()+3);
    for(unsigned n=0;n<result.size();++n)if(result[n]!=words[29+n]) {
      std::cerr<<"operation="<<words[0]<<" case="<<cases[words[0]]<<" output="<<n
          <<" expected="<<std::hex<<words[29+n]<<" actual="<<result[n]<<'\n';
      throw FormatError("Native barrel timing differs from original TU1.5/TU1.1");
    }
    ++cases[words[0]];
  }
  for(auto n:cases)if(n<100)throw FormatError("Original barrel operation coverage missing");
  std::cout<<"PASS original TU1.5/TU1.1 gate="<<cases[0]<<" recovery="<<cases[1]
      <<" recovery tail="<<cases[2]<<" reset="<<cases[3]<<"; all 52 retained barrel bytes exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
