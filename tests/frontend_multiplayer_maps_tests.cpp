#include "halo2/frontend_multiplayer_maps.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tuple>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
template<class F> void rejects(F call, const char *message) {
  try { call(); } catch (const FormatError &) { return; }
  throw FormatError(message);
}
std::vector<std::string> fields(const std::string &line) {
  std::vector<std::string> output;
  std::size_t begin = 0;
  for (;;) {
    const auto end = line.find('\t', begin);
    output.push_back(line.substr(begin, end == line.npos ? line.size() - begin : end - begin));
    if (end == line.npos) return output;
    begin = end + 1;
  }
}
std::vector<UiLobbyMap> original_catalog(const std::filesystem::path &fixture) {
  std::ifstream input(fixture);
  check(bool(input), "original metadata fixture is unavailable");
  std::vector<UiLobbyMap> maps;
  for (std::string line; std::getline(input, line);) {
    const auto f = fields(line);
    if (f[0] != "map") continue;
    check(f.size() == 8, "original metadata fixture is truncated");
    UiLobbyMap map;
    map.map_id = std::stoi(f[1]);
    map.bitmap = std::uint32_t(std::stoul(f[2], nullptr, 16));
    map.sort_order = std::stoi(f[3]); map.flags = std::uint8_t(std::stoi(f[4]));
    map.name = f[5]; map.description = f[6]; map.scenario_path = f[7];
    maps.push_back(std::move(map));
  }
  check(maps.size() == 12, "original metadata catalog coverage changed");
  return maps;
}
std::vector<UiLobbyMap> mcc_catalog() {
  // Literal ID/name/path/order/flag fields from imported MCC mainmenu's first
  // matg+178 multiplayer block, read2026-10-19. Bitmap/description tokens below
  // are explicit routing fixtures; no Xbox/Vista runtime assets are opened.
  using Row = std::tuple<int,const char *,const char *,int,unsigned>;
  const Row rows[] = {
    {80,"Ascension",R"(scenarios\multi\ascension\ascension)",20,0},
    {1201,"Backwash",R"(scenarios\multi\backwash\backwash)",170,0},
    {100,"Beaver Creek",R"(scenarios\multi\halo\beavercreek\beavercreek)",50,0},
    {60,"Burial Mounds",R"(scenarios\multi\burial_mounds\burial_mounds)",60,0},
    {110,"Coagulation",R"(scenarios\multi\halo\coagulation\coagulation)",90,0},
    {70,"Colossus",R"(scenarios\multi\colossus\colossus)",70,0},
    {1300,"Containment",R"(scenarios\multi\containment\containment)",130,0},
    {10,"Ivory Tower",R"(scenarios\multi\cyclotron\cyclotron)",40,0},
    {1302,"Sanctuary",R"(scenarios\multi\deltatap\deltatap)",150,0},
    {1200,"Relic",R"(scenarios\multi\dune\dune)",200,0},
    {1001,"Elongation",R"(scenarios\multi\elongation\elongation)",180,0},
    {120,"Foundation",R"(scenarios\multi\marathon\foundation\foundation)",120,1},
    {1002,"Gemini",R"(scenarios\multi\marathon\gemini\gemini)",190,0},
    {800,"Headlong",R"(scenarios\multi\headlong\headlong)",100,0},
    {50,"Lockout",R"(scenarios\multi\lockout\lockout)",10,0},
    {20,"Midship",R"(scenarios\multi\midship\midship)",30,0},
    {444678,"Uplift",R"(scenarios\multi\needle\needle)",230,0},
    {91101,"District",R"(scenarios\multi\street_sweeper\street_sweeper)",240,0},
    {1101,"Terminal",R"(scenarios\multi\triplicate\triplicate)",210,0},
    {1000,"Turf",R"(scenarios\multi\turf\turf)",160,0},
    {1109,"Warlock",R"(scenarios\multi\halo\warlock\warlock)",140,0},
    {40,"Waterworks",R"(scenarios\multi\waterworks\waterworks)",110,0},
    {30,"Zanzibar",R"(scenarios\multi\zanzibar\zanzibar)",80,0},
    {1239,"",R"(scenarios\multi\halo\highplains\highplains)",0,0},
    {1240,"",R"(scenarios\multi\halo\derelict\derelict)",0,0},
  };
  std::vector<UiLobbyMap> maps;
  for (const auto &[id,name,path,order,flags] : rows) {
    UiLobbyMap map;
    map.map_id=id; map.name=name; map.scenario_path=path;
    map.sort_order=order; map.flags=std::uint8_t(flags);
    map.bitmap=0xa0000000u+unsigned(maps.size());
    map.description="fixture description " + std::to_string(id);
    maps.push_back(std::move(map));
  }
  return maps;
}
void catalog_bindings(const std::vector<UiLobbyMap> &maps,
                      const std::vector<int> &expected) {
  UiLobbyMap final_campaign; final_campaign.campaign_id=1; final_campaign.map_id=805;
  const std::array campaign{final_campaign};
  const auto list=frontend_lobby_maps(maps,campaign,frontend_lobby_new_profile_map_progress);
  check(list.admission_known && list.catalog_indices.size()==expected.size(),
        "catalog admission lost an authored map or admitted locked Foundation");
  for (unsigned row=0; row<expected.size(); ++row) {
    const auto index=list.catalog_indices[row];
    const auto map=frontend_multiplayer_map(maps,index);
    const auto preview=frontend_lobby_map_preview(maps,list,row);
    check(map && map->map_id==expected[row] && preview && preview->map==map &&
          preview->map->bitmap==maps[index].bitmap &&
          preview->map->description==maps[index].description,
          "sorted row changed its authored label/preview/catalog identity");
    if (map->name.empty()) {
      rejects([&]{frontend_multiplayer_launch_file(maps,index);},
              "unqualified blank catalog title was silently substituted");
    } else {
      check(frontend_multiplayer_launch_file(maps,index)==frontend_multiplayer_map_file(*map),
            "launch stopped using the selected authored catalog datum");
    }
    check(frontend_multiplayer_energy_start(*map)==(map->map_id==20 || map->map_id==40),
          "energy-start gate changed with catalog order");
  }
  const auto initial=frontend_multiplayer_initial_map(maps,campaign);
  check(frontend_multiplayer_map(maps,initial)->map_id==50 &&
        frontend_multiplayer_launch_file(maps,initial)=="lockout.map",
        "default selection changed when the catalog record order changed");
  const auto unlocked=frontend_lobby_maps(maps,campaign,805);
  check(unlocked.catalog_indices.size()==maps.size(),"source Foundation unlock rule changed");
}
void malformed_paths() {
  UiLobbyMap map; map.map_id=5000; map.name="Authored test";
  for (const auto path : {"", "../lockout", "scenarios/../lockout", "/lockout",
                         "\\lockout", "C:\\lockout", "scenarios//lockout", "lockout/",
                         "lockout.map", "scenarios/lockout:other", "scenarios/lockout\n"}) {
    map.scenario_path=path;
    rejects([&]{frontend_multiplayer_map_file(map);},"unsafe authored path accepted");
  }
  map.scenario_path=R"(scenarios\multi\halo\highplains\highplains)";
  check(frontend_multiplayer_map_file(map)=="highplains.map","MCC scenario leaf was aliased");
  map.scenario_path=R"(scenarios\multi\halo\highplains2\highplains2)";
  check(frontend_multiplayer_map_file(map)=="highplains2.map","distinct Xbox reference path was aliased");
  map.scenario_path="scenarios/multi/test-map/test-map";
  check(frontend_multiplayer_map_file(map)=="test-map.map","safe authored path changed");
  check(!frontend_multiplayer_map({},0),"empty catalog manufactured a selected map");
  rejects([&]{frontend_multiplayer_launch_file({},0);},"empty catalog manufactured a launch");
  check(!frontend_multiplayer_map(std::span<const UiLobbyMap>(&map,1),1),
        "out-of-range catalog datum manufactured a preview");
}
} // namespace
int main(int argc,char **argv) { try {
  if (argc!=2) throw FormatError("map selection check needs the frozen original metadata TSV");
  const auto retail=original_catalog(argv[1]);
  catalog_bindings(retail,{50,80,20,10,100,60,70,30,110,800,40});
  const auto mcc=mcc_catalog();
  catalog_bindings(mcc,{1239,1240,50,80,20,10,100,60,70,30,110,800,40,
                        1300,1109,1302,1000,1201,1001,1002,1200,1101,444678,91101});
  check(frontend_multiplayer_map_file(mcc[7])=="cyclotron.map" &&
        frontend_multiplayer_map_file(mcc[16])=="needle.map" &&
        frontend_multiplayer_map_file(mcc[17])=="street_sweeper.map",
        "display titles were guessed into cache filenames");
  check(mcc[23].map_id==1239 && mcc[24].map_id==1240 &&
        frontend_multiplayer_map_file(mcc[23])=="highplains.map" &&
        frontend_multiplayer_map_file(mcc[24])=="derelict.map",
        "unqualified imported placeholders were renumbered or renamed");
  auto reordered=mcc;
  std::reverse(reordered.begin(),reordered.end());
  const auto initial=frontend_multiplayer_initial_map(reordered,{});
  check(reordered[initial].map_id==50,"catalog reordering changed selected default identity");
  malformed_paths();
  std::cout<<"12 original metadata rows and25 MCC rows retain catalog labels, previews, paths and gates;2 placeholders reject launch\n";
  return 0;
} catch (const std::exception &error) { std::cerr<<error.what()<<'\n';return 1;} }
