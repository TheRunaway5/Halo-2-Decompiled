#include "halo2/frontend.hpp"
#include "halo2/mcc_cache.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void require(bool condition, const char *why) { if (!condition) throw FormatError(why); }
void choose(MenuState &state, const std::string &label) {
  const auto &rows = state.frame().entries;
  const auto row = std::find_if(rows.begin(), rows.end(), [&](const auto &entry) { return entry.label == label; });
  require(row != rows.end(), "Workshop navigation row is missing");
  state.select(unsigned(row - rows.begin())); state.accept();
}
}
int main(int argc, char **argv) { try {
  if (argc != 2 || !std::filesystem::is_regular_file(std::filesystem::path(argv[1]) / "mainmenu.map")) return 77;
  auto content = Cache(std::filesystem::path(argv[1]) / "mainmenu.map").load_frontend();
  content.vista=load_vista_settings(std::filesystem::path(argv[1])/"mainmenu.map");
  MenuState state(content);
  unsigned scans{};
  state.set_workshop_scanner([&] {
    ++scans;
    WorkshopCatalog catalog;
    WorkshopMod mod;
    mod.title = "E3 2003";
    mod.description = "Reconstructed campaign.";
    mod.missions.push_back({"/installed/workshop/3360515088", "earthcity_e3", "EARTH CITY", "Clear the LZ.", true});
    catalog.mods.push_back(std::move(mod)); return catalog;
  });
  state.open(6);
  require(state.frame().entries.back().label == "EXIT GAME", "Exit Game lost its final main-menu row");
  choose(state, "MODS");
  require(scans == 1 && state.frame().title == "MODS", "Mods did not refresh the installed catalog");
  require(state.workshop_details(state.frame())==std::pair{std::string("E3 2003"),std::string("Reconstructed campaign.")},
          "Mods help retained its gametype description");
  Mesh mesh;FrontendPainter painter(content,std::filesystem::canonical(argv[1]).parent_path()/"h2_fonts",mesh);
  const auto draw=painter.draw(state,5.f);
  for(const auto &v:draw.vertices)
    require(mesh.materials.at(v.material).name.find("variant_settings")==std::string::npos,
            "Mods pane retained gametype artwork");
  choose(state, "E3 2003");
  choose(state, "EARTH CITY");
  require(state.workshop_details(state.frame())->first=="EARTH CITY",
          "Mission help lost its authored title");
  choose(state, "Play Campaign");
  require(state.frame().screen == 12 && state.frame().context == workshop_menu_context,
          "Workshop campaign missed the existing difficulty picker");
  state.select(2); state.accept();
  require(state.launch && state.launch->campaign && state.launch->difficulty == 2 && state.launch->workshop &&
          state.launch->workshop->scenario == "earthcity_e3" && state.launch->map.empty(),
          "Workshop launch used a stock map or lost its selected mission/difficulty");
  state.launch.reset();
  state.begin_workshop_preparation();
  state.back(); state.accept();
  require(state.frame().title == "PREPARING MOD", "Preparation progress lost its owner before completion");
  state.finish_workshop_preparation("Fixture unsupported material");
  require(state.frame().title == "UNAVAILABLE", "Preparation failure did not return an actionable menu dialog");
  state.back(); state.back(); state.back(); state.back();
  choose(state, "Refresh");
  require(scans == 2 && state.frame().title == "MODS", "Workshop refresh did not rebuild the catalog");
  state.back();
  state.set_workshop_scanner([] { return WorkshopCatalog{}; });
  choose(state, "MODS");
  require(state.frame().entries.size() == 1 && state.frame().entries[0].label == "Refresh" &&
          state.frame().description.find("No Halo 2") != std::string::npos,
          "Empty Workshop catalog has no usable refresh/explanation");
  std::cout << "Workshop menu navigation and launch ownership passed\n";
  return 0;
} catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; } }
