#include "halo2/projectile_material_response.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try{
  if(argc!=2)throw FormatError("Supply original projectile contact inputs");
  std::ifstream file(argv[1],std::ios::binary);if(!file)return 77;
  const std::string data((std::istreambuf_iterator<char>(file)),{});
  const Bytes all(std::as_bytes(std::span(data.data(),data.size())));
  if(all.size()<20 || std::memcmp(data.data(),"PIC3",4))throw FormatError("Invalid contact input fixture");
  const auto count=all.u32(4),extent=all.u32(8);const auto one=all.f32(12),random_scale=all.f32(16);
  if(extent!=184 || all.size()!=20+count*(extent+24) || count<900)throw FormatError("Incomplete original contact input fixture");
  unsigned perturbed{},falloff{},choices{},defaults{};
  for(unsigned i=0;i<count;++i){
    const Bytes in(all.slice(20+i*(extent+24),extent)),expected(all.slice(20+i*(extent+24)+extent,24));
    const auto vector=[&](unsigned at){return Vec3{in.f32(at),in.f32(at+4),in.f32(at+8)};};
    const auto row=in.slice(36,88);auto rng=in.u32(124);
    const SourceProjectileResponseServices services{one,random_scale,[&]{return std::uint8_t(in.u32(132));},[&]()->std::uint32_t&{return rng;}};
    const auto impact=source_projectile_impact_angles(row,vector(0),vector(12),vector(24),services);
    const auto response=choose_source_projectile_response(row,{std::int32_t(in.u32(128)),impact.angle,in.f32(136)},services);
    const auto scale=source_projectile_contact_damage_scale(in.u32(140),vector(168),vector(144),in.f32(156),in.f32(160),in.f32(164));
    const auto effect_scale=source_projectile_effect_scale(row,impact.angle,scale);
    if(in.u32(180)){
      const auto selected=source_projectile_default_impact_presentation(vector(0),vector(24),scale,0.f);
      if(selected.response!=std::int16_t(expected.u16(8)) ||
          std::bit_cast<std::uint32_t>(selected.scale_a)!=expected.u32(20) || selected.scale_b!=0 ||
          selected.admitted!=(expected.f32(4)>.25f) || rng!=in.u32(124))
        throw FormatError("Original initialized default response differs at case "+std::to_string(i));
      ++defaults;
    }
    if(std::bit_cast<std::uint32_t>(impact.angle)!=expected.u32(0) ||
        std::bit_cast<std::uint32_t>(impact.approach)!=expected.u32(4) ||
        response!=std::int16_t(expected.u16(8)) || rng!=expected.u32(12) ||
        std::bit_cast<std::uint32_t>(scale)!=expected.u32(16) ||
        std::bit_cast<std::uint32_t>(effect_scale)!=expected.u32(20))
      throw FormatError("Original contact noise/response/RNG/falloff differs at case "+std::to_string(i));
    const Bytes r(row);perturbed+=r.f32(0x38)>0 || r.f32(0x3c)>0;falloff+=bool(in.u32(140)&16);choices+=rng!=in.u32(124);
  }
  if(!perturbed || !falloff || !choices || defaults!=4)throw FormatError("Missing contact branch controls");
  std::cout<<"PASS "<<count<<" original contact inputs; perturbed="<<perturbed<<" falloff="<<falloff<<" RNG choices="<<choices<<" initialized defaults="<<defaults<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
