#include "halo2/projectile_material_response.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv){try{
  if(argc!=2)throw FormatError("Supply original angle fixture");
  std::ifstream file(argv[1]);if(!file)return 77;
  unsigned cases{};std::string line;
  while(std::getline(file,line)){
    if(line.empty()||line[0]=='#')continue;
    std::istringstream row(line);std::uint32_t words[7];
    for(auto &word:words)if(!(row>>std::hex>>word))throw FormatError("Truncated original angle fixture");
    const auto f=[&](unsigned n){return std::bit_cast<float>(words[n]);};
    const auto got=source_projectile_vector_angle({f(0),f(1),f(2)},{f(3),f(4),f(5)});
    if(std::bit_cast<std::uint32_t>(got)!=words[6])throw FormatError("Original projectile vector angle differs at "+std::to_string(cases));
    ++cases;
  }
  if(cases<520)throw FormatError("Incomplete original angle coverage");
  for(std::int16_t response=-1;response<8;++response){
    const auto slot=source_projectile_material_effect_slot(response);
    const int wanted=response<0||response>6?-1:response<5?response+15:20;
    if((slot?int(*slot):-1)!=wanted)throw FormatError("Invalid original projectile effect slot");
  }
  std::cout<<"PASS "<<cases<<" original projectile angles and response slots\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
