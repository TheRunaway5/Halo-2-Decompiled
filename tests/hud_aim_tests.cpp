#include "halo2/source_camera.hpp"
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
std::array<float,17> sample(const std::array<float,24> &a) {
  const Vec3 position{a[0],a[1],a[2]},forward{a[3],a[4],a[5]},up{a[6],a[7],a[8]};
  const auto aim=source_observer_aim(forward,up,a[9],{a[11],a[12]},a[10]);
  const auto anchor=source_hud_aim_anchor({position,aim.forward,aim.up,a[13],bool(a[14])},
    {a[15],a[16],a[17],a[18]},{a[19],a[20],a[21],a[22]},aim.displacement,bool(a[23]));
  const auto frustum=source_camera_frustum({a[15],a[16],a[17],a[18]},
    {a[19],a[20],a[21],a[22]});
  const auto projection=source_camera_projection(a[13],{a[15],a[16],a[17],a[18]},
    {a[19],a[20],a[21],a[22]},bool(a[14]));
  return {aim.forward.x,aim.forward.y,aim.forward.z,aim.up.x,aim.up.y,aim.up.z,
    aim.displacement,anchor.x,anchor.y,frustum[0],frustum[1],frustum[2],frustum[3],
    projection.clip_scale.x,projection.clip_scale.y,
    projection.clip_offset.x,projection.clip_offset.y};
}
}
int main(int argc,char **argv) {
  try {
    if(argc==2 && std::string(argv[1])=="--probe") {
      std::array<float,24> a{};std::cout<<std::setprecision(9);
      while(std::cin>>a[0]) {for(unsigned i=1;i<a.size();++i)std::cin>>a[i];
        for(auto v:sample(a))std::cout<<v<<' ';std::cout<<'\n';}
      return 0;
    }
    if(argc!=2)throw std::runtime_error("expected original HUD aim fixture file");
    std::ifstream input(argv[1]);if(!input)throw std::runtime_error("HUD aim fixture unavailable");
    unsigned count=0;std::string line;
    while(std::getline(input,line)) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream fields(line);std::array<float,24> a{};std::array<float,17> expected{};
      for(auto &v:a)fields>>v;for(auto &v:expected)fields>>v;
      if(!fields)throw std::runtime_error("invalid HUD aim fixture");
      const auto actual=sample(a);
      for(unsigned i=0;i<actual.size();++i)
        if((i>=7 && i<=8 && actual[i]!=expected[i]) ||
           ((i<7 || i>8) && std::abs(actual[i]-expected[i])>4e-7f*std::max(1.f,std::abs(expected[i]))))
          throw std::runtime_error("source HUD aim mismatch fixture "+std::to_string(count)+
            " component "+std::to_string(i)+" original "+std::to_string(expected[i])+
            " native "+std::to_string(actual[i]));
      ++count;
    }
    if(!count)throw std::runtime_error("no original HUD aim fixtures");
    std::cout<<"Original observer rotation and HUD/scope aim: "<<count<<" fixtures passed\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
