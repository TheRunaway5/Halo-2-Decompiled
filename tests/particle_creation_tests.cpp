#include "halo2/particle_creation.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

static_assert(sizeof(halo2::ParticleSimulationState)==64);
namespace {
template<class T>T read(std::ifstream &s) {
  T v;if(!s.read(reinterpret_cast<char*>(&v),sizeof(v)))throw halo2::FormatError("truncated original particle creation fixture");return v;
}
halo2::Vec3 vec(const halo2::Bytes &b,unsigned at){return {b.f32(at),b.f32(at+4),b.f32(at+8)};}
}
int main(int argc,char **argv) {
  try {
    const bool controlled=argc==5 && std::string_view(argv[1])=="--controlled";
    if(argc!=4 && !controlled)throw halo2::FormatError("particle creation comparison requires map, inputs, original");
    const unsigned first=controlled?2:1;
    if(!std::filesystem::exists(argv[first+1]) || !std::filesystem::exists(argv[first+2]))return 77;
    halo2::Cache assets(argv[first]);const auto tables=assets.shader_function_tables();const auto data=assets.original_data();
    std::ifstream input(argv[first+1],std::ios::binary),original(argv[first+2],std::ios::binary);
    const auto count=read<std::uint32_t>(input);if(!count || count>65536)throw halo2::FormatError("invalid original particle creation count");
    for(unsigned n=0;n<count;++n) {
      const auto header=read<std::array<std::uint32_t,6>>(input);
      auto definition=assets.load_particle_system_render_definition(header[0],header[1],header[2]);
      if(controlled){definition.coordinates=std::int16_t(read<std::uint32_t>(input));definition.flags=std::uint16_t(read<std::uint32_t>(input));}
      auto particle=read<halo2::ParticleSimulationState>(input);auto emitter=read<halo2::ParticleEmitterState>(input);
      const auto raw_cache=read<std::array<std::byte,84>>(input),raw_system=read<std::array<std::byte,84>>(input);
      const auto raw_location=read<std::array<std::byte,52>>(input);const bool has_effect=read<std::uint32_t>(input)!=0;
      const auto raw_effect=read<std::array<std::byte,400>>(input);
      const auto raw_clock=read<std::array<std::byte,12>>(input);
      const auto raw_camera=read<std::array<std::byte,36>>(input);
      const auto request=read<halo2::ParticleEmissionRequest>(input);auto seed=read<std::uint32_t>(input);
      const halo2::Bytes cb(raw_cache),sb(raw_system),lb(raw_location),eb(raw_effect),clock(raw_clock),camera(raw_camera);
      halo2::ParticleRenderValueCache cache;std::memcpy(&cache.values,raw_cache.data(),72);
      cache.system_identity=cb.u32(72);cache.location_identity=cb.u32(76);cache.particle_identity=cb.u32(80);
      halo2::ParticleValueSources owners;
      if(cb.u32(72))owners.system=halo2::ParticleSystemValueState{sb.f32(4),sb.f32(40),sb.f32(44),{}};
      if(has_effect && owners.system)owners.system->effect_scales=std::array<float,2>{eb.f32(108),eb.f32(112)};
      if(cb.u32(76))owners.location=halo2::ParticleLocationValueState{std::to_integer<std::uint8_t>(raw_location[3]),lb.f32(28)};
      owners.game_tick=std::int32_t(clock.u32(8));owners.game_rate=clock.f32(4);
      owners.camera_position=vec(camera,0);owners.camera_forward=vec(camera,12);owners.camera_up=vec(camera,24);
      const halo2::ParticleCreationInput context{header[4],header[5],sb.f32(4),request,has_effect?vec(eb,48):halo2::Vec3{}};
      halo2::create_emitter_particle(definition,header[3],emitter,particle,context,cache,owners,seed,*data,*tables);
      std::array<std::byte,228> actual{};std::memcpy(actual.data(),&particle,64);std::memcpy(actual.data()+64,&cache.values,72);
      const std::array<std::uint32_t,3> keys{std::uint32_t(cache.system_identity),std::uint32_t(cache.location_identity),std::uint32_t(cache.particle_identity)};
      std::memcpy(actual.data()+136,keys.data(),12);std::memcpy(actual.data()+148,&emitter,76);std::memcpy(actual.data()+224,&seed,4);
      const auto expected=read<std::array<std::byte,228>>(original);
      if(actual!=expected) {
        const auto at=std::mismatch(actual.begin(),actual.end(),expected.begin()).first-actual.begin();
        throw halo2::FormatError("particle creation differs at record "+std::to_string(n)+", byte "+std::to_string(at));
      }
    }
    if(input.peek()!=std::char_traits<char>::eof() || original.peek()!=std::char_traits<char>::eof())throw halo2::FormatError("extra original particle creation records");
    std::cout<<count<<" original particle creations: all particle/cache/emitter/RNG bytes exact\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
