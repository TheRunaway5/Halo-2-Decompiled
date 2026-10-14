#include "halo2/weapon_magazine.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw std::runtime_error("Supply original magazine fixtures");
  std::ifstream input(argv[1]);if(!input)return 77;
  unsigned cases{},transfers{},transitions{};std::string line;
  for(;std::getline(input,line);) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::array<std::uint32_t,16> words;
    for(auto &word:words)if(!(row>>std::hex>>word))throw std::runtime_error("Truncated magazine input");
    std::array<std::uint32_t,4> before;std::copy_n(words.begin()+12,4,before.begin());
    auto state=std::bit_cast<WeaponMagazineState>(before);
    const auto short_word=[](std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));};
    const WeaponMagazineDefinition definition{words[1],short_word(words[2]),short_word(words[3]),
        std::bit_cast<float>(words[4])};
    std::vector<std::uint32_t> events;
    const auto event=[&](unsigned kind,std::uint32_t argument) {
      events.push_back(kind);events.push_back(argument);
      for(auto word:std::bit_cast<std::array<std::uint32_t,4>>(state))events.push_back(word);
    };
    if(words[0]==0) {
      WeaponMagazineTransferServices services;
      services.available=[&]{event(0,0);return std::bit_cast<std::int32_t>(words[7]);};
      services.spend=[&](std::int32_t amount){event(1,std::uint32_t(amount));};
      services.changed=[&]{event(2,0);};
      weapon_magazine_transfer(state,definition,words[6],services);++transfers;
    } else {
      WeaponMagazineTransitionServices services;
      services.activity_symbol=[&](unsigned action){event(3,action);return words[8];};
      services.timing=[&](std::uint32_t symbol,unsigned section) {
        if(symbol!=words[8] || (section!=0 && section!=3))throw std::runtime_error("Magazine timing ABI");
        event(4,section);return std::bit_cast<std::int32_t>(words[section==0?9:10]);
      };
      services.activity=[&](unsigned action){event(5,action);};
      weapon_magazine_transition(state,definition,short_word(words[11]),short_word(words[5]),services);++transitions;
    }
    const auto after=std::bit_cast<std::array<std::uint32_t,4>>(state);
    std::vector<std::uint32_t> actual(after.begin(),after.end());
    actual.push_back(unsigned(events.size()/6));actual.insert(actual.end(),events.begin(),events.end());
    std::vector<std::uint32_t> expected;std::uint32_t word;
    while(row>>word)expected.push_back(word);
    if(actual!=expected) {
      std::cerr<<"Magazine case "<<cases<<" operation "<<words[0]<<" differs\n";
      throw std::runtime_error("Magazine stores/ordered service calls differ from untouched original");
    }
    ++cases;
  }
  if(cases!=3052 || transfers!=964 || transitions!=2088)throw std::runtime_error("Incomplete magazine fixture coverage");
  std::cout<<"PASS "<<transfers<<" magazine transfers and "<<transitions
      <<" mode transitions: all16 retained bytes and ordered service ABI exact\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
