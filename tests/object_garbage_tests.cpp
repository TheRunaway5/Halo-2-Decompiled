#include "halo2/object_garbage.hpp"
#include "halo2/spawn.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <filesystem>
using namespace halo2;
namespace {
std::uint32_t word(std::istringstream &row) {
  std::string value;
  if (!(row >> value)) throw std::runtime_error("Missing original garbage fixture word");
  return std::stoul(value, nullptr, 16);
}
ObjectGarbageState body(std::istringstream &row) {
  const auto flags=word(row), next=word(row), deadline=word(row);
  return {flags,next,std::bit_cast<std::int32_t>(deadline)};
}
ObjectGarbageQueue queue(std::istringstream &row) {
  const auto head=word(row), count=word(row);
  return {head,std::bit_cast<std::int16_t>(std::uint16_t(count))};
}
void check(bool value, const std::string &name) {
  if (!value) throw std::runtime_error("Original garbage decision differs: " + name);
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2) throw std::runtime_error("Supply original garbage fixture");
    if(!std::filesystem::exists(argv[1])) return 77;
    std::ifstream file(argv[1]); unsigned schedules=0,levels=0,pressures=0,gates=0,writers=0;
    for(std::string line;std::getline(file,line);) {
      std::istringstream row(line);std::string kind,name;row>>kind>>name;
      if(kind=="S") {
        const auto datum=word(row),active=word(row),tick=word(row),enable=word(row),interval=word(row);
        auto current=body(row);auto list=queue(row);
        const auto expected=body(row);const auto expected_list=queue(row);
        schedule_object_garbage(list,datum,current,active,enable,tick,
            std::bit_cast<std::int32_t>(interval),[&](std::uint32_t found)->ObjectGarbageState & {
              check(found==datum,name+" full list identity");return current;
            });
        check(current.flags==expected.flags && current.next==expected.next &&
              current.deadline==expected.deadline && list.head==expected_list.head &&
              list.active_count==expected_list.active_count,name);++schedules;
      } else if(kind=="L") {
        const auto request=word(row),force=word(row),heap=word(row),count=word(row),active=word(row),expected=word(row);
        check(object_garbage_level(request,force,std::bit_cast<std::int32_t>(heap),int(count),
            std::bit_cast<std::int16_t>(std::uint16_t(active)))==std::bit_cast<std::int32_t>(expected),name);++levels;
      } else if(kind=="P") {
        const auto level=word(row),heap=word(row),free=word(row),active=word(row),proceed=word(row),critical=word(row);
        const auto result=object_garbage_pressure(int(level),std::bit_cast<std::int32_t>(heap),int(free),
            std::bit_cast<std::int16_t>(std::uint16_t(active)));
        check(result.proceed==bool(proceed)&&result.critical==bool(critical),name);++pressures;
      } else if(kind=="C") {
        const auto level=word(row),tick=word(row),active=word(row),retained=word(row),expected=word(row);
        const auto current=body(row);
        check(object_garbage_collectible(int(level),tick,current,active,bool(retained))==std::optional<bool>(bool(expected)),name);
        // This compares the final decision, not BC970 invocation/side effects.
        // Missing retention is unresolved when it could change that decision.
        if(level && std::bit_cast<std::int32_t>(tick)>=current.deadline && !(level==2&&!active))
          check(!object_garbage_collectible(int(level),tick,current,active,std::nullopt).has_value(),name+" absent retention boundary");
        ++gates;
      } else if(kind=="D") {
        const auto tick=word(row),interval=word(row),penalty=word(row),countdown=word(row),elapsed=word(row);
        RespawnTimer state;
        initialize_respawn_timer(state,tick,0,int(interval),30,int(penalty));
        check(state.countdown==int(countdown)&&state.death_elapsed==int(elapsed)&&state.penalty==0,name);++writers;
      } else throw std::runtime_error("Unknown original garbage fixture");
      check(!row.fail(),name+" incomplete row");std::string extra;check(!(row>>extra),name+" extra word");
    }
    check(schedules==12 && levels==24 && pressures==9 && gates==19 && writers==3,"incomplete original fixture");
    std::cout<<schedules<<" original schedule states, "<<levels<<" dispatcher decisions, "<<pressures
        <<" pressure predicates, "<<gates<<" removal gates and "<<writers<<" real writer prefixes matched\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
