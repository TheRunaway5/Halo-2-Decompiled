#include "halo2/water.hpp"
#include "halo2/texture_program.hpp"
#include "halo2/vertex_program.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>

namespace {
using namespace halo2;
void check(bool v,const char *text) {if(!v)throw std::runtime_error(text);}
void probe() {
  std::cout<<std::setprecision(9);
  WaterDefinition definition;
  while(std::cin>>definition.parameters[0]) {
    for(unsigned i=1;i<43;++i)std::cin>>definition.parameters[i];
    Vec3 eye,forward;float fov,aspect,anamorphic;
    std::cin>>eye.x>>eye.y>>eye.z>>forward.x>>forward.y>>forward.z>>fov>>aspect>>anamorphic;
    if(!std::cin)throw std::runtime_error("truncated water oracle fixture");
    const auto frame=water_frame(definition,eye,forward,fov,aspect,anamorphic!=0);
    for(const auto &v:frame.constants)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.bump_matrices)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.local_to_world)for(float f:v)std::cout<<f<<' ';
    for(const auto &v:frame.corners)std::cout<<v.x<<' '<<v.y<<' '<<v.z<<' ';
    std::cout<<frame.horizon_argb<<'\n';
  }
}
}
int main(int argc,char **argv) {
  try {
    if(argc>1 && std::string(argv[1])=="--probe"){probe();return 0;}
    const std::array<std::byte,10> source{std::byte{0},std::byte{127},std::byte{128},std::byte{255},
      std::byte{31},std::byte{64},std::byte{192},std::byte{254},std::byte{15},std::byte{240}};
    const auto v8u8=prepare_texture(Bytes(source),2,2,22,true,1);
    check(v8u8.format==11 && v8u8.mips.size()==2 && v8u8.pixels.size()==20,
          "V8U8 original mip chain changed");
    for(unsigned i=0;i<5;++i)check(v8u8.pixels[i*4]==source[i*2] && v8u8.pixels[i*4+2]==source[i*2] &&
      v8u8.pixels[i*4+1]==source[i*2+1] && v8u8.pixels[i*4+3]==source[i*2+1],
      "V8U8 original U,V,U,V fetch swizzle changed");
    check(prepare_texture(Bytes(source),2,2,23,true,1).pixels==v8u8.pixels,
          "retail V8U8/G8B8 format aliases decode differently");
    const auto signed_image=prepare_signed_texture(v8u8,0xe0000000u);
    check(signed_image.format==native_rgba32f && signed_image.pixels.size()==80 &&
      signed_image.mips[1].offset==64 && signed_image.mips[1].size==16,
      "signed water mip layout changed");
    for(unsigned i=0;i<20;++i) {
      const unsigned byte=std::to_integer<unsigned>(v8u8.pixels[i]);
      const float expected=i%4==3?float(byte)/255.f:
        float(byte>=128?int(byte)-256:int(byte))/128.f;
      check(Bytes(signed_image.pixels).f32(i*4)==expected,
            "signed water channels were converted after filtering or lost unsigned alpha");
    }
    const auto disc=argc>1?std::filesystem::path(argv[1]):std::filesystem::path("assets/disc/maps");
    const auto dlc=argc>2?std::filesystem::path(argv[2]):std::filesystem::path("assets/map-pack/maps");
    constexpr std::array names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron",
      "foundation","headlong","lockout","midship","waterworks","zanzibar","containment","warlock","deltatap",
      "turf","backwash","dune","elongation","gemini","triplicate"};
    unsigned definitions=0,resident=0,drawn=0,pools=0;
    for(unsigned i=0;i<names.size();++i) {
      const Cache cache((i<12?disc:dlc)/(std::string(names[i])+".map"));
      const auto water=cache.load_bsp_water(0);definitions+=unsigned(water.definitions.size());
      for(const auto &definition:water.definitions) {
        const auto frame=water_frame(definition,{3,-2,5},{.8f,.6f,-.2f},1.2f,4.f/3.f);
        check(frame.constants[0]==ShaderVector{1,-1,-1,.5f},"water c21 changed from retail state");
        check(frame.constants[2][3]==.4970000088214874f && frame.constants[4][3]==1,
              "water sun constants changed from retail state");
        check(!(frame.horizon_argb&0xff000000u),"water horizon clear has nonzero source alpha");
        for(const auto &v:frame.constants)for(float f:v)check(std::isfinite(f),"nonfinite native water constant");
        if(!definition.geometry.vertices.empty()) {
          ++resident;check(definition.geometry.vertices.size()==8064,"authored water quad expansion changed");
          check(frame.infinite,"resident infinite grid lost authored camera basis");
          check(std::all_of(definition.geometry.vertices.begin(),definition.geometry.vertices.end(),
            [](const auto &v){return v.position.z==0 && v.material==0;}),"water source position-only stream changed");
          const auto transformed=water_world_position(frame,definition.geometry.vertices.front().position);
          check(transformed.z==definition.parameters[33]-std::abs(5-definition.parameters[33])*definition.parameters[42],
                "water dynamic plane height changed");
          if(definition.shader==0xffffffffu) {
            check(definition.geometry.materials.empty(),"NONE water shader created an unauthored draw");
          } else {
            ++drawn;check(definition.geometry.materials.size()==1,"water grid lost shader");
            const auto &m=definition.geometry.materials.front();
            check(m.water_pass && m.forward_vertex==8 && texture_program_supported(m.forward_program),
                  "water grid lost original executable texture program");
            check(decode_vertex_program(m.vertex_program).count>0,"water grid vertex program lost");
          }
        }
      }
      for(const auto &tag:cache.tags())if(tag.group=="shad" && tag.name.find("water")!=std::string::npos) {
        // Admission below is checked against the original template identity;
        // the filename filter only bounds this resident-cache fixture scan.
        const auto material=cache.load_material(tag.datum);
        if(material.template_name!="shaders\\shader_templates\\water\\water_pool" &&
           material.template_name!="shaders\\shader_templates\\water\\water_pool_static")continue;
        auto emitted=material;check(prepare_water_material(cache,emitted),"authored pool pass was skipped");
        check(emitted.water_pass && emitted.forward_vertex==7 && texture_program_supported(emitted.forward_program),
              "water pool addressing not executable");++pools;
      }
      std::cout<<names[i]<<": "<<water.definitions.size()<<" original water definitions\n";
    }
    check(definitions==12 && resident==5 && drawn==3,"21-map authored water definition coverage changed");
    std::cout<<definitions<<" water records, "<<resident<<" resident grids, "<<drawn<<" submitted infinite grids, "
      <<pools<<" resident pool shader records\n";return 0;
  } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
