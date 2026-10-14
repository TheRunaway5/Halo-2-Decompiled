#include "halo2/source_unit_control.hpp"
#include "halo2/native_unit_interaction.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool result, const char *message) {
  if (!result)
    throw std::runtime_error(message);
}
std::vector<std::uint8_t> decode(const std::string &hex) {
  check(hex.size()%2 == 0, "Original unit fixture has incomplete bytes");
  std::vector<std::uint8_t> bytes(hex.size()/2);
  for (unsigned i = 0; i < bytes.size(); ++i)
    bytes[i] = std::uint8_t(std::stoul(hex.substr(i*2, 2), nullptr, 16));
  return bytes;
}
void write(std::vector<std::uint8_t> &bytes, unsigned offset, std::uint32_t value) {
  for (unsigned i = 0; i < 4; ++i)
    bytes[offset+i] = std::uint8_t(value >> (8*i));
}
std::uint32_t read(const std::vector<std::uint8_t> &bytes, unsigned offset) {
  std::uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i)
    value |= std::uint32_t(bytes[offset+i]) << (8*i);
  return value;
}
nlohmann::json fixture(const char *path) {
  std::ifstream file(path);
  std::string line;
  bool found = false;
  while (std::getline(file, line))
    if (line == "```json") { found = true; break; }
  check(found, "Unit-control fixture has no JSON block");
  nlohmann::json result;
  file >> result;
  check(result.at("passed").get<bool>(), "Original unit-control oracle failed");
  return result;
}
std::vector<std::uint8_t> sparse(const nlohmann::json &deltas, unsigned size,
                               std::uint8_t baseline) {
  std::vector<std::uint8_t> bytes(size, baseline);
  for (const auto &delta : deltas) {
    const auto offset = delta.at(0).get<unsigned>();
    const auto data = decode(delta.at(1).get<std::string>());
    check(offset <= bytes.size() && data.size() <= bytes.size()-offset,
          "Original unit sparse record exceeds its body");
    std::copy(data.begin(), data.end(), bytes.begin()+offset);
  }
  return bytes;
}
void seats(const char *path) {
  const auto data = fixture(path);
  const auto sizes = data.at("record_sizes").get<std::vector<unsigned>>();
  const auto baseline = data.at("baseline_byte").get<std::uint8_t>();
  unsigned cases = 0, callbacks = 0;
  for (const auto &row : data.at("cases")) {
    std::vector<std::vector<std::uint8_t>> bodies;
    for (unsigned i = 0; i < sizes.size(); ++i)
      bodies.push_back(sparse(row.at("before").at(i), sizes[i], baseline));
    std::vector<NativeUnitInteractionDefinition> definitions;
    for (const auto &authored : row.at("definitions")) {
      auto &definition = definitions.emplace_back();
      definition.unit_flags = authored.at("flags").get<std::uint32_t>();
      for (const auto &entry : authored.at("seats")) {
        auto &seat = definition.seats.emplace_back();
        seat.flags = entry.at("flags").get<std::uint32_t>();
        seat.boarding_seat = entry.at("other").get<std::int16_t>();
      }
    }
    nlohmann::json events = nlohmann::json::array();
    SourceUnitControl control({
      [&](std::uint32_t id) -> std::span<std::uint8_t> {
        const auto index = id & 0xffffu;
        check(index < bodies.size() && id == 0xe26f0000u+index,
              "Seat fixture received a foreign full object datum");
        return bodies[index];
      },
      [&](std::uint32_t id) { return std::uint8_t((id&0xffffu) < 6 ? (id&1u) : 2); },
      [&](std::uint32_t id) -> const NativeUnitInteractionDefinition & {
        const auto index = id & 0xffffu;
        check(index < definitions.size() && id == 0xe1000000u+index,
              "Seat fixture lost its canonical definition");
        return definitions[index];
      },
      [](std::uint32_t) { throw std::runtime_error("Seat-only fixture requested an item-effect owner"); },
      [&](std::uint32_t id) {
        // The fixture's actual original AI controller is inactive. Its
        // complete1BBDF0 early-out has no actor or object effects.
        nlohmann::json owners = nlohmann::json::array();
        for (unsigned i = 0; i < 6; ++i)
          owners.push_back({read(bodies[i], 0x248), read(bodies[i], 0x24c)});
        events.push_back({{"unit", id}, {"owners", owners}});
      }});
    control.refresh_seats(0xe26f0000u);
    if (events != row.at("events")) {
      std::cerr << "Seat graph=" << cases << " native_events=" << events.dump()
                << " original_events=" << row.at("events").dump() << '\n';
      throw std::runtime_error("Native seat driver callback order/state differs from originalCC810");
    }
    for (unsigned i = 0; i < sizes.size(); ++i)
      if (bodies[i] != sparse(row.at("after").at(i), sizes[i], baseline)) {
        std::cerr << "Seat graph=" << cases << " body=" << i << '\n';
        throw std::runtime_error("Native recursive seat bytes differ from originalCC810");
      }
    callbacks += unsigned(events.size());
    ++cases;
  }
  std::cout << "PASS " << cases << " complete original recursive seat graphs and "
            << callbacks << " inactive-AI driver callbacks\n";
}
}
int main(int argc, char **argv) try {
  check(argc == 2 || argc == 3, "Supply unit-control and optional recursive-seat fixtures");
  const auto data = fixture(argv[1]);
  unsigned cases = 0;
  for (const auto &row : data.at("cases")) {
    std::array<std::vector<std::uint8_t>, 6> bodies;
    bodies[0] = decode(row.at("before").at(0).get<std::string>());
    bodies[1].resize(0x300);
    write(bodies[1], 0x13c, row.at("owners").get<unsigned>() & 4 ? 0xffffffffu : 0xe2700001u);
    for (unsigned i = 0; i < 4; ++i)
      bodies[i+2] = decode(row.at("before").at(i+1).get<std::string>());
    NativeUnitInteractionDefinition definition;
    nlohmann::json events = nlohmann::json::array();
    bool refreshing_seats = false;
    SourceUnitControl control({
        [&](std::uint32_t id) -> std::span<std::uint8_t> {
          const auto slot = id & 0xffffu;
          check(slot < bodies.size() && id == 0xe26f0000u+slot,
                "Unit-control fixture received a foreign object datum");
          if (slot >= 2 && !refreshing_seats)
            events.push_back({"item", id});
          return bodies[slot];
        },
        [&](std::uint32_t id) { return std::uint8_t((id & 0xffffu) < 2 ? 0 : 2); },
        [&](std::uint32_t id) -> const NativeUnitInteractionDefinition & {
          check(id == 0xe1000000u, "Unit-control fixture lost its canonical definition");
          refreshing_seats = true;
          events.push_back({"seats", 0xe26f0000u});
          return definition;
        },
        [](std::uint32_t) { throw std::runtime_error("Cold fixture reached an unprovided live item-effect owner"); },
        [](std::uint32_t) { throw std::runtime_error("Cold fixture reached an unprovided actor/driver owner"); }});
    control.set_active(0xe26f0000u, row.at("requested").get<bool>());
    check(events == row.at("events"), "Unit-control inventory/seat call order differs from originalCBF60");
    for (unsigned i = 0; i < 5; ++i) {
      const auto slot = i == 0 ? 0 : i+1;
      if (bodies[slot] != decode(row.at("after").at(i).get<std::string>())) {
        std::cerr << "Unit-control case=" << cases << " body=" << slot << '\n';
        throw std::runtime_error("Native unit/item control bytes differ from the whole original");
      }
    }
    ++cases;
  }
  std::cout << "PASS " << cases << " original unit/item control cases; root-unit seats, no active effect/driver owner\n";
  if (argc == 3)
    seats(argv[2]);
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
