#include "halo2/particle_sprite.hpp"
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace {
template<class T> void read(std::ifstream &in,T &value) {
  in.read(reinterpret_cast<char*>(&value),sizeof(value));
  if(!in)throw std::runtime_error("short particle sprite fixture");
}
template<class T> void write(std::ofstream &out,const T &value) {
  out.write(reinterpret_cast<const char*>(&value),sizeof(value));
  if(!out)throw std::runtime_error("short particle sprite output");
}
}
int main(int argc,char **argv) {
  try {
    using namespace halo2;
    if(argc!=2 && argc!=4 && argc!=5)throw std::runtime_error("map and optional fixture input/output required");
    if(!std::filesystem::exists(argv[1]))return 77;
    Cache cache(argv[1]);const auto tables=cache.shader_function_tables();
    const std::array<std::uint32_t,3> tags{0xe21a00a5u,0xe21f00aau,0xe22100acu};
    std::array<ParticleSpriteDefinition,3> definitions;
    for(unsigned i=0;i<3;++i)definitions[i]=cache.load_particle_sprite(tags[i]);
    if(definitions[0].bitmap!=0xe21b00a6u || definitions[1].bitmap!=0xe22000abu ||
        definitions[2].bitmap!=0xe22200adu || definitions[0].sequences[0].size()!=1 ||
        definitions[1].sequences[0].size()!=4 || definitions[2].sequences[0].size()!=3)
      throw std::runtime_error("original title particle definition changed");
    if(argc==4 || argc==5) {
      const bool compare=argc==5;
      if(compare && std::string_view(argv[2])!="--compare")throw std::runtime_error("unknown particle comparison mode");
      std::ifstream in(argv[compare?3:2],std::ios::binary),expected;
      std::ofstream out;
      if(compare)expected.open(argv[4],std::ios::binary);
      else out.open(argv[3],std::ios::binary);
      const auto field=[&]<class T>(const T &v) {
        if(!compare) {write(out,v);return;}
        std::array<char,sizeof(T)> original{};
        expected.read(original.data(),original.size());
        if(!expected || std::memcmp(original.data(),&v,sizeof(T)))
          throw std::runtime_error("particle sprite differs from independent retail fixture");
      };
      std::uint32_t count{};read(in,count);
      for(unsigned i=0;i<count;++i) {
        std::uint32_t index{};ParticleSpriteInput input;
        read(in,index);if(index>=3)throw std::runtime_error("particle fixture tag outside definitions");
        read(in,input.values);read(in,input.random1);read(in,input.random2);read(in,input.seed_color);
        read(in,input.position);read(in,input.direction);read(in,input.scale);read(in,input.width);
        read(in,input.color);read(in,input.selected_sequence);
        const auto value=particle_sprite_submission(definitions[index],input,*tables);
        field(value.position);field(value.direction);field(value.size);field(value.angle);
        field(value.argb);field(value.blend);field(value.geometry);field(value.uv_a);field(value.uv_b);
      }
      std::cout<<count<<" original particle sprite fixtures\n";
      if(compare && expected.peek()!=std::char_traits<char>::eof())
        throw std::runtime_error("extra retail particle fixture records");
    } else {
      std::cout<<"Original title particle assets loaded; executable differential required for parity\n";
    }
    return 0;
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
