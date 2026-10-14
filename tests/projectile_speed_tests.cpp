#include "halo2/native_weapon_projectile.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw FormatError("Supply original projectile speed fixture");
  std::ifstream input(argv[1]);if(!input)return 77;
  std::string line;unsigned cases{},player_cases{},difficulty_cases{},missing_cases{};
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,11> v{};
    for(auto &x:v)if(!(row>>std::hex>>x))throw FormatError("Truncated original projectile speed fixture");
    std::optional<std::array<float,4>> table;
    if(v[5]==3)table=std::array{std::bit_cast<float>(v[6]),std::bit_cast<float>(v[7]),std::bit_cast<float>(v[8]),std::bit_cast<float>(v[9])};
    const auto actual=source_projectile_speed_scale(v[1],v[2],v[3],std::bit_cast<std::int16_t>(std::uint16_t(v[4])),table);
    if(std::bit_cast<std::uint32_t>(actual)!=v[10]) {
      std::cerr<<line<<" native="<<std::hex<<std::bit_cast<std::uint32_t>(actual)<<'\n';
      throw FormatError("Native projectile speed differs from original owner");
    }
    ++cases;if(v[1]&0x400u){if(v[2]!=0xffffffffu)++player_cases;else if(table)++difficulty_cases;else ++missing_cases;}
  }
  if(cases!=1920 || player_cases!=768 || difficulty_cases!=96 || missing_cases!=288)
    throw FormatError("Original projectile speed coverage incomplete");
  std::cout<<"PASS "<<cases<<" original TU1.1/TU1.5 speed scalar cases, player/difficulty/missing-table paths exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
