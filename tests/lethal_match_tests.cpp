#include "halo2/multiplayer_network.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const std::string &message) {
  if (!value) throw FormatError(message);
}
std::uint32_t word(std::istringstream &row) {
  std::string value;
  if (!(row >> value)) throw FormatError("Missing lethal match fixture");
  return std::stoul(value, nullptr, 16);
}
int role(std::uint32_t source_index) {
  if (source_index == null_datum) return -1;
  if (source_index == 0) return 0;
  if (source_index == 3) return 1;
  if (source_index == 7) return 2;
  if (source_index == 15) return 3;
  throw FormatError("Unknown source role");
}
struct HistoryInput { std::uint32_t tick{}, amount{}, object_role{}, player_role{}, cause{}; };
void run(const LevelContent &original, std::span<const Spawn> starts,
         std::uint32_t weapon, const std::string &name, unsigned shooter_slot,
         bool stale, std::uint32_t body, std::uint32_t shield,
         const std::array<HistoryInput, 4> &history,
         const std::array<std::array<int, 5>, 4> &expected, bool lan) {
  auto content = original;
  content.collision.clear(); content.kill_triggers.clear();
  auto rules = slayer_rules({"Lethal integration", 0, {{0x59, 5}}});
  rules.weapon = weapon; rules.time_ticks = rules.score_limit = 0;
  rules.random_seed = 0x12345678;
  MultiplayerMatch match(content, 0, starts, rules);
  std::unique_ptr<LanHost> host;
  std::unique_ptr<LanClient> client;
  std::array<Controls, 16> controls{};
  auto connect = [&] {
    client = std::make_unique<LanClient>("127.0.0.1:" + std::to_string(host->port()), 0x1ed960, 1);
    for (unsigned i = 0; i < 64 && !client->ready(); ++i) {
      client->send(std::array<Controls, 1>{}); host->receive(controls);
      host->broadcast(); client->receive();
    }
    check(client->ready() && client->slots()[0] == shooter_slot, name + " UDP slot");
  };
  if (lan) host = std::make_unique<LanHost>(match, 0, 0x1ed960);
  for (unsigned i = 0; i < 4; ++i) {
    if (lan && i == shooter_slot) connect();
    else check(match.join("Source role") == i, name + " initial local slot");
  }
  // Native supported allocation order differs from full authored world startup.
  // Relabel source roles bijectively, preserving full-generation equality,
  // membership inequality and all amount/timestamp/cause bits.
  std::array<std::uint32_t, 4> full_players{}, full_units{};
  for (unsigned i = 0; i < 4; ++i) {
    full_players[i] = match.player(i).source_datum;
    full_units[i] = match.unit(i).source_datum;
    check(match.player_from_datum(full_players[i]) && match.unit_from_datum(full_units[i]),
          name + " full player/unit pool lookup");
  }
  while (match.simulation().ticks() < 200) match.step(controls);
  for (unsigned i = 0; i < 4; ++i) {
    auto &unit = const_cast<Unit &>(match.unit(i));
    unit.body.position = {0, 10.f + 5.f * i, 0}; unit.body.velocity = {};
    unit.yaw = 0; unit.pitch = 0;
  }
  auto &shooter = const_cast<Unit &>(match.unit(shooter_slot));
  auto &target = const_cast<Unit &>(match.unit(1));
  shooter.body.position = {0, 0, 0}; target.body.position = {20, 0, 0};
  target.body.radius = 1.f; // Explicit supplied contact volume, not a retail biped-radius claim.
  target.vitality.body = std::bit_cast<float>(body);
  target.vitality.shield = std::bit_cast<float>(shield);
  target.vitality.body_stun = target.vitality.shield_stun = 32767;
  for (unsigned i = 0; i < 4; ++i) {
    const auto &source = history[i];
    const auto p = role(source.player_role), u = role(source.object_role);
    target.source_damage_history[i] = {source.tick, std::bit_cast<float>(source.amount),
        u == -1 ? null_datum : full_units[u], p == -1 ? null_datum : full_players[p],
        std::uint8_t(source.cause)};
  }
  auto step = [&](bool fire) {
    controls = {}; controls[shooter_slot].fire = fire;
    if (lan) {
      client->send(std::array<Controls, 1>{controls[shooter_slot]}); host->receive(controls);
    }
    match.step(controls);
  };
  step(true);
  check(match.simulation().shots() == 1 && match.simulation().hits() == 0,
        name + " deferred-shot fixture was not reached");
  if (stale) {
    if (lan) { client.reset(); host->receive(controls); connect(); }
    else { match.leave(shooter_slot); check(match.join("Rejoined") == shooter_slot, name + " rejoin slot"); }
    check(match.player(shooter_slot).source_datum != full_players[shooter_slot] &&
        !match.player_from_datum(full_players[shooter_slot]) &&
        match.unit(shooter_slot).source_datum != full_units[shooter_slot] &&
        match.unit_from_datum(full_units[shooter_slot]), name + " stale captured identities");
    auto &replacement = const_cast<Unit &>(match.unit(shooter_slot));
    replacement.body.position = {0, 10, 0}; replacement.body.velocity = {};
  }
  for (unsigned i = 0; i < 16 && !match.simulation().hits(); ++i) step(false);
  check(match.simulation().hits() == 1 && match.unit(1).vitality.dead,
        name + " fatal production projectile: hits=" + std::to_string(match.simulation().hits()) +
        " shots=" + std::to_string(match.simulation().shots()) +
        " health=" + std::to_string(match.unit(1).vitality.body));
  for (unsigned i = 0; i < 4; ++i) {
    const auto &player = match.player(i);
    check(std::array<int, 5>{player.score, player.kills, player.deaths, player.suicides, player.assists} ==
        expected[i], name + " production statistics for role " + std::to_string(i));
  }
  check(match.unit(1).source_player == null_datum &&
        match.unit(1).source_previous_player == full_players[1], name + " victim ownership cleanup");
  const auto death_tick = std::uint32_t(match.simulation().ticks() - 1);
  check(match.player(1).source_current_object == null_datum &&
        match.player(1).source_previous_unit == full_units[1] &&
        match.unit(1).source_death_tick == death_tick &&
        match.player(1).respawn.death_elapsed == int(death_tick) &&
        match.player(1).respawn.countdown == int(rules.respawn_ticks),
        name + " synchronous full unit detach and source writer clock");
  check(match.garbage_queue().head == full_units[1] &&
        match.garbage_queue().active_count == 1 &&
        match.unit(1).source_garbage.flags == 0x4000u &&
        match.unit(1).source_garbage.next == null_datum &&
        match.unit(1).source_garbage.deadline == int(death_tick + 300),
        name + " original garbage admission before retained death tick");
  check(std::any_of(target.source_damage_history.begin(), target.source_damage_history.end(),
      [&](const auto &entry) { return entry.player == full_players[shooter_slot] &&
          entry.object == full_units[shooter_slot] && entry.tick >= 200; }),
      name + " history replaced captured owner with current occupant");
  const auto *counter_owner=stale ? match.unit_from_datum(full_units[shooter_slot]) : &match.unit(shooter_slot);
  check(counter_owner && counter_owner->source_owner_counter.count==1 &&
        counter_owner->source_owner_counter.last_tick==death_tick &&
        (!stale || match.unit(shooter_slot).source_owner_counter.count==0),
        name + " incoming-owner counter credited the selected killer or replacement generation");
  if (lan) {
    host->broadcast(); client->receive(); const auto state = client->state();
    for (unsigned i = 0; i < 4; ++i) {
      const auto &player = state.players[i];
      check(std::array<int, 5>{player.score, player.kills, player.deaths, player.suicides, player.assists} ==
          expected[i], name + " actual UDP statistics for role " + std::to_string(i));
    }
  }
  // The original deadline is eligibility, not a deletion timer. A new unit
  // must leave the old full header/body/history/owner and previous binding intact.
  while (match.simulation().ticks() <= death_tick + 300) step(false);
  const auto *corpse = match.unit_from_datum(full_units[1]);
  check(corpse && corpse->vitality.dead && corpse->source_death_tick == death_tick &&
        corpse->source_previous_player == full_players[1] &&
        corpse->source_garbage.deadline == int(death_tick + 300) &&
        match.unit(1).source_datum != full_units[1] &&
        match.player(1).source_current_object == match.unit(1).source_datum &&
        match.player(1).source_previous_unit == full_units[1] &&
        match.garbage_queue().head == full_units[1] && match.garbage_queue().active_count == 1,
        name + " corpse was lost, freed or rebound after respawn/deadline");
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Supply original map and lethal match fixtures");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
    Cache cache(argv[1]); const auto content = cache.load_content(0, true);
    std::ifstream file(argv[2]); unsigned cases = 0;
    for (std::string line; std::getline(file, line);) {
      std::istringstream row(line); std::string name; row >> name;
      const auto shooter = role(word(row)); const bool stale = word(row);
      const auto body = word(row), shield = word(row);
      std::array<HistoryInput, 4> history;
      for (auto &entry : history)
        entry = {word(row), word(row), word(row), word(row), word(row)};
      std::array<std::array<int, 5>, 4> expected;
      for (auto &player : expected)
        for (auto &value : player) value = int(word(row));
      check(!row.fail(), name + " incomplete integration fixture");
      std::string extra; check(!(row >> extra), name + " trailing integration fixture");
      for (bool lan : {false, true})
        run(content, cache.spawns(), cache.slayer_starting_weapon(5), name, shooter, stale,
            body, shield, history, expected, lan);
      ++cases;
    }
    check(cases == 3, "Incomplete lethal match cases");
    std::cout << "6 production local/actual-UDP lethal history, assists, synchronous detach, corpse retention and deferred-shot leave/rejoin cases matched source outcomes\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
