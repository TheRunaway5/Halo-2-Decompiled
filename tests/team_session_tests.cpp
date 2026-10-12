#include "halo2/multiplayer.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const char *message) { if (!value) throw FormatError(message); }
template<class T> void read(std::istringstream &row, T &value) {
  if (!(row >> value)) throw FormatError("Incomplete original team-session fixture");
}
std::vector<std::int8_t> members(std::istringstream &row) {
  unsigned count;read(row,count);check(count <= 16,"Invalid original roster size");
  std::vector<std::int8_t> result;
  for(unsigned i=0;i<count;++i) {int value;read(row,value);result.push_back(std::int8_t(value));}
  return result;
}
}
int main(int argc, char **argv) {
  try {
    if(argc != 3) throw FormatError("Supply project root and original team-session fixtures");
    Cache cache(std::filesystem::path(argv[1])/"assets/disc/maps/lockout.map");
    const auto content = cache.load_content(0,true);
    check(bool(content.multiplayer_spawn),"Original Lockout spawn settings missing");
    const auto &settings = *content.multiplayer_spawn;
    std::ifstream file(argv[2]);check(bool(file),"Original team-session fixtures missing");
    std::array<unsigned,4> counts{};
    for(std::string line;std::getline(file,line);) {
      if(line.empty() || line[0]=='#') continue;
      std::istringstream row(line);char kind;read(row,kind);
      if(kind == 'M') {
        unsigned present;int maximum;read(row,present);read(row,maximum);
        const auto roster = members(row);
        unsigned session,assigned,teams;read(row,session);read(row,assigned);read(row,teams);
        const auto mask=source_session_team_mask(std::uint16_t(present),maximum,roster);
        const auto actual=source_team_mapping(std::uint16_t(present),mask);
        check(mask==session && actual.assigned==assigned && actual.teams==teams,
            "Original session/assigned/logical team masks differ");
        for(auto designator:actual.designators) {
          int expected;read(row,expected);check(designator==expected,"Original team designator mapping differs");
        }
        ++counts[0];
      } else if(kind == 'S') {
        int team_mode,initial,member,maximum;unsigned present;
        read(row,team_mode);read(row,initial);read(row,member);read(row,maximum);read(row,present);
        const auto roster=members(row);
        const auto mapping=source_team_mapping(std::uint16_t(present),
            source_session_team_mask(std::uint16_t(present),maximum,roster));
        auto influences=source_spawn_zone_bank(initial ? settings.source_initial_zones : settings.source_respawn_zones,
            2,team_mode,std::int8_t(member),mapping,[](const auto &){return true;});
        unsigned player_count;read(row,player_count);
        for(unsigned i=0;i<player_count;++i) {
          int membership,dead;Vec3 position;
          read(row,membership);read(row,dead);read(row,position.x);read(row,position.y);read(row,position.z);
          if(dead || influences.size()==128) continue;
          const auto &type=settings.types[member==membership ? 1 : 0];
          influences.push_back({position,type.inner,type.outer,settings.upper,settings.lower,type.weight});
        }
        unsigned expected_count;read(row,expected_count);
        check(influences.size()==expected_count,"Original team static/player influence count differs");
        for(const auto &influence:influences)
          for(float value:{influence.position.x,influence.position.y,influence.position.z,
              influence.inner,influence.outer,influence.upper,influence.lower,influence.weight}) {
            std::uint32_t expected;read(row,expected);
            check(std::bit_cast<std::uint32_t>(value)==expected,"Original team influence float bits differ");
          }
        int selected;read(row,selected);
        std::vector<Vec3> points;for(const auto &start:cache.spawns())points.push_back(start.position);
        auto seed=spawn_private_seed(0x10000,0x13579bdf,0x2468ace0,0);
        const auto choice=choose_spawn(points,influences,settings.random_bias,seed,[](auto,auto){return true;});
        check(choice.index==selected,"Original actual-map team spawn selection differs");
        ++counts[1];
      } else if(kind == 'T') {
        int modifier,attacker;read(row,modifier);read(row,attacker);
        std::array<RespawnTimer,3> timers{};
        for(auto &timer:timers)read(row,timer.countdown);
        std::array<std::int32_t,3> after{},penalties{};
        for(auto &value:after)read(row,value);
        for(auto &value:penalties)read(row,value);
        int elapsed;read(row,elapsed);
        if(attacker==0 && !timers[1].countdown)timers[0].penalty+=300;
        const std::array<TeamRespawnPlayer,3> roster{{{0,1,timers[0].countdown},
            {0,1,timers[1].countdown},{1,1,timers[2].countdown}}};
        initialize_team_respawn_timer(timers[1],0,roster,200,0,150,30,attacker==3 ? 150 : 0,modifier);
        for(unsigned i=0;i<3;++i)
          check(timers[i].countdown==after[i] && timers[i].penalty==penalties[i],
              "Original team countdown/attacker penalty differs");
        check(timers[1].death_elapsed==elapsed,"Original team death elapsed time differs");
        ++counts[2];
      } else if(kind == 'P') {
        unsigned previous,count;read(row,previous);read(row,count);
        std::vector<TeamRespawnPlayer> roster;
        for(unsigned i=0;i<count;++i) {
          int member;unsigned flags;read(row,member);read(row,flags);
          roster.push_back({std::int8_t(member),std::uint16_t(flags),0});
        }
        unsigned playing,retained;read(row,playing);read(row,retained);
        const auto actual=source_team_presence(std::uint16_t(previous),roster);
        check(actual.playing==playing && actual.retained==retained,"Original playing/sticky team presence differs");
        ++counts[3];
      } else throw FormatError("Unknown original team-session fixture kind");
      std::string extra;check(!(row>>extra),"Trailing original team-session fields");
    }
    check(counts==std::array<unsigned,4>{168,40,48,40},"Incomplete original team-session comparison suite");
    std::cout<<counts[0]<<" original session mappings, "<<counts[1]<<" actual-map team spawn banks/choices and "
        <<counts[2]<<" original team respawn-prefix cases and "<<counts[3]<<" sticky team presence cases matched\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
