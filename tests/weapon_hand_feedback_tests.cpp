#include "halo2/weapon_hand_feedback.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw FormatError("Supply ORIGINAL_HAND_FEEDBACK_FIXTURE");
  std::ifstream input(argv[1]);if(!input)return 77;
  unsigned actions{},blends{},decays{},mapping{};
  for(std::string line;std::getline(input,line);) {
    std::istringstream row(line);char kind;row>>kind;
    if(kind=='A') {
      unsigned action,registered,secondary,rate,a,b,x,y;row>>std::hex>>action>>registered>>secondary>>rate>>a>>b>>x>>y;
      std::array<std::uint8_t,2> timers{std::uint8_t(a),std::uint8_t(b)};
      weapon_hand_feedback_action(timers,action,secondary,registered,std::bit_cast<std::int16_t>(std::uint16_t(rate)));
      if(timers[0]!=x || timers[1]!=y)throw FormatError("Original hand feedback action differs");++actions;
    } else if(kind=='B') {
      unsigned unit,weapon,primary,dt,a,b,expected;row>>std::hex>>unit>>weapon>>primary>>dt>>a>>b>>expected;
      const auto actual=std::bit_cast<unsigned>(weapon_hand_feedback_blend({std::uint8_t(a),std::uint8_t(b)},unit,weapon,primary,std::bit_cast<float>(dt)));
      if(actual!=expected){std::cerr<<std::hex<<"blend expected="<<expected<<" actual="<<actual<<" dt="<<dt<<" timers="<<a<<','<<b<<'\n';throw FormatError("Original opposite-hand blend differs");}++blends;
    } else if(kind=='T') {
      unsigned a,b,x,y;row>>std::hex>>a>>b>>x>>y;std::array<std::uint8_t,2> timers{std::uint8_t(a),std::uint8_t(b)};
      weapon_hand_feedback_tick(timers);if(timers[0]!=x || timers[1]!=y)throw FormatError("Original shared hand timer decay differs");++decays;
    } else if(kind=='M') {
      unsigned action,symbol;row>>std::hex>>action>>symbol;
      if(weapon_hand_feedback_action_id(symbol)!=(action>=1 && action<=4?action:0))
        throw FormatError("Original feedback activity mapping differs");++mapping;
    } else throw FormatError("Unknown hand feedback fixture row");
    if(!row)throw FormatError("Truncated hand feedback row");
  }
  if(actions!=1296 || blends!=16384 || decays!=1024 || mapping!=41)throw FormatError("Incomplete original hand feedback coverage");
  std::cout<<"PASS original TU1.1/TU1.5 actions="<<actions<<" blends="<<blends<<" decay="<<decays<<" mappings="<<mapping<<"; timer bytes and float bits exact\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
