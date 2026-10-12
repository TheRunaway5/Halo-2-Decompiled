#include "halo2/vertex_program.hpp"
#include "gpu_compute.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <iostream>

namespace {
using namespace halo2;
struct Fixture {
  std::array<VertexInstruction,136> instructions{};
  std::array<ShaderVector,16> attributes{};
  std::array<std::uint32_t,4> flags{};
  std::array<ShaderVector,192> constants{};
};
static_assert(sizeof(Fixture)==5520 && offsetof(Fixture,constants)==2448);
template<class T> void read(std::ifstream &file,T &v) {
  if(!file.read(reinterpret_cast<char*>(&v),sizeof(v)))throw std::runtime_error("short particle vertex input");
}
}
int main(int argc,char **argv) {
  try {
    using namespace halo2;
    if(argc!=5 && argc!=6)throw std::runtime_error("map, input, output, CPU/GPU mode and optional GPU shader required");
    const bool compare=std::string_view(argv[4])=="--compare-gpu";
    Cache cache(argv[1]);std::ifstream input(argv[2],std::ios::binary);
    std::ofstream output;
    if(!compare)output.open(argv[3],std::ios::binary);
    if(!input || (!compare && !output))throw std::runtime_error("particle vertex fixture files missing");
    std::uint32_t count{};read(input,count);std::vector<Fixture> fixtures(count);
    for(auto &fixture:fixtures) {
      std::uint32_t vertex{},classification{};read(input,vertex);read(input,classification);
      const auto program=decode_vertex_program(cache.load_vertex_shader_code(vertex,classification));
      fixture.instructions=program.instructions;fixture.flags[3]=program.count;
      fixture.flags[1]=vertex;fixture.flags[2]=classification;
      read(input,fixture.constants);read(input,fixture.attributes);
    }
    std::vector<ShaderVector> values;
    if(std::string_view(argv[4])=="--gpu" || compare) {
      halo2_test::Compute<Fixture,16> gpu(argc==6?argv[5]:HALO2_PARTICLE_VERTEX_TEST_SHADER);values=gpu.run(fixtures);
      VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(gpu.physical,&props);
      std::cout<<"Particle vertex GPU: "<<props.deviceName<<'\n';
    } else if(std::string_view(argv[4])=="--cpu") {
      if(argc==6)throw std::runtime_error("custom shader requires GPU mode");
      for(const auto &fixture:fixtures) {
        VertexProgram program{fixture.instructions,fixture.flags[3],true};
        VertexProgramState state;state.constants=fixture.constants;state.attributes=fixture.attributes;
        execute_vertex_program(program,state);values.insert(values.end(),state.outputs.begin(),state.outputs.end());
      }
    } else throw std::runtime_error("expected --cpu or --gpu");
    if(compare) {
      std::ifstream expected(argv[3],std::ios::binary);
      constexpr std::array<unsigned,9> outputs{0,3,4,7,8,9,10,11,12};
      for(unsigned i=0;i<count;++i)for(auto reg:outputs) {
        ShaderVector original;read(expected,original);
        auto native=values[i*16+reg];
        if(reg==0)for(unsigned a=0;a<2;++a)native[a]=std::trunc(native[a]*16.f)/16.f;
        if(reg==3 || reg==4 || reg==7 || reg==8)
          for(auto &v:native)v=std::clamp(v,0.f,1.f);
        for(unsigned a=0;a<4;++a)
          if(std::bit_cast<std::uint32_t>(native[a])!=std::bit_cast<std::uint32_t>(original[a]))
            throw std::runtime_error("particle GPU output differs from original capture at fixture "+
              std::to_string(i)+", o"+std::to_string(reg)+"."+std::to_string(a));
      }
      if(expected.peek()!=std::char_traits<char>::eof())throw std::runtime_error("extra captured particle GPU records");
    } else if(!output.write(reinterpret_cast<const char*>(values.data()),values.size()*sizeof(ShaderVector)))
        throw std::runtime_error("cannot save particle vertex output");
    std::cout<<count<<" original particle transform fixtures\n";
    return 0;
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
