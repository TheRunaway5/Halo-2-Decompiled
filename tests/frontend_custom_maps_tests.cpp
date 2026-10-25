#include "halo2/frontend_custom_maps.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_multiplayer_maps.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
void path_guard() {
  const char *scratch = std::getenv("HALO2_TEST_SCRATCH");
  check(scratch, "binding guards require owned scratch");
  const auto root = std::filesystem::path(scratch) / "custom-menu";
  const auto maps = root / "halo2/h2_maps_win64_dx11";
  std::filesystem::create_directories(maps);
  FrontendContent content;
  UiLobbyMap map;
  map.map_id = 20261022; map.name = "Dire Valley";
  map.scenario_path = R"(scenarios\multi\dire_valley\dire_valley)";
  content.multiplayer_maps.push_back(map);
  nlohmann::json binding = {{"schema", "H2_FRONTEND_MAP_BINDINGS_V1"},
    {"maps", {{{"map_id", map.map_id}, {"scenario_path", map.scenario_path},
               {"cache_file", "halo2/Mods/../outside.map"}}}}};
  { std::ofstream out(root / "frontend-map-bindings.json"); out << binding; }
  bool rejected = false;
  try { bind_frontend_custom_maps(content, maps); }
  catch (const FormatError &e) {
    rejected = std::string(e.what()).find("exact relative cache path") != std::string::npos;
  }
  check(rejected && content.multiplayer_maps[0].cache_file.empty(),
        "traversal supplied a custom launch binding");
  content.multiplayer_maps[0].cache_file = "../dire_valley.map";
  rejected = false;
  try { (void)frontend_multiplayer_launch_file(content.multiplayer_maps, 0); }
  catch (const FormatError &) { rejected = true; }
  check(rejected, "relative custom launch owner was accepted");
}
void actual_binding(const std::filesystem::path &root) {
  const auto maps = root / "halo2/h2_maps_win64_dx11";
  const Cache mainmenu(maps / "mainmenu.map");
  auto content = mainmenu.load_frontend();
  bind_frontend_custom_maps(content, maps);
  unsigned custom = 0;
  for (unsigned i = 0; i < content.multiplayer_maps.size(); ++i) {
    const auto &map = content.multiplayer_maps[i];
    if (map.map_id == 20261022) {
      const auto path = frontend_multiplayer_launch_file(content.multiplayer_maps, i);
      const auto custom_root = root / "custom-maps/dire_valley";
      const auto custom_shared = custom_root / "halo2/h2_maps_win64_dx11";
      check(path == (custom_root / "halo2/Mods/dire_valley/dire_valley.map").generic_string(),
            "Dire Valley did not retain its exact prepared custom owner");
      check(map.shared_maps == custom_shared.generic_string(), "custom map lost its own resource bank");
      check(Cache(path, custom_shared, {}, custom_root).is_custom_mcc(), "Dire Valley acquired a stock whole-map alias");
      check(content.images.contains(map.bitmap), "custom map lost its preview image");
      ++custom;
    } else {
      check(map.cache_file.empty(), "custom map binding changed an unrelated stock row");
      if (!map.name.empty())
        check(frontend_multiplayer_launch_file(content.multiplayer_maps, i) ==
              frontend_multiplayer_map_file(map), "stock launch routing changed");
    }
  }
  check(custom == 1, "actual map catalog did not bind exactly one Dire Valley");
}
}
int main(int argc, char **argv) { try {
  if (argc == 1) path_guard();
  else if (argc == 2 && *argv[1]) actual_binding(std::filesystem::canonical(argv[1]));
  else return 77;
  std::cout << "Frontend custom-map owner checks passed\n";
  return 0;
} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; } }
