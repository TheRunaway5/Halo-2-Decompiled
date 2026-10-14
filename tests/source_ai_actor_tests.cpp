#include "halo2/source_ai_actor.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
using namespace halo2;
void check(bool result, const char *message) {
  if (!result)
    throw std::runtime_error(message);
}
int main(int argc, char **argv) try {
  if (argc != 2)
    throw std::runtime_error(
        "Supply the retained whole-original actor constructor fixture");
  std::ifstream file(argv[1]);
  std::string line;
  bool found = false;
  while (std::getline(file, line))
    if (line == "```json") {
      found = true;
      break;
    }
  check(found, "Actor fixture lacks its retained JSON block");
  nlohmann::json fixture;
  file >> fixture;
  const auto baseline = fixture.at("baseline_record_hex").get<std::string>();
  check(fixture.at("passed").get<bool>(),
        "Original constructor fixture failed");
  unsigned cases = 0;
  const auto constants = source_retail_ai_actor_constructor_constants();
  for (const auto &row : fixture.at("cases")) {
    SourceAiActorBank bank;
    auto counter = row.at("counter_before").get<std::uint16_t>();
    const SourceAiActorDefinition definition{
        row.at("definition").get<std::uint32_t>(),
        row.at("general_flags").get<std::uint32_t>(),
        row.at("actor_type").get<std::int16_t>()};
    const auto actor = bank.allocate(definition, constants,
                                     {row.at("tick").get<std::int32_t>(),
                                      row.at("rate").get<std::int16_t>(),
                                      row.at("bsp").get<std::int16_t>()},
                                     counter);
    check(actor == row.at("actor").get<std::uint32_t>(),
          "Actor allocation used a unit handle or the wrong source salt");
    check(counter == row.at("counter_after").get<std::uint16_t>(),
          "Actor constructor lost the original flag3 counter");
    const auto bytes = bank.record(actor);
    auto expected = baseline;
    for (const auto &delta : row.at("record_delta")) {
      const auto start = delta.at(0).get<std::size_t>();
      const auto hex = delta.at(1).get<std::string>();
      check(start <= bytes.size() && hex.size() % 2 == 0 &&
                hex.size() / 2 <= bytes.size() - start,
            "Original actor fixture delta lies outside its record");
      expected.replace(start * 2, hex.size(), hex);
    }
    check(bytes.size() * 2 == expected.size(),
          "Actor record stride differs from the original");
    for (unsigned i = 0; i < bytes.size(); ++i)
      if (bytes[i] != std::stoul(expected.substr(i * 2, 2), nullptr, 16)) {
        std::cerr << "Constructor case=" << cases << " offset=" << i
                  << " actual=" << unsigned(bytes[i])
                  << " expected=" << expected.substr(i * 2, 2) << '\n';
        throw std::runtime_error(
            "Native actor record differs from the whole original constructor");
      }
    SourceAiActorBank saved = bank;
    check(saved.record(actor).size() == 0x888 &&
              saved.pool().next_salt() == bank.pool().next_salt(),
          "Actor checkpoint lost its source pool identity");
    if (cases == 0) {
      const auto original = std::vector<std::uint8_t>(
          saved.pool().elements().begin(), saved.pool().elements().end());
      const auto second =
          bank.allocate(definition, constants, {900, 30, 4}, counter);
      check(bank.pool().count() == 2 && saved.pool().count() == 1 &&
                std::equal(original.begin(), original.end(),
                           saved.pool().elements().begin()),
            "Actor checkpoint aliases the continued pool backing");
      bool rejected = false;
      try {
        (void)saved.record(second);
      } catch (const std::invalid_argument &) {
        rejected = true;
      }
      check(rejected, "Actor checkpoint admitted a later generation");
    }
    ++cases;
  }
  SourceAiActorBank bank;
  std::uint16_t counter = 7;
  check(bank.allocate({}, constants, {1, 30, 0}, counter) ==
                SourceAiActorBank::none &&
            bank.pool().count() == 0 && counter == 7,
        "NONE actor definition allocated an identity or changed the counter");
  SourceAiActorDefinition definition{0xe1000000u, 8, 0};
  for (unsigned i = 0; i < 256; ++i)
    check(bank.allocate(definition, constants, {1, 30, 0}, counter) !=
              SourceAiActorBank::none,
          "Actual actor pool exhausted before256");
  DatumPool units("object", 1, 12);
  const auto unit = units.allocate();
  bool rejected = false;
  try {
    (void)bank.record(unit);
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected,
        "Actor bank accepted a unit object datum with the same low slot");
  const auto retained = counter;
  check(
      bank.allocate(definition, constants, {1, 30, 0}, counter) ==
              SourceAiActorBank::none &&
          counter == retained && bank.pool().count() == 256,
      "Exhausted source actor pool fabricated an identity or counter mutation");
  std::cout << "PASS " << cases
            << " whole-original actor records, pool identity/checkpoint and "
               "early-outs\n";
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
