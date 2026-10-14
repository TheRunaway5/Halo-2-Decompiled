#include "halo2/projectile_material_response.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try{
  if(argc!=2)throw FormatError("Supply original damage presentation fixture");
  std::ifstream file(argv[1],std::ios::binary);if(!file)return 77;
  const std::string data((std::istreambuf_iterator<char>(file)),{});
  const Bytes all(std::as_bytes(std::span(data.data(),data.size())));
  if(all.size()<12 || std::memcmp(data.data(),"PDP1",4) || all.u32(8)!=36 ||
      all.u32(4)<350 || all.size()!=12+all.u32(4)*44)throw FormatError("Incomplete damage presentation fixture");
  unsigned shields{},bodies{},zero{};
  for(unsigned i=0;i<all.u32(4);++i){
    const Bytes in(all.slice(12+i*44,36)),out(all.slice(12+i*44+36,8));
    const auto result=source_projectile_damage_presentation({std::int16_t(in.u16(0)),std::int16_t(in.u16(2)),
        in.f32(4),in.f32(8),in.f32(12),in.f32(16),in.f32(20),in.f32(24),in.f32(28),bool(in.u32(32))});
    if(result.material!=std::int16_t(out.u16(0)) || std::bit_cast<std::uint32_t>(result.scale)!=out.u32(4))
      throw FormatError("Original damage material/scaleB differs at case "+std::to_string(i));
    shields+=result.material==std::int16_t(in.u16(2));bodies+=result.material==std::int16_t(in.u16(0));zero+=in.f32(4)==0;
  }
  if(!shields || !bodies || !zero)throw FormatError("Missing damage presentation branch controls");
  std::cout<<"PASS "<<all.u32(4)<<" TU1.5 damage presentation cases; shield="<<shields<<" body="<<bodies<<" zero="<<zero<<'\n';
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
