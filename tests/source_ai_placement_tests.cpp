#include "halo2/source_ai_placement.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using Json = nlohmann::json;
using namespace halo2;
namespace {
void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
void required_owners() {
  source_place_ai(0xffffffffu, 1, {});
  bool rejected = false;
  try {
    source_place_ai(0xffffffffu, {}, {});
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected,
        "Default NONE reference skipped its required original action");
  rejected = false;
  try {
    source_place_ai(0, 1, {});
  } catch (const std::invalid_argument &) {
    rejected = true;
  }
  check(rejected, "Missing squad action supplied success");
  bool remembered = false;
  SourceAiPlacementOwners owners;
  owners.place_squad = [](auto, auto) {
    throw std::runtime_error("Reached action unavailable");
  };
  owners.remember_reference = [&](auto) { remembered = true; };
  rejected = false;
  try {
    source_place_ai(0, 1, owners);
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  check(rejected && !remembered, "Failed action published a pending reference");
}
} // namespace
int main(int argc, char **argv) try {
  if (argc != 2)
    throw std::invalid_argument("Expected original wrapper fixture");
  Json fixture;
  std::ifstream(argv[1]) >> fixture;
  for (const auto &row : fixture.at("cases")) {
    auto pending = row.at("pending_before").get<std::uint32_t>();
    Json calls = Json::array();
    SourceAiPlacementOwners owners;
    owners.place_reference = [&](auto reference) {
      calls.push_back(Json{{"kind", "reference"}, {"reference", reference}});
    };
    owners.place_squad = [&](auto squad, auto count) {
      calls.push_back(Json{{"kind", "squad"},
                           {"squad", squad},
                           {"count", count},
                           {"arguments", Json::array({0xffffffffu, 0u, 1u})}});
    };
    owners.remember_reference = [&](auto reference) { pending = reference; };
    std::optional<std::int16_t> count;
    if (!row.at("count").is_null())
      count = row.at("count").get<std::int16_t>();
    if (row.at("arguments_ready").get<bool>())
      source_place_ai(row.at("reference"), count, owners);
    check(calls == row.at("calls"),
          "Action request sequence differs from whole original wrapper");
    check(pending == row.at("pending_after"),
          "Pending AI reference differs from whole original wrapper");
  }
  required_owners();
  std::cout
      << "PASS " << fixture.at("cases").size()
      << " original placement overload cases and required-owner failures\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
