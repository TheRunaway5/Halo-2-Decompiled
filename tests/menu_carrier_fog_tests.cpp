#include "halo2/atmosphere.hpp"
#include "halo2/model.hpp"
#include "halo2/vertex_program.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc,char **argv) {
  try {
    if(argc!=3)throw halo2::FormatError("carrier fog test requires original cache and capture fixture");
    halo2::Cache cache(argv[1]);
    std::array<char,200> fixture{};
    std::ifstream file(argv[2],std::ios::binary);
    if(!file.read(fixture.data(),fixture.size()) || std::memcmp(fixture.data(),"H2FOGOBJ",8) ||
        file.peek()!=std::char_traits<char>::eof())throw halo2::FormatError("invalid carrier fog fixture");
    halo2::Vec3 eye{},forward{};
    std::memcpy(&eye,fixture.data()+8,12);std::memcpy(&forward,fixture.data()+20,12);
    const auto atmosphere=cache.load_menu_atmosphere();
    const auto constants=halo2::menu_fog_constants(atmosphere,eye,forward);
    float maximum_relative_error=0;
    for(unsigned i=0;i<6;++i)for(unsigned c=0;c<4;++c) {
      float expected{};std::memcpy(&expected,fixture.data()+32+i*16+c*4,4);
      maximum_relative_error=std::max(maximum_relative_error,
          std::abs(constants[i][c]-expected)/std::max(1.f,std::abs(expected)));
    }
    if(maximum_relative_error>1e-6f)throw halo2::FormatError("carrier fog vertex inputs differ from original capture");
    const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &t){
      return t.group=="scen" && t.name=="scenarios\\objects\\covenant\\military\\capital_ship\\capital_ship";
    });
    if(tag==cache.tags().end())throw halo2::FormatError("carrier tag missing");
    auto model=cache.load_model(tag->datum,true);
    halo2::prepare_menu_bsp_fog(cache,model.mesh);
    unsigned covered=0;
    for(const auto &body:model.mesh.materials)if(body.planar_fog_pass) {
      const auto &fog=*body.planar_fog_pass;
      const auto &pass=fog.passes.at(0);
      if(pass.implementation!=0 || pass.vertex_shader!="rasterizer\\vertex_shaders_xbox\\fog_planar" ||
          fog.forward_raster.depth_function!=0x202 || fog.forward_raster.depth_write!=0 || fog.forward_blend!=5)
        throw halo2::FormatError("carrier planar pass or depth/blend state differs from original");
      halo2::decode_vertex_program(fog.vertex_program);
      const auto program=halo2::menu_planar_fog_program(pass,atmosphere,eye);
      for(unsigned i=0;i<18;++i) {
        std::uint32_t expected{};std::memcpy(&expected,fixture.data()+128+i*4,4);
        const auto word=i<16?10+i/2+(i%2)*8:43+i-16;
        if(program.words[word]!=expected)throw halo2::FormatError("carrier fog pixel constant differs from original draw 1880");
      }
      ++covered;
    }
    if(covered!=2)throw halo2::FormatError("carrier fog section coverage changed");
    std::cout<<"Both carrier body fog passes; 18 captured pixel constants match byte for byte, vertex relative error "
             <<maximum_relative_error<<'\n';
    return 0;
  }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
