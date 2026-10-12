#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/slayer.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <memory>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const char *message) { if (!value) throw FormatError(message); }
std::uint32_t word(std::istringstream &row) {
  std::string value;
  if (!(row >> value)) throw FormatError("Missing original datum");
  return std::stoul(value, nullptr, 16);
}
MultiplayerPlayer &player(MultiplayerMatch &match, unsigned id) {
  return const_cast<MultiplayerPlayer &>(match.player(id));
}
Unit &unit(MultiplayerMatch &match, unsigned id) { return const_cast<Unit &>(match.unit(id)); }
std::unique_ptr<MultiplayerMatch> fixture(const LevelContent &content,
    std::span<const Spawn> starts, SlayerRules rules) {
  auto result = std::make_unique<MultiplayerMatch>(content, 0, starts, rules);
  for (unsigned slot = 0; slot < multiplayer_capacity; ++slot) result->join("Source scoring");
  for (unsigned slot = 0; slot < multiplayer_capacity; ++slot)
    if (slot != 0 && slot != 3 && slot != 15) result->leave(slot);
  return result;
}
void death(MultiplayerMatch &match, unsigned victim, int attacker) {
  auto &u = unit(match, victim);
  u.vitality.dead = true; u.last_damager = attacker < 0 ? -2 : attacker - 1;
}
struct Observer {
  LanHost host;
  LanClient client;
  explicit Observer(MultiplayerMatch &match)
      : host(match, 0, 0x15ce00),
        client("127.0.0.1:" + std::to_string(host.port()), 0x15ce00, 1) {
    std::array<Controls, 1> local{};
    std::array<Controls, multiplayer_capacity> controls{};
    for (unsigned n = 0; n < 64 && !client.ready(); ++n) {
      client.send(local); host.receive(controls); host.broadcast(); client.receive();
    }
    check(client.ready(), "Slayer completion observer did not join actual UDP");
  }
  void compare(const MultiplayerMatch &match) {
    host.broadcast(); client.receive();
    const auto expected = match.state();
    const auto &actual = client.state();
    check(actual.tick == expected.tick && actual.finished == expected.finished &&
        actual.winner == expected.winner && actual.clock_seconds == expected.clock_seconds,
        "Actual UDP completion state or persistent clock differs");
    for (unsigned slot : {0u, 3u, 15u})
      check(actual.players[slot].score == expected.players[slot].score,
          "Actual UDP completion score differs");
  }
};
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Supply project and original Slayer fixtures");
    if (!std::filesystem::exists(argv[2])) return 77;
    const std::filesystem::path root(argv[1]);
    Cache cache(root / "assets/disc/maps/lockout.map");
    const auto content = cache.load_content(0, true);
    const auto starts = cache.spawns();
    auto rules = slayer_rules({"Source Slayer", 0, {{0x59, 5}}});
    rules.weapon = cache.slayer_starting_weapon(5); rules.score_limit = 0; rules.time_ticks = 0;
    std::ifstream file(argv[2]);
    std::array<unsigned, 8> counts{};
    for (std::string line; std::getline(file, line);) {
      std::istringstream row(line); char kind; row >> kind;
      if (kind == 'D' || kind == 'N') {
        unsigned flags, relation, victim_flags; row >> flags;
        const auto attacker = word(row), victim = word(row); row >> relation >> victim_flags;
        if (kind == 'D') {
          unsigned count; row >> count;
          const auto result = slayer_death_scores(attacker, victim, relation, victim_flags, flags & 2, flags & 4);
          check(result.count == count, "Original Slayer request count differs");
          for (unsigned n = 0; n < count; ++n) {
            const auto expected = word(row); int delta; row >> delta;
            check(result.values[n].player == expected && result.values[n].delta == delta,
                "Original Slayer request order or delta differs");
          }
          ++counts[0];
        } else {
          rules.suicide_loss = flags & 2; rules.death_loss = flags & 4;
          auto match = fixture(content, starts, rules);
          player(*match, victim & 0xffff).source_flags = std::uint16_t(victim_flags);
          death(*match, victim & 0xffff, attacker == 0xffffffffu ? -1 : int(attacker & 0xffff));
          match->step({});
          for (unsigned slot : {0u, 3u, 15u}) {
            int expected; row >> expected;
            check(match->player(slot).score == expected, "Integrated native callback score differs");
          }
          check(!match->finished(), "Unlimited native scoring ended the match"); ++counts[1];
        }
      } else if (kind == 'W') {
        int start, delta, limit, engine, playing, active, present, applied, end, expected;
        row >> start >> delta >> limit >> engine >> playing >> active >> present >> applied >> end >> expected;
        auto score = std::int16_t(start);
        const auto result = slayer_score_write(score, delta, limit, engine, playing, active, present);
        check(score == expected && result.applied == bool(applied) && result.end_match == bool(end),
            "Original signed score write, clamp, gate or ending differs"); ++counts[2];
      } else if (kind == 'R') {
        unsigned count; row >> count; std::vector<SlayerRankingPlayer> ranks;
        for (unsigned n = 0; n < count; ++n) {
          const auto datum = word(row); int score, flags, member; row >> score >> flags >> member;
          ranks.push_back({datum, std::int16_t(score), std::uint16_t(flags), std::uint8_t(member)});
        }
        check(slayer_ffa_winner(ranks) == word(row), "Original FFA winner differs"); ++counts[3];
      } else if (kind == 'T') {
        std::uint32_t clock, start, limit; int active, expected, seconds;
        row >> clock >> start >> limit >> active >> expected >> seconds;
        check(bool(active && limit && slayer_remaining_ticks(clock, start, limit) == 0) == bool(expected),
            "Original time expiry boundary differs");
        check(slayer_clock_seconds(clock, start, limit) == seconds,
            "Original persistent display clock differs"); ++counts[4];
      } else if (kind == 'E') {
        unsigned first, second, winner; int score0, score3; row >> first >> second >> score0 >> score3 >> winner;
        std::array<std::int16_t, 16> scores{}; scores[0] = scores[3] = 24; bool active = true;
        unsigned actual_winner = 16;
        for (unsigned slot : {first, second}) {
          const auto result = slayer_score_write(scores[slot], 1, 25, true, true, active);
          if (result.end_match) { active = false; actual_winner = slot; }
        }
        check(scores[0] == score0 && scores[3] == score3 && actual_winner == winner,
            "Original ordered score-limit callbacks differ"); ++counts[5];
      } else if (kind == 'F') {
        int seconds; std::string expected; row >> seconds >> std::quoted(expected);
        check(slayer_clock_text(std::int16_t(seconds)) == expected,
            "Original clock text differs"); ++counts[7];
      } else throw FormatError("Unknown original Slayer case");
      check(!row.fail(), "Incomplete original Slayer row"); std::string extra;
      check(!(row >> extra), "Trailing original Slayer fields");
    }
    check(counts == std::array<unsigned, 8>{20, 16, 22, 9, 17, 2, 0, 16}, "Incomplete original Slayer suite");
    // Same-pass native callback order: victim zero credits slot three first.
    // This directly catches the former end-of-pass ascending-slot score scan.
    rules.score_limit = 25; rules.suicide_loss = true; rules.death_loss = false;
    auto ordered = fixture(content, starts, rules);
    Observer ordered_observer(*ordered);
    player(*ordered, 0).score = player(*ordered, 3).score = 24;
    death(*ordered, 0, 3); death(*ordered, 15, 0); ordered->step({});
    check(ordered->finished() && ordered->state().winner == 3 &&
        ordered->player(3).score == 25 && ordered->player(0).score == 24,
        "Native score-limit writer did not complete immediately");
    ordered_observer.compare(*ordered);
    // Original clock samples 149/150: time phase precedes world/clock update.
    for (bool tie : {false, true}) {
      rules.score_limit = 0; rules.time_ticks = 150;
      auto timed = fixture(content, starts, rules);
      Observer timed_observer(*timed);
      player(*timed, 0).score = tie ? 5 : 4; player(*timed, 3).score = 5;
      player(*timed, 15).score = 1;
      timed->step({});
      check(timed->state().tick == 1 && timed->state().clock_seconds == 5,
          "Native display clock used the advanced tick");
      for (unsigned tick = 1; tick < 150; ++tick) timed->step({});
      check(!timed->finished() && timed->state().tick == 150, "Native time checked the advanced clock");
      timed_observer.compare(*timed);
      // Supply an ordinary death on expiry; source inactive score gate rejects it.
      death(*timed, 15, 0); timed->step({});
      check(timed->finished() && timed->state().winner == (tie ? -1 : 3) &&
          timed->player(0).score == (tie ? 5 : 4), "Native expiry winner or inactive score gate differs");
      timed_observer.compare(*timed);
    }
    rules.time_ticks = 0;
    auto unlimited = fixture(content, starts, rules);
    Observer unlimited_observer(*unlimited);
    for (unsigned tick = 0; tick < 30; ++tick) unlimited->step({});
    check(unlimited->state().tick == 30 && unlimited->state().clock_seconds == 0,
        "Unlimited display clock used the advanced tick");
    unlimited_observer.compare(*unlimited);
    unlimited->step({});
    check(unlimited->state().tick == 31 && unlimited->state().clock_seconds == 1,
        "Unlimited display clock did not count elapsed seconds");
    unlimited_observer.compare(*unlimited);
    std::cout << "20 original callbacks, 16 native callbacks, 22 score writes, 9 winners, 17 clock cases, "
        "16 clock strings, 2 source orderings and 4 native/actual-UDP completion and clock timelines matched\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
