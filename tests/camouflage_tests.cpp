#include "halo2/source_camouflage.hpp"
#include "halo2/source_king.hpp"
#include <nlohmann/json.hpp>
#include <array>
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
  check(argc==3,"Supply the original camouflage and disruption fixtures");
  std::ifstream file(argv[1]);Json fixture;file>>fixture;unsigned calls=0;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),
      "Original camouflage evidence substituted game returns");
  constexpr std::uint32_t player=0x9abc0000u,unit=0x8aaa0000u,none=0xffffffffu;
  for(const auto& [version,output]:fixture.at("versions").items()) {
    const auto& inputs=fixture.at("camouflage_inputs");
    for(unsigned n=0;n<inputs.size();++n) {
      const auto& input=inputs[n];const auto& expected=output.at("camouflage")[n];
      Json queries=Json::array();
      std::array<std::int16_t,2> timers={std::int16_t(input.value("powerup_ticks",0)),std::int16_t(input.value("secondary_ticks",0))};
      auto flags=input.value("unit_flags",0x1020u);
      float primary=input.value("camo",0.5f),secondary=input.value("secondary_camo",0.25f),speed=input.value("fade_speed",0.25f);
      SourceCamouflageServices services;
      services.player=[&](std::uint32_t datum) {
        check(datum==player,"Camouflage requested an undeclared player");
        return SourceCamouflagePlayerView{timers,input.value("unit_none",false)?none:unit};
      };
      services.unit=[&](std::uint32_t datum) {
        check(datum==unit,"Camouflage requested an undeclared unit");
        return SourceCamouflageUnitView{flags,primary,secondary,speed,none};
      };
      services.trait=[&](std::uint32_t datum,std::int32_t selector) {
        queries.push_back(Json::array({datum,selector}));
        return source_king_trait(std::int16_t(input.value("attacker_inside",0)),selector,
            input.value("hill_flags",0u),input.value("global_flags",0u));
      };
      const auto operation=input.at("operation").get<std::string>();
      check(expected.at("returned_fpcw")==input.value("cw",0x27f) &&
          expected.at("returned_mxcsr")==input.value("mxcsr",0x1f80),
          "Original camouflage evidence did not preserve its declared FP controls");
      for(const auto& observation:expected.at("fp_observations"))
        check(observation.at("fpcw")==input.value("cw",0x27f) &&
            observation.at("mxcsr")==input.value("mxcsr",0x1f80),
            "Original FISTP did not execute with the declared FP controls");
      if(operation=="refresh")source_camouflage_refresh(input.value("engine_present",true),player,
          std::int16_t(input.value("rate",30)),std::uint16_t(input.value("cw",0x27f)),services);
      else if(operation=="timer")source_powerup_tick(player,services);
      else if(operation=="fade")source_camouflage_fade(unit,input.value("dt",1.f/30.f),services);
      else throw std::runtime_error("Unknown original camouflage operation");
      const Json actual={{"powerup_ticks",timers},{"unit_flags",flags},
          {"camo_bits",std::bit_cast<std::uint32_t>(primary)},
          {"secondary_camo_bits",std::bit_cast<std::uint32_t>(secondary)},
          {"fade_speed_bits",std::bit_cast<std::uint32_t>(speed)}};
      if(actual!=expected.at("after") || queries!=expected.at("queries")) {
        std::cerr<<version<<" camouflage="<<n<<' '<<input.dump()<<"\nactual="<<actual.dump()
            <<"\nexpected="<<expected.dump()<<'\n';
        throw std::runtime_error("Whole original camouflage state or query order differs");
      }
      ++calls;
    }
  }
  source_camouflage_refresh(false,player,30,0x27f,{});
  SourceCamouflageServices absent;absent.trait=[](auto,auto){return false;};
  source_camouflage_refresh(true,player,30,0x27f,absent);
  std::ifstream disruption_file(argv[2]);Json disruption;disruption_file>>disruption;
  check(disruption.at("instruction_patches").empty() && disruption.at("supplied_returns").empty(),
      "Original disruption evidence substituted game returns");
  unsigned disruption_calls=0;
  for(const auto& [version,output]:disruption.at("versions").items())
    for(unsigned n=0;n<disruption.at("inputs").size();++n) {
      const auto& input=disruption.at("inputs")[n];const auto& expected=output.at("results")[n];
      auto flags=input.at("flags").get<std::uint32_t>();
      auto primary=std::bit_cast<float>(input.at("primary_bits").get<std::uint32_t>());
      auto speed=std::bit_cast<float>(input.at("speed_bits").get<std::uint32_t>());
      float secondary=.25f;SourceCamouflageServices services;
      services.unit=[&](std::uint32_t datum) {
        check(datum==unit,"Disruption requested an undeclared unit");
        return SourceCamouflageUnitView{flags,primary,secondary,speed,none,input.at("intrinsic_mode").get<std::uint8_t>()};
      };
      source_camouflage_disrupt(unit,std::bit_cast<float>(input.at("amount_bits").get<std::uint32_t>()),
          std::bit_cast<float>(input.at("limit_bits").get<std::uint32_t>()),services);
      const Json actual={{"primary_bits",std::bit_cast<std::uint32_t>(primary)},
          {"speed_bits",std::bit_cast<std::uint32_t>(speed)},{"flags",flags},
          {"secondary_bits",std::bit_cast<std::uint32_t>(secondary)},
          {"intrinsic_mode",0},{"intrinsic_ticks",0}};
      if(actual!=expected) {
        std::cerr<<version<<" disruption="<<n<<' '<<input.dump()<<"\nactual="<<actual.dump()<<"\nexpected="<<expected.dump()<<'\n';
        throw std::runtime_error("Whole original camouflage disruption differs");
      }
      ++disruption_calls;
    }
  std::cout<<"Original camouflage: "<<calls<<" whole refresh/timer/fade states and query orders; "
      <<disruption_calls<<" whole disruption states passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
