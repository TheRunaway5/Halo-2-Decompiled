#include "halo2/particle_sprite.hpp"
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace {
template<class T> void read(std::ifstream &stream,T &value) {
  if(!stream.read(reinterpret_cast<char*>(&value),sizeof(value)))
    throw std::runtime_error("short original particle submission fixture");
}
}
int main(int argc,char **argv) {
  try {
    const bool compare=argc==4 && std::string_view(argv[1])=="--compare";
    if(argc!=3 && !compare)throw std::runtime_error("input/output or --compare input/retail required");
    if(compare && (!std::filesystem::exists(argv[2]) || !std::filesystem::exists(argv[3])))return 77;
    std::ifstream input(argv[compare?2:1],std::ios::binary),expected;
    std::ofstream output;
    if(compare)expected.open(argv[3],std::ios::binary);
    else output.open(argv[2],std::ios::binary);
    if(!input || (compare?!expected:!output))throw std::runtime_error("particle submission files missing");
    static_assert(sizeof(halo2::ParticleSpriteSubmission)==88);
    std::uint32_t count{};read(input,count);
    for(unsigned i=0;i<count;++i) {
      halo2::ParticleSpriteSubmission submission;
      std::array<std::uint32_t,8> bindings;
      read(input,submission);read(input,bindings);
      const auto packet=halo2::particle_sprite_pushbuffer(submission,bindings);
      if(compare) {
        std::array<std::uint32_t,46> retail{};read(expected,retail);
        if(packet!=retail)throw std::runtime_error("particle submission differs from original executable");
        const auto attributes=halo2::particle_sprite_vertex_attributes(submission);
        // Actual unchanged 23ED30 packet: method SET_VERTEX_DATA4UB then the
        // packed word. NV2A expands bytes from least to most significant.
        for(unsigned component=0;component<4;++component)
          if(attributes[8][component]!=float((retail[17]>>(component*8))&255)/255.f)
            throw std::runtime_error("particle graphics input differs from original 4UB upload at fixture "+std::to_string(i));
      } else if(!output.write(reinterpret_cast<const char*>(packet.data()),sizeof(packet)))
        throw std::runtime_error("cannot save particle submission output");
    }
    if(input.peek()!=std::char_traits<char>::eof() ||
        (compare && expected.peek()!=std::char_traits<char>::eof()))
      throw std::runtime_error("extra particle submission fixture records");
    std::cout<<count<<" complete original particle submissions\n";
    return 0;
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
