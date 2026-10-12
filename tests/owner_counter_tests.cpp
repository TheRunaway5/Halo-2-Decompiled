#include "halo2/owner_counter.hpp"
#include <bit>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>
using namespace halo2;
namespace {
std::uint32_t word(std::istringstream &row) {
  std::string value;if(!(row>>value))throw std::runtime_error("Incomplete original owner fixture");
  return std::stoul(value,nullptr,16);
}
std::int16_t short_word(std::istringstream &row) {
  return std::bit_cast<std::int16_t>(std::uint16_t(word(row)));
}
void check(bool value,const std::string &name) {
  if(!value)throw std::runtime_error("Original incoming-owner counter differs: "+name);
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original owner-counter fixture");
    if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream file(argv[1]);unsigned cases=0;
    for(std::string line;std::getline(file,line);) {
      std::istringstream row(line);std::string name;row>>name;
      OwnerCounterInput input;
      input.tick=word(row);input.incoming_player=word(row);input.incoming_object=word(row);
      input.seconds_per_tick=std::bit_cast<float>(word(row));input.damage_class=short_word(row);
      input.incoming_designator=short_word(row);input.victim_membership=short_word(row);
      input.lethal=std::uint8_t(word(row));input.enabled_cap=std::uint8_t(word(row));
      std::vector<OwnerCounterPlayer> players(word(row));
      for(auto &p:players) {p.datum=word(row);p.current_unit=word(row);}
      std::vector<OwnerCounterObject> objects(word(row));
      std::vector<OwnerCounterState> expected;
      for(auto &o:objects) {
        o.datum=word(row);o.player=word(row);o.forward_9=word(row);o.forward_other=word(row);
        o.type=std::uint8_t(word(row));o.body_present=word(row);o.dead=word(row);
        o.counter.count=short_word(row);o.counter.last_tick=word(row);
        const auto count=short_word(row);const auto tick=word(row);expected.push_back({count,tick});
      }
      const auto selected=word(row);const auto result=incoming_owner_counter(input,players,objects);
      check(result== (selected==0xffffffffu ? std::optional<std::size_t>{} : std::optional<std::size_t>{selected}),name+" recipient");
      for(unsigned i=0;i<objects.size();++i)
        check(objects[i].counter.count==expected[i].count && objects[i].counter.last_tick==expected[i].last_tick,
              name+" full body state"+std::to_string(i));
      check(!row.fail(),name+" incomplete row");std::string extra;check(!(row>>extra),name+" trailing word");++cases;
    }
    check(cases==81,"incomplete original cases");
    std::cout<<cases<<" original owner admission, forwarding, timing, wrap, cap and salted lifecycle states matched\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
