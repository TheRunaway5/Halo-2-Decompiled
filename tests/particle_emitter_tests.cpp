#include "halo2/particle_emitter.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

static_assert(sizeof(halo2::ParticleEmitterState)==76);
static_assert(sizeof(halo2::ParticleEmitterFrame)==52);
static_assert(sizeof(halo2::ParticleEmissionRequest)==16);
namespace {
template<class T>T read(std::ifstream &s) {
  T v;
  if(!s.read(reinterpret_cast<char*>(&v),sizeof(v)))throw halo2::FormatError("truncated original emitter fixture");
  return v;
}
template<std::size_t N>void compare(std::ifstream &s,const std::array<std::byte,N> &actual,unsigned group,unsigned call) {
  const auto expected=read<std::array<std::byte,N>>(s);
  if(actual!=expected) {
    const auto at=std::mismatch(actual.begin(),actual.end(),expected.begin()).first-actual.begin();
    throw halo2::FormatError("particle emitter differs at group "+std::to_string(group)+", call "+std::to_string(call)+", byte "+std::to_string(at));
  }
}
}
int main(int argc,char **argv) {
  try {
    const bool controlled=argc==5 && std::string_view(argv[1])=="--controlled";
    if(argc!=4 && !controlled)throw halo2::FormatError("particle emitter comparison requires map, inputs, original");
    const unsigned first=controlled?2:1;
    if(!std::filesystem::exists(argv[first+1]) || !std::filesystem::exists(argv[first+2]))return 77;
    halo2::Cache assets(argv[first]);const auto tables=assets.shader_function_tables();
    std::ifstream input(argv[first+1],std::ios::binary),expected(argv[first+2],std::ios::binary);
    const auto groups=read<std::uint32_t>(input);unsigned total=0;
    if(!groups || groups>65536)throw halo2::FormatError("invalid original emitter fixture count");
    for(unsigned n=0;n<groups;++n) {
      const auto header=read<std::array<std::uint32_t,4>>(input);
      auto definition=assets.load_particle_system_render_definition(header[0],header[1],header[2]);
      if(header[3]>=definition.emitters.size())throw halo2::FormatError("original emitter definition missing");
      if(controlled) {
        definition.flags=std::uint16_t(read<std::uint32_t>(input));
        definition.emitters[header[3]].offset=read<halo2::Vec3>(input);
        definition.emitters[header[3]].orientation=read<halo2::Vec2>(input);
      }
      const auto flags=read<std::uint32_t>(input);const auto dt=read<float>(input);
      const auto color=read<std::uint32_t>(input);
      const bool has_frame=read<std::uint32_t>(input)!=0;const auto frame=read<halo2::ParticleEmitterFrame>(input);
      auto state=read<halo2::ParticleEmitterState>(input);
      const auto values=read<std::array<float,17>>(input);
      const auto count=read<std::uint32_t>(input);if(count>4096)throw halo2::FormatError("original emitter birth count exceeds fixture bound");
      std::vector<std::array<std::uint32_t,2>> updates;
      for(unsigned i=0;i<count;++i)updates.push_back(read<std::array<std::uint32_t,2>>(input));
      const auto expected_count=read<std::uint32_t>(expected);unsigned calls=0;
      halo2::update_particle_emitter(definition,header[3],state,std::uint16_t(flags),dt,has_frame?&frame:nullptr,values,color,*tables,
        [&](const halo2::ParticleEmissionRequest &request,halo2::ParticleEmitterState &current) {
          if(calls>=updates.size())throw halo2::FormatError("native emitter produced extra birth request");
          std::array<std::byte,92> bytes{};std::memcpy(bytes.data(),&request,16);std::memcpy(bytes.data()+16,&current,76);
          compare(expected,bytes,n,calls);
          current.first_particle=updates[calls][0];current.particle_count=std::uint16_t(updates[calls][1]);++calls;
        });
      if(calls!=count || calls!=expected_count)throw halo2::FormatError("native emitter birth request count differs");
      std::array<std::byte,76> after{};std::memcpy(after.data(),&state,76);compare(expected,after,n,calls);total+=calls;
    }
    if(input.peek()!=std::char_traits<char>::eof() || expected.peek()!=std::char_traits<char>::eof())throw halo2::FormatError("extra original emitter records");
    std::cout<<groups<<" original emitter updates, "<<total<<" ordered birth requests: all bytes exact\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
