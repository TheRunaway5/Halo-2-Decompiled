#include "halo2/native_tu15_sound_ep_stereo_numeric.hpp"
#include <iostream>
#include <iomanip>
#include <string_view>
using namespace halo2::tu15_ep_stereo_numeric;
int main(int argc,char**argv)try{
 const bool packed=argc==2&&std::string_view(argv[1])=="--pack";
 if(argc!=1&&!packed)throw std::runtime_error("Unknown numeric probe option");
 for(;;){std::uint32_t sr,enabled,scale;if(!(std::cin>>sr))break;if(!(std::cin>>enabled>>scale))throw std::runtime_error("Short header");
  Controls c{Word24(sr),{}};c.address_modes.fill(AddressingMode::linear);Inputs input{};
  for(auto&channel:input)for(auto&sample:channel){std::uint32_t v;if(!(std::cin>>v))throw std::runtime_error("Short sample input");sample=Word24(v);}
  const auto r=process(c,{Word24(enabled),Word24(scale)},input);bool first=true;
  const auto emit=[&](Word24 v){std::cout<<(first?"":" ")<<v.bits;first=false;};
  for(const auto&channel:r.output)for(auto sample:channel)emit(sample);
  for(auto v:r.peak)emit(v);
  for(auto v:r.energy_root)emit(v);
  for(auto v:r.coefficients)emit(v);
  if(packed){std::cout<<' ';for(auto b:pack_interleaved_s16(r))std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<std::to_integer<unsigned>(b);std::cout<<std::dec;}
  std::cout<<'\n';
 }return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
