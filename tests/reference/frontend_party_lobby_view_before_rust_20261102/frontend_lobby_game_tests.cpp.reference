#include "halo2/frontend_lobby_game.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
std::vector<std::string> fields(const std::string &line) {
  std::vector<std::string> out;
  std::size_t start = 0;
  do {
    const auto end = line.find('\t', start);
    out.push_back(line.substr(start, end == std::string::npos ? end : end - start));
    if (end == std::string::npos) break;
    start = end + 1;
  } while (true);
  return out;
}
int integer(const std::string &value) { return std::stoi(value); }
std::uint32_t hex(const std::string &value) {
  return std::uint32_t(std::stoul(value, nullptr, 16));
}
const UiLobbyMap &map(const FrontendContent &content, bool campaign, int id) {
  const auto &catalog = campaign ? content.campaign_maps : content.multiplayer_maps;
  const auto found = std::find_if(catalog.begin(), catalog.end(),
                                [&](const auto &m) { return m.map_id == id; });
  check(found != catalog.end(), "original lobby map missing from loaded catalog");
  return *found;
}
} // namespace
int main(int argc, char **argv) try {
  if (argc != 3 || !std::filesystem::exists(argv[1]) ||
      !std::filesystem::exists(std::filesystem::path(argv[2]) / "mainmenu.map"))
    return 77;
  Cache cache(std::filesystem::path(argv[2]) / "mainmenu.map");
  const auto content = cache.load_frontend();
  const auto &screen = content.screens.at(14);
  const auto separator = content.localized(screen, 0x1a001820u);
  check(separator == ": ", "original quick-option separator was not localized");
  check(content.campaign_maps.size() == 15 && content.multiplayer_maps.size() == 12,
        "literal lobby catalog membership differs");
  unsigned icons=0, defaults=0, quick=0, maps=0, sorts=0, help=0, counts=0, difficulty=0;
  std::ifstream input(argv[1]);
  for (std::string line; std::getline(input, line);) {
    if (line.empty() || line.front() == '#') continue;
    const auto f = fields(line);
    if (f.front() == "icon") {
      check(f.size() == 4, "truncated original lobby icon fixture");
      const auto engine = integer(f[1]), type = integer(f[2]);
      if (engine >= 0)
        check(frontend_lobby_variant_type(engine) == type,
              "frontend engine mapping differs from original19D220");
      check(frontend_lobby_game_icon(type) == integer(f[3]),
            "mode icon differs from original250A43");
      ++icons;
    } else if (f.front() == "default") {
      check(f.size() == 4, "truncated original variant default fixture");
      check(frontend_lobby_parameter_default(integer(f[1]), integer(f[2])) == integer(f[3]),
            "variant default differs from original19D220/2374F0");
      ++defaults;
    } else if (f.front() == "quick") {
      check(f.size() == 7, "truncated original quick-option fixture");
      std::map<unsigned, int> values;
      if (integer(f[5])) values.emplace(integer(f[3]), integer(f[4]));
      const auto actual = frontend_lobby_quick_options(content, integer(f[1]), values, separator);
      check(actual.at(integer(f[2])) == f[6],
            "quick-option text differs from original25092C");
      ++quick;
    } else if (f.front() == "map") {
      check(f.size() == 16, "truncated original lobby map fixture");
      const auto &actual = map(content, integer(f[1]) != 0, integer(f[3]));
      check(actual.campaign_id == integer(f[2]) && actual.bitmap == hex(f[4]) &&
            actual.name == f[5] && actual.description == f[6] &&
            actual.sort_order == integer(f[13]) && actual.flags == integer(f[14]) &&
            actual.scenario_path == f[15], "loaded map fields differ from literal original catalog");
      const auto &image = content.images.at(actual.bitmap).images.at(0);
      check(image.width == unsigned(integer(f[11])) && image.height == unsigned(integer(f[12])),
            "map preview did not preload original descriptor0 dimensions");
      const auto position = screen.panes.at(0).bitmaps.at(2).position;
      const UiRect rect{position.y, position.x, position.y - image.height,
                        position.x + image.width};
      check(rect.top == integer(f[7]) && rect.left == integer(f[8]) &&
            rect.bottom == integer(f[9]) && rect.right == integer(f[10]),
            "map descriptor replacement differs from original2B0A7B rectangle");
      ++maps;
    } else if (f.front() == "sort") {
      check(f.size() == 4, "truncated original map-sort fixture");
      const auto a = map(content, false, integer(f[1])).sort_order;
      const auto b = map(content, false, integer(f[2])).sort_order;
      check((a > b ? 1 : a < b ? -1 : 0) == integer(f[3]),
            "map order differs from original19C9A0");
      ++sorts;
    } else if (f.front() == "help") {
      check(f.size() == 9, "truncated original party-help fixture");
      const auto actual = frontend_lobby_help(integer(f[1]), integer(f[2]) ? 2 : 0,
          integer(f[3]), integer(f[4]), integer(f[5]));
      check(actual.text_index == unsigned(integer(f[6])) && actual.visible == bool(integer(f[7])) &&
            actual.string_id == hex(f[8]), "party-help binding differs from original2508A8");
      if (actual.visible)
        check(!content.localized(screen, actual.string_id).empty(),
              "party-help string is absent from literal screen14 value block1");
      ++help;
    } else if (f.front() == "count") {
      check(f.size() == 6, "truncated original player-count fixture");
      const auto actual = frontend_lobby_player_count(integer(f[1]), integer(f[2]), integer(f[3]));
      check(actual == integer(f[4]) && frontend_lobby_player_count_string(actual) == hex(f[5]),
            "player-count cache/token differs from original22DAD1");
      ++counts;
    } else if (f.front() == "difficulty") {
      check(f.size() == 4, "truncated original campaign difficulty fixture");
      const auto id = frontend_lobby_difficulty_string(std::int16_t(integer(f[1])));
      check(id == hex(f[2]) && content.localized(screen, id) == f[3],
            "campaign difficulty token differs from original22DC2A");
      ++difficulty;
    } else throw FormatError("unknown original lobby fixture case");
  }
  check(icons==12 && defaults==784 && quick==70 && maps==27 && sorts==144 && help==144 && counts==28 && difficulty==7,
        "original lobby game-panel coverage changed");
  check(!frontend_lobby_variant_type(7) && !frontend_lobby_parameter_default(7, 0) &&
        !frontend_lobby_parameter_default(0, 112) && !frontend_lobby_player_count_string(-1) &&
        !frontend_lobby_player_count_string(17), "invalid lobby state was silently substituted");
  std::cout << icons << " icons, " << defaults << " defaults, " << quick << " quick rows, "
            << maps << " maps, " << sorts << " sort pairs, " << help << " party-help and "
            << counts << " count and " << difficulty << " difficulty cases match original executable/tag data\n";
  return 0;
} catch (const std::exception &e) {
  std::cerr << e.what() << '\n';
  return 1;
}
