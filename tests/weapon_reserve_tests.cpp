#include "halo2/weapon_magazine.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original reserve fixtures");
  std::ifstream input(argv[1]);if(!input)return 77;
  unsigned count{};std::string line;
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,54> words;
    for(auto &word:words)if(!(row>>std::hex>>word))throw std::runtime_error("Truncated original reserve input");
    std::array<std::array<WeaponMagazineState,2>,5> states;
    std::array<WeaponMagazineDefinition,2> definitions;
    for(unsigned m=0;m<2;++m)definitions[m].maximum_reserve=std::bit_cast<std::int16_t>(std::uint16_t(words[12+m]));
    std::array<WeaponAmmunitionView,5> views;
    for(unsigned n=0;n<5;++n) {
      std::array<std::uint32_t,8> raw;std::copy_n(words.begin()+14+n*8,8,raw.begin());
      states[n]=std::bit_cast<std::array<WeaponMagazineState,2>>(raw);
      views[n]={0x12340000u+n,n && (words[7]&(1u<<n))?8u:7u,
          std::span(states[n]).first(words[1]),std::span(definitions).first(words[1])};
    }
    WeaponAmmunitionContext context;context.current=views[0];context.has_unit=words[3];context.infinite=words[4];
    for(unsigned n=0;n<4;++n)if(words[8+n]!=0xffffffffu)context.inventory[n]=views.at(words[8+n]);
    unsigned changed{};std::uint32_t result;
    if(words[0])result=weapon_reserve_spend(context,words[2],std::bit_cast<std::int32_t>(words[6]),[&]{++changed;});
    else result=std::uint32_t(weapon_reserve_available(context,words[2],words[5]));
    std::vector<std::uint32_t> actual{result,changed};
    for(const auto &state:states)for(auto value:std::bit_cast<std::array<std::uint32_t,8>>(state))actual.push_back(value);
    std::vector<std::uint32_t> expected;std::uint32_t word;
    while(row>>word)expected.push_back(word);
    if(actual!=expected) {
      std::cerr<<"Reserve case "<<count<<" operation "<<words[0]<<'\n';
      throw std::runtime_error("Shared reserve stores/result differ from untouched original");
    }
    ++count;
  }
  if(count!=15180)throw std::runtime_error("Incomplete original reserve coverage");
  std::cout<<"PASS "<<count<<" original shared reserve queries/spends across all four slots\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
