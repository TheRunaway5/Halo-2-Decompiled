// Isolated CPU-only observer of the actual Cache/Match admission and loadout.
// Links an explicitly pinned immutable native checkpoint. Only fire controls
// are supplied; no unit/ammo/pose/weapon-readiness/health/physics mutations.
#include "halo2/cache.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/original_data.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
std::string quote(const std::string &s) {
  std::ostringstream o;
  o << '"';
  for(unsigned char c:s) {
    if(c=='"'||c=='\\')o << '\\' << char(c);
    else if(c<32)o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unsigned(c) << std::dec;
    else o << char(c);
  }
  o << '"';
  return o.str();
}
}
int main(int argc,char **argv) {
  if(argc<4)return 2; // shared maps, actual gameplay XBE, then MP map paths.
  const auto original=OriginalData::load_xbe(argv[2]);
  for(int i=3;i<argc;++i) {
    try {
      Cache cache(argv[i],argv[1],original);
      const auto content=cache.load_content(0,true);
      std::cout << "{\"map\":" << quote(argv[i]) << ",\"imported\":[";
      std::map<std::string,const WeaponDefinition *> ordered;
      for(const auto &[id,w]:content.weapons)ordered.emplace(w.name,&w);
      bool comma=false;
      for(const auto &[name,w]:ordered) {
        if(comma)std::cout << ',';comma=true;
        std::cout << "{\"name\":" << quote(name) << ",\"datum\":" << w->datum
                  << ",\"capacity\":" << w->capacity << ",\"smg\":" << (w->smg?"true":"false") << '}';
      }
      std::cout << "],\"loadouts\":[";
      for(unsigned option:{0u,5u}) {
        if(option)std::cout << ',';
        std::cout << "{\"option\":" << option;
        try {
          auto rules=slayer_rules({"Slayer",0,{{0x59,int(option)}}});
          rules.weapon=cache.slayer_starting_weapon(option);
          const auto &w=content.weapons.at(rules.weapon);
          std::cout << ",\"selected\":" << quote(w.name) << ",\"datum\":" << w.datum;
          MultiplayerMatch match(content,0,cache.spawns(),rules);
          const auto slot=match.join("Inventory observer");
          const auto initial=match.unit(slot).weapon.loaded;
          const auto unit=match.unit(slot).source_datum;
          std::array<Controls,multiplayer_capacity> controls{};
          controls[slot].fire=true;
          unsigned shot_changes=0;std::uint64_t last=match.unit(slot).weapon.last_shot;
          for(unsigned tick=0;tick<90;++tick) {
            match.step(controls);
            const auto now=match.unit(slot).weapon.last_shot;
            if(now!=last)++shot_changes;
            last=now;
          }
          const auto &u=match.unit(slot);
          std::cout << ",\"admitted\":true,\"initial_loaded\":" << initial
                    << ",\"after_loaded\":" << u.weapon.loaded << ",\"initial_unit\":" << unit
                    << ",\"after_unit\":" << u.source_datum << ",\"after_dead\":" << (u.vitality.dead?"true":"false")
                    << ",\"ticks\":" << match.state().tick << ",\"shot_changes\":" << shot_changes;
        } catch(const std::exception &e) {
          std::cout << ",\"admitted\":false,\"error\":" << quote(e.what());
        }
        std::cout << '}';
      }
      std::cout << "]}" << std::endl;
    } catch(const std::exception &e) {
      std::cout << "{\"map\":" << quote(argv[i]) << ",\"error\":" << quote(e.what()) << '}';
      std::cout << std::endl;
    }
  }
}
