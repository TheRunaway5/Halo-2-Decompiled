#include "halo2/weapon_energy.hpp"
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)return 2;
  std::ifstream input(argv[1]);if(!input)return 77;
  std::array<unsigned,2> counts{};std::string line;
  while(std::getline(input,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::array<std::uint32_t,9> words;std::istringstream row(line);
    for(auto &word:words)if(!(row>>std::hex>>word))throw std::runtime_error("Truncated original preparation decision");
    const auto f=[&](unsigned index){return std::bit_cast<float>(words[index]);};
    auto seed=words[6];
    const bool result=words[0]==0?weapon_battery_misfire(f(5),f(1),f(2),f(3),f(4),seed):
                                 weapon_heat_detonation(f(5),f(1),f(2),f(4),seed);
    if(result!=bool(words[7]) || seed!=words[8]) {
      std::cerr<<"operation="<<words[0]<<" case="<<counts.at(words[0])<<" expected="<<std::hex
          <<words[7]<<'/'<<words[8]<<" actual="<<result<<'/'<<seed<<'\n';
      throw std::runtime_error("Native preparation decision/RNG differs from original");
    }
    ++counts.at(words[0]);
  }
  if(counts[0]<100 || counts[1]<100)throw std::runtime_error("Original preparation decision coverage missing");
  std::cout<<"PASS original TU1.5/TU1.1 battery misfire="<<counts[0]<<" heat detonation="<<counts[1]
      <<"; decisions and retained shared RNG exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
