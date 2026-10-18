#include "halo2/cache.hpp"
#include "../gpu_compute.hpp"
#include "../scalar_combiners.hpp"
#include <cmath>
#include <iostream>
#include <random>
using namespace halo2;
namespace {
struct Fixture {std::array<std::uint32_t,60> program;std::array<ShaderVector,4> samples;ShaderVector facing,glancing,fog;};
static_assert(sizeof(Fixture)==352);
void require(bool value,const char*message){if(!value)throw FormatError(message);}
}
int main(int argc,char**argv){try {
 require(argc==3,"Fog-alpha test needs opening.map and compute shader");Cache cache(argv[1]);
 const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t){return t.group=="shad" && t.name.ends_with("\\panel_mech_gunbarrel");});
 require(tag!=cache.tags().end(),"Actual projected material missing");const auto material=cache.load_material(tag->datum);std::vector<Fixture> fixtures;unsigned passes=0;
 for(const auto&pass:material.passes)if(pass.vertex_shader.find("\\lightaccum_frustum")!=std::string::npos) {
  require(((pass.program.words[8]>>16)&255u)==0x33u,"Projected pass changed its original inverse-fog-alpha input");
  auto switches=material.shader_switches;switches[0]=switches[1]=0;switches[6]=switches[7]=1;
  const auto completed=assemble_pixel_program(pass.definition,pass.fragments,pass.permutations,pass.combiners,switches);
  // Select deterministic, nonzero controlled inputs for each actual program.
  // Some authored gel variants reject particular normal/light sample pairs.
  std::mt19937 random(2003u+passes);Fixture seed{};seed.program=completed.words;bool visible=false;
  const auto unit=[&]{return float(random()&65535u)/65535.f;};
  for(unsigned trial=0;trial<4096 && !visible;++trial) {
   for(auto&sample:seed.samples)for(auto&lane:sample)lane=unit();
   for(auto*color:{&seed.facing,&seed.glancing})for(auto&lane:*color)lane=unit();
   for(unsigned i=0;i<18;++i){const auto slot=i<16?((i&1)?18+i/2:10+i/2):43+i-16;seed.program[slot]=random();}
   const auto pixel=halo2_test::scalar_combiners(seed.program,seed.samples,seed.facing,seed.glancing,{0,0,0,0});
   visible=pixel[0]>.001f || pixel[1]>.001f || pixel[2]>.001f;
  }
  require(visible,"Authored projected program has no nonzero controlled fog fixture");
  for(float fog:{-2.f,0.f,.125f,.5f,.875f,1.f,2.f}) {
   auto f=seed;f.fog={fog,0,0,0};fixtures.push_back(f);
  }++passes;
 }
 require(passes==8,"Authored projected pass scope changed");halo2_test::Compute<Fixture,1> gpu(argv[2]);const auto pixels=gpu.run(fixtures);unsigned controls=0;
 for(unsigned n=0;n<fixtures.size();++n) {
  const auto&f=fixtures[n];const auto expected=halo2_test::scalar_combiners(f.program,f.samples,f.facing,f.glancing,{0,0,0,std::clamp(f.fog[0],0.f,1.f)});
  for(unsigned c=0;c<4;++c)require(std::abs(pixels[n][c]-expected[c])<=.000001f,"GPU fog-alpha combiner differs from scalar arithmetic");
  if(n%7==1) {bool changed=false;for(unsigned c=0;c<3;++c)changed|=pixels[n][c]!=pixels[n+4][c];if(!changed)throw FormatError("Original inverse-fog-alpha control did not attenuate light: pass="+std::to_string(n/7)+" RGB="+std::to_string(pixels[n][0])+","+std::to_string(pixels[n][1])+","+std::to_string(pixels[n][2]));++controls;}
 }
 require(controls==passes,"Fog-alpha controls incomplete");std::cout<<"PASS "<<passes<<" actual projected programs, "<<fixtures.size()<<" fog-alpha GPU cases, "<<controls<<" attenuation controls; explicit fixture inputs, not an original framebuffer\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
