// Read-only actual particle import admission for the Lockout destruction effect.
#include "halo2/cache.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/particle_material.hpp"
#include "halo2/particle_vertex.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <iostream>
using namespace halo2;
int main(int argc,char**argv){try{
  if(argc!=4)return 77;
  Cache c(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  for(const auto [event,count]:{std::pair{1u,5u},std::pair{2u,1u},std::pair{3u,1u}})
    for(unsigned system=0;system<count;++system){
      try{
        const auto d=c.load_particle_system_render_definition(0xe70a0595,event,system);
        std::cout<<"event="<<event<<" system="<<system<<" particle="<<std::hex<<d.particle_datum<<std::dec
            <<" coordinates="<<d.coordinates<<" flags="<<d.flags<<" shapes=";
        for(const auto& e:d.emitters)std::cout<<e.birth.shape<<',';
        const auto v=c.load_particle_vertex_selection(d.particle_datum,15,0);
        std::cout<<" classification="<<v.classification;
        const auto sprite=c.load_particle_sprite(d.particle_datum);
        const auto texture=c.load_bitmap(sprite.bitmap,0);
        std::cout<<" bitmap="<<std::hex<<sprite.bitmap<<std::dec<<" bitmap_type="<<sprite.bitmap_type
            <<" width="<<texture.width<<" height="<<texture.height<<" depth="<<texture.depth
            <<" layers="<<texture.layers<<" format="<<texture.format<<" mip_count="<<texture.mips.size();
        const auto m=c.load_particle_material(d.particle_datum,15,0,0);
        std::cout<<" material=available\n";
      }catch(const std::exception& e){std::cout<<"event="<<event<<" system="<<system<<" unavailable="<<e.what()<<'\n';}
    }
  return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
