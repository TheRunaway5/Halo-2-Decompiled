#include "halo2/effect_light.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv){try{
 if(argc==3 && std::string(argv[1])=="--view-uniforms"){
  std::ifstream f(argv[2],std::ios::binary);if(!f)throw FormatError("Missing original view-uniform fixture");
  const auto word=[&](){std::uint32_t v{};f.read(reinterpret_cast<char*>(&v),4);if(!f)throw FormatError("Truncated view-uniform fixture");return v;};
  const auto scalar=[&](){return std::bit_cast<float>(word());};const auto vector=[&](){return Vec3{scalar(),scalar(),scalar()};};
  if(word()!=0x31564548)throw FormatError("Invalid view-uniform fixture");const auto count=word();std::array<std::byte,228> raw{};
  f.read(reinterpret_cast<char*>(raw.data()),raw.size());const auto definition=source_effect_light_definition(raw);
  std::array<ShaderVector,9> previous;for(auto& v:previous)for(auto& x:v)x=scalar();
  for(unsigned n=0;n<count;++n){const auto position=vector(),forward=vector(),up=vector();EffectLightState state;
   state.size=scalar();state.diffuse=vector();state.specular=vector();state.brightness=scalar();
   EffectLightViewFades fades;fades.diffuse=scalar();fades.specular=scalar();fades.secondary=scalar();fades.overall=scalar();fades.volume=scalar();
   const float selected=scalar();const auto out=source_effect_light_view_uniforms(definition,state,position,forward,up,fades,selected,previous);
   const auto exact=[&](float got){const auto expected=word();if(std::bit_cast<std::uint32_t>(got)!=expected){std::cerr<<"case "<<n<<" native="<<std::hex<<std::bit_cast<std::uint32_t>(got)<<" original="<<expected<<'\n';throw FormatError("Original view-uniform word mismatch");}};
   for(const auto& v:out.constants)for(float x:v)exact(x);for(const auto& v:{out.gel_basis_u,out.gel_basis_v})for(float x:v)exact(x);
   if(out.color_shift!=word())throw FormatError("Original view-uniform shift mismatch");
  }
  std::cout<<"PASS "<<count<<" original4A1E0 prefix/497A0 calls / "<<count*45<<" exact words\n";return 0;
 }
 if(argc==3 && std::string(argv[1])=="--sphere"){
  std::ifstream f(argv[2],std::ios::binary);if(!f)throw FormatError("Missing original sphere fixture");
  const auto word=[&](){std::uint32_t v{};f.read(reinterpret_cast<char*>(&v),4);if(!f)throw FormatError("Truncated sphere fixture");return v;};
  const auto scalar=[&](){return std::bit_cast<float>(word());};const auto vector=[&](){return Vec3{scalar(),scalar(),scalar()};};
  if(word()!=0x31534548)throw FormatError("Invalid sphere fixture");const auto count=word();
  for(unsigned n=0;n<count;++n){std::array<std::byte,228> raw{};f.read(reinterpret_cast<char*>(raw.data()),raw.size());
   const auto definition=source_effect_light_definition(raw);const auto position=vector(),forward=vector(),up=vector();const float size=scalar();
   const auto sphere=source_effect_light_sphere(definition,position,forward,up,size);
   const auto exact=[&](std::uint32_t got){const auto expected=word();if(got!=expected){std::cerr<<"case "<<n<<" native="<<std::hex<<got<<" original="<<expected<<'\n';throw FormatError("Original sphere word mismatch");}};
   for(auto w:sphere.shape)exact(w);
   for(unsigned i=0;i<2;++i){for(float v:{position.x,position.y,position.z,sphere.spatial_radius})exact(std::bit_cast<std::uint32_t>(v));}
  }
  std::cout<<"PASS "<<count<<" whole-original sphere shape/spatial calls / "<<count*39<<" exact words\n";return 0;
 }
 if(argc==4 && std::string(argv[1])=="--attenuation"){
  const Cache cache(argv[2]);const auto texture=cache.load_effect_light_attenuation_bitmap();
  std::ifstream f(argv[3],std::ios::binary|std::ios::ate);if(!f)throw FormatError("Missing independent I8 volume fixture");
  const auto length=f.tellg();f.seekg(0);std::vector<std::byte> expected{std::size_t(length)};
  f.read(reinterpret_cast<char*>(expected.data()),expected.size());
  if(!f||texture.width!=32||texture.height!=32||texture.depth!=32||texture.layers!=1||texture.mips.size()!=6||texture.pixels!=expected)
   throw FormatError("Authored effect-light I8 volume differs from independent xemu decode");
  std::size_t offset=0;unsigned n=32;
  for(const auto& mip:texture.mips){const auto size=std::size_t(n)*n*n*4;
   if(mip.width!=n||mip.height!=n||mip.depth!=n||mip.offset!=offset||mip.size!=size)throw FormatError("I8 volume mip layout differs");
   offset+=size;n=std::max(1u,n/2);
  }
  std::cout<<"PASS "<<expected.size()<<" independent I8 volume bytes / six mip levels\n";return 0;
 }
 if(argc==3 && std::string(argv[1])=="--view-fades"){
  std::ifstream f(argv[2],std::ios::binary);if(!f)throw FormatError("Missing original view-fade fixture");
  const auto word=[&](){std::uint32_t v{};f.read(reinterpret_cast<char*>(&v),4);if(!f)throw FormatError("Truncated view-fade fixture");return v;};
  const auto scalar=[&](){return std::bit_cast<float>(word());};const auto vector=[&](){return Vec3{scalar(),scalar(),scalar()};};
  if(word()!=0x31464548)throw FormatError("Invalid view-fade fixture");const auto count=word();
  for(unsigned n=0;n<count;++n){
   const auto position=vector();const float radius=scalar();const auto camera=vector();EffectLightDefinition definition;
   f.read(reinterpret_cast<char*>(definition.bytes.data()+0xb8),6);if(!f)throw FormatError("Truncated original fade classes");
   const auto out=source_effect_light_view_fades(definition,position,radius,camera);
   for(float value:{out.diffuse,out.specular,out.overall,out.secondary,out.volume}){
    const auto expected=word();if(std::bit_cast<std::uint32_t>(value)!=expected){std::cerr<<"case "<<n<<" native="<<std::hex<<std::bit_cast<std::uint32_t>(value)<<" original="<<expected<<'\n';throw FormatError("Original light view fade mismatch");}
   }
  }
  std::cout<<"PASS "<<count<<" whole-original light view-fade calls / "<<count*5<<" exact words\n";return 0;
 }
 if(argc==3 && std::string(argv[1])=="--uniforms"){
  std::ifstream f(argv[2],std::ios::binary);if(!f)throw FormatError("Missing original effect-light uniform fixture");
  const auto word=[&](){std::uint32_t v{};f.read(reinterpret_cast<char*>(&v),4);if(!f)throw FormatError("Truncated light uniform fixture");return v;};
  const auto scalar=[&](){return std::bit_cast<float>(word());};
  const auto vector=[&](){return Vec3{scalar(),scalar(),scalar()};};
  if(word()!=0x31554548)throw FormatError("Invalid light uniform fixture");const auto count=word();
  for(unsigned n=0;n<count;++n){EffectLightUniformInput in;
   in.position=vector();in.forward=vector();in.up=vector();in.left=vector();in.radius=scalar();
   in.diffuse=vector();in.specular=vector();in.brightness=scalar();in.gel_u=scalar();in.gel_v=scalar();
   for(auto&v:in.previous)for(auto&x:v)x=scalar();const auto out=source_effect_light_uniforms(in);
   const auto exact=[&](float x){const auto expected=word();if(std::bit_cast<std::uint32_t>(x)!=expected){std::cerr<<"case="<<n<<" native="<<std::hex<<std::bit_cast<std::uint32_t>(x)<<" original="<<expected<<'\n';throw FormatError("Original light uniform word mismatch");}};
   for(auto&v:out.constants)for(auto x:v)exact(x);for(auto&v:{out.gel_basis_u,out.gel_basis_v})for(auto x:v)exact(x);
   if(out.color_shift!=word())throw FormatError("Original light color shift mismatch");
  }
  std::cout<<"PASS "<<count<<" whole-original light uniform calls / "<<count*45<<" exact words\n";return 0;
 }
 if(argc!=4)throw FormatError("Supply MAP UPDATE FIXTURE");
 const auto original=OriginalData::load_xbe(argv[2]);Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),original);
 std::ifstream f(argv[3],std::ios::binary);if(!f)throw FormatError("Missing original effect-light fixture");
 const auto word=[&](){std::uint32_t v{};f.read(reinterpret_cast<char*>(&v),4);if(!f)throw FormatError("Truncated effect-light fixture");return v;};
 if(word()!=0x314c4548)throw FormatError("Invalid effect-light fixture");const auto count=word();
 for(unsigned n=0;n<count;++n){const auto tag=word(),control=word(),elapsed=word();const auto rate=word();
  const auto d=load_effect_light(cache,tag);const auto state=source_effect_light_state(d,std::bit_cast<float>(control),std::int32_t(elapsed),std::int16_t(rate),original->fade_curve);
  const std::array<float,9> got{state.size,state.control,state.diffuse.x,state.diffuse.y,state.diffuse.z,state.specular.x,state.specular.y,state.specular.z,state.brightness};
  for(unsigned i=0;i<got.size();++i){const auto expected=word();if(std::bit_cast<std::uint32_t>(got[i])!=expected){std::cerr<<"case="<<n<<" field="<<i<<" native="<<std::hex<<std::bit_cast<std::uint32_t>(got[i])<<" original="<<expected<<'\n';throw FormatError("Original effect-light word mismatch");}}
 }
 std::cout<<"PASS "<<count<<" whole-original effect-light updates / "<<count*9<<" exact scalar/color words\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
