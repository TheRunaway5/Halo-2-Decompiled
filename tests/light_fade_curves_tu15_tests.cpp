#include "halo2/effect_light.hpp"
#include "halo2/kernels.hpp"
#include "halo2/original_data.hpp"
#include "halo2/native_tu15_physics_branch_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "rendering/native_mcc_shader_function_data.hpp"
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
  SourceRuntimeMemory native;
  mount_native_tu15_physics_branch_data(native);
  auto cold=native_tu15_world_memory({0x70000000u,{},"transition cold-world test clock"});
  if(std::to_integer<unsigned>(cold.view(0x47a2a8u,1).front())!=1 || Bytes(cold.view(0x47a2acu,4)).u32(0)!=0xffffffffu)
    throw FormatError("Transition lookup displaced original cold placement policy");
  cold.validate_mutable_view(0x47a2a8u,1);cold.validate_mutable_view(0x47a2acu,4);
  for(unsigned n=0;n<16;++n) {
    const auto hex=fixture.at("tables")[n].get<std::string>();
    for(unsigned k=0;k<1024;++k)curves[n][k]=std::stoul(hex.substr(k*2,2),nullptr,16);
    if(n>=1 && n<=7)for(unsigned k=0;k<1024;++k) {
      const auto address=0x4782a8u+n*1024u+k;
      if(!native.is_read_only(address,1) || std::to_integer<unsigned>(native.view(address,1).front())!=curves[n][k] ||
          !cold.is_read_only(address,1) || std::to_integer<unsigned>(cold.view(address,1).front())!=curves[n][k])
        throw FormatError("Native transition bank differs from the independent original fixture");
    }
  }
  const auto checkpoint=native.checkpoint();mount_native_tu15_physics_branch_data(native);
  if(native.checkpoint()!=checkpoint)throw FormatError("Repeated native curve admission changed data/owners");
  for(const auto address:{0x4782a8u,0x4786a7u,0x47a2a8u}) {
    bool absent=false;try{(void)native.view(address,1);}catch(const std::out_of_range&){absent=true;}
    if(!absent)throw FormatError("Transition bank admitted unused or adjacent mutable data");
  }
  cold.write(0x47a2a8u,std::array{std::byte{0x80}});
  cold.write(0x47a2acu,std::array{std::byte{1},std::byte{2},std::byte{3},std::byte{4}});
  const auto live_checkpoint=cold.checkpoint();mount_native_tu15_physics_branch_data(cold);
  if(cold.checkpoint()!=live_checkpoint)throw FormatError("Transition admission changed live placement data/owners");
  unsigned cases{},authored_cases{};
  for(const auto &row:fixture.at("cases")) {
    const auto type=row[0].get<unsigned>();const float x=std::bit_cast<float>(row[1].get<std::uint32_t>());
    const auto value=type?fade_curve_fraction(x,curves[type]):std::clamp(x,0.f,1.f);
    if(std::bit_cast<std::uint32_t>(value)!=row[2].get<std::uint32_t>())
      throw FormatError("Original transition lookup changed: "+row.dump());
    ++cases;
    authored_cases+=type<8;
  }
  Cache cache(argv[2],argv[3],OriginalData::load_xbe(argv[4]));unsigned alternate{};
  const auto tables=cache.shader_function_tables();
  if(!tables)throw FormatError("Retained material transition samples are absent");
  detail::mount_native_mcc_shader_functions(cold,*tables);
  const auto material_checkpoint=cold.checkpoint();
  detail::mount_native_mcc_shader_functions(cold,*tables);
  mount_native_tu15_physics_branch_data(cold);
  if(cold.checkpoint()!=material_checkpoint)
    throw FormatError("Shared material/contrail admission changed existing data/owners");
  for(const auto &tag:cache.select_tags({.group=TagGroup::from_string("ligh"),.require_address=true})) {
    std::optional<EffectLightDefinition> definition;
    try{definition=load_effect_light(cache,tag.datum);}catch(const FormatError&){continue;}
    const auto type=Bytes(definition->bytes).u16(0xb6);
    if(type>=8)throw FormatError("Actual light has an undeclared transition selector");
    if(type && type!=5) {
      if(!definition->timed_curve || *definition->timed_curve!=curves[type])
        throw FormatError("Authored light did not retain its selected original transition table");
      ++alternate;
    }
  }
  if(cases!=18592 || authored_cases!=9296 || !alternate)throw FormatError("Original transition/actual light coverage was omitted");
  std::cout<<"PASS "<<authored_cases<<" authored-domain lookups; "<<cases
      <<" historical numeric probes; seven native rows and live cold-world placement ownership; "
      <<alternate<<" actual alternate-curve lights\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
