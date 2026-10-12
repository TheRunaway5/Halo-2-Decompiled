#include "halo2/multiplayer.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool ok, const char *message) {
  if (!ok) throw FormatError(message);
}
struct Fixture {
  std::string kind, name;
  std::uint32_t unit{}, weapon{}, definition{};
  SourceInventory inventory;
  int loaded{}, reserve{}, weapon_state{}, blocking_ticks{};
};
std::uint32_t hex(const std::string &value) {
  return std::uint32_t(std::stoul(value, nullptr, 16));
}
void compare(const SourceInventory &actual, const Fixture &expected,
             std::uint32_t native_weapon, std::uint32_t timestamp) {
  check(actual.current == expected.inventory.current &&
            actual.requested == expected.inventory.requested &&
            actual.inventory_sequence == expected.inventory.inventory_sequence &&
            actual.replicated_sequence == expected.inventory.replicated_sequence,
        "Original initial hand selectors or sequences differ");
  for (unsigned slot = 0; slot < 4; ++slot) {
    const auto expected_datum = expected.inventory.slots[slot] == expected.weapon &&
        expected.weapon != null_datum ? native_weapon : expected.inventory.slots[slot];
    check(actual.slots[slot] == expected_datum, "Starting inventory datum/empty slot differs");
    check(actual.ready_ticks[slot] == (slot == 0 && native_weapon != null_datum
                  ? timestamp : expected.inventory.ready_ticks[slot]),
          "Original selected-slot timestamp differs");
    check(!actual.inactive_weapons[slot], "Starting carry manufactured an inactive weapon state");
  }
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 3) throw FormatError("Supply original Lockout map and retained-inventory fixture");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
    std::ifstream file(argv[2]); std::vector<Fixture> fixtures;
    for (std::string line; std::getline(file, line);) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream row(line); Fixture f; std::string unit, weapon, definition;
      unsigned sequence, replicated; int current0, current1, requested0, requested1;
      check(bool(row >> f.kind >> f.name >> unit >> weapon >> definition >> sequence >> replicated >>
              current0 >> current1 >> requested0 >> requested1), "Malformed inventory fixture header");
      f.unit=hex(unit); f.weapon=hex(weapon); f.definition=hex(definition);
      f.inventory.inventory_sequence=std::uint16_t(sequence);
      f.inventory.replicated_sequence=std::uint16_t(replicated);
      f.inventory.current={std::int8_t(current0),std::int8_t(current1)};
      f.inventory.requested={std::int8_t(requested0),std::int8_t(requested1)};
      for (auto &slot:f.inventory.slots) { std::string value; check(bool(row>>value),"Missing source slot"); slot=hex(value); }
      for (auto &tick:f.inventory.ready_ticks) check(bool(row>>tick),"Missing source slot timestamp");
      check(bool(row >> f.loaded >> f.reserve >> f.weapon_state >> f.blocking_ticks), "Missing original weapon fields");
      fixtures.push_back(f);
    }
    check(fixtures.size()==4, "Expected two complete constructors and two real player loadouts");
    Cache cache(argv[1]); auto content=cache.load_content(0,true);
    content.kill_triggers.clear(); // Inventory retention test excludes kill-volume delivery.
    auto rules=slayer_rules({"Retained inventory",0,{{0x59,5}}});
    rules.weapon=cache.slayer_starting_weapon(5); rules.time_ticks=rules.score_limit=0;
    check(content.weapons.at(rules.weapon).smg.has_value(), "Source starting SMG is absent");
    unsigned constructors=0,equips=0;
    for (const auto &fixture:fixtures) {
      MultiplayerMatch match(content,0,cache.spawns(),rules);
      if (fixture.kind=="constructor") {
        compare(match.source_inventory(0),fixture,null_datum,0); ++constructors; continue;
      }
      check(fixture.kind=="equipped" && fixture.definition==rules.weapon,
            "Expected actual original SMG loadout");
      const auto id=match.join("Inventory owner"); const auto &unit=match.unit(id);
      const auto datum=unit.weapon.source_datum;
      check(datum!=null_datum && unit.source_datum!=null_datum && datum!=unit.source_datum,
            "Live starting carry did not retain distinct full object datums");
      compare(match.source_inventory(id),fixture,datum,0);
      check(&match.source_inventory(id)==&unit.source_inventory &&
                match.inventory_weapon(id,0)==&unit.weapon && match.weapon_from_datum(datum)==&unit.weapon,
            "Read-only inventory did not resolve the sole active WeaponState");
      check(!match.inventory_weapon(id,1) && !match.inventory_weapon(id,-1) &&
                !match.inventory_weapon(id,4) && !match.weapon_from_datum(null_datum) &&
                !match.weapon_from_datum(unit.source_datum) && !match.weapon_from_datum(datum^0x10000u),
            "Empty slot, wrong type or stale salt resolved as a carried weapon");
      check(unit.weapon.loaded==fixture.loaded && unit.weapon.reserve==fixture.reserve &&
                int(unit.weapon.smg.weapon_state)==fixture.weapon_state &&
                unit.weapon.smg.blocking_ticks==fixture.blocking_ticks,
            "Original allocated/equipped SMG ammo/ready state differs");
      std::array<Controls,multiplayer_capacity> controls{};
      controls[id].yaw=unit.yaw; controls[id].fire=true;
      for (unsigned i=0;i<16;++i) match.step(controls);
      const auto saved=match.unit(id).weapon;
      check(saved.loaded<fixture.loaded && saved.source_datum==datum &&
                match.weapon_from_datum(datum)==&match.unit(id).weapon &&
                match.weapon_from_datum(datum)->loaded==saved.loaded,
            "Actual firing lost identity or left an authoritative stale ammo copy");
      compare(match.source_inventory(id),fixture,datum,0);
      match.leave(id);
      const auto tick=std::uint32_t(match.simulation().ticks());
      check(match.join("New inventory owner")==id,"Departed player slot was not reusable");
      const auto fresh=match.unit(id).weapon.source_datum;
      compare(match.source_inventory(id),fixture,fresh,tick);
      check(fresh!=datum && match.weapon_from_datum(fresh)==&match.unit(id).weapon,
            "Rejoin replaced a retained item datum or failed active ownership");
      const auto *retained=match.weapon_from_datum(datum);
      check(retained && retained!=&match.unit(id).weapon && retained->source_datum==saved.source_datum &&
                retained->definition==saved.definition && retained->loaded==saved.loaded &&
                retained->reserve==saved.reserve && retained->last_shot==saved.last_shot &&
                retained->smg.weapon_state==saved.smg.weapon_state &&
                retained->smg.magazine_mode==saved.smg.magazine_mode &&
                retained->smg.magazine_ticks==saved.smg.magazine_ticks,
            "Departed body's original carried state was reset by new spawn");
      const auto other=match.join("Other inventory owner");
      check(other!=id && match.unit(other).weapon.source_datum!=fresh &&
                match.inventory_weapon(other,0)==&match.unit(other).weapon,
            "Two live units share a starting weapon state");
      ++equips;
    }
    check(constructors==2 && equips==2,"Missing source constructor/equip comparison");
    std::cout << "Two original constructors and two real Match initial carries matched; active ammo/identity survived firing, leave and rejoin without duplicate state\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
