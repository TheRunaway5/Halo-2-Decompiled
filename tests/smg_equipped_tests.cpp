#include "halo2/multiplayer_network.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const std::string &message) { if (!value) throw FormatError(message); }
std::uint32_t word(std::istringstream &row) {
  std::string value; if (!(row >> value)) throw FormatError("Missing equipped SMG field");
  return std::stoul(value, nullptr, 16);
}
struct Expected {
  std::array<std::uint32_t, 19> fields{};
  std::string barrel;
  std::uint32_t seed{};
};
Expected read(std::istringstream &row) {
  Expected state;
  for (auto &value : state.fields) value = word(row);
  row >> state.barrel; state.seed = word(row);
  return state;
}
void compare(const SmgState &state, const Expected &expected, const std::string &name) {
  const std::array<std::uint32_t, 19> actual{
      std::uint32_t(state.loaded), std::uint32_t(state.reserve), std::uint32_t(state.magazine_mode),
      std::uint32_t(state.magazine_ticks), std::uint32_t(state.transfer_ticks),
      std::uint32_t(state.magazine_other_ticks), std::uint32_t(state.blocking_ticks), state.weapon_state,
      state.trigger_state, std::uint32_t(state.trigger_ticks), state.trigger_flags, state.trigger_effect_handle,
      state.weapon_flags, state.input_flags, state.hand_timers[0], state.hand_timers[1],
      state.own_flags, state.own_events, state.own_manager};
  for (unsigned i = 0; i < actual.size(); ++i)
    check(actual[i] == expected.fields[i], name + " field " + std::to_string(i) +
        " native=" + std::to_string(actual[i]) + " original=" + std::to_string(expected.fields[i]));
  constexpr char hex[] = "0123456789abcdef";
  const auto *bytes = reinterpret_cast<const unsigned char *>(&state.barrel);
  std::string barrel;
  for (unsigned i = 0; i < sizeof(state.barrel); ++i) {
    barrel += hex[bytes[i] >> 4]; barrel += hex[bytes[i] & 15];
  }
  check(barrel == expected.barrel, name + " full barrel bytes");
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Supply original map and equipped SMG fixtures");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
    Cache cache(argv[1]); auto content = cache.load_content(0, true);
    content.kill_triggers.clear();
    const auto weapon_id = cache.slayer_starting_weapon(5);
    const auto &definition = *content.weapons.at(weapon_id).smg;
    check(definition.single_own_frames == 1, "Original selected own animation frame count differs");
    unsigned gates = 0, phase_ticks = 0, local_ticks = 0, udp_ticks = 0, requests = 0;
    for (int mode = 0; mode < 3; ++mode) {
      std::ifstream file(argv[2]);
      std::string active;
      SmgState isolated; std::uint32_t seed = 0x12345678;
      std::unique_ptr<MultiplayerMatch> match;
      std::unique_ptr<LanHost> host; std::unique_ptr<LanClient> client;
      for (std::string line; std::getline(file, line);) {
        std::istringstream row(line); char kind; row >> kind;
        if (kind == 'G') {
          SmgState state;
          state.weapon_state = word(row); state.trigger_state = std::uint8_t(word(row));
          state.second_trigger_state = std::uint8_t(word(row)); const auto expected = word(row);
          check(smg_reload_admitted(state) == bool(expected), "Original106030 three-operand predicate");
          if (mode == 0) ++gates;
        } else if (kind == 'I') {
          row >> active; const auto expected = read(row);
          if (mode == 0) {
            isolated = {}; isolated.loaded = 60; isolated.reserve = 120;
            isolated.hand_timer_registered = false; // Frozen actual weapon10002 is absent from167D32's registry.
            smg_initial_equip(isolated, definition); seed = 0x12345678;
            compare(isolated, expected, active + " initial original equip");
          } else {
            client.reset(); host.reset();
            auto rules = slayer_rules({"Equipped trace", 0, {{0x59, 5}}});
            rules.weapon = weapon_id; rules.time_ticks = rules.score_limit = 0;
            rules.random_seed = 0x12345678;
            match = std::make_unique<MultiplayerMatch>(content, 0, cache.spawns(), rules);
            if (mode == 2) {
              host = std::make_unique<LanHost>(*match, 0, 0x109a60);
              client = std::make_unique<LanClient>("127.0.0.1:" + std::to_string(host->port()), 0x109a60, 1);
              std::array<Controls, 16> controls{};
              for (unsigned i = 0; i < 64 && !client->ready(); ++i) {
                client->send(std::array<Controls, 1>{}); host->receive(controls);
                host->broadcast(); client->receive();
              }
              check(client->ready() && client->slots()[0] == 0, "Equipped source UDP join");
            } else match->join("Equipped source");
            compare(match->unit(0).weapon.smg, expected, active + " production initial equip");
          }
        } else if (kind == 'T') {
          const auto tick = word(row); const bool held = word(row), reload = word(row);
          const auto before = read(row), unit = read(row), weapon = read(row), postpass = read(row);
          const auto expected_requests = word(row);
          const auto name = active + " tick " + std::to_string(tick);
          if (mode == 0) {
            compare(isolated, before, name + " before");
            check(seed == before.seed, name + " previous shared seed");
            smg_input(isolated, held);
            if (reload) smg_reload(isolated, definition);
            compare(isolated, unit, name + " unit admission");
            smg_update(isolated, definition, tick_seconds);
            compare(isolated, weapon, name + " weapon update");
            check(seed == weapon.seed, name + " weapon shared seed");
            const bool emitted = bool(smg_dispatch(isolated, definition, {1, 0, 0}, seed,
                content.original_data->weapon_directions));
            check(emitted == bool(expected_requests), name + " projectile request presence");
            compare(isolated, postpass, name + " postpass");
            check(seed == postpass.seed, name + " original shared seed progression");
            ++phase_ticks; requests += emitted;
          } else {
            check(match->simulation().ticks() == tick, name + " nonsequential production clock");
            std::array<Controls, 16> controls{};
            controls[0].fire = held; controls[0].reload = reload; controls[0].yaw = match->unit(0).yaw;
            if (mode == 2) { client->send(std::array<Controls, 1>{controls[0]}); host->receive(controls); }
            const auto shots = match->simulation().shots(); match->step(controls);
            check(match->simulation().shots() - shots == expected_requests, name + " production dispatch");
            compare(match->unit(0).weapon.smg, postpass, name + " production postpass");
            if (mode == 2) { host->broadcast(); client->receive(); }
            const auto published = mode == 2 ? client->state() : match->state();
            const auto &p = published.players[0]; const auto &w = match->unit(0).weapon;
            check(p.loaded == w.loaded && p.reserve == w.reserve && published.tick == tick + 1 &&
                p.shot_tick == w.last_shot && p.reload_tick == w.reload_started &&
                p.reload_empty == w.reload_empty && p.reload_seconds == w.reload,
                name + " published ammunition/action state");
            if (mode == 2) ++udp_ticks; else ++local_ticks;
          }
        } else throw FormatError("Unknown equipped source fixture");
        check(!row.fail(), "Incomplete equipped source fixture");
        std::string extra; check(!(row >> extra), "Trailing equipped source fixture");
      }
    }
    check(gates == 12 && phase_ticks == 599 && local_ticks == 599 && udp_ticks == 599 && requests == 165,
          "Incomplete original equipped suite");
    std::cout << "12 original reload gates, 599 phase traces, 165 requests, and 599 local + 599 actual UDP equipped-weapon ticks matched\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
