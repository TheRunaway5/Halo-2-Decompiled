#include "halo2/multiplayer.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
std::uint32_t word(std::istringstream &row) {
  std::string value;
  if (!(row >> value)) throw FormatError("Missing source placement field");
  return std::stoul(value, nullptr, 16);
}
void check(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Supply project and local placement fixtures");
    if (!std::filesystem::exists(argv[2])) return 77;
    const std::filesystem::path root(argv[1]);
    Cache cache(root / "assets/disc/maps/lockout.map");
    const auto content = cache.load_content(0, true);
    auto rules = slayer_rules({"Source placement", 0, {{0x59, 5}}});
    rules.weapon = cache.slayer_starting_weapon(5);
    rules.time_ticks = 0;
    std::ifstream file(argv[2]);
    unsigned cases = 0, metadata_cases = 0;
    for (std::string line; std::getline(file, line);) {
      std::istringstream row(line);
      unsigned index, bsp;
      if (!(row >> index >> bsp)) throw FormatError("Missing source placement identity");
      auto start = cache.spawns().at(index);
      start.bsp = std::uint16_t(bsp);
      std::array<std::uint32_t, 3> expected;
      for (auto &value : expected) value = word(row);
      const auto facing = word(row);
      std::string extra;
      if (row >> extra) throw FormatError("Trailing source placement data");
      MultiplayerMatch match(content, 0, std::span(&start, 1), rules);
      match.join("Placement");
      const auto &unit = match.unit(0);
      unsigned axis = 0;
      for (float value : {unit.body.position.x, unit.body.position.y, unit.body.position.z})
        check(std::bit_cast<std::uint32_t>(value) == expected[axis++],
              "Native placement moved an original authored coordinate");
      check(std::bit_cast<std::uint32_t>(unit.yaw) == facing,
            "Native placement changed authored facing");
      check(match.player(0).source_current_unit && !match.player(0).source_initial_spawn &&
                match.player(0).source_spawn_count == 1 && unit.exists && !unit.vitality.dead,
            "Native successful placement state differs");
      ++cases;
      if (bsp == 7) ++metadata_cases;
    }
    check(cases == 67 && metadata_cases == 1, "Incomplete source placement suite");
    MultiplayerMatch empty(content, 0, {}, rules);
    const auto a = empty.join("Awaiting initial start"), b = empty.join("Also awaiting");
    std::array<Controls, multiplayer_capacity> controls{};
    for (unsigned tick = 0; tick < 100; ++tick) {
      empty.step(controls);
      for (unsigned id : {a, b}) {
        const auto &p = empty.player(id);
        check(p.connected && !p.source_current_unit && p.source_initial_spawn &&
                  p.source_spawn_count == 0 && p.respawn.countdown == 0 && p.deaths == 0,
              "Failed initial selection changed player lifecycle state");
        check(!empty.unit(id).exists, "Failed selection invented a player unit");
      }
    }
    std::cout << "66 native authored placements, one source BSP override and 100 failed-retry phases matched\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
