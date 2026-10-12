#include "halo2/particle_birth.hpp"
#include "halo2/particle_render_inputs.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
template<class T>T read(std::ifstream &stream) {
  T out;
  if(!stream.read(reinterpret_cast<char*>(&out),sizeof(out)))
    throw halo2::FormatError("truncated original particle birth fixture");
  return out;
}
}
int main(int argc,char **argv) {
  try {
    const bool controlled=argc==5 && std::string_view(argv[1])=="--controlled-shapes";
    if(argc!=4 && !controlled)throw halo2::FormatError("particle birth comparison requires map, inputs, original");
    const unsigned first=controlled?2:1;
    if(!std::filesystem::exists(argv[first+1]) || !std::filesystem::exists(argv[first+2]))return 77;
    halo2::Cache assets(argv[first]);const auto functions=assets.shader_function_tables();
    const auto data=assets.original_data();
    std::ifstream inputs(argv[first+1],std::ios::binary),original(argv[first+2],std::ios::binary);
    const auto count=read<std::uint32_t>(inputs);
    if(!count || count>65536)throw halo2::FormatError("invalid original particle birth count");
    for(unsigned n=0;n<count;++n) {
      const auto header=read<std::array<std::uint32_t,4>>(inputs);
      auto definition=assets.load_particle_system_render_definition(header[0],header[1],header[2]);
      if(header[3]>=definition.emitters.size())throw halo2::FormatError("original birth emitter missing");
      if(controlled)definition.emitters[header[3]].birth.shape=read<std::uint32_t>(inputs);
      const auto values=read<std::array<float,17>>(inputs);auto seed=read<std::uint32_t>(inputs);
      auto particle=read<std::array<std::byte,64>>(inputs);
      const auto state=halo2::initialize_particle_birth(definition.emitters[header[3]].birth,values,seed,*data,*functions);
      std::memcpy(particle.data()+12,&state.inverse_lifetime,4);
      std::memcpy(particle.data()+28,&state.position,12);
      std::memcpy(particle.data()+40,&state.velocity,12);
      std::memcpy(particle.data()+52,&state.rotation_rate,4);
      std::array<std::byte,68> actual{};
      std::memcpy(actual.data(),particle.data(),64);std::memcpy(actual.data()+64,&seed,4);
      const auto expected=read<std::array<std::byte,68>>(original);
      if(actual!=expected) {
        const auto at=std::mismatch(actual.begin(),actual.end(),expected.begin()).first-actual.begin();
        if(const char *directory=std::getenv("HALO2_PARTICLE_BIRTH_DIAGNOSTICS")) {
          const auto stem=std::filesystem::path(directory)/("birth-"+std::to_string(n));
          std::filesystem::create_directories(stem.parent_path());
          std::ofstream(stem.string()+".native.bin",std::ios::binary).write(reinterpret_cast<const char*>(actual.data()),actual.size());
          std::ofstream(stem.string()+".original.bin",std::ios::binary).write(reinterpret_cast<const char*>(expected.data()),expected.size());
        }
        throw halo2::FormatError("particle birth differs at record "+std::to_string(n)+", byte "+std::to_string(at));
      }
    }
    if(inputs.peek()!=std::char_traits<char>::eof() || original.peek()!=std::char_traits<char>::eof())
      throw halo2::FormatError("extra original particle birth records");
    std::cout<<count<<" original particle initializers: all particle and RNG bytes exact\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
