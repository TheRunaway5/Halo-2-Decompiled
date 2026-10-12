#include "halo2/multiplayer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
bool same(const RespawnTimer &a, const RespawnTimer &b) {
  return a.countdown==b.countdown && a.penalty==b.penalty &&
      a.displayed_seconds==b.displayed_seconds && a.death_elapsed==b.death_elapsed;
}
}
int main(int argc, char **argv) {
  try {
    if (argc!=3) throw FormatError("Supply project root and local timing fixtures");
    if (!std::filesystem::exists(argv[2])) return 77;
    std::ifstream file(argv[2]);
    unsigned visits=0, deaths=0;
    for (std::string line;std::getline(file,line);) {
      std::istringstream row(line);std::string kind;row>>kind;
      RespawnTimer actual,expected;
      std::uint32_t clock,start,datum;int rate,phase;
      if (kind=="death") {
        std::int32_t interval,second,suicide;
        row>>clock>>start>>interval>>second>>suicide>>actual.penalty
            >>actual.displayed_seconds>>actual.death_elapsed;
        initialize_respawn_timer(actual,clock,start,interval,second,suicide);
        row>>expected.countdown>>expected.penalty>>expected.displayed_seconds>>expected.death_elapsed;
        ++deaths;
      } else if(kind=="tick") {
        row>>datum>>clock>>start>>rate>>phase>>actual.countdown>>actual.penalty
            >>actual.displayed_seconds>>actual.death_elapsed;
        row>>expected.countdown>>expected.penalty>>expected.displayed_seconds>>expected.death_elapsed;
        int eligible;unsigned notice;row>>eligible>>notice;
        const auto result=tick_respawn_timer(actual,datum,clock,start,rate,true,true,phase);
        if(result.eligible!=bool(eligible) || result.notice!=notice)
          throw FormatError("Original eligibility or notification differs");
        ++visits;
      } else throw FormatError("Unknown timing fixture row");
      if (!row || !same(actual,expected)) throw FormatError("Original signed timer state differs");
      std::string trailing;if(row>>trailing) throw FormatError("Trailing timing fixture fields");
    }
    if(!visits || !deaths) throw FormatError("Empty original timing fixture");
    Cache cache(std::filesystem::path(argv[1])/"assets/disc/maps/lockout.map");
    auto content=cache.load_content(0,true);
    // Isolate the recorded death/ownership-release boundary from unrelated
    // native fall/kill-volume approximation. No projectile hit is supplied.
    content.kill_triggers.clear();
    for (unsigned slot:{0u,3u,15u}) {
      auto rules=slayer_rules({"Respawn source",0,{{0x59,5}}});
      rules.weapon=cache.slayer_starting_weapon(5);rules.respawn_ticks=30;
      rules.time_ticks=0;rules.score_limit=0;
      MultiplayerMatch match(content,0,cache.spawns(),rules);
      for(unsigned i=0;i<=slot;++i) match.join("Timing");
      for(unsigned i=0;i<slot;++i) match.leave(i);
      std::array<Controls,16> controls{};
      controls[slot].yaw=match.unit(slot).yaw;
      while(match.simulation().ticks()<100) match.step(controls);
      auto &unit=const_cast<Unit &>(match.unit(slot));
      unit.vitality.dead=true;unit.last_damager=-2;
      match.step(controls);
      if(match.player(slot).respawn.countdown!=30 || match.player(slot).respawn.death_elapsed!=100)
        throw FormatError("Native death used the advanced clock");
      const unsigned source_spawn_clock=130+((slot-130)&31u);
      while(match.simulation().ticks()<source_spawn_clock) {
        match.step(controls);
        if(!match.unit(slot).vitality.dead) throw FormatError("Native respawn precedes original slot gate");
      }
      match.step(controls);
      if(match.unit(slot).vitality.dead || match.simulation().ticks()!=source_spawn_clock+1 ||
          !match.player(slot).source_current_unit || match.player(slot).deaths!=1)
        throw FormatError("Native early respawn timing or ownership admission differs");
    }
    std::cout<<deaths<<" original death writes, "<<visits
        <<" eligibility calls and three native Lockout slot timelines matched\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
