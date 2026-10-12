#include "halo2/source_player_damage.hpp"
#include "halo2/source_king.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply the original player-trait consumer fixture");
  std::ifstream file(argv[1]);Json fixture;file>>fixture;unsigned calls=0;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),
      "Original damage evidence substituted game returns");
  constexpr std::uint32_t attacker=0x9abc0000u,victim=0x9abc0003u,none=0xffffffffu;
  for(const auto& [version,output]:fixture.at("versions").items()) {
    const auto& inputs=fixture.at("damage_inputs");
    for(unsigned n=0;n<inputs.size();++n) {
      const auto& input=inputs[n];Json queries=Json::array();
      const auto flags=input.value("global_flags",0u),hill=input.value("hill_flags",0u);
      SourcePlayerDamageServices services;
      services.player=[&](std::uint32_t datum) {
        check(datum==attacker || datum==victim,"Damage requested an undeclared player");
        return SourcePlayerDamageState{datum==attacker ? input.value("strength",std::uint8_t{}) : std::uint8_t{},
            std::int8_t(datum==attacker ? input.value("attacker_team",0) : input.value("victim_team",7)),
            datum==attacker ? 0 : input.value("protection",0)};
      };
      services.trait=[&](std::uint32_t datum,std::int32_t selector) {
        queries.push_back(Json::array({datum,selector}));
        return source_king_trait(std::int16_t(input.value(datum==attacker ? "attacker_inside" : "victim_inside",0)),selector,hill,flags);
      };
      services.enemies=[](std::int16_t a,std::int16_t b){return a!=b;};
      const auto from=input.value("attacker",std::string{})=="none" ? none : attacker;
      const auto kind=input.value("victim",std::string{});
      const auto to=kind=="none" ? none : kind=="self" ? attacker : victim;
      const auto scale=source_player_damage_scale(input.value("engine_present",true),flags,from,to,services);
      const auto& expected=output.at("damage")[n];
      if(std::bit_cast<std::uint32_t>(scale)!=expected.at("multiplier_bits") || queries!=expected.at("queries")) {
        std::cerr<<version<<" damage="<<n<<' '<<input.dump()<<'\n';
        throw std::runtime_error("Whole original player damage multiplier or query order differs");
      }
      ++calls;
    }
  }
  check(source_player_damage_scale(false,0,attacker,victim,{})==1.f,"Missing-engine gate called an absent owner");
  check(source_player_damage_scale(true,0,none,victim,{})==1.f,"NONE attacker gate called an absent owner");
  check(source_player_damage_scale(true,0,attacker,attacker,{})==1.f,"Self gate called an absent owner");
  std::cout<<"Original player damage: "<<calls<<" whole multipliers and ordered trait queries passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
