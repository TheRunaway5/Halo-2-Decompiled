#include "halo2/vista_lobby_sort.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <array>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json = nlohmann::json;
int main(int argc, char** argv) try {
  if (argc != 2) return 77;
  std::ifstream input(argv[1]); if (!input) return 77;
  const auto fixture = Json::parse(input);
  if (fixture.at("schema") != "H2_VISTA_LOBBY_SORT_ORIGINAL_V1") throw FormatError("sort fixture schema");
  unsigned count{};
  for (const auto& c : fixture.at("cases")) {
    const auto& initial = c.at("input");
    std::array<std::array<std::byte,0x84>,16> configurations{};
    for (unsigned i = 0; i < 16; ++i)
      configurations[i][0x7c] = std::byte(initial.at("teams").at(i).get<int>() & 255);
    auto players = initial.at("players").get<std::vector<std::uint32_t>>();
    auto trace = Json::array(); unsigned lookups{};
    const VistaLobbySortConfiguration service = [&](std::uint32_t id) -> std::span<const std::byte> {
      trace.push_back(Json::array({"configuration",id})); ++lookups;
      if (initial.contains("mutation")) {
        const auto& mutation = initial.at("mutation");
        if (lookups == mutation.at("lookup"))
          configurations.at(mutation.at("player").get<unsigned>())[0x7c] = std::byte(mutation.at("team").get<int>() & 255);
      }
      return configurations.at(id);
    };
    // A separate wrapper records comparator entry before either lookup.
    // Sort itself exposes the same sequence as two adjacent configuration
    // calls; reconstruct that entry from the recorded source callback IDs.
    std::int32_t result{};
    if (initial.at("kind") == "compare") result = vista_lobby_compare_players(players.at(0),players.at(1),service);
    else vista_lobby_sort_players(players,service);
    Json full_trace = Json::array();
    for (std::size_t i = 0; i < trace.size(); i += 2) {
      full_trace.push_back(Json::array({"compare",trace.at(i).at(1),trace.at(i+1).at(1)}));
      full_trace.push_back(trace.at(i)); full_trace.push_back(trace.at(i+1));
    }
    if (Json(players) != c.at("players") || full_trace != c.at("trace"))
      throw FormatError("source sort order/callback trace case" + std::to_string(count));
    if (initial.at("kind") == "compare" && result != c.at("result"))
      throw FormatError("source signed team comparison");
    for (unsigned i = 0; i < 16; ++i)
      if (std::to_integer<unsigned>(configurations[i][0x7c]) != (c.at("teams").at(i).get<int>() & 255))
        throw FormatError("retained configuration mutation");
    ++count;
  }
  // No comparator is consumed for a zero/one-element original call.
  std::array<std::uint32_t,1> single{15};
  vista_lobby_sort_players({},{}); vista_lobby_sort_players(single,{});
  bool rejected{};
  try { (void)vista_lobby_compare_players(0,1,{}); }
  catch (const FormatError&) { rejected = true; }
  if (!rejected) throw FormatError("missing consumed configuration owner admitted");
  std::cout << "PASS " << count << " original sort/comparator cases\n";
  return 0;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
