#include "halo2/source_ai_references.hpp"
#include "halo2/source_ai_squad_team.hpp"
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
void write(std::span<std::uint8_t> bytes, unsigned offset,
           std::uint32_t value, unsigned count = 4) {
  for (unsigned i = 0; i < count; ++i)
    bytes[offset+i] = std::uint8_t(value >> (i*8));
}
void compare(std::span<const std::uint8_t> bytes, const std::string &hex) {
  check(bytes.size()*2 == hex.size(), "Original record bank size differs");
  for (unsigned i = 0; i < bytes.size(); ++i)
    if (bytes[i] != std::stoul(hex.substr(i*2, 2), nullptr, 16)) {
      std::cerr << "Record offset=" << i << " actual=" << unsigned(bytes[i])
                << " expected=" << hex.substr(i*2, 2) << '\n';
      throw std::runtime_error("Native squad bank differs from original200B30");
    }
}
}
int main(int argc, char **argv) try {
  check(argc == 2, "Supply the retained original AI reference fixture");
  std::ifstream file(argv[1]);
  std::string line;
  bool found = false;
  while (std::getline(file, line))
    if (line == "```json") { found = true; break; }
  check(found, "AI reference fixture has no JSON block");
  nlohmann::json fixture;
  file >> fixture;
  check(fixture.at("passed").get<bool>(), "Original AI oracle failed");
  unsigned cases = 0, queries = 0;
  for (const auto &original : fixture.at("cases")) {
    std::vector<SourceAiSquadDefinition> definitions;
    for (const auto &squad : original.at("squads")) {
      const auto &team = squad.at("team_inputs");
      check(source_ai_squad_team(team.at("authored_team").get<std::uint8_t>(),
                                 team.at("local_unit_team").get<std::uint8_t>(),
                                 team.at("general_type").get<std::int16_t>()) ==
                squad.at("resolved_team").get<std::uint8_t>(),
            "Canonical squad team inputs differ from whole original200C20");
      definitions.push_back({squad.at("flags").get<std::uint32_t>(),
                             squad.at("parent").get<std::int16_t>(),
                             squad.at("resolved_team").get<std::uint8_t>(),
                             squad.at("starting_locations").get<std::vector<std::uint32_t>>()});
    }
    const auto parents = original.at("groups").get<std::vector<std::int16_t>>();
    SourceAiSquadBank bank(definitions, parents);
    compare(bank.squads().elements().first(definitions.size()*0x98),
            original.at("squad_records_hex").get<std::string>());
    compare(bank.groups().elements().first(parents.size()*0x38),
            original.at("group_records_hex").get<std::string>());
    check(bank.squads().count() == std::int32_t(definitions.size()) &&
              bank.groups().count() == std::int32_t(parents.size()),
          "Squad bank count differs from the constructor owner");
    // The fixture supplies published actor membership. Actual birth and
    // activation are deliberately not substituted by this read-only test.
    DatumPool actors("actor", 256, 0x888, 0);
    auto squads = bank.squads();
    for (const auto &entry : original.at("actors")) {
      const auto id = actors.allocate();
      check(id == entry.at("datum").get<std::uint32_t>(),
            "Actor fixture lost its separate pool identity");
      auto bytes = actors.lookup(id);
      for (const auto &[offset, field] :
           {std::pair{0x18u, "unit"}, {0x1cu, "perception"},
            {0x20u, "next"}, {0x30u, "squad"}, {0x38u, "name"}})
        write(bytes, offset, entry.at(field).get<std::uint32_t>());
      write(bytes, 9, entry.at("flag9").get<std::uint8_t>(), 1);
      write(squads.element_storage().subspan(entry.at("squad").get<unsigned>()*0x98, 0x98), 0x68, id);
    }
    for (const auto &query : original.at("queries")) {
      const SourceAiReferenceView references{
          {actors, squads, bank.groups(), bank.definitions(),
           query.at("active").get<bool>()}};
      const auto packed = query.at("reference").get<std::uint32_t>();
      const auto expected = query.at("actor").get<std::uint32_t>();
      if (references.actor(packed) != expected) {
        std::cerr << "Bank=" << cases << " reference=" << std::hex << packed
                  << " actor=" << references.actor(packed)
                  << " expected=" << expected << std::dec << '\n';
        throw std::runtime_error("Packed AI resolution differs from original272B70");
      }
      const auto result = references.actors(packed);
      if (result != query.at("actors").get<std::vector<std::uint32_t>>()) {
        std::cerr << "Bank=" << cases << " reference=" << std::hex << packed << std::dec << '\n';
        throw std::runtime_error("Linked AI actor order differs from original272D90/272E20");
      }
      ++queries;
    }
    SourceAiSquadBank saved = bank;
    check(saved.squads().next_salt() == bank.squads().next_salt() &&
              saved.groups().next_salt() == bank.groups().next_salt(),
          "Squad checkpoint lost source pool allocation state");
    ++cases;
  }
  std::cout << "PASS " << cases << " full original squad/group banks and "
            << queries << " packed AI queries\n";
  return 0;
} catch (const std::exception &error) {
  std::cerr << error.what() << '\n';
  return 1;
}
