#include "halo2/particle_movement.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
static_assert(sizeof(ParticleSimulationState)==64);
namespace {
template<class T>T read(std::ifstream &stream) {
  T value;
  if(!stream.read(reinterpret_cast<char*>(&value),sizeof value))
    throw FormatError("Truncated particle movement fixture");
  return value;
}
void magic(std::ifstream &stream,const char *expected) {
  if(read<std::array<char,4>>(stream)!=std::array<char,4>{expected[0],expected[1],expected[2],expected[3]})
    throw FormatError("Incorrect particle movement fixture magic");
}
Vec3 vec(const Bytes &b,unsigned at){return {b.f32(at),b.f32(at+4),b.f32(at+8)};}
template<class F>void rejects(F &&fn) {
  try{fn();}catch(const std::exception &){return;}
  throw FormatError("Malformed particle movement input was admitted");
}
}
int main(int argc,char **argv) {try {
  if(argc!=5)throw FormatError("Supply map, PMV0 fixtures, PMVC context and PMVS stores");
  for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
  auto cache=std::make_shared<Cache>(argv[1]);
  const auto tables=cache->shader_function_tables();
  std::ifstream context(argv[3],std::ios::binary);
  magic(context,"PMVC");const auto definitions=read<std::uint32_t>(context);
  if(definitions!=6)throw FormatError("Unexpected authored movement inventory");
  const auto system_raw=read<std::array<std::byte,84>>(context);
  const auto effect_raw=read<std::array<std::byte,400>>(context);
  const auto clock_raw=read<std::array<std::byte,12>>(context);
  const auto camera_raw=read<std::array<std::byte,36>>(context);
  const float gravity=read<float>(context),one=read<float>(context);
  const Bytes system(system_raw),effect(effect_raw),clock(clock_raw),camera(camera_raw);
  ParticleValueSources values;
  values.system=ParticleSystemValueState{system.f32(4),system.f32(40),system.f32(44),{}};
  if(system.u32(0x18)!=0xffffffffu)
    values.system->effect_scales=std::array<float,2>{effect.f32(0x6c),effect.f32(0x70)};
  values.game_tick=std::int32_t(clock.u32(8));values.game_rate=clock.f32(4);
  values.camera_position=vec(camera,0);values.camera_forward=vec(camera,12);values.camera_up=vec(camera,24);
  std::vector<ParticleMovementDefinition> movement;
  for(unsigned i=0;i<definitions;++i)movement.push_back(load_particle_movement_definition(*cache,read<std::uint32_t>(context)));
  if(context.peek()!=std::char_traits<char>::eof())throw FormatError("Trailing movement context bytes");
  const std::array<std::size_t,6> controller_counts{1,2,2,2,1,1};
  for(unsigned i=0;i<definitions;++i)
    if(movement[i].controllers.size()!=controller_counts[i]||movement[i].controllers[0].type!=0)
      throw FormatError("Actual authored movement controller inventory differs");
  std::ifstream fixture(argv[2],std::ios::binary),store_fixture(argv[4],std::ios::binary);
  magic(fixture,"PMV0");magic(store_fixture,"PMVS");
  const auto count=read<std::uint32_t>(fixture);
  if(count!=1080||read<std::uint32_t>(store_fixture)!=count)
    throw FormatError("Unexpected original movement case count");
  std::size_t store_count=0;
  for(unsigned index=0;index<count;++index) {
    const auto header=read<std::array<std::uint32_t,3>>(fixture);
    const auto before=read<std::array<ParticleSimulationState,2>>(fixture);
    const auto expected=read<std::array<std::byte,128>>(fixture);
    auto particles=before;
    if(header[0]>=definitions||header[1]>=particles.size())throw FormatError("Movement fixture index escapes storage");
    const auto expected_count=read<std::uint32_t>(store_fixture);
    std::vector<std::array<std::uint32_t,4>> expected_stores;
    for(unsigned i=0;i<expected_count;++i)expected_stores.push_back(read<std::array<std::uint32_t,4>>(store_fixture));
    std::vector<std::array<std::uint32_t,4>> actual_stores;
    const ParticleMovementPool pool{
      [&](std::uint32_t datum)->ParticleSimulationState&{return particles.at(std::uint16_t(datum));},particles.size()};
    const auto full_head=(std::uint32_t(particles[header[1]].salt)<<16)|header[1];
    apply_particle_physics_controller(movement[header[0]].controllers[0],full_head,
      std::bit_cast<float>(header[2]),gravity,one,values,*tables,pool,
      [&](const ParticleMovementStore &s){actual_stores.push_back({s.source_instruction,std::uint16_t(s.particle)*64u+s.offset,4,s.value_bits});});
    if(std::memcmp(particles.data(),expected.data(),128)||actual_stores!=expected_stores)
      throw FormatError("Original full particle/store mismatch at case"+std::to_string(index));
    store_count+=expected_count;
  }
  if(fixture.peek()!=std::char_traits<char>::eof()||store_fixture.peek()!=std::char_traits<char>::eof())
    throw FormatError("Trailing original movement fixtures");
  std::array<ParticleSimulationState,2> particles{};
  const ParticleMovementPool pool{[&](std::uint32_t d)->ParticleSimulationState&{return particles.at(std::uint16_t(d));},2};
  const auto &physics=movement[0].controllers[0];
  rejects([&]{apply_particle_physics_controller(movement[1].controllers[1],0xffffffffu,0,gravity,one,values,*tables,pool);});
  rejects([&]{apply_particle_physics_controller(physics,2,0,gravity,one,values,*tables,pool);});
  auto invalid=physics;invalid.parameters.resize(2);
  rejects([&]{apply_particle_physics_controller(invalid,0xffffffffu,0,gravity,one,values,*tables,pool);});
  particles[0].next_particle=0;
  rejects([&]{apply_particle_physics_controller(physics,0,0,gravity,one,values,*tables,pool);});
  apply_particle_physics_controller(physics,0xffffffffu,0,gravity,one,values,*tables,pool);
  rejects([&]{load_particle_movement_definition(*cache,0xffffffffu);});
  std::cout<<"PASS "<<count<<" paired1.1/u5 original physics0 cases, "<<count*128
    <<" complete particle bytes, "<<store_count<<" ordered stores, actual Cache pmov/effe curves\n";
  std::cout<<"LIMIT cross-composed source-created SMG particles + canister controllers; no canister birth/lifetime, collider1, world contact or GPU claim\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
