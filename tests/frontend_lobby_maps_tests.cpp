#include "halo2/frontend_lobby_maps.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool value, const char *message) { if (!value) throw FormatError(message); }
std::vector<std::string> fields(const std::string &line) {
  std::vector<std::string> result; std::size_t start = 0;
  for (auto i = line.find('\t'); i != std::string::npos; i = line.find('\t', start)) {
    result.push_back(line.substr(start, i-start)); start=i+1;
  }
  result.push_back(line.substr(start)); return result;
}
int number(const std::string &text) { return std::stoi(text); }
std::uint32_t hexadecimal(const std::string &text) { return std::uint32_t(std::stoul(text,nullptr,16)); }
}
int main(int argc, char **argv) { try {
  if (argc != 3) throw FormatError("lobby maps check needs original cache and fixture");
  if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
  Cache cache(argv[1]); const auto content=cache.load_frontend();
  const auto &maps=content.multiplayer_maps; const auto &campaign=content.campaign_maps;
  std::ifstream input(argv[2]); unsigned map_count=0, progress=0, admitted=0, previews=0, selected=0, keys=0, sorts=0;
  for (std::string line; std::getline(input,line);) {
    if (line.empty() || line[0]=='#') continue;
    const auto f=fields(line);
    if (f[0]=="threshold") {
      check(f.size()==3 && frontend_lobby_map_unlock_threshold(campaign)==number(f[1]) &&
        frontend_lobby_new_profile_map_progress==number(f[2]),"map unlock threshold/default differs from original instructions");
    } else if (f[0]=="campaign_key") {
      check(std::any_of(campaign.begin(),campaign.end(),[&](const auto &m){return m.map_id==number(f[1]);}),
        "original valid progress key is missing from the campaign catalog"); ++keys;
    } else if (f[0]=="map") {
      check(f.size()==8,"truncated map catalog fixture");
      const auto i=std::find_if(maps.begin(),maps.end(),[&](const auto &m){return m.map_id==number(f[1]);});
      check(i!=maps.end() && i->bitmap==hexadecimal(f[2]) && i->sort_order==number(f[3]) &&
        i->flags==number(f[4]) && i->name==f[5] && i->description==f[6] && i->scenario_path==f[7],
        "map name/description/order/path differs from literal original metadata"); ++map_count;
    } else if (f[0]=="progress") {
      check(f.size()==7,"truncated original progress fixture");
      const auto actual=frontend_lobby_map_progress(campaign,{number(f[1]),number(f[2]),number(f[3]),number(f[4])});
      check(actual.best==number(f[5]) && actual.key_index==unsigned(number(f[6])),
        "profile map key admission/tie differs from original1A06F0"); ++progress;
    } else if (f[0]=="sort") {
      check(f.size()==4,"truncated original map comparator case");
      auto order=[&](int id) { const auto i=std::find_if(maps.begin(),maps.end(),
          [&](const auto &map){return map.map_id==id;});
        if(i==maps.end()) throw FormatError("source sort key is absent"); return i->sort_order; };
      const auto a=order(number(f[1])), b=order(number(f[2]));
      check((a>b?1:a<b?-1:0)==number(f[3]),"map ordering differs from original19C9A0"); ++sorts;
    } else if (f[0]=="admitted") {
      const auto actual=frontend_lobby_maps(maps,campaign,number(f[1]));
      check(actual.admission_known && actual.catalog_indices.size()==f.size()-2,
        "map list population differs from original2C9928");
      for (unsigned i=0;i<actual.catalog_indices.size();++i)
        check(maps[actual.catalog_indices[i]].map_id==number(f[i+2]),
          "map list order/admission differs from original19C9A0/2C9928"); ++admitted;
    } else if (f[0]=="preview") {
      const auto list=frontend_lobby_maps(maps,campaign,805,number(f[1]));
      const auto actual=frontend_lobby_map_preview(maps,list,list.last_selected_row);
      check(f.size()==8 && actual && actual->bitmap_index==1 && actual->description_text==0 &&
        actual->map->bitmap==hexadecimal(f[2]) && actual->map->description==f[3],
        "focused preview differs from original2C9B7C"); ++previews;
    } else if (f[0]=="selected") {
      const auto list=frontend_lobby_maps(maps,campaign,805,number(f[1]));
      check(list.last_selected_row==unsigned(number(f[2])),"last map selection differs from original2C9A73"); ++selected;
    } else throw FormatError("unknown original map case");
  }
  const auto fresh=frontend_lobby_maps(maps,campaign,frontend_lobby_new_profile_map_progress);
  const auto unknown=frontend_lobby_maps(maps,campaign,{});
  check(fresh.catalog_indices.size()==11 && unknown.catalog_indices.size()==12 && !unknown.admission_known,
    "new/unknown profile progress was silently conflated");
  const auto foundation=std::find_if(maps.begin(),maps.end(),[](const auto &m){return m.flags&1;});
  check(foundation!=maps.end() && !frontend_lobby_maps(maps,campaign,-1,foundation->map_id).last_selected_row,
    "unavailable last map manufactured a visible selection");
  check(!frontend_lobby_map_preview(maps,fresh,{}) &&
    !frontend_lobby_map_preview(maps,fresh,99),"absent focused map manufactured a preview");
  check(map_count==12 && progress==7 && admitted==7 && previews==12 && selected==12 && keys==15 && sorts==144,
    "original map fixture coverage changed");
  std::cout<<map_count<<" literal maps, "<<progress<<" progress, "<<admitted<<" admission and "
    <<previews<<" focused preview/"<<selected<<" retained selection cases match original\n";
  return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;} }
