#include "halo2/unit_inventory.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool value, const std::string &context) {
  if (!value) throw std::runtime_error("Original unit inventory differs: " + context);
}
std::uint32_t number(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &text) {
  check(text.size() % 2 == 0, "hex extent");
  std::vector<std::byte> result;
  for (std::size_t i = 0; i < text.size(); i += 2)
    result.push_back(std::byte(number(text.substr(i, 2))));
  return result;
}
struct Callback {
  char kind;
  std::array<std::uint32_t, 3> arguments;
  std::vector<std::byte> before, after;
};
struct Case {
  char kind;
  std::string label;
  std::uint32_t unit, item, removal;
  std::int16_t slot;
  std::uint8_t mode;
  std::vector<std::byte> before, after, sequence_before;
  std::vector<Callback> callbacks;
  std::vector<halo2::UnitInventoryStore> stores;
};
void run(const Case &c, unsigned &callback_count, unsigned &store_count) {
  auto unit = c.before;
  check(unit.size() == 0x800 && c.after.size() == 0x800 &&
        c.sequence_before.size() == 0x800, c.label + " source unit extents");
  std::size_t callback_index = 0;
  std::vector<halo2::UnitInventoryStore> stores;
  halo2::UnitInventoryServices services;
  services.unit_body = [&](std::uint32_t id) -> std::span<std::byte> {
    check(id == c.unit, c.label + " full unit service argument"); return unit;
  };
  unsigned mode_queries = 0;
  services.options_mode = [&] {
    check(unit == c.sequence_before, c.label + " complete prefix before options read");
    ++mode_queries; return c.mode;
  };
  auto callback = [&](char kind, std::array<std::uint32_t, 3> arguments) {
    check(callback_index < c.callbacks.size(), c.label + " extra owning leaf");
    const auto &expected = c.callbacks[callback_index++];
    check(kind == expected.kind && arguments == expected.arguments,
          c.label + " owning leaf order/arguments");
    check(unit == expected.before, c.label + " complete prefix at owning leaf entry");
    // Literal original leaf outputs are an explicit service boundary here.
    // This does not test or implement ownership/world/drop inside those leaves.
    check(expected.after.size() == unit.size(), c.label + " owning leaf output extent");
    std::copy(expected.after.begin(), expected.after.end(), unit.begin()); ++callback_count;
  };
  constexpr std::uint32_t none = 0xffffffffu;
  services.assign_owner = [&](auto item, auto owner) { callback('A', {item, owner, none}); };
  services.remove_world_membership = [&](auto item) { callback('R', {item, none, none}); };
  services.release_item = [&](auto id, auto item, auto mode) { callback('D', {id, item, mode}); };
  services.network_dirty = [&](auto, auto) {
    throw std::runtime_error(c.label + " unobserved non-NONE network service");
  };
  const halo2::UnitInventoryStoreObserver observe = [&](const auto &s) { stores.push_back(s); };
  if (c.kind == 'T')
    halo2::set_source_inventory_slot(c.unit, c.item, c.slot, c.removal, services, observe);
  else halo2::advance_source_inventory_sequence(c.unit, services, observe);
  check(mode_queries == 1 && callback_index == c.callbacks.size(), c.label + " complete leaf consumption");
  check(unit == c.after, c.label + " complete final prefix, including untouched bytes");
  check(stores == c.stores, c.label + " ordered owning instruction stores, including zero writes");
  store_count += unsigned(stores.size());
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Supply immutable-source unit inventory fixture export");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream file(argv[1]); std::vector<Case> cases;
    for (std::string line; std::getline(file, line);) {
      std::istringstream stream(line); std::vector<std::string> r;
      for (std::string s; stream >> s;) r.push_back(s);
      if (r.empty()) continue;
      if (r[0] == "T" || r[0] == "S") {
        check(r.size() == 12, "case row extent");
        Case c; c.kind = r[0][0]; c.label = "case " + r[1] + " call " + r[2];
        c.unit = number(r[3]); c.item = number(r[4]); c.slot = std::int16_t(std::uint16_t(number(r[5])));
        c.removal = number(r[6]); c.mode = std::uint8_t(number(r[7]));
        c.before = bytes(r[8]); c.after = bytes(r[9]); c.sequence_before = bytes(r[10]);
        check(r[11] == "literal", "literal fixture marker"); cases.push_back(std::move(c));
      } else if (r[0] == "K") {
        check(!cases.empty() && r.size() == 7, "callback row extent");
        cases.back().callbacks.push_back({r[1].at(0), {number(r[2]), number(r[3]), number(r[4])}, bytes(r[5]), bytes(r[6])});
      } else if (r[0] == "W") {
        check(!cases.empty() && r.size() == 6, "store row extent");
        cases.back().stores.push_back({number(r[1]), number(r[4]), number(r[5]), number(r[2]), number(r[3])});
      } else throw std::runtime_error("Unknown unit inventory fixture row");
    }
    unsigned transfers = 0, sequences = 0, callbacks = 0, stores = 0;
    for (const auto &c : cases) {
      run(c, callbacks, stores); if (c.kind == 'T') ++transfers; else ++sequences;
    }
    check(transfers == 10 && sequences == 10 && callbacks == 19 && stores == 59,
          "complete measured orchestration suite");
    std::cout << transfers << " original slot orchestrations and " << sequences
              << " sequence consumers matched; " << callbacks << " explicit recorded leaf boundaries and "
              << stores << " ordered native stores\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
