#include "halo2/particle_sprite.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>

struct ValueFixture {
  std::uint32_t flags{},requested{},evaluated{};
  std::int32_t game_tick{};
  std::uint32_t field03{};
  std::array<float,17> initial{};
  std::array<float,3> system{};
  float location{};
  std::array<float,6> particle{};
  std::array<float,2> effects{};
  float rate{};
  std::array<float,9> view{};
  std::array<std::uint16_t,4> random{};
};
static_assert(sizeof(ValueFixture)==184);
struct SpriteFixture {
  std::uint32_t datum{};
  ValueFixture value;
  halo2::Vec3 position{},direction{};
  float scale{},width{};
  std::array<float,4> color{};
  std::uint16_t random1{},random2{};
  std::uint32_t seed_color{};
  std::int32_t sequence{};
};
static_assert(sizeof(SpriteFixture)==248);
static_assert(sizeof(halo2::ParticleValueCache)==72);
static_assert(sizeof(halo2::ParticleSpriteSubmission)==88);
halo2::ParticleValueSources sources(const ValueFixture &f) {
  halo2::ParticleValueSources s;
  if(f.flags&1) {
    s.system=halo2::ParticleSystemValueState{f.system[0],f.system[1],f.system[2],{}};
    if(f.flags&8)s.system->effect_scales=f.effects;
  }
  if(f.flags&2)s.location=halo2::ParticleLocationValueState{std::uint8_t(f.field03),f.location};
  if(f.flags&4)s.particle=halo2::ParticleValueState{f.particle[0],f.particle[1],f.particle[2],
    {f.particle[3],f.particle[4],f.particle[5]},f.random};
  s.game_tick=f.game_tick;s.game_rate=f.rate;
  s.camera_position={f.view[0],f.view[1],f.view[2]};s.camera_forward={f.view[3],f.view[4],f.view[5]};
  s.camera_up={f.view[6],f.view[7],f.view[8]};
  return s;
}
int main(int argc,char **argv) {
  try {
    const bool live=argc==5 && std::string_view(argv[1])=="--live-sprites";
    const bool compare=argc==4 && std::string_view(argv[1])=="--compare";
    if(!live && !compare && !(argc==4 && std::string_view(argv[1])=="--write"))
      throw halo2::FormatError("particle value --compare/--write inputs output, or --live-sprites map inputs original required");
    const unsigned first=live?3:2;
    if((live || compare) && (!std::filesystem::exists(argv[first]) || !std::filesystem::exists(argv[first+1])))return 77;
    std::ifstream input(argv[first],std::ios::binary),expected;
    std::ofstream output;
    if(live || compare)expected.open(argv[first+1],std::ios::binary);else output.open(argv[first+1],std::ios::binary);
    if(!input || ((live || compare)?!expected:!output))throw halo2::FormatError("particle value fixture missing");
    std::unique_ptr<halo2::Cache> cache;
    std::shared_ptr<const halo2::ShaderFunctionTables> tables;
    std::map<std::uint32_t,halo2::ParticleSpriteDefinition> definitions;
    if(live){cache=std::make_unique<halo2::Cache>(argv[2]);tables=cache->shader_function_tables();}
    std::uint32_t count{};
    if(!input.read(reinterpret_cast<char*>(&count),4) || !count || count>65536)throw halo2::FormatError("invalid particle value count");
    for(unsigned n=0;n<count;++n) {
      ValueFixture f;SpriteFixture sprite;
      if(live) {
        if(!input.read(reinterpret_cast<char*>(&sprite),sizeof(sprite)))throw halo2::FormatError("short live particle fixture");
        f=sprite.value;
      }else if(!input.read(reinterpret_cast<char*>(&f),sizeof(f)))throw halo2::FormatError("short particle value fixture");
      halo2::ParticleValueCache values{f.initial,f.evaluated};
      const auto source=sources(f);
      std::array<std::byte,160> actual{};unsigned size=sizeof(values);
      if(live) {
        if(!definitions.contains(sprite.datum))definitions[sprite.datum]=cache->load_particle_sprite(sprite.datum);
        const auto &definition=definitions.at(sprite.datum);
        if((definition.required_values&0xf80f)!=f.requested)throw halo2::FormatError("live original particle value mask differs from asset");
        halo2::ParticleSpriteInput in;
        in.position=sprite.position;in.direction=sprite.direction;in.scale=sprite.scale;in.width=sprite.width;
        in.color=sprite.color;in.random1=sprite.random1;in.random2=sprite.random2;
        in.seed_color=sprite.seed_color;in.selected_sequence=sprite.sequence;
        const auto submission=halo2::particle_sprite_submission(definition,in,values,source,*tables);
        std::memcpy(actual.data()+sizeof(values),&submission,sizeof(submission));size+=sizeof(submission);
      }else halo2::complete_particle_values(values,f.requested,source);
      std::memcpy(actual.data(),&values,sizeof(values));
      if(live || compare) {
        std::array<std::byte,160> original{};
        if(!expected.read(reinterpret_cast<char*>(original.data()),size))throw halo2::FormatError("short original particle value output");
        if(std::memcmp(actual.data(),original.data(),size)) {
          unsigned at=0;while(at<size && actual[at]==original[at])++at;
          throw halo2::FormatError("particle value/submission differs at fixture "+std::to_string(n)+", byte "+std::to_string(at));
        }
      }else output.write(reinterpret_cast<const char*>(actual.data()),size);
    }
    if(input.peek()!=std::char_traits<char>::eof() || ((live || compare) && expected.peek()!=std::char_traits<char>::eof()))
      throw halo2::FormatError("extra particle value records");
    std::cout<<count<<(live?" live original cache-to-sprite records: ":" unchanged original value-cache records: ")
      <<(live?160:72)<<" bytes each\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
