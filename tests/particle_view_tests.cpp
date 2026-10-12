#include "halo2/particle_view.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc,char **argv) {
  try {
    const bool compare=argc==4 && std::string_view(argv[1])=="--compare";
    if(argc!=3 && !compare)throw halo2::FormatError("input/output or --compare/input/original required");
    if(compare && (!std::filesystem::exists(argv[2]) || !std::filesystem::exists(argv[3])))return 77;
    std::ifstream input(argv[compare?2:1],std::ios::binary),expected;
    std::ofstream output;
    if(compare)expected.open(argv[3],std::ios::binary);else output.open(argv[2],std::ios::binary);
    if(!input || (compare?!expected:!output))throw halo2::FormatError("particle view fixture missing");
    unsigned count{};if(!input.read(reinterpret_cast<char*>(&count),4) || !count || count>65536)throw halo2::FormatError("invalid particle view fixture count");
    for(unsigned n=0;n<count;++n) {
      std::array<float,9> orientation;
      if(!input.read(reinterpret_cast<char*>(orientation.data()),sizeof(orientation)))throw halo2::FormatError("short particle view input");
      const auto native=halo2::particle_view_constants(orientation);
      if(compare) {
        halo2::ParticleViewConstants original;
        if(!expected.read(reinterpret_cast<char*>(original.data()),sizeof(original)))throw halo2::FormatError("short original particle view constants");
        if(std::memcmp(native.data(),original.data(),sizeof(native)))throw halo2::FormatError("particle view constants differ at fixture "+std::to_string(n));
      }else if(!output.write(reinterpret_cast<const char*>(native.data()),sizeof(native)))throw halo2::FormatError("cannot save particle view constants");
    }
    if(input.peek()!=std::char_traits<char>::eof() || (compare && expected.peek()!=std::char_traits<char>::eof()))throw halo2::FormatError("extra particle view records");
    std::cout<<count<<" original particle view uploads: 22 vectors each, byte exact\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
