#include "halo2/particle_render_inputs.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

namespace {
template<class T> T read(std::istream &input) {
  T value;
  if(!input.read(reinterpret_cast<char*>(&value),sizeof(value)))throw halo2::FormatError("short original particle render input");
  return value;
}
template<std::size_t N> auto bytes(std::istream &input) { return read<std::array<std::byte,N>>(input); }
halo2::Vec3 vec(const halo2::Bytes &b,unsigned offset) { return {b.f32(offset),b.f32(offset+4),b.f32(offset+8)}; }
std::array<std::uint16_t,4> random(const halo2::Bytes &b) { return {b.u16(20),b.u16(22),b.u16(24),b.u16(26)}; }
void compare(std::istream &expected,std::span<const std::byte> actual,unsigned group,unsigned call) {
  std::vector<std::byte> original(actual.size());
  if(!expected.read(reinterpret_cast<char*>(original.data()),original.size()))throw halo2::FormatError("short original particle render result");
  if(!std::equal(actual.begin(),actual.end(),original.begin())) {
    const auto at=std::mismatch(actual.begin(),actual.end(),original.begin()).first-actual.begin();
    if(const char *directory=std::getenv("HALO2_PARTICLE_RENDER_DIAGNOSTICS")) {
      const auto stem=std::filesystem::path(directory)/("group-"+std::to_string(group)+"-call-"+std::to_string(call));
      std::ofstream(stem.string()+".native.bin",std::ios::binary).write(reinterpret_cast<const char*>(actual.data()),actual.size());
      std::ofstream(stem.string()+".original.bin",std::ios::binary).write(reinterpret_cast<const char*>(original.data()),original.size());
    }
    throw halo2::FormatError("particle render handoff differs at group "+std::to_string(group)+", call "+std::to_string(call)+", byte "+std::to_string(at));
  }
}
}
int main(int argc,char **argv) {
  try {
    const bool controlled=argc==5 && std::string_view(argv[1])=="--coordinates";
    if(argc!=4 && !controlled)throw halo2::FormatError("particle render input test requires map, inputs, original");
    const unsigned first=controlled?2:1;
    if(!std::filesystem::exists(argv[first+1]) || !std::filesystem::exists(argv[first+2]))return 77;
    halo2::Cache assets(argv[first]);const auto tables=assets.shader_function_tables();
    std::ifstream input(argv[first+1],std::ios::binary),expected(argv[first+2],std::ios::binary);
    if(!input || !expected)throw halo2::FormatError("particle render input fixtures missing");
    const auto groups=read<std::uint32_t>(input);unsigned total=0;
    if(!groups || groups>65536)throw halo2::FormatError("invalid particle render group count");
    for(unsigned group=0;group<groups;++group) {
      const auto header=read<std::array<std::uint32_t,5>>(input);
      const auto coordinates=controlled?read<std::uint32_t>(input):0u;
      const auto raw_cache=bytes<84>(input),raw_system=bytes<84>(input);
      const auto raw_location=bytes<52>(input);
      const bool has_effect=read<std::uint32_t>(input)!=0;
      const auto raw_effect=bytes<400>(input);
      const auto raw_clock=bytes<12>(input);
      const auto raw_camera=bytes<36>(input);
      const auto sequence=read<std::int32_t>(input);const auto width=read<float>(input);
      const auto count=read<std::uint32_t>(input);
      const halo2::Bytes cb(raw_cache),sb(raw_system),lb(raw_location),eb(raw_effect),clock(raw_clock),camera(raw_camera);
      auto definition=assets.load_particle_system_render_definition(header[0],header[1],header[2]);
      if(controlled) {
        if(coordinates>2)throw halo2::FormatError("controlled particle coordinates outside original enum");
        definition.coordinates=std::int16_t(coordinates);
      }
      const auto sprite=assets.load_particle_sprite(definition.particle_datum);
      halo2::ParticleRenderValueCache cache;
      std::memcpy(&cache.values,raw_cache.data(),72);
      cache.system_identity=cb.u32(72);cache.location_identity=cb.u32(76);cache.particle_identity=cb.u32(80);
      halo2::ParticleRenderState state;
      state.system_identity=header[3];state.location_identity=header[4];
      state.first_person=std::to_integer<unsigned>(raw_location[2])!=0;state.uses_camera_scale=(sb.u16(12)&0x400)!=0;
      auto &source=state.sources;
      source.system=halo2::ParticleSystemValueState{sb.f32(4),sb.f32(40),sb.f32(44),{}};
      if(has_effect)source.system->effect_scales=std::array<float,2>{eb.f32(108),eb.f32(112)};
      source.location=halo2::ParticleLocationValueState{std::to_integer<std::uint8_t>(raw_location[3]),lb.f32(28)};
      source.game_tick=std::int32_t(clock.u32(8));source.game_rate=clock.f32(4);
      source.camera_position=vec(camera,0);source.camera_forward=vec(camera,12);source.camera_up=vec(camera,24);
      if(count>256)throw halo2::FormatError("invalid original emitter count");
      for(unsigned n=0;n<count;++n) {
        const auto raw=bytes<76>(input);const halo2::Bytes b(raw);
        auto &emitter=state.emitters.emplace_back();emitter.rotation={vec(b,16),vec(b,28),vec(b,40)};emitter.position=vec(b,52);
        const auto particles=read<std::uint32_t>(input);
        if(particles>1024)throw halo2::FormatError("invalid original particle count");
        for(unsigned i=0;i<particles;++i) {
          auto &p=emitter.particles.emplace_back();p.identity=read<std::uint32_t>(input);
          const auto raw_particle=bytes<64>(input);const halo2::Bytes b(raw_particle);
          p.values={b.f32(8),b.f32(16),b.f32(56),vec(b,28),random(b)};
          p.velocity=vec(b,40);p.seed_color=b.u32(60);
        }
      }
      const auto expected_calls=read<std::uint32_t>(expected);unsigned calls=0;
      halo2::for_each_particle_render_input(definition,state,cache,*tables,[&](std::uint64_t identity,
          const halo2::ParticleSpriteInput &completed,halo2::ParticleValueCache &values,const halo2::ParticleValueSources &sources) {
        std::array<std::byte,208> actual{};const auto key=std::uint32_t(identity);
        std::memcpy(actual.data(),&key,4);std::memcpy(actual.data()+4,&values,72);
        std::memcpy(actual.data()+76,&completed.position,12);std::memcpy(actual.data()+88,&completed.direction,12);
        std::memcpy(actual.data()+100,&completed.scale,4);std::memcpy(actual.data()+104,completed.color.data(),16);
        auto in=completed;in.width=width;in.selected_sequence=sequence;
        const auto submission=halo2::particle_sprite_submission(sprite,in,values,sources,*tables);
        std::memcpy(actual.data()+120,&submission,88);compare(expected,actual,group,calls++);
      });
      if(calls!=expected_calls)throw halo2::FormatError("original particle handoff count differs");
      std::array<std::byte,84> after{};std::memcpy(after.data(),&cache.values,72);
      const std::array<std::uint32_t,3> owners{std::uint32_t(cache.system_identity),std::uint32_t(cache.location_identity),std::uint32_t(cache.particle_identity)};
      std::memcpy(after.data()+72,owners.data(),12);compare(expected,after,group,calls);total+=calls;
    }
    if(input.peek()!=std::char_traits<char>::eof() || expected.peek()!=std::char_traits<char>::eof())throw halo2::FormatError("extra original particle render records");
    std::cout<<groups<<" original render groups, "<<total<<" ordered sprite handoffs: all bytes exact\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
