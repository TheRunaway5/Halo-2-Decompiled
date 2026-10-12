#include "halo2/cache.hpp"
#include "halo2/lethal_attribution.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
using namespace halo2;
namespace {
void check(bool value, const std::string &message) {
  if (!value) throw FormatError(message);
}
std::uint32_t word(std::istringstream &row) {
  std::string value;
  if (!(row >> value)) throw FormatError("Missing lethal attribution fixture field");
  return std::stoul(value, nullptr, 16);
}
}
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw FormatError("Supply original lethal attribution fixtures");
    if (!std::filesystem::exists(argv[1])) return 77;
    std::ifstream file(argv[1]);
    unsigned rows = 0, admitted = 0;
    for (std::string line; std::getline(file, line);) {
      std::istringstream row(line);
      std::string name; row >> name;
      LethalAttributionInput input;
      input.tick = word(row); input.incoming_player = word(row);
      input.victim_player = word(row); input.result_flags = word(row);
      input.clock_rate = std::bit_cast<std::int16_t>(std::uint16_t(word(row)));
      input.incoming_designator = std::bit_cast<std::int16_t>(std::uint16_t(word(row)));
      input.victim_membership = std::bit_cast<std::int8_t>(std::uint8_t(word(row)));
      input.cause = std::uint8_t(word(row)); input.mode = std::uint8_t(word(row));
      input.unit_type = std::uint8_t(word(row)); input.engine_present = word(row);
      input.statistics_valid = word(row);
      std::vector<AttributionPlayer> players(word(row));
      for (auto &player : players) {
        player.datum = word(row);
        player.membership = std::bit_cast<std::int8_t>(std::uint8_t(word(row)));
      }
      DamageHistory history;
      for (auto &entry : history) {
        entry.tick = word(row); entry.amount = std::bit_cast<float>(word(row));
        entry.object = word(row); entry.player = word(row); entry.cause = std::uint8_t(word(row));
      }
      const bool expected_admission = word(row);
      const auto result = lethal_attribution(input, history, players);
      check(bool(result) == expected_admission, name + " admission");
      if (expected_admission) {
        const auto &actual = *result;
        check(actual.credited_player == word(row), name + " credited full player");
        check(actual.cause == word(row), name + " selected cause");
        check(actual.relationship == bool(word(row)), name + " relationship byte");
        check(actual.credited_kill == bool(word(row)), name + " enemy kill");
        check(actual.nonenemy_credit == bool(word(row)), name + " nonenemy credit");
        check(actual.victim_death == bool(word(row)), name + " victim death");
        check(actual.suicide == bool(word(row)), name + " incoming self comparison");
        const auto assists = word(row);
        check(actual.assist_count == assists, name + " assist count");
        for (unsigned i = 0; i < assists; ++i)
          check(actual.assists[i] == word(row), name + " assist full identity/order");
        const auto nonenemy = word(row);
        check(actual.nonenemy_history_count == nonenemy, name + " nonenemy history count");
        for (unsigned i = 0; i < nonenemy; ++i)
          check(actual.nonenemy_history[i] == word(row), name + " nonenemy history identity/order");
        ++admitted;
      }
      check(!row.fail(), name + " incomplete fixture");
      std::string extra; check(!(row >> extra), name + " trailing fixture");
      ++rows;
    }
    check(rows == 191 && admitted == 18, "Incomplete original lethal attribution suite");
    std::cout << rows << " original damage/result gates and " << admitted
              << " full-salt lethal credit/assist selections matched\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
  }
}
