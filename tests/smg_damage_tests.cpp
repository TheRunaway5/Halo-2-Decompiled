#include "halo2/unit_damage.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_network.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const std::string &message) { if (!value) throw FormatError(message); }
std::uint32_t word(std::istringstream &row) {
  std::string value; if (!(row >> value)) throw FormatError("Missing original damage field");
  return std::stoul(value, nullptr, 16);
}
float real(std::istringstream &row) { return std::bit_cast<float>(word(row)); }
std::uint32_t bits(float value) { return std::bit_cast<std::uint32_t>(value); }
RecentDamageState recent(std::istringstream &row) {
  const float visible=real(row),accumulated=real(row);
  return {visible,accumulated,std::bit_cast<std::int8_t>(std::uint8_t(word(row)))};
}
bool same(const DamageFeedbackState &a,const DamageFeedbackState &b) {
  const auto channel=[](const auto &a,const auto &b) {
    return bits(a.visible)==bits(b.visible)&&bits(a.accumulated)==bits(b.accumulated)&&a.ticks==b.ticks;
  };
  return channel(a.shield,b.shield)&&channel(a.body,b.body);
}
void live_hit(const LevelContent &original, std::span<const Spawn> starts,
    std::uint32_t weapon, float shield, float body, std::uint32_t expected_shield,
    std::uint32_t expected_body, bool dead, bool lan,
    std::span<const DamageFeedbackState,7> feedback) {
  // Supplied clear-world/contact fixture. All weapon data and native SMG
  // dispatch/projectile/damage/score/UDP code remain the production path.
  auto content = original; content.collision.clear();
  auto rules = slayer_rules({"Damage integration", 0, {{0x59, 5}}});
  rules.weapon = weapon; rules.time_ticks = rules.score_limit = 0;
  rules.random_seed = 0x12345678;
  MultiplayerMatch match(content, 0, starts, rules);
  std::unique_ptr<LanHost> host;
  std::unique_ptr<LanClient> client;
  std::array<Controls, 16> controls{};
  if (lan) {
    host = std::make_unique<LanHost>(match, 0, 0x40980000);
    client = std::make_unique<LanClient>("127.0.0.1:" + std::to_string(host->port()), 0x40980000, 1);
    for (unsigned n = 0; n < 64 && !client->ready(); ++n) {
      client->send(std::array<Controls, 1>{}); host->receive(controls);
      host->broadcast(); client->receive();
    }
    check(client->ready() && client->slots()[0] == 0, "SMG damage client failed to join actual UDP");
  } else match.join("Shooter");
  check(match.join("Target") == 1, "SMG damage target slot differs");
  auto &shooter = const_cast<Unit &>(match.unit(0));
  auto &target = const_cast<Unit &>(match.unit(1));
  shooter.body.position = {0, 0, 0}; shooter.yaw = 0;
  target.body.position = {2, 0, 0}; target.yaw = 0;
  controls = {}; // Supplied aim replaces the joined authored facing as well.
  // These original amount/vitality references use an unprotected target.
  // Successful native spawn starts player+194 at the source tick rate; let
  // its real pre-world updates expire instead of bypassing player scaling.
  for (unsigned tick=0;tick<30;++tick) match.step(controls);
  check(match.player(0).source_field_194==0 && match.player(1).source_field_194==0,
      "SMG supplied-contact fixture did not expire source spawn protection");
  target.vitality.shield = shield; target.vitality.body = body;
  target.vitality.shield_stun = target.vitality.body_stun = 32767;
  // Hold both recharge countdowns in the supplied one-hit fixture.
  for (unsigned n = 0; n < 64 && !match.simulation().hits(); ++n) {
    controls[0].fire = match.simulation().shots() == 0;
    if (lan) {
      client->send(std::array<Controls, 1>{controls[0]}); host->receive(controls);
    }
    match.step(controls);
  }
  check(match.simulation().shots() == 1 && match.simulation().hits() == 1,
      "Production SMG projectile hit count differs: shots=" + std::to_string(match.simulation().shots()) +
      " hits=" + std::to_string(match.simulation().hits()) + " LAN=" + std::to_string(lan) +
      " target_body=" + std::to_string(body) + " target_shield=" + std::to_string(shield));
  check(bits(target.vitality.body) == expected_body && bits(target.vitality.shield) == expected_shield &&
      target.vitality.dead == dead && shooter.weapon.loaded == 59,
      "Production SMG amount/vitality differs: shield=" + std::to_string(bits(target.vitality.shield)) +
      " expected=" + std::to_string(expected_shield) + " body=" + std::to_string(bits(target.vitality.body)) +
      " expected=" + std::to_string(expected_body) + " dead=" + std::to_string(target.vitality.dead) +
      " ammo=" + std::to_string(shooter.weapon.loaded));
  check(match.player(0).score == int(dead) && match.player(0).kills == int(dead) &&
      match.player(1).deaths == int(dead), "Native SMG death/score integration differs");
  for(unsigned tick=0;tick<feedback.size();++tick) {
    check(same(target.damage_feedback,feedback[tick]),
        "Production SMG feedback differs from original report/fade at tick "+std::to_string(tick));
    check(same(match.state().players[1].damage_feedback,feedback[tick]),
        "Authoritative SMG feedback snapshot differs");
    if (lan) {
      host->broadcast(); client->receive(); const auto state = client->state();
      check(bits(state.players[1].health) == expected_body && bits(state.players[1].shield) == expected_shield &&
          state.players[1].dead == dead && state.players[1].deaths == int(dead) &&
          state.players[0].score == int(dead) && state.players[0].loaded == 59,
          "Actual UDP SMG damage/death/ammunition state differs");
      check(same(state.players[1].damage_feedback,feedback[tick]),
          "Actual UDP SMG feedback differs from original report/fade");
    }
    if(tick+1<feedback.size()) {
      controls={};
      if(lan){client->send(std::array<Controls,1>{});host->receive(controls);}
      match.step(controls);
    }
  }
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 4) throw FormatError("Supply original map, damage and feedback fixtures");
    for(unsigned n=1;n<4;++n)if(!std::filesystem::exists(argv[n]))return 77;
    std::array<std::array<DamageFeedbackState,7>,3> feedback{};
    unsigned feedback_rows=0;
    std::ifstream feedback_file(argv[3]);
    for(std::string line;std::getline(feedback_file,line);) {
      if(line.empty()||line[0]=='#')continue;
      std::istringstream row(line);const auto id=word(row),tick=word(row);
      check(id==feedback_rows/7&&tick==feedback_rows%7&&feedback_rows<21,"Unexpected SMG feedback row");
      const auto shield=recent(row),body=recent(row);feedback[id][tick]={shield,body};++feedback_rows;
      std::string extra;check(!(row>>extra),"Trailing SMG feedback field");
    }
    check(feedback_rows==21,"Incomplete original SMG feedback trajectory");
    Cache cache(argv[1]); const auto content = cache.load_content(0, true);
    const auto &weapon = content.weapons.at(cache.slayer_starting_weapon(5));
    check(weapon.smg && weapon.damage_amount, "Original SMG damage amount was not imported");
    const auto &definition = *weapon.damage_amount;
    check(definition.upper_minimum == definition.upper_maximum,
          "This fixed-upper suite does not verify random amount selection");
    unsigned amounts = 0, hits = 0;
    std::ifstream file(argv[2]);
    for (std::string line; std::getline(file, line);) {
      std::istringstream row(line); char kind; unsigned id; row >> kind >> id;
      const auto name = "Original SMG damage " + std::to_string(id);
      if (kind == 'A') {
        const auto strength = word(row); const auto fraction = real(row), scale = real(row);
        const auto expected = word(row);
        check(bits(damage_amount(definition.lower, definition.upper_minimum, fraction,
            scale, damage_strength_scale(std::uint8_t(strength)))) == expected, name + " amount");
        ++amounts;
      } else if (kind == 'H') {
        RechargeState state;
        state.body = real(row); state.shield = real(row);
        state.body_stun = int(word(row)); state.shield_stun = int(word(row));
        const auto before_flags = word(row);
        state.dead = (before_flags & 4) != 0;
        state.shield_damaged = (before_flags & 2) != 0;
        state.shield_depleted = (before_flags & 8) != 0;
        const auto amount = real(row);
        const auto body = word(row), shield = word(row);
        const auto body_stun = word(row), shield_stun = word(row), flags = word(row);
        const auto normalized_body = word(row), normalized_shield = word(row);
        const auto result = apply_unit_damage(state, content.player.vitality, weapon,
            content.damage_groups, content.damage_materials, amount);
        const unsigned actual_flags = (state.dead ? 4 : 0) |
            (state.shield_damaged ? 2 : 0) | (state.shield_depleted ? 8 : 0);
        check(bits(state.body) == body && bits(state.shield) == shield &&
            state.body_stun == int(body_stun) && state.shield_stun == int(shield_stun) &&
            actual_flags == (flags & 14) && bits(result.body) == normalized_body &&
            bits(result.shield) == normalized_shield, name + " vitality/result");
        ++hits;
      } else throw FormatError("Unknown original damage fixture");
      check(!row.fail(), "Incomplete original damage fixture"); std::string extra;
      check(!(row >> extra), "Trailing original damage fixture");
    }
    check(amounts == 54 && hits == 170, "Incomplete original SMG damage suite");
    for (bool lan : {false, true}) {
      live_hit(content, cache.spawns(), weapon.datum, 1, 1, 0x3f6ea0ea, 0x3f800000, false, lan,feedback[0]);
      live_hit(content, cache.spawns(), weapon.datum, 0, 1, 0, 0x3f64fa50, false, lan,feedback[1]);
      live_hit(content, cache.spawns(), weapon.datum, 0, .01f, 0, 0xbdc3b2a2, true, lan,feedback[2]);
    }
    std::cout << amounts << " original amounts and " << hits
              << " complete-entry vitality/normalized-result samples matched; "
                 "6 production local/UDP supplied-contact hit and 7-tick feedback trajectories passed\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
