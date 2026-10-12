#include "halo2/hud_bitmap_geometry.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
float value(std::istream &in) {
  std::string text;
  if(!(in>>text))throw std::runtime_error("Missing original geometry word");
  return std::bit_cast<float>(std::uint32_t(std::stoul(text,nullptr,16)));
}
void exact(float actual,float expected,const std::string &name) {
  if(std::bit_cast<std::uint32_t>(actual)!=std::bit_cast<std::uint32_t>(expected))
    throw std::runtime_error("Original bitmap geometry differs in "+name);
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply original HUD bitmap fixtures");
    std::ifstream input(argv[1]);if(!input)return 77;
    unsigned cases=0,requests=0;
    for(std::string line;std::getline(input,line);) {
      if(line.empty())continue;
      std::istringstream in(line);std::string name;unsigned flags,count;
      if(!(in>>name>>flags))throw std::runtime_error("Malformed original geometry row");
      halo2::HudBitmapGeometryConstants constants{value(in),value(in)};
      halo2::HudBitmapGeometryInput state{};state.flags=std::uint16_t(flags);
      for(auto *pair:{&state.anchor,&state.horizontal_axis,&state.vertical_axis})for(auto &v:*pair)v=value(in);
      state.bitmap_width=value(in);state.bitmap_height=value(in);
      for(auto &v:state.sprite_bounds)v=value(in);
      for(auto &v:state.registration)v=value(in);
      if(!(in>>count))throw std::runtime_error("Missing original draw count");
      const auto result=halo2::source_hud_bitmap_geometry(state,constants);
      if(result.count!=count)throw std::runtime_error("Original draw count differs in "+name);
      for(unsigned n=0;n<count;++n) {
        for(const auto &point:result.quads[n].positions)for(float v:point)exact(v,value(in),name);
        for(float v:result.quads[n].sprite_bounds)exact(v,value(in),name);
      }
      ++cases;requests+=count;
    }
    if(cases!=152 || requests!=312)throw std::runtime_error("Incomplete original HUD bitmap suite");
    std::cout<<cases<<" original HUD bitmap cases and "<<requests<<" ordered draw requests matched bit-exactly\n";
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
