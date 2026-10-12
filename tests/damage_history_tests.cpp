#include "halo2/cache.hpp"
#include "halo2/damage_history.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value,const std::string &message) {if(!value)throw FormatError(message);}
std::uint32_t word(std::istringstream &row) {
  std::string value;if(!(row>>value))throw FormatError("Missing original damage-history field");
  return std::stoul(value,nullptr,16);
}
DamageHistory history(std::istringstream &row) {
  DamageHistory result;
  for(auto &entry:result) {
    entry.tick=word(row);entry.amount=std::bit_cast<float>(word(row));
    entry.object=word(row);entry.player=word(row);entry.cause=std::uint8_t(word(row));
  }
  return result;
}
void compare(const DamageHistory &actual,const DamageHistory &expected,const std::string &name) {
  for(unsigned i=0;i<actual.size();++i) {
    const auto &a=actual[i],&b=expected[i];
    check(a.tick==b.tick && std::bit_cast<std::uint32_t>(a.amount)==std::bit_cast<std::uint32_t>(b.amount) &&
        a.object==b.object && a.player==b.player && a.cause==b.cause,name+" entry "+std::to_string(i));
  }
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw FormatError("Supply original damage-history fixtures");
    if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream file(argv[1]);unsigned initializers=0,writes=0,consumers=0;
    for(std::string line;std::getline(file,line);) {
      std::istringstream row(line);char kind;unsigned id;row>>kind>>id;
      const auto name="Original damage history "+std::to_string(id);
      if(kind=='I') {
        auto state=history(row);const auto expected=history(row);
        state={};compare(state,expected,name);++initializers;
      } else if(kind=='H' || kind=='C') {
        const auto tick=word(row);const auto amount=std::bit_cast<float>(word(row));
        const auto shield=kind=='C'?std::bit_cast<float>(word(row)):0.f;
        const auto object=word(row),player=word(row),cause=word(row);
        auto state=history(row);const auto expected=history(row);
        if(kind=='H') {
          record_damage_history(state,tick,amount,object,player,std::uint8_t(cause));++writes;
        } else {
          record_damage_result_history(state,tick,amount,shield,object,player,std::uint8_t(cause));++consumers;
        }
        compare(state,expected,name);
      } else throw FormatError("Unknown original damage-history fixture");
      check(!row.fail(),"Incomplete original damage-history fixture");std::string extra;
      check(!(row>>extra),"Trailing original damage-history fixture");
    }
    check(initializers==3 && writes==14 && consumers==174,"Incomplete original damage-history suite");
    std::cout<<initializers<<" original history resets and "<<writes
        <<" history writes and "<<consumers<<" normalized result consumers matched with full salted identities\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
