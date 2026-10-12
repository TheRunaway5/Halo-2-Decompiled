#include "halo2/particle_material.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>

namespace {
std::vector<std::byte> record(const halo2::ParticleMaterial &material) {
  std::vector<std::byte> result;
  const auto append=[&](const void *p,std::size_t n) {
    const auto *bytes=static_cast<const std::byte*>(p);
    result.insert(result.end(),bytes,bytes+n);
  };
  const auto word=[&](std::uint32_t value){append(&value,4);};
  append(material.pass.program.words.data(),240);
  append(material.pass.constants.data(),512);
  word(material.pass.states.size());
  for(const auto &s:material.pass.states) {append(&s.index,1);word(s.value);}
  word(material.pass.texture_states.size());
  static_assert(sizeof(halo2::ShaderTextureState)==56);
  for(const auto &s:material.pass.texture_states)append(&s,56);
  for(unsigned stage=0;stage<4;++stage) {
    word(material.textures[stage].has_value());
    if(!material.textures[stage])continue;
    const auto &bitmap=*material.bitmaps[stage];
    word(bitmap.datum);word(bitmap.index);append(&bitmap.log_dimension,4);
    const auto &t=*material.textures[stage];
    if(t.format!=11 || t.layers!=1 || t.depth)throw std::runtime_error("particle texture was not expanded to RGBA8");
    word(t.width);word(t.height);word(t.mips.size());
    for(const auto &mip:t.mips) {
      word(mip.width);word(mip.height);word(mip.size);
      append(t.pixels.data()+mip.offset,mip.size);
    }
  }
  return result;
}
}
int main(int argc,char **argv) {
  try {
    const bool compare=argc==5 && std::string_view(argv[2])=="--compare";
    if(argc!=4 && !compare)throw std::runtime_error("map/input/output or map/--compare/input/original required");
    if(compare && (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[3]) ||
        !std::filesystem::exists(argv[4])))return 77;
    halo2::Cache cache(argv[1]);std::ifstream input(argv[compare?3:2],std::ios::binary),expected;
    std::ofstream output;
    if(compare)expected.open(argv[4],std::ios::binary);
    else output.open(argv[3],std::ios::binary);
    if(!input || (compare?!expected:!output))throw std::runtime_error("particle material fixture missing");
    std::uint32_t count{};
    if(!input.read(reinterpret_cast<char*>(&count),4))throw std::runtime_error("short particle material fixture count");
    for(unsigned n=0;n<count;++n) {
      std::array<std::uint32_t,4> args;
      if(!input.read(reinterpret_cast<char*>(args.data()),sizeof(args)))throw std::runtime_error("short particle material fixture");
      const auto material=cache.load_particle_material(args[0],args[1],args[2],args[3]);
      const auto bytes=record(material);const std::uint32_t size=bytes.size();
      if(compare) {
        std::uint32_t original_size{};
        if(!expected.read(reinterpret_cast<char*>(&original_size),4) || original_size!=size)
          throw std::runtime_error("particle material size differs from original");
        std::vector<std::byte> original(size);
        if(!expected.read(reinterpret_cast<char*>(original.data()),size))throw std::runtime_error("short original particle material");
        if(original!=bytes) {
          const auto difference=std::mismatch(original.begin(),original.end(),bytes.begin());
          throw std::runtime_error("particle material "+std::to_string(n)+" differs at byte "+std::to_string(difference.first-original.begin()));
        }
      } else {
        output.write(reinterpret_cast<const char*>(&size),4);
        output.write(reinterpret_cast<const char*>(bytes.data()),size);
        if(!output)throw std::runtime_error("cannot save particle material");
      }
    }
    if(input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra particle material input records");
    if(compare && expected.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra original particle materials");
    std::cout<<count<<" particle material records: programs, constants, states, samplers, all bound mip texels\n";
    return 0;
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
