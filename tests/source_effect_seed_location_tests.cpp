#include "halo2/source_effect_seed_location.hpp"
#include <fstream>
#include <iostream>
#include <cstring>
namespace {
template<class T>T read(std::ifstream &f){T v;if(!f.read(reinterpret_cast<char*>(&v),sizeof(v)))throw halo2::FormatError("Truncated seed location fixture");return v;}
}
int main(int argc,char **argv){try{
  if(argc!=3)throw halo2::FormatError("Supply seed location inputs and original outputs");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  std::ifstream in(argv[1],std::ios::binary),expected(argv[2],std::ios::binary);
  const auto count=read<std::uint32_t>(in);if(!count || count>65536)throw halo2::FormatError("Invalid seed location count");
  for(unsigned n=0;n<count;++n){
    const auto present=read<std::uint32_t>(in);const auto node=read<halo2::Transform>(in);
    const auto point=read<halo2::Vec3>(in),forward=read<halo2::Vec3>(in);
    const auto actual=halo2::source_effect_seed_location(present?&node:nullptr,point,forward);
    const auto original=read<halo2::Transform>(expected);
    if(std::memcmp(&actual,&original,sizeof(actual)))throw halo2::FormatError("Original seed location differs at case "+std::to_string(n));
  }
  if(in.peek()!=std::char_traits<char>::eof() || expected.peek()!=std::char_traits<char>::eof())throw halo2::FormatError("Extra seed location fixture bytes");
  std::cout<<"PASS "<<count<<" complete TU1.5 seed location matrices, all bytes exact\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
