#include "halo2/effect_light.hpp"
#include "halo2/kernels.hpp"
#include "halo2/original_data.hpp"
#include <fstream>
#include <algorithm>
#include <bit>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=5)throw FormatError("Supply FIXTURE MAP SHARED TU15");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  std::array<std::array<std::uint8_t,1024>,16> curves;
  for(unsigned n=0;n<16;++n) {
    const auto hex=fixture.at("tables")[n].get<std::string>();
    for(unsigned k=0;k<1024;++k)curves[n][k]=std::stoul(hex.substr(k*2,2),nullptr,16);
  }
  unsigned cases{};
  for(const auto &row:fixture.at("cases")) {
    const auto type=row[0].get<unsigned>();const float x=std::bit_cast<float>(row[1].get<std::uint32_t>());
    const auto value=type?fade_curve_fraction(x,curves[type]):std::clamp(x,0.f,1.f);
    if(std::bit_cast<std::uint32_t>(value)!=row[2].get<std::uint32_t>())
      throw FormatError("Original transition lookup changed: "+row.dump());
    ++cases;
  }
  Cache cache(argv[2],argv[3],OriginalData::load_xbe(argv[4]));unsigned alternate{};
  for(const auto &tag:cache.select_tags({.group=TagGroup::from_string("ligh"),.require_address=true})) {
    std::optional<EffectLightDefinition> definition;
    try{definition=load_effect_light(cache,tag.datum);}catch(const FormatError&){continue;}
    const auto type=Bytes(definition->bytes).u16(0xb6);
    if(type && type!=5) {
      if(!definition->timed_curve || *definition->timed_curve!=curves[type])
        throw FormatError("Authored light did not retain its selected original transition table");
      ++alternate;
    }
  }
  if(cases!=18592 || !alternate)throw FormatError("Original transition/actual light coverage was omitted");
  std::cout<<"PASS "<<cases<<" whole original fade lookups and "<<alternate<<" actual alternate-curve lights\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
