#include "halo2/world_material.hpp"
#include "halo2/shader_animation.hpp"
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char*m){if(!v)throw FormatError(m);}
void one(const Cache &cache,const Tag &tag){
 auto unresolved=cache.load_material(tag.datum);check(!prepare_default_object_material(cache,unresolved),"weapon camouflage silently received an unproved default");
 for(float value:{0.f,.5f,1.f}){
  auto m=cache.load_material(tag.datum);check(prepare_default_object_material(cache,m,value),"proved original weapon camouflage context was rejected");
  const auto &p=m.passes.at(m.forward_pass_index);unsigned fields=0;
  for(const auto&r:p.pixel_externs)if(r.source==37){check(r.component==3,"weapon fixture scalar component changed");
   unsigned slot=(r.destination>>6)&15,bank=(r.destination>>10)&1;const auto fragment=r.destination&7;
   if(slot!=8)slot+=m.forward_program.fragment_offsets.at(fragment);
   const auto index=slot==8?43+bank:10+bank*8+slot;const unsigned expected=value==0?0:value==.5f?128:255;
   check((m.forward_program.words.at(index)>>24)==expected,"source scalar camouflage alpha was not remapped to authored destination");++fields;
  }
  check(fields==1,"weapon camouflage source fixture changed");
  const auto before=m.forward_program.words;animate_forward_material(m,0,[](auto){return 0.f;});
  for(const auto&r:p.pixel_externs)if(r.source==37){unsigned slot=(r.destination>>6)&15,bank=(r.destination>>10)&1;if(slot!=8)slot+=m.forward_program.fragment_offsets.at(r.destination&7);const auto index=slot==8?43+bank:10+bank*8+slot;
   check((m.forward_program.words.at(index)&0xff000000)==(before.at(index)&0xff000000),"weapon animation reset the source-bound camouflage context");}
 }
}
}
int main(int argc,char**argv){try{
 if(argc!=2)throw FormatError("weapon extern check needs mainmenu map");if(!std::filesystem::exists(argv[1]))return 77;Cache cache(argv[1]);unsigned count=0;
 for(const auto&t:cache.tags())if(t.group=="shad"&&(t.name.ends_with("\\plasma_rifle_gauge")||t.name.ends_with("\\plasma_rifle_current"))){one(cache,t);++count;}
 check(count==2,"retail held-plasma weapon shader fixtures changed");std::cout<<"both held-plasma authored passes require explicit source camouflage; zero/half/one and animation retention passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
