#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;
namespace {void check(bool value,const char *reason){if(!value)throw FormatError(reason);}}
int main(int argc,char **argv)try {
  check(argc==2,"Supply unchanged English TU1.5 turret selector fixture");
  std::ifstream file(argv[1]);if(!file)return 77;
  nlohmann::json fixture;file>>fixture;
  check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Turret selector requires English TU1.5");
  const auto original=OriginalData::load_xbe(fixture.at("xbe_path").get<std::string>());
  unsigned compared{},none{},independent{};
  for(const auto &map:fixture.at("maps")) {
    Cache cache(map.at("cache_path").get<std::string>(),fixture.at("shared_maps").get<std::string>(),original);
    std::set<std::pair<unsigned,int>> selections;
    for(const auto &expected:map.at("cases"))selections.emplace(expected.at("parameter").get<unsigned>(),expected.at("mode").get<int>());
    for(const auto [parameter,mode]:selections) {
      auto variant=source_game_variant_default(0);source_game_variant_set(variant,int(parameter),mode);
      const auto content=cache.load_content(0,true,variant);
      check(content.turret_vehicle_options[parameter-81]==mode && content.turret_vehicle_options[82-parameter]==0,
          "Cold turret templates lost their independent primary/secondary input");
      for(const auto &expected:map.at("cases"))if(expected.at("parameter")==parameter && expected.at("mode")==mode) {
        const auto row=expected.at("row").get<unsigned>();
        const auto definition=expected.at("definition").get<std::uint32_t>();
        const auto actual=std::ranges::find_if(content.objects,[&](const auto &object){return object.netgame_row==int(row) && !object.model_attachment;});
        if(definition==null_datum) {check(actual==content.objects.end(),"Original NONE turret choice created a body template");++none;}
        else {
          check(actual!=content.objects.end() && actual->definition==definition && actual->variant==expected.at("variant"),
              "Cold turret definition/variant differs from unchanged15CDF0");
          const auto &placed=content.netgame_equipment.at(row);
          check(actual->position.x==placed.position.x && actual->position.y==placed.position.y && actual->position.z==placed.position.z &&
              actual->rotation.x==placed.rotation.x && actual->rotation.y==placed.rotation.y && actual->rotation.z==placed.rotation.z && actual->flags==1,
              "Turret selector changed authored placement or automatic birth policy");
        }
        ++compared;
      }
      UiVariant ui{"Turret Choices",0,{}};ui.values.set(parameter,mode);
      auto rules=slayer_rules(ui);rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
      validate_slayer_map_options(content,rules);
      auto mismatch=ui;mismatch.values.set(parameter,mode?0:1);
      bool rejected=false;try{validate_slayer_map_options(content,slayer_rules(mismatch));}catch(const FormatError &){rejected=true;}
      check(rejected,"Mismatched turret renderer templates were admitted");
      if(mode) {
        MultiplayerMatch replica(content,0,cache.spawns(),rules);const auto before=replica.state().tick;
        rejected=false;try{replica.step({});}catch(const FormatError &){rejected=true;}
        check(rejected && replica.state().tick==before,"Turret replica advanced without original source authority");
      }
      ++independent;
    }
    for(unsigned parameter:{81u,82u}) {
      auto variant=source_game_variant_default(0);source_game_variant_set(variant,int(parameter),5);
      bool rejected=false;try{cache.load_content(0,true,variant);}catch(const FormatError &){rejected=true;}
      check(rejected,"Random turret template was guessed");
    }
  }
  check(compared==252 && none>0,"Turret selector fixture is incomplete");
  std::cout<<"PASS "<<compared<<" unchanged TU1.5 turret choices, "<<none<<" NONE results, "<<independent
      <<" independent variant/template admissions and source authority gates\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
