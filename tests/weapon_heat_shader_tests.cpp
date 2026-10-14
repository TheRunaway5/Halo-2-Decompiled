#include "halo2/source_weapon_shader_input.hpp"
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
  std::string line;unsigned cases{};
  while(std::getline(input,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::array<std::uint32_t,5> words;std::istringstream row(line);
    for(auto &word:words)if(!(row>>std::hex>>word))throw std::runtime_error("Truncated original heat shader input");
    const SourceWeaponShaderState state{0,0,std::bit_cast<float>(words[1]),std::bit_cast<float>(words[0])};
    const auto value=source_weapon_shader_input(words[2],state);
    if(!value || std::bit_cast<std::uint32_t>(*value)!=words[3] || (*value>0.f)!=bool(words[4]))
      throw std::runtime_error("Heat/age shader input differs from complete original owner");
    ++cases;
  }
  if(cases<100)throw std::runtime_error("Original heat shader coverage missing");
  std::cout<<"PASS "<<cases<<" retail/TU1.1/TU1.5 heat/age inputs exact, including NaN payloads\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
