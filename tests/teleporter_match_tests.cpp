#include "halo2/multiplayer.hpp"
#include <bit>
#include <iostream>
using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 4)
      throw FormatError("Supply Beaver Creek MAP SHARED_MAPS UPDATE_XBE");
    if (!std::filesystem::is_regular_file(argv[1]))
      return 77;
    Cache cache(argv[1], argv[2], OriginalData::load_xbe(argv[3]));
    auto content = cache.load_content(0, true);
    auto rules = slayer_rules(UiVariant{"Slayer", 0, {{89, 5}}});
    rules.weapon = cache.slayer_starting_weapon(rules.starting_weapon);
    MultiplayerMatch match(content, 0, cache.spawns(), rules);
    const auto player = match.join("Original entrance regression");
    // Literal original B8490 diagnostic input and complete1A0580 output:
    // teleporters-lifts-20261010/player-probe/report.json. No root/velocity,
    // contact, teleporter result or movement implementation is replaced.
    Spawn entrance{};
    entrance.position = {std::bit_cast<float>(0xc18376b3u),
                         std::bit_cast<float>(0xbd89f86bu),
                         std::bit_cast<float>(0x3e19999au)};
    match.debug_request({DebugGameRequest::Teleport, player}, &entrance);
    std::array<Controls, 16> controls{};
    controls[player].yaw = match.unit(player).yaw;
    match.step(controls);
    const auto p = match.unit(player).body.position;
    std::cout << "entrance=" << entrance.position.x << ","
              << entrance.position.y << " after native Match::step=" << p.x
              << "," << p.y << " original exit=18.3940754,-0.0493806005\n";
    const Vec3 exit{std::bit_cast<float>(0x41932711u),
                    std::bit_cast<float>(0xbd4a4350u),
                    std::bit_cast<float>(0x3e19999au)};
    const auto xy_distance = [](Vec3 a, Vec3 b) {
      const float x = a.x - b.x, y = a.y - b.y;
      return x * x + y * y;
    };
    // This seam includes the native world mover after the original engine
    // phase. It detects the missing traversal, not exact post-world
    // coordinates. Exact controller coordinates are tested separately
    // against1A0580.
    if (xy_distance(p, exit) >= xy_distance(p, entrance.position))
      throw FormatError("RED: live native match did not traverse original "
                        "Beaver Creek teleporter");
    std::cout
        << "PASS live native entrance-to-exit traversal; exact post-world "
           "coordinates and complete physics are separate\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
