#include "halo2/frontend.hpp"
#include "halo2/frontend_lobby_maps.hpp"
#include "halo2/frontend_multiplayer_maps.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool value, const char *message) {
  if (!value) throw FormatError(message);
}
std::array<std::byte,464> mcc_header(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary);
  std::array<std::byte,464> header{};
  check(bool(file.read(reinterpret_cast<char *>(header.data()),header.size())),
        "selected MCC cache header is unavailable");
  check(Bytes(header).u32(4)==13,"map routing test rejects Xbox/Vista runtime assets");
  return header;
}
void choose(MenuState &state,unsigned index,unsigned screen) {
  state.select(index); state.accept();
  check(state.frame().screen==screen,"ordinary local menu route changed");
}
struct ProfileFixture {
  std::filesystem::path directory;
  ProfileFixture() {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
    check(scratch && *scratch,"map integration test requires tools/run_test.py scratch ownership");
    directory=std::filesystem::path(scratch)/"mcc-multiplayer-map-selection";
    check(std::filesystem::create_directory(directory),"profile fixture directory already exists");
    std::ofstream file(path());
    file<<"halo2-native-menu-v1\nactive \"Map Selection Test\"\nprofile \"Map Selection Test\"\n";
    check(bool(file),"cannot write scratch-owned local profile fixture");
  }
  ~ProfileFixture() {
    std::error_code error;
    std::filesystem::remove_all(directory,error);
  }
  std::filesystem::path path() const { return directory/"preferences"; }
};
struct Totals { unsigned previews{},launches{},placeholders{},energy{},additional_maps{}; };
Totals exercise(const FrontendContent &content,const std::filesystem::path &maps,
                const std::filesystem::path &preferences,bool converted) {
  MenuState state(content,preferences);
  check(!content.vista && !content.gameplay_overlay,"MCC map integration acquired another runtime asset owner");
  state.accept();
  check(state.frame().screen==10,"title did not open actual profile selector");
  const auto profile=std::find_if(state.frame().entries.begin(),state.frame().entries.end(),
      [](const auto &entry){return entry.label=="Map Selection Test" && entry.destination==-3;});
  check(profile!=state.frame().entries.end(),"scratch profile did not use native profile loading");
  choose(state,unsigned(profile-state.frame().entries.begin()),6);
  check(state.signed_in_profile()=="Map Selection Test","normal profile sign-in failed");
  choose(state,2,30); // Actual local Split Screen main-menu row.
  choose(state,0,14);
  check(state.local_players().size()==1,"local map routing lost the signed-in player");
  const auto initial=state.lobby_view();
  check(initial && initial->map && initial->map->map_id==50 && initial->map->name=="Lockout",
        "initial local map changed with imported catalog record order");
  check(state.selected_multiplayer_configuration().map=="lockout.map",
        "initial native configuration no longer selects exact Lockout cache");

  // This callback validates routing only: actual selected MCC file/header path.
  // It supplies no gameplay capability, source session status, or completion.
  unsigned validations{};
  const UiLobbyMap *accepted{};
  state.set_multiplayer_map_validator([&](const FrontendLaunch &launch) {
    check(accepted && launch.multiplayer && launch.local_players==1 &&
          launch.selected_profiles.size()==1,"map launch lost actual local selection");
    check(launch.map.parent_path().empty(),"map launch escaped the selected cache directory");
    const auto header=mcc_header(maps/launch.map);
    check(Bytes(header).string(208,256)==accepted->scenario_path,
          "launch path and authored selected scenario are different maps");
    ++validations;
  });
  const auto source_list=frontend_lobby_maps(content.multiplayer_maps,content.campaign_maps,
                                           frontend_lobby_new_profile_map_progress);
  check(source_list.catalog_indices.size()==24,"current imported25-map admission coverage changed");
  if(converted) {
    check(content.multiplayer_maps.at(source_list.catalog_indices[22]).map_id==1400 &&
          content.multiplayer_maps.at(source_list.catalog_indices[23]).map_id==1402,
          "converted authored sort order did not place Desolation/Tombstone last");
    for(const auto &map:content.multiplayer_maps) {
      check(map.map_id!=1239 && map.map_id!=1240,"converted catalog retained an unqualified placeholder ID");
      if(map.map_id!=1400 && map.map_id!=1402)continue;
      const bool tombstone=map.map_id==1402;
      check(map.name==(tombstone?"Tombstone":"Desolation") &&
            map.sort_order==(tombstone?1500:1400) && map.flags==0 &&
            map.scenario_path==(tombstone?R"(scenarios\multi\halo\highplains\highplains)":R"(scenarios\multi\halo\derelict\derelict)") &&
            map.team_counts==std::array<std::uint8_t,16>{0,2,8,8,8,0,0,8,8,2,0,0,0,0,0,0},
            "converted logical map fields differ from their authored MCC scenario definition");
      check(map.description==(tombstone?
                "The UNSC decommissioned this munitions testing complex after safety concerns came to light.":
                "The jungle is slowly reclaiming this once pristine but now derelict structure."),
            "converted description differs from the authored MCC scenario definition");
      const auto image=content.images.find(map.bitmap);
      check(image!=content.images.end() && image->second.name==(tombstone?
                R"(ui\code_global_bitmaps\multiplayer23)":R"(ui\code_global_bitmaps\multiplayer24)") &&
            image->second.images.size()==1 && image->second.images.front().width==220 &&
            image->second.images.front().height==207,
            "converted preview lost its destination bitmap namespace or native image owner");
    }
  }
  Totals totals;
  const std::set<std::int32_t> original_map_ids{50,80,20,10,100,60,70,30,110,800,40,120};
  std::set<std::int32_t> selected_ids;
  for (unsigned row=0;row<source_list.catalog_indices.size();++row) {
    state.open(206);
    check(state.frame().entries.size()==source_list.catalog_indices.size(),
          "MenuState filtered authored maps through a fixed retail list");
    state.select(row);
    const auto &entry=state.frame().entries.at(row);
    const auto index=source_list.catalog_indices.at(row);
    const auto &map=content.multiplayer_maps.at(index);
    check(entry.destination==-2 && entry.value==int(index) && entry.label==map.name,
          "map picker row lost its authored catalog datum or title");
    check(state.focused_lobby_map(state.frame())==&map,
          "focused preview is bound to another sorted/catalog row");
    ++totals.previews;
    choose(state,row,14);
    const auto lobby=state.lobby_view();
    check(lobby && lobby->map==&map && state.expand("\xee\x90\x88")==map.name,
          "accepted lobby preview/title is not the focused authored map");
    accepted=&map;
    selected_ids.insert(map.map_id);
    const auto before=validations;
    try {
      const auto launch=state.selected_multiplayer_configuration();
      check(!map.name.empty() && map.map_id!=20 && map.map_id!=40,
            "unfinished or unqualified map bypassed its existing launch boundary");
      check(validations==before+1 && launch.map==frontend_multiplayer_map_file(map),
            "launch omitted selected-cache validation or changed authored path");
      if(!original_map_ids.contains(map.map_id))++totals.additional_maps;
      ++totals.launches;
    } catch(const FormatError &error) {
      const std::string message=error.what();
      check(validations==before,"rejected map reached the selected-cache launch validator");
      if (map.name.empty()) {
        check(!converted && (map.map_id==1239 || map.map_id==1240) &&
              message.find("catalog identity is unqualified")!=message.npos,
              "blank authored placeholder was silently renamed or failed for another reason");
        ++totals.placeholders;
      } else {
        check((map.map_id==20 || map.map_id==40) &&
              message.find("battery combat is unfinished")!=message.npos,
              "qualified map routing unexpectedly failed");
        ++totals.energy;
      }
    }
  }
  check(!selected_ids.contains(120) && selected_ids.contains(1201) &&
        selected_ids.contains(1300) && selected_ids.contains(444678) && selected_ids.contains(91101),
        "Foundation unlock or imported DLC/PC map admission changed");
  if(converted)check(selected_ids.contains(1400) && selected_ids.contains(1402),
                    "converted authored DLC IDs disappeared from actual menu selection");
  check(totals.previews==24 && totals.launches==(converted?22u:20u) &&
        totals.placeholders==(converted?0u:2u) && totals.energy==2 &&
        totals.additional_maps==(converted?13u:11u),
        "all-row map routing coverage changed");
  // Legacy screen17 must use the same catalog datums, without adding a second
  // fixed map array or bypassing the existing admission rule.
  state.open(17);
  check(state.frame().entries.size()==source_list.catalog_indices.size(),
        "legacy map picker retained a fixed retail-only map list");
  for(unsigned row=0;row<source_list.catalog_indices.size();++row) {
    const auto index=source_list.catalog_indices[row];
    check(state.frame().entries[row].value==int(index) &&
          state.frame().entries[row].label==content.multiplayer_maps[index].name,
          "legacy map picker changed authored map identity");
  }
  return totals;
}
} // namespace
int main(int argc,char **argv) { try {
  if(argc<2)throw FormatError("map integration check needs imported MCC maps directory or mainmenu.map [--converted --validation-maps MCC_MAPS]");
  auto mainmenu=std::filesystem::path(argv[1]);
  if(std::filesystem::is_directory(mainmenu))mainmenu/="mainmenu.map";
  bool converted{};
  std::filesystem::path validation_maps;
  for(int i=2;i<argc;++i) {
    const std::string option=argv[i];
    if(option=="--converted" && !converted)converted=true;
    else if(option=="--validation-maps" && validation_maps.empty() && i+1<argc)
      validation_maps=argv[++i];
    else throw FormatError("unknown or duplicate map integration option: "+option);
  }
  if(converted && validation_maps.empty())
    throw FormatError("converted menu integration requires its actual MCC --validation-maps owner");
  if(validation_maps.empty())validation_maps=mainmenu.parent_path();
  (void)mcc_header(validation_maps/"shared.map");
  (void)mcc_header(mainmenu); // Reject other cache families before construction.
  Cache cache(mainmenu,validation_maps);
  check(cache.is_mcc(),"map integration requires actual imported MCC native cache construction");
  auto content=cache.load_frontend();
  check(content.multiplayer_maps.size()==25,"current imported MCC catalog coverage changed");
  ProfileFixture profile;
  const auto first=exercise(content,validation_maps,profile.path(),converted);
  std::reverse(content.multiplayer_maps.begin(),content.multiplayer_maps.end());
  const auto reordered=exercise(content,validation_maps,profile.path(),converted);
  std::cout<<first.previews+reordered.previews<<" actual MCC menu previews, "
      <<first.launches+reordered.launches<<" selected-cache launch routes, "
      <<first.placeholders+reordered.placeholders<<" honest placeholder boundaries and "
      <<first.energy+reordered.energy<<" preserved battery-combat boundaries; original/reversed catalogs\n";
  std::cout<<"Catalog domain: "<<(converted?"converted authored MCC DLC":"unconverted MCC placeholder baseline")<<'\n';
  std::cout<<"CPU menu/catalog routing only; no gameplay session, renderer, source return, or all-map readiness claim\n";
  return 0;
} catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;} }
