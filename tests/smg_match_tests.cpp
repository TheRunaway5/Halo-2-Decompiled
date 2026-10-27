#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 3 && argc != 4) throw FormatError("SMG match test requires map and instruction fixtures");
    const bool lan = argc == 4 && std::string(argv[3]) == "--lan";
    if (!std::filesystem::exists(argv[1])) return 77;
    Cache cache(argv[1]);
    auto content = cache.load_content(0, true);
    auto rules = slayer_rules({"SMG trace", 0, {{0x59, 5}}});
    rules.weapon = cache.slayer_starting_weapon(5);
    rules.time_ticks = rules.score_limit = 0;
    rules.random_seed = 0x12345678; // supplied original instruction fixture
    if (!content.weapons.at(rules.weapon).smg)
      throw FormatError("Real SMG did not enter recovered native path");
    std::ifstream file(argv[2]);
    if (!file) throw FormatError("Missing original instruction fixtures");
    std::unique_ptr<MultiplayerMatch> match;
    std::unique_ptr<LanHost> host;
    std::unique_ptr<LanClient> client;
    std::string line, active_case;
    unsigned rows = 0;
    for (; std::getline(file, line);) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream row(line);
      std::string name, weapon_flags, trigger_flags, barrel_hex;
      unsigned tick, held, reload, loaded, reserve, mode, timer, transfer, hand;
      if (!(row >> name >> tick >> held >> reload >> loaded >> reserve >> mode >>
            timer >> transfer >> hand >> weapon_flags >> trigger_flags >> barrel_hex))
        throw FormatError("Malformed original SMG fixture");
      if (name != active_case) {
        active_case = name;
        client.reset(); host.reset();
        match = std::make_unique<MultiplayerMatch>(content, 0, cache.spawns(), rules);
        if (lan) {
          host = std::make_unique<LanHost>(*match, 0, 0x1021d0);
          client = std::make_unique<LanClient>(host->join_address(), 0x1021d0, 1);
          std::array<Controls, 1> local{};
          std::array<Controls, 16> remote{};
          for (unsigned attempt = 0; attempt < 64 && !client->ready(); ++attempt) {
            client->send(local); host->receive(remote); host->broadcast(); client->receive();
          }
          if (!client->ready() || client->slots().size() != 1 || client->slots()[0] != 0)
            throw FormatError("Original-map UDP trace did not join slot zero");
        } else match->join("Original trace");
        // This frozen suite supplies a parented, manager-absent bare weapon,
        // with zero initial magazine timers. Keep that exact fixture explicit;
        // production initial equip is verified by the separate constructor suite.
        auto &bare = const_cast<Unit &>(match->unit(0)).weapon.smg;
        bare.own_manager = false; bare.weapon_state = 0;
        bare.hand_timer_registered = true; // Original bare fixture registers its exact weapon in FPS+10.
        bare.magazine_ticks = bare.transfer_ticks = bare.magazine_other_ticks = 0;
      }
      std::array<Controls, 16> controls{};
      controls[0].fire = held;
      controls[0].reload = reload;
      controls[0].yaw = match->unit(0).yaw;
      if (match->state().tick != tick) throw FormatError("Nonsequential original trace");
      if (lan) {
        std::array<Controls, 1> local{controls[0]};
        client->send(local); host->receive(controls);
      }
      match->step(controls);
      if (lan) { host->broadcast(); client->receive(); }
      const auto &weapon = match->unit(0).weapon;
      const auto &s = weapon.smg;
      std::string actual_hex;
      const auto *bytes = reinterpret_cast<const unsigned char *>(&s.barrel);
      constexpr char hex[] = "0123456789abcdef";
      for (unsigned i = 0; i < sizeof(s.barrel); ++i) {
        actual_hex += hex[bytes[i] >> 4]; actual_hex += hex[bytes[i] & 15];
      }
      if (match->unit(0).vitality.dead || weapon.loaded != int(loaded) ||
          weapon.reserve != int(reserve) || s.magazine_mode != int(mode) ||
          std::uint16_t(s.magazine_ticks) != timer ||
          std::uint16_t(s.transfer_ticks) != transfer || s.hand_timers[0] != hand ||
          s.weapon_flags != std::stoul(weapon_flags, nullptr, 16) ||
          s.trigger_flags != std::stoul(trigger_flags, nullptr, 16) || actual_hex != barrel_hex)
        throw FormatError("Actual match differs from original " + name + " tick " + std::to_string(tick));
      const auto published = lan ? client->state() : match->state();
      const auto replicated = published.players[0];
      if (replicated.loaded != weapon.loaded || replicated.reserve != weapon.reserve ||
          published.tick != tick + 1 || replicated.reload_tick != weapon.reload_started ||
          replicated.shot_tick != weapon.last_shot ||
          replicated.reload_empty != s.reload_empty ||
          replicated.grenades != match->unit(0).grenades ||
          replicated.grenade_current != match->unit(0).grenade_current ||
          ((replicated.reload_seconds > 0) != (s.magazine_mode == 1)))
        throw FormatError("Actual match published inconsistent SMG state");
      ++rows;
    }
    if (!rows) throw FormatError("Empty original SMG fixtures");
    std::cout << (lan ? "Actual original-map UDP match matched " : "Actual original-map match matched ")
              << rows << " instruction-derived ticks\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
