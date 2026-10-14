#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/original_data.hpp"
#include "halo2/multiplayer.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {void check(bool yes,const char *why){if(!yes)throw FormatError(why);}}
int main(int argc,char **argv)try {
  check(argc==2,"Supply original TU1.5 light-choice fixture");
  std::ifstream file(argv[1]);if(!file)return 77;
  nlohmann::json fixture;file>>fixture;
  check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
      "Light choices require the pinned English TU1.5 reference");
  const auto original=OriginalData::load_xbe(fixture.at("xbe_path").get<std::string>());
  check(original->image==OriginalData::Image::TitleUpdate15,"Light choices have the wrong original image");
  unsigned compared{},children{};
  for(const auto &map:fixture.at("maps")) {
    Cache cache(map.at("cache_path").get<std::string>(),fixture.at("shared_maps").get<std::string>(),original);
    for(unsigned parameter:{76u,77u})for(int mode:{0,1,2,3,4,5,7}) {
      auto variant=source_game_variant_default(0);source_game_variant_set(variant,parameter,mode);
      const auto content=cache.load_content(0,true,variant);
      check(content.light_vehicle_options[parameter-76]==mode,"Cold light templates lost their variant input");
      for(const auto &expected:map.at("cases"))if(expected.at("parameter")==parameter && expected.at("mode")==mode) {
        const auto row=expected.at("row").get<unsigned>(),definition=expected.at("definition").get<std::uint32_t>();
        const auto actual=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &object) {
          return object.netgame_row==int(row) && !object.model_attachment;
        });
        if(definition==null_datum)check(actual==content.objects.end(),"Original NONE light choice created a template");
        else {
          check(actual!=content.objects.end() && actual->definition==definition && actual->variant==expected.at("variant"),
              "Cold light definition or variant differs from unchanged15CB60");
          const auto words=[](Vec3 v){return std::array{std::bit_cast<std::uint32_t>(v.x),std::bit_cast<std::uint32_t>(v.y),std::bit_cast<std::uint32_t>(v.z)};};
          check(words(actual->position)==expected.at("position_words").get<std::array<std::uint32_t,3>>() &&
              words(actual->rotation)==expected.at("rotation_words").get<std::array<std::uint32_t,3>>() && actual->flags==1,
              "Cold light choice changed original position, rotation or birth admission");
          const auto placement=std::size_t(actual-content.objects.begin());
          for(const auto &object:content.objects)if(object.model_attachment && object.model_attachment->parent==placement)++children;
        }
        ++compared;
      }
      // Primary4 is exercised as a literal selector input but the original
      // UI excludes it. Ordinary native rules reject it independently.
      if(parameter==76 && mode!=4) {
        UiVariant ui{"Slayer",0,{}};ui.values.set(76,mode);auto rules=slayer_rules(ui);
        rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
        validate_slayer_map_options(content,rules);
        auto mismatch=rules;mismatch.primary_light_vehicle=mode==0?5:0;
        bool rejected=false;try{validate_slayer_map_options(content,mismatch);}catch(const FormatError &){rejected=true;}
        check(rejected,"Mismatched primary light templates were admitted");
        if(mode) {
          MultiplayerMatch replica(content,0,cache.spawns(),rules);
          const auto before=replica.state().tick;
          rejected=false;try{replica.step({});}catch(const FormatError &){rejected=true;}
          check(rejected && replica.state().tick==before,"Light replica stepped without original source authority");
        }
      }
      if(parameter==77) {
        UiVariant ui{"Secondary Light",0,{}};ui.values.set(77,mode);
        check(source_game_variant_get(source_game_variant(ui),77)==mode,
            "Secondary Light variant serialization changed its original selector input");
        const auto rules=slayer_rules(ui);
        check(rules.session_configuration.has_value(),"Validated secondary configuration was not retained");
      }
    }
    for(unsigned parameter:{76u,77u}) {
      auto variant=source_game_variant_default(0);source_game_variant_set(variant,parameter,6);
      bool rejected=false;try{cache.load_content(0,true,variant);}catch(const FormatError &){rejected=true;}
      check(rejected,"Unresolved random light choice was admitted");
    }
  }
  check(compared>0 && children>0,"Light choices omitted original comparisons or authored children");
  std::cout<<"PASS "<<compared<<" unchanged TU1.5 light choices, "<<children
      <<" child templates, matching variant admission and source-free replica rejection\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
