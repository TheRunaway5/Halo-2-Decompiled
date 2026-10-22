#include "rendering/effects/source_lens_cpu_cache_tu15.hpp"
#include "halo2/render_program.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace halo2;
void need(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
int main()try{
 const auto plan=private_lens_cpu::mcc_cache_plan();unsigned total{};SourceRuntimeMemory bank;
 for(const auto&r:plan){total+=r.bytes.size();bank.mount(r.address,r.bytes,r.access,r.role);}
 const ShaderTextureState native;
 const std::array<unsigned,14> values{native.address_u,native.address_v,native.address_w,native.mag_filter,native.min_filter,native.mip_filter,native.lod_bias,native.max_mip,native.anisotropy,native.color_key_op,native.color_sign,native.alpha_kill,native.border_color,native.key_color};
 for(unsigned stage=0;stage<4;++stage){const auto row=bank.view(0x40eeb0+stage*128,128);for(unsigned n=0;n<values.size();++n){unsigned value{};for(unsigned b=0;b<4;++b)value|=std::to_integer<unsigned>(row[n*4+b])<<(b*8);need(value==values[n],"Native default sampler differs from source CPU cache");}for(unsigned n=56;n<128;++n)need(row[n]==std::byte{},"Original cold sampler cache is warm");}
 // Actual1438D/14394 store the bound bitmap width/height at stage*4.
 // Exercise those exact CPU writes before the inventory assertion, so the
 // previous sparse plan fails on the real missing destination at492314.
 for(unsigned stage=0;stage<4;++stage)for(unsigned axis=0;axis<2;++axis){
  const auto at=0x492314u+axis*16u+stage*4u;const auto cold=bank.view(at,4);
  need(!bank.is_read_only(at,4)&&std::ranges::all_of(cold,[](auto b){return b==std::byte{};}),"Lens dimension CPU destination is warm or readonly");
  const unsigned expected=640u+stage*128u+axis*16u;bank.write(at,std::as_bytes(std::span(&expected,1)));
  const auto bytes=bank.view(at,4);unsigned actual{};for(unsigned n=0;n<4;++n)actual|=std::to_integer<unsigned>(bytes[n])<<(n*8);
  need(actual==expected,"Original lens dimension producer lost its stage/axis destination");
 }
 for(unsigned axis=0;axis<2;++axis){
  const auto at=0x56e9b8u+axis*2u;const auto cold=bank.view(at,2);
  need(!bank.is_read_only(at,2)&&cold[0]==std::byte{}&&cold[1]==std::byte{},"Fallback bitmap dimension output is warm or readonly");
  const std::uint16_t expected=axis?480:640;bank.write(at,std::as_bytes(std::span(&expected,1)));
  const auto bytes=bank.view(at,2);need((std::to_integer<unsigned>(bytes[0])|(std::to_integer<unsigned>(bytes[1])<<8))==expected,"Original145C1/145C7 dimension output changed");
 }
 // Actual40A037 reads40DAC4 for the current linear-filter combination.
 need(bank.view(0x40dac4u,4).size()==4,"Original SDK filter encoding access is absent");
 constexpr std::array filter_encodings{0x10000u,0x30000u,0x50000u,0x20000u,0x40000u,0x60000u};
 for(unsigned i=0;i<filter_encodings.size();++i){const auto at=0x40dab0u+i*4u;const auto bytes=bank.view(at,4);unsigned actual{};for(unsigned n=0;n<4;++n)actual|=std::to_integer<unsigned>(bytes[n])<<(n*8);need(actual==filter_encodings[i]&&bank.is_read_only(at,4),"Original SDK filter encoding bytes/access differ");}
 need(plan.size()==19&&total==1068,"Sparse lens CPU cache shape changed");
 for(auto at:{0x40f0b0u,0x40f0c0u,0x41156cu,0x41165cu,0x411704u,0x4c4b00u,0x4c4c5cu,0x45c487u,0x45c548u,0x45f28fu,0x45f2c0u,0x492310u,0x492334u,0x56e9b7u,0x56e9bcu,0x40daafu,0x40dac8u}){bool denied=false;try{(void)bank.view(at,1);}catch(const std::exception&){denied=true;}need(denied,"Unknown SDK/device/raster span received backing");}
 const auto before=bank.checkpoint();bool denied=false;try{bank.write(0x411700,std::array{std::byte{1}});}catch(const std::exception&){denied=true;}need(denied&&bank.checkpoint()==before,"Coordinate table readonly input became a mutable domain");
 for(auto at:{0x45c488u,0x45c4b0u,0x45c4e8u,0x45f290u,0x45f2beu,0x40dab0u,0x40dac4u}){const auto pre=bank.checkpoint();bool read_only=false;try{bank.write(at,std::array{std::byte{0xaa}});}catch(const std::exception&){read_only=true;}need(read_only&&bank.checkpoint()==pre,"Authored source table became mutable");}
 need(bank.native_address(0x411578,228).value>UINT32_MAX,"Native CPU cache backing narrowed to source token");
 std::cout<<"GREEN19typedregions/1068B; original dimension outputs/filter encodings; actual native sampler inputs; unknown device/raster/coordinate gaps unbacked; readonly source tables/native64 checks\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
