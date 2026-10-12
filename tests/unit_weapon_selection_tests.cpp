#include "halo2/unit_weapon_selection.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool value, const std::string &message) {
  if (!value) throw std::runtime_error("Original weapon selection differs: " + message);
}
std::uint32_t number(const std::string &s) { return std::stoul(s, nullptr, 16); }
std::vector<std::byte> bytes(const std::string &s) {
  check(s.size() % 2 == 0, "hex extent");
  std::vector<std::byte> result;
  for (std::size_t i = 0; i < s.size(); i += 2) result.push_back(std::byte(number(s.substr(i, 2))));
  return result;
}
struct Capability {
  std::uint32_t unit, item, mode, result;
  std::vector<std::byte> before, after;
};
struct Case {
  std::string name;
  std::uint32_t unit, mode, previous, direction, result;
  std::vector<std::byte> before, after;
  std::vector<Capability> capabilities;
  std::map<std::uint32_t, std::vector<std::byte>> definitions;
};
void run(const Case &c, unsigned &count) {
  auto unit = c.before;
  std::size_t next = 0;
  unsigned definition_reads = 0;
  halo2::UnitWeaponSelectionServices services;
  services.unit_body = [&](std::uint32_t id)->std::span<const std::byte> {
    check(id == c.unit, c.name + " full unit datum");return unit;
  };
  services.weapon_definition = [&](std::uint32_t item)->std::span<const std::byte> {
    check(next != 0, c.name + " capability precedes definition read");
    check(item == c.capabilities.at(next - 1).item, c.name + " retained full item datum");
    ++definition_reads;return c.definitions.at(item);
  };
  services.accepts_weapon = [&](std::uint32_t owner, std::uint32_t item, std::uint32_t mode) {
    check(next < c.capabilities.size(), c.name + " extra capability call");
    const auto &call = c.capabilities[next++];
    check(owner == call.unit && item == call.item && mode == call.mode, c.name + " CEAF0 ABI/order");
    check(unit == call.before, c.name + " complete unit prefix at capability entry");
    // ONLY this original differential supplies a captured capability result.
    // Runtime services must own the real equipped-manager producer.
    check(unit.size() == call.after.size(), c.name + " stable unit extent");
    std::copy(call.after.begin(), call.after.end(), unit.begin());
    return call.result != 0;
  };
  const auto previous = std::bit_cast<std::int16_t>(std::uint16_t(c.previous));
  const auto direction = std::bit_cast<std::int16_t>(std::uint16_t(c.direction));
  const auto result = halo2::select_source_inventory_weapon(c.unit, c.mode, previous, direction, services);
  check(std::uint16_t(result) == c.result, c.name + " original AX");
  check(next == c.capabilities.size() && definition_reads == c.definitions.size(), c.name + " complete callback consumption");
  check(unit == c.after, c.name + " full original final unit prefix");
  count += unsigned(next);
  services.accepts_weapon = {};
  unit = c.before;
  bool stopped = false;
  try { (void)halo2::select_source_inventory_weapon(c.unit, c.mode, previous, direction, services); }
  catch (const std::runtime_error &) { stopped = true; }
  check(stopped && unit == c.before, c.name + " missing capability remains unavailable before mutation");
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("Supply original one-SMG weapon-selection fixture");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream input(argv[1]);std::vector<Case> cases;std::string line;
    while (std::getline(input, line)) {
      std::istringstream row(line);std::string kind;row >> kind;
      if (kind == "C") {
        Case c;std::string unit, mode, previous, direction, result;
        row >> c.name >> unit >> mode >> previous >> direction >> result;
        c.unit=number(unit);c.mode=number(mode);c.previous=number(previous);c.direction=number(direction);c.result=number(result);cases.push_back(c);
      } else if (kind == "B" || kind == "F") {
        std::string raw;row >> raw;(kind == "B" ? cases.back().before : cases.back().after)=bytes(raw);
      } else if (kind == "K") {
        std::string unit,item,mode,result,before,after;row >> unit >> item >> mode >> result >> before >> after;
        cases.back().capabilities.push_back({number(unit),number(item),number(mode),number(result),bytes(before),bytes(after)});
      } else if (kind == "D") {
        std::string item,raw;row >> item >> raw;cases.back().definitions.emplace(number(item),bytes(raw));
      } else throw std::runtime_error("Unknown original selection fixture row");
    }
    check(cases.size()==6,"complete original fixture population");unsigned capabilities=0;
    for (const auto &c:cases) run(c,capabilities);
    std::cout << "Original one-SMG selection: " << cases.size() << " calls, " << capabilities << " capability boundaries/full unit buffers PASS\n";
    return 0;
  } catch (const std::exception &e) { std::cerr << e.what() << '\n';return 1; }
}
