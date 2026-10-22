#include "halo2/cache.hpp"
#include "halo2/player_screen_flash.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <set>

using namespace halo2;
namespace {
void check(bool ok,const std::string &why) {if(!ok)throw FormatError(why);}
}
int main(int argc,char **argv)try {
  if(argc!=3)return 2;
  const std::filesystem::path maps(argv[1]);
  if(!std::filesystem::is_regular_file(maps/"shared.map"))return 77;
  std::ifstream input(argv[2]);nlohmann::json inventory;input>>inventory;
  check(inventory.at("schema")=="H2_MCC_PLAYER_SCREEN_FLASHES_V1","Missing imported player-flash inventory");
  std::size_t tags{},rows{},aliases{},map_count{};
  for(const auto &map:inventory.at("maps")) {
    const auto name=map.at("map").get<std::string>();
    check(std::filesystem::path(name).filename()==name,"Flash inventory map escapes its source directory");
    std::cout<<"Checking "<<name<<std::endl;
    const Cache cache(maps/name);check(cache.is_mcc(),"Flash runtime acceptance requires MCC assets");
    std::set<std::uint32_t> seen;
    for(const auto &tag:map.at("tags")) {
      const auto datum=tag.at("datum").get<std::uint32_t>();
      const auto *actual=cache.find_tag({datum},tag_groups::damage_effect);
      check(actual && actual->name==tag.at("name").get<std::string>(),"MCC flash lost its full datum/group/path identity");
      check(seen.insert(datum).second,"Repeated damage tag in flash inventory");
      // Includes zero-sized whole-tag aliases and interned response blocks.
      const auto responses=load_player_screen_responses(cache,datum);
      check(responses.size()==tag.at("rows").size(),"MCC flash response count changed during canonicalization");
      for(std::size_t i=0;i<responses.size();++i) {
        const auto &r=responses[i];const auto &f=r.flash;
        const std::array<std::int64_t,10> fields{r.type,f.type,f.priority,std::bit_cast<std::uint32_t>(f.duration),
          f.curve,std::bit_cast<std::uint32_t>(f.intensity),std::bit_cast<std::uint32_t>(f.color[0]),
          std::bit_cast<std::uint32_t>(f.color[1]),std::bit_cast<std::uint32_t>(f.color[2]),std::bit_cast<std::uint32_t>(f.color[3])};
        check(nlohmann::json(fields)==tag.at("rows").at(i).at("fields"),"MCC import changed an authored player-flash field: "+actual->name);
        ++rows;
      }
      ++tags;aliases+=tag.at("datum")!=tag.at("backing_datum");
    }
    std::size_t available{};
    for(const auto &tag:cache.select_tags({.group=tag_groups::damage_effect,.require_address=true})) {
      check(seen.contains(tag.datum),"Runtime damage definition is missing from the import flash census");++available;
    }
    check(available==seen.size(),"Imported damage flash census is incomplete");++map_count;
  }
  check(tags==inventory.at("summary").at("damage_tags").get<std::size_t>() && map_count>0,
      "Incomplete MCC player-flash runtime checks");
  std::cout<<"PASS "<<map_count<<" MCC caches, "<<tags<<" damage tags, "<<rows<<" responses, "<<aliases<<" exact aliases\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
