#include "halo2/content.hpp"
#include "halo2/inventory.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Netgame test requires local root/fixtures");
    if (!std::filesystem::exists(argv[2])) return 77;
    const std::filesystem::path root(argv[1]);
    Cache cache(root/"assets/disc/maps/lockout.map",root/"assets/disc/maps");
    const auto content=cache.load_content(0,true);
    std::ifstream file(argv[2]);
    std::string line;
    unsigned imports=0,visits=0,requests=0;
    for (;std::getline(file,line);) {
      std::istringstream row(line);
      std::string kind;unsigned index;
      if (!(row>>kind>>index)) throw FormatError("Malformed netgame fixture");
      const auto &entry=content.netgame_equipment.at(index);
      const auto integer=[&](auto value) {
        long long expected;
        if (!(row>>expected) || expected!=static_cast<long long>(value))
          throw FormatError("Imported netgame integer differs");
      };
      const auto word=[&]() {
        std::string text;
        if (!(row>>text)) throw FormatError("Missing netgame word");
        return std::uint32_t(std::stoul(text,nullptr,16));
      };
      const auto real=[&](float value) {
        if (std::bit_cast<std::uint32_t>(value)!=word())
          throw FormatError("Imported netgame float differs");
      };
      if (kind=="equipment") {
        integer(entry.flags);
        for (auto mode:entry.modes) integer(mode);
        integer(entry.team);integer(entry.respawn_seconds);integer(entry.empty_seconds);
        integer(entry.timer_start);integer(entry.classification);
        for (float value:{entry.position.x,entry.position.y,entry.position.z,
                          entry.rotation.x,entry.rotation.y,entry.rotation.z}) real(value);
        if (entry.collection!=word()) throw FormatError("Netgame collection differs");
        integer(entry.collection_delay);
        if (entry.items.size()!=1) throw FormatError("Expected source one-item collection");
        real(entry.items[0].weight);
        if (entry.items[0].definition!=word() || entry.items[0].variant!=word())
          throw FormatError("Source collection choice differs");
        ++imports;
      } else if (kind=="visit") {
        std::int32_t tick;std::uint32_t flags;int option;
        if (!(row>>tick>>flags)) throw FormatError("Missing source visit input");
        Vec3 position;
        position.x=std::bit_cast<float>(word());position.y=std::bit_cast<float>(word());
        position.z=std::bit_cast<float>(word());
        if (!(row>>option)) throw FormatError("Missing source timer option");
        NetgameWeaponState state;
        state.object=word();
        int timer,empty,expected_request;std::uint32_t allocation;
        if (!(row>>timer>>empty>>expected_request>>allocation))
          throw FormatError("Missing source timer state");
        state.timer=std::int16_t(timer);state.empty_counter=std::int16_t(empty);
        if (entry.classification || entry.timer_start ||
            !std::any_of(entry.modes.begin(),entry.modes.end(),[](auto m){return m==12;}))
          throw FormatError("Fixture outside bounded stock weapon path");
        const auto requested=netgame_weapon_timer(state,index,tick,30,true,false,
            entry.position,position,flags,entry.respawn_seconds,option);
        if (requested!=bool(expected_request)) throw FormatError("Source allocation phase differs");
        if (requested) {netgame_weapon_spawn_result(state,allocation);++requests;}
        if (state.object!=word()) throw FormatError("Source row identity differs");
        integer(state.timer);integer(state.empty_counter);++visits;
      } else throw FormatError("Unknown netgame fixture operation");
      std::string extra;
      if (row>>extra) throw FormatError("Trailing netgame fixture data");
    }
    if (!imports || !visits || !requests) throw FormatError("Empty netgame coverage");
    std::cout<<"Actual source imports="<<imports<<" timer visits="<<visits
             <<" allocation requests="<<requests<<'\n';
    return 0;
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
