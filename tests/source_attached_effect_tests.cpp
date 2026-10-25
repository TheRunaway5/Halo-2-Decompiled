#include "halo2/source_attached_effect.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cstring>
#include <iostream>
#include <limits>
using namespace halo2;
namespace {
void check(bool v,const char *s){if(!v)throw FormatError(s);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const FormatError&){failed=true;}check(failed,"Malformed attached effect input was accepted");}
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t v,unsigned n=4){auto b=m.mutable_view(p,n);for(unsigned i=0;i<n;++i)b[i]=std::byte(v>>(i*8));}
}
int main()try {
  constexpr std::uint32_t objects=0x10000,headers=0x11000,object_bits=0x18000,body=0x19000,
      effects=0x1a000,effect_bits=0x1b000,locations=0x1c000,location_bits=0x1d000,
      tags=0x24000,definition=0x25000,names=0x25100,data=0x30000,location_data=0x50000,
      object=0xe1000000,effect=0xe6650000,tag=0xe5500001;
  SourceRuntimeMemory m;m.mount(objects,std::vector<std::byte>(0x50000),SourceRuntimeMemory::Access::ReadWrite,"Declared attachment banks");
  m.mount(0x4f0000,std::vector<std::byte>(0x8000),SourceRuntimeMemory::Access::ReadWrite,"Declared TU1.5 globals");
  m.mount(0x4ecbd0,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"Declared object pool binding");
  for(auto p:{objects,effects,locations})put(m,p+0x29,1,1);
  put(m,objects+0x20,2048);put(m,objects+0x24,12);put(m,objects+0x38,1);
  put(m,objects+0x44,headers);put(m,objects+0x48,object_bits);put(m,object_bits,1);
  put(m,headers,object>>16,2);put(m,headers+6,0x300,2);put(m,headers+8,body);
  put(m,headers+3,6,1);put(m,body+0x14,0xffffffffu);
  put(m,body+0x11c,24,2);put(m,body+0x11e,0x140,2);
  put(m,body+0x140,0,1);put(m,body+0x144,0xeeee0000);
  put(m,body+0x148,2,1);put(m,body+0x14c,effect);
  put(m,body+0x150,2,1);put(m,body+0x154,0xffffffffu);
  put(m,effects+0x20,256);put(m,effects+0x24,400);put(m,effects+0x38,1);
  put(m,effects+0x44,data);put(m,effects+0x48,effect_bits);put(m,effect_bits,1);
  put(m,data,effect>>16,2);put(m,data+2,0x83,2);put(m,data+4,tag);put(m,data+0x48,object);
  put(m,0x4ecbd0,objects);put(m,0x4f720c,effects);put(m,data+0x58,0xffffffffu);
  put(m,data+0x6c,0x80000000u);put(m,data+0x70,0x3f800000);
  put(m,data+0x104,0x01234567);put(m,data+0x108,0x89abcdef);
  for(unsigned i=0;i<32;++i)put(m,data+0x7c+i*4,0xffffffffu);
  put(m,data+0x7c,0xe9000000);put(m,data+0x80,0xe9010001);
  put(m,0x4f0414,tags);put(m,tags+16,0x65666665);put(m,tags+20,tag);put(m,tags+24,definition);
  put(m,definition+0xc,2);put(m,definition+0x10,names);put(m,names,0x070000c0);put(m,names+4,0x020000ca);
  put(m,0x4f7208,locations);put(m,locations+0x20,512);put(m,locations+0x24,60);put(m,locations+0x38,2);
  put(m,locations+0x44,location_data);put(m,locations+0x48,location_bits);put(m,location_bits,3);
  Transform local;local.position={-0.f,.25f,-3};
  for(unsigned i=0;i<2;++i){const auto p=location_data+i*60;put(m,p,0xe900+i,2);put(m,p+2,i?0xffffu:0u,2);put(m,p+4,0xffffffffu);m.write(p+8,std::as_bytes(std::span(&local,1)));}
  const auto native_objects=m.native_address(objects,0x4c),native_effects=m.native_address(effects,0x4c);
  const std::vector<std::byte> saved(m.view(objects,0x50000).begin(),m.view(objects,0x50000).end());
  auto read=[&]{return source_attached_effects_tu15(m,native_objects,native_effects,object);};
  const auto rows=read();check(rows.size()==2 && rows[0].slot==1 && rows[1].slot==2,"Readback changed original slot ordinals or non-effect rows");
  const auto &v=rows[0];check(v.instance==effect && v.definition==tag && v.flags==0x83 &&
      std::bit_cast<std::uint32_t>(v.scale_a)==0x80000000u && v.scale_b==1 &&
      v.colors==std::array<std::uint32_t,2>{0x01234567,0x89abcdef},"Readback changed completed effect identity/controls/colors");
  check(v.locations.size()==2 && v.locations[0].node==0 && v.locations[1].node==-1 &&
      v.locations[0].name==0x070000c0 && !std::memcmp(&v.locations[0].local,&local,sizeof(local)),"Readback changed actual local location matrices or names");
  check(v.render&&v.render->identity==effect&&v.render->attached_owner==object&&
      v.render->root_owner==object&&v.render->root_type==6&&v.render_clock.flags==0x83,
      "Readback lost the actual attached render root or current event clock");
  check(rows[1].instance==0xffffffffu && rows[1].locations.empty(),"Absent source allocation became a replacement effect");
  check(!std::memcmp(saved.data(),m.view(objects,saved.size()).data(),saved.size()),"Readback mutated original banks");
  rejects([&]{source_attached_effects_tu15(m,native_objects,native_effects,object+0x10000u);});
  put(m,body+0x14c,effect+0x10000);rejects(read);put(m,body+0x14c,effect);
  put(m,effect_bits,0);rejects(read);put(m,effect_bits,1);
  put(m,data+0x48,object+1);rejects(read);put(m,data+0x48,object);
  put(m,body+0x11c,9,2);rejects(read);put(m,body+0x11c,24,2);
  put(m,body+0x11e,0x2f8,2);rejects(read);put(m,body+0x11e,0x140,2);
  put(m,location_data+4,0xe9000000);rejects(read);put(m,location_data+4,0xffffffffu);
  auto bad=rows;bad[1].scale_a=1;rejects([&]{validate_source_attached_effects(bad);});
  bad=rows;bad[1].slot=bad[0].slot;rejects([&]{validate_source_attached_effects(bad);});
  bad=rows;bad[0].scale_b=std::numeric_limits<float>::infinity();rejects([&]{validate_source_attached_effects(bad);});
  put(m,body+0x11c,0,2);put(m,body+0x11e,0xffff,2);check(read().empty(),"Absent attachment bank consumed its unused offset");
  std::cout<<"PASS completed TU1.5 effect identities, controls, colors, local frames, read-only banks and stale/malformed rejection\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
