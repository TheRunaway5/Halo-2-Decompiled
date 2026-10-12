#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void check(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
template<std::size_t N> std::array<std::byte,N> raw(const std::string &s) {
  check(s.size()==N*2,"Incomplete original constant");
  std::array<std::byte,N> out;
  for(std::size_t i=0;i<N;++i)out[i]=std::byte(std::stoul(s.substr(i*2,2),nullptr,16));
  return out;
}
}
int main(int argc,char **argv) {
  try {
    check(argc==5,"Supply literal operands and base/1.1/u5 executable");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    std::ifstream file(argv[1]);std::string kind,one,minus,z,xy,zero;
    check(bool(file>>kind>>one>>minus>>z>>xy>>zero) && kind=="C","Incomplete source operands");
    const std::array expected{halo2::Bytes(raw<4>(one)).u32(0),halo2::Bytes(raw<4>(minus)).u32(0),
                              halo2::Bytes(raw<4>(z)).u32(0),halo2::Bytes(raw<4>(xy)).u32(0)};
    for(int image=3;image<=4;++image) {
      const auto imported=halo2::OriginalData::load_xbe(argv[image]);
      check(bool(imported->sound_object_request_constants),"Verified sound object operands unavailable");
      const auto &c=*imported->sound_object_request_constants;
      check(std::array{c.one_bits,c.minus_one_bits,c.z_scale_bits,c.xy_scale_bits}==expected &&
            c.zero_vector==raw<12>(zero),"Imported sound object words differ from independent original bytes");
    }
    check(!halo2::OriginalData::load_xbe(argv[2])->sound_object_request_constants,"Unverified base object constants invented");
    std::cout<<"supportedImages=2 scalarWords=8 dereferencedVectorBytes=24 unavailableBase=passed liveWorldInputs=absent\n";
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
