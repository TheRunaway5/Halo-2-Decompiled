#include "halo2/model.hpp"
#include "halo2/multiplayer_network.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const std::string &message) { if (!value) throw FormatError(message); }
struct Expected {
  int blocking{}, loaded{}, reserve{}, hand0{}, hand1{};
  unsigned input{}, weapon{}, trigger{};
  std::string barrel;
};
Expected read_state(std::istringstream &row) {
  Expected e;
  row >> e.blocking >> e.loaded >> e.reserve >> e.hand0 >> e.hand1 >>
      std::hex >> e.input >> e.weapon >> e.trigger >> e.barrel >> std::dec;
  return e;
}
void compare(const SmgState &s, const Expected &e, const std::string &name) {
  constexpr char hex[] = "0123456789abcdef";
  std::string barrel;
  const auto *bytes = reinterpret_cast<const unsigned char *>(&s.barrel);
  for (unsigned i = 0; i < sizeof(s.barrel); ++i) {
    barrel += hex[bytes[i] >> 4]; barrel += hex[bytes[i] & 15];
  }
  check(s.blocking_ticks == e.blocking && s.loaded == e.loaded && s.reserve == e.reserve &&
      s.hand_timers[0] == e.hand0 && s.hand_timers[1] == e.hand1 &&
      s.input_flags == e.input && s.weapon_flags == e.weapon && s.trigger_flags == e.trigger &&
      barrel == e.barrel, name + " native countdown/admission/weapon state differs");
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Supply original map and ready fixtures");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
    Cache cache(argv[1]); const auto content = cache.load_content(0, true);
    const auto &weapon = content.weapons.at(cache.slayer_starting_weapon(5));
    check(bool(weapon.smg), "Real equipped SMG path missing");
    const auto clip = cache.load_action(weapon.first_person_animation, "any", "any", "any", "ready");
    std::ifstream file(argv[2]);
    unsigned rates = 0, gates = 0, local_ticks = 0, udp_ticks = 0;
    for (bool lan : {false, true}) {
      file.clear(); file.seekg(0);
      std::string active;
      std::unique_ptr<MultiplayerMatch> match;
      std::unique_ptr<LanHost> host;
      std::unique_ptr<LanClient> client;
      for (std::string line; std::getline(file, line);) {
        std::istringstream row(line); char kind; row >> kind;
        if (kind == 'C') {
          unsigned index, frames, events; row >> index >> frames >> events;
          check(index == 29 && frames == clip.frame_count && events == clip.frame_events.size() &&
              events == clip.frame_event_count, "Original FP ready clip metadata differs");
          for (const auto &e : clip.frame_events) {
            int type, frame; row >> type >> frame;
            check(type == e.type && frame == e.frame, "Original FP ready event bytes differ");
          }
        } else if (kind == 'R') {
          int rate, expected; row >> rate >> expected;
          check(animation_ready_ticks(clip, std::int16_t(rate)) == expected,
              "Original ready clock query differs");
          if (!lan) ++rates;
        } else if (kind == 'G') {
          unsigned id, input, flags, requests, expected_seed;
          int blocking; row >> id >> blocking >> input >> flags;
          const auto expected = read_state(row);
          row >> std::hex >> expected_seed >> std::dec >> requests;
          SmgState state; state.loaded = 60; state.reserve = 120;
          state.magazine_ticks = state.transfer_ticks = -1; // explicit original fixture
          state.blocking_ticks = std::int16_t(blocking);
          state.input_flags = std::uint16_t(input); state.weapon_flags = std::uint16_t(flags);
          auto seed = std::uint32_t(0x12345678);
          smg_update(state, *weapon.smg, 1.f/30);
          const bool request = bool(smg_dispatch(state, *weapon.smg, {1,0,0}, seed,
              content.original_data->weapon_directions));
          compare(state, expected, "Original admission " + std::to_string(id));
          check(request == (requests != 0) && seed == expected_seed, "Original admission dispatch/RNG differs");
          if (!lan) ++gates;
        } else if (kind == 'T') {
          std::string name; unsigned tick, held, requests; row >> name >> tick >> held;
          const auto expected = read_state(row); row >> requests;
          if (active != name) {
            active = name; client.reset(); host.reset();
            auto rules = slayer_rules({"Ready trace", 0, {{0x59,5}}});
            rules.weapon = weapon.datum; rules.time_ticks = rules.score_limit = 0;
            rules.random_seed = 0x12345678;
            match = std::make_unique<MultiplayerMatch>(content, 0, cache.spawns(), rules);
            if (lan) {
              host = std::make_unique<LanHost>(*match, 0, 0x1040f0);
              client = std::make_unique<LanClient>("127.0.0.1:" + std::to_string(host->port()), 0x1040f0, 1);
              std::array<Controls,16> controls{};
              for (unsigned i=0; i<64 && !client->ready(); ++i) {
                client->send(std::array<Controls,1>{}); host->receive(controls);
                host->broadcast(); client->receive();
              }
              check(client->ready() && client->slots()[0] == 0, "Actual ready UDP join failed");
            } else match->join("Original ready trace");
            check(weapon.smg->ready_ticks == 15, "Original timed equip event was not imported");
            // The frozen oracle explicitly calls timed E9D50(0,0). Starting
            // loadout CE510 instead requests instant E9D50(1,1); its production
            // countdown must remain zero. Supply this distinct timed boundary.
            const_cast<Unit &>(match->unit(0)).weapon.smg.blocking_ticks = weapon.smg->ready_ticks;
            const_cast<Unit &>(match->unit(0)).weapon.smg.hand_timer_registered = true;
          }
          check(match->state().tick == tick, "Nonsequential original ready trace");
          std::array<Controls,16> controls{};
          controls[0].fire = held; controls[0].yaw = match->unit(0).yaw;
          if (lan) { client->send(std::array<Controls,1>{controls[0]}); host->receive(controls); }
          const auto before = match->simulation().shots();
          match->step(controls);
          if (lan) { host->broadcast(); client->receive(); }
          const auto &unit = match->unit(0);
          compare(unit.weapon.smg, expected, "Original ready " + name + " tick " + std::to_string(tick));
          check(!unit.vitality.dead && match->simulation().shots() - before == requests,
              "Production ready shot admission differs");
          const auto state = lan ? client->state() : match->state();
          check(state.tick == tick+1 && state.players[0].loaded == expected.loaded &&
              state.players[0].reserve == expected.reserve &&
              state.players[0].shot_tick == unit.weapon.last_shot,
              "Production ready published ammo/action state differs");
          if (lan) ++udp_ticks; else ++local_ticks;
        } else throw FormatError("Unknown ready fixture");
        check(!row.fail(), "Missing ready fixture field"); std::string extra;
        check(!(row >> extra), "Trailing ready fixture field");
      }
    }
    check(rates == 7 && gates == 7 && local_ticks == 44 && udp_ticks == 44,
        "Incomplete original ready suite");
    std::cout << "Original FP event, 7 clock queries, 7 admission states and 44 local + 44 actual UDP ticks matched\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
