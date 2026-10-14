#include "halo2/sampler.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char**argv)try {
  if(argc!=2)throw FormatError("Supply unchanged TU1.5 sampler fixture");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" || !fixture.at("service_replacements").empty())throw FormatError("Invalid original sampler authority");
  VkPhysicalDeviceLimits limits{};limits.maxSamplerLodBias=16;limits.maxSamplerAnisotropy=16;
  unsigned count{},mismatches{};
  for(const auto& c:fixture.at("cases")) {
    ShaderTextureState state;state.min_filter=c["min_filter"];state.mag_filter=c["mag_filter"];state.mip_filter=c["mip_filter"];state.anisotropy=c["anisotropy"];
    const auto actual=native_sampler(state,limits,true).info;
    const auto min=c["expected_min_filter"].get<unsigned>()==1?VK_FILTER_NEAREST:VK_FILTER_LINEAR;
    const auto mag=c["expected_mag_filter"].get<unsigned>()==1?VK_FILTER_NEAREST:VK_FILTER_LINEAR;
    const auto samples=actual.anisotropyEnable?unsigned(actual.maxAnisotropy):1u;
    if(actual.minFilter!=min || actual.magFilter!=mag || samples!=c["expected_anisotropy"].get<unsigned>()) {
      if(mismatches<3)std::cerr<<"case "<<count<<" actual filters "<<actual.minFilter<<','<<actual.magFilter<<" anisotropy "<<samples<<" expected "<<c<<'\n';++mismatches;
    }
    ++count;
  }
  if(mismatches)throw FormatError(std::to_string(mismatches)+" sampler submissions differ from unchanged TU1.5");
  ShaderTextureState disabled;disabled.min_filter=3;disabled.anisotropy=0;
  if(native_sampler(disabled,limits,false).info.anisotropyEnable)throw FormatError("Original point path requires no anisotropy feature");
  ShaderTextureState required;required.min_filter=3;required.anisotropy=4;unsigned rejected{};
  try{(void)native_sampler(required,limits,false);}catch(const FormatError&){++rejected;}
  limits.maxSamplerAnisotropy=4;
  try{(void)native_sampler(required,limits,true);}catch(const FormatError&){++rejected;}
  if(rejected!=2)throw FormatError("Insufficient native sampler capability was admitted");
  std::cout<<"PASS "<<count<<" original sampler submissions and actual feature-limit gates\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
