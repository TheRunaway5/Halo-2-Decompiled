#include "halo2/bsp_atmosphere.hpp"
#include "halo2/math.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <sstream>
#include <array>
#include <algorithm>
#include <bit>
namespace {
void planar_depth_operation_order() {
  using namespace halo2;
  BspFogSample sample;
  sample.planar.opaque_depth=2.f;
  // Original25AEA/25AF4..25BAF: MULSS, then SUBSS from XORPS +0.
  // Both signs of zero in plane.xyz produce +0; plane.w is only multiplied.
  sample.planar.plane={0.f,-0.f,4.f,-0.f};
  const auto first=bsp_fog_constants(sample,{},{1,0,0})[1];
  if(std::bit_cast<std::array<std::uint32_t,4>>(first)!=
     std::array<std::uint32_t,4>{0x00000000u,0x00000000u,0xc0000000u,0x80000000u})
    throw FormatError("source c13 multiply/subtract order or unchanged plane.w sign differs");
  sample.planar.plane={-0.f,0.f,0.f,0.f};
  const auto zeros=bsp_fog_constants(sample,{},{1,0,0})[1];
  if(std::bit_cast<std::array<std::uint32_t,4>>(zeros)!=std::array<std::uint32_t,4>{})
    throw FormatError("source c13 positive-zero subtraction changed a plane coefficient zero sign");
}
}
int main(int argc,char **argv) {
  using namespace halo2;
  planar_depth_operation_order();
  if(argc<2) {std::cout<<"Original assets not requested\n";return 0;}
  if(argc==2 || std::filesystem::is_directory(argv[2])) {
    const std::array<const char*,21> names{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar","backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
    unsigned samples=0;
    const unsigned map_count=argc==2?12:21;
    for(unsigned i=0;i<map_count;++i) {
      const auto *name=names[i];
      Cache cache(std::filesystem::path(argv[i<12?1:2])/(std::string(name)+".map"));
      const auto bank=cache.load_bsp_atmosphere(0);
      if(bank.clusters.empty() || bank.skies.empty() || bank.gradient.pixels.empty())throw FormatError("missing multiplayer atmosphere inputs");
      for(const auto &spawn:cache.spawns())if(spawn.bsp==0) {
        const auto sample=bank.sample(spawn.position+Vec3{0,0,.7f},direction(spawn.facing));
        if(sample.classification>5 || sample.classification==2)throw FormatError("unexpected multiplayer source fog type");
        const auto constants=bsp_fog_constants(sample,spawn.position,direction(spawn.facing));
        for(const auto &row:constants)for(float v:row)if(!std::isfinite(v))throw FormatError("nonfinite source fog constants");
        const auto composite=bsp_fog_composite(sample,1024);
        for(const auto &row:composite.lookup_coordinates)for(float v:row)if(!std::isfinite(v))throw FormatError("nonfinite source fog lookup");
        ++samples;
      }
    }
    std::cout<<map_count<<" multiplayer maps, "<<samples<<" spawn atmosphere queries passed\n";return 0;
  }
  std::ifstream input(argv[2]);if(!input)throw FormatError("missing differential atmosphere input");
  std::string name;float x,y,z;int cluster;
  std::map<std::string,BspAtmosphere> banks;
  std::cout<<std::setprecision(9);
  while(input>>name>>x>>y>>z>>cluster) {
    if(!banks.contains(name)) {
      auto path=std::filesystem::path(argv[1])/name;
      if(!std::filesystem::exists(path) && argc>3)path=std::filesystem::path(argv[3])/name;
      banks.emplace(name,Cache(path).load_bsp_atmosphere(argc>4?std::stoul(argv[4]):0));
    }
    const auto &bank=banks.at(name);const Vec3 eye{x,y,z};const auto f=bank.sample(eye,{1,0,0},cluster);
    std::cout<<name<<' '<<cluster<<' '<<bank.cluster_at(eye)<<' '<<f.classification;
    auto value=[&](float v){std::cout<<' '<<v;};
    auto color=[&](Vec3 c){value(c.x);value(c.y);value(c.z);};
    auto fog=[&](const AtmosphericFog &a){color(a.color);value(a.maximum_density);value(a.start);value(a.opaque);};
    fog(f.primary);fog(f.secondary);color(f.planar.color);value(f.planar.maximum_density);
    value(f.planar.opaque_distance);value(f.planar.opaque_depth);
    if(f.patchy) {color(f.patchy->color);value(0);value(0);value(0);value(f.patchy->density[0]);value(f.patchy->density[1]);value(f.patchy->distance[0]);value(f.patchy->distance[1]);}
    else for(unsigned i=0;i<10;++i)value(0);
    value(f.eye_height);value(f.eye_fraction);value(f.eye_offset);value(f.transition);value(f.planar_only);
    for(float v:f.bloom)value(v);color(f.sky_color_premultiplied);value(f.sky_opacity);
    const auto external=bsp_fog_externs(f);
    for(unsigned i=24;i<37;++i)value(external.scalars[i]);
    for(unsigned i=25;i<33;++i)for(unsigned j=0;j<3;++j)value(external.colors[i][j]);
    for(const auto &row:bsp_fog_constants(f,eye,{1,0,0}))for(float v:row)value(v);
    const auto composite=bsp_fog_composite(f,1024);
    for(auto w:composite.program.words)std::cout<<' '<<w;
    for(const auto &row:composite.lookup_coordinates)for(float v:row)value(v);
    const auto bloom=bsp_bloom_frame(f);
    value(bloom.threshold);value(bloom.strength);value(bloom.gamma);
    for(auto w:bank.potentially_visible_clusters(cluster))std::cout<<' '<<w;
    for(const auto mask:std::array<std::uint32_t,3>{0,~0u,0xaaaaaaaau}) {
      const std::vector<std::uint32_t> portals((bank.portals.size()+31)/32,mask);
      for(auto w:bank.visible_clusters(cluster,portals))std::cout<<' '<<w;
    }
    std::cout<<'\n';
  }
}
