#include "halo2/sound_variation.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv){try{
  if(argc!=5)throw FormatError("Supply LOCKOUT TRIPLICATE SHARED FIXTURE");
  std::array<Cache,2> caches{Cache(argv[1],argv[3]),Cache(argv[2],argv[3])};
  std::ifstream input(argv[4]);if(!input)throw FormatError("Missing original sound control fixture");
  std::map<std::pair<unsigned,std::uint32_t>,SoundVariationDefinition> definitions;
  std::string line;unsigned cases{};
  while(std::getline(input,line)){
    std::istringstream row(line);char kind;unsigned map;std::uint32_t sound;
    row>>kind>>map>>std::hex>>sound;
    const auto key=std::pair{map,sound};
    auto it=definitions.find(key);
    if(it==definitions.end())it=definitions.emplace(key,caches.at(map).load_sound_variation(sound)).first;
    const auto &d=it->second;
    if(kind=='C'){
      std::uint32_t control,base,pitch,range,gain;row>>control>>base>>pitch>>range>>gain;
      const float actual_pitch=source_sound_control_pitch(d,std::bit_cast<float>(base),std::bit_cast<float>(control));
      const auto actual_range=source_sound_pitch_range(d.pitch_ranges,-1,actual_pitch);
      const auto actual_gain=std::bit_cast<std::uint32_t>(source_sound_control_gain(d,std::bit_cast<float>(control)));
      if(std::bit_cast<std::uint32_t>(actual_pitch)!=pitch || std::uint16_t(actual_range)!=range || actual_gain!=gain){
        std::cerr<<"Case "<<std::dec<<cases<<" sound="<<std::hex<<sound<<" pitch="<<std::bit_cast<std::uint32_t>(actual_pitch)<<"/"<<pitch<<" range="<<actual_range<<"/"<<range<<" gain="<<actual_gain<<"/"<<gain<<'\n';
        throw FormatError("Original sound caller-control mismatch");
      }
    }else if(kind=='R'){
      int previous;std::uint32_t pitch,range;row>>std::dec>>previous>>std::hex>>pitch>>range;
      if(std::uint16_t(source_sound_pitch_range(d.pitch_ranges,std::int16_t(previous),std::bit_cast<float>(pitch)))!=range)throw FormatError("Original sound pitch-range mismatch");
    }else throw FormatError("Invalid sound fixture row");
    if(!row)throw FormatError("Truncated sound control fixture");++cases;
  }
  if(cases<100)throw FormatError("Missing sound control cases");
  std::cout<<"PASS "<<cases<<" original caller-control/range cases across "<<definitions.size()<<" authored sounds\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
