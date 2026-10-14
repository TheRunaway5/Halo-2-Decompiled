#include "halo2/weapon_energy.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original animation timing fixture");
  std::ifstream input(argv[1]);if(!input)return 77;
  unsigned count{};std::string line;
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,8> words;
    for(auto &word:words)if(!(row>>std::hex>>word))throw std::runtime_error("Truncated animation timing fixture");
    const auto value=[&](unsigned index){return std::bit_cast<std::int16_t>(std::uint16_t(words[index]));};
    const auto actual=words[0]?weapon_animation_timing(value(1),value(2),value(3),value(4),value(5),std::bit_cast<float>(words[6])):0;
    if(std::uint16_t(actual)!=words[7]) {
      std::cerr<<"Animation timing case "<<count<<" section "<<value(4)<<'\n';
      throw std::runtime_error("Animation section timing differs from unchanged original");
    }
    ++count;
  }
  if(count!=25515)throw std::runtime_error("Incomplete animation timing coverage");
  std::cout<<"PASS "<<count<<" original animation timing sections/conversions\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
