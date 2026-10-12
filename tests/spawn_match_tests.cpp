#include "halo2/multiplayer.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Spawn test requires project root and local fixtures");
    if (!std::filesystem::exists(argv[2])) return 77;
    const std::filesystem::path root(argv[1]);
    std::ifstream file(argv[2]);
    std::string line;
    unsigned cases = 0;
    for (; std::getline(file, line);) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream row(line);
      std::string path, seed, salt;
      int selected; unsigned count;
      if (!(row >> path >> seed >> salt >> selected >> count))
        throw FormatError("Malformed source spawn fixture");
      Cache cache(root / path, root / "assets/disc/maps");
      const auto content = cache.load_content(0, true);
      if (!content.multiplayer_spawn) throw FormatError("Missing actual-map spawn settings");
      const auto &settings = *content.multiplayer_spawn;
      auto check = [&](float value) {
        std::string expected;
        if (!(row >> expected) || std::bit_cast<std::uint32_t>(value) != std::stoul(expected, nullptr, 16))
          throw FormatError("Imported source spawn fields differ for " + path);
      };
      check(settings.upper); check(settings.lower);
      for (const auto &type : settings.types) { check(type.inner); check(type.outer); check(type.weight); }
      if (count != settings.initial_zones.size()) throw FormatError("Static initial zone bank differs");
      for (const auto &influence : settings.initial_zones)
        for (float value : {influence.position.x, influence.position.y, influence.position.z,
            influence.inner, influence.outer, influence.upper, influence.lower, influence.weight}) check(value);
      std::string extra;
      if (row >> extra) throw FormatError("Trailing source spawn fields");
      auto rules = slayer_rules({"Source spawn", 0, {{0x59, 5}}});
      rules.weapon = cache.slayer_starting_weapon(5);
      rules.random_seed = std::stoul(seed, nullptr, 16);
      rules.spawn_salt = std::stoul(salt, nullptr, 16);
      MultiplayerMatch match(content, 0, cache.spawns(), rules);
      match.join("Source initial");
      const auto &position = match.unit(0).body.position;
      const auto &expected = cache.spawns().at(selected);
      if (position.x != expected.position.x || position.y != expected.position.y ||
          position.z != expected.position.z || match.unit(0).yaw != expected.facing ||
          match.player(0).source_spawn_count != 1 || match.player(0).source_initial_spawn)
        throw FormatError("Actual-map initial spawn differs for " + path);
      ++cases;
    }
    if (!cases) throw FormatError("Empty source spawn fixtures");
    std::cout << "Actual native initial spawn/metadata matched " << cases << " original maps\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
