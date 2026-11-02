#!/usr/bin/env python3
"""Native2E5320 plus real native cache and sphere/sweep support, no child return.

Declared fully defined source-coherent shape-method DATA uses the actual
23/24/5 getters and sphere/sweep/triangle algorithms. The oracle stops at the
real2F8B60 entry after cache publication. No contact outcome is supplied.
"""
import copy,inspect,struct
from pathlib import Path
import validate_native_moving_shape_process_tu15 as base
ROOT=Path(__file__).resolve().parents[1]
ORIGINAL_CASES=base.cases
EXTRA=('native_tu15_contact_cache.cpp','native_tu15_contact_shape.cpp')
def cases(image):
 rows=[]
 for seed in ORIGINAL_CASES(image):
  if seed['name'].startswith('fault_seed'):continue
  c=copy.deepcopy(seed);raw=bytearray.fromhex(next(b['before']for b in c['banks']if b['address']==base.BASE))
  def put(p,n):struct.pack_into('<I',raw,p-base.BASE,n)
  def word(p):return struct.unpack_from('<I',raw,p-base.BASE)[0]
  for index,shape in enumerate((base.SHAPE_A,base.SHAPE_B)):
   table=word(shape);put(table+0x30,0x2e5db0);put(table+0x38,0x2e5da0);put(table+0x3c,0x1cb030)
   sphere=base.BASE+0x7000+index*0x100;sphere_table=base.BASE+0x7400+index*0x100;put(shape+0x30,sphere);put(sphere,sphere_table);put(sphere_table+0x30,0x2e8d20);put(sphere_table+0x38,0x2e8d70)
   if word(table+0x14)==0x31dd70:
    for row,values in enumerate(((0,0,0,0),(0x3f800000,0,0,0),(0,0x3f800000,0,0))):
     for n,value in enumerate(values):put(shape+0x10+row*16+n*4,value)
  next(b for b in c['banks']if b['address']==base.BASE)['before']=raw.hex()
  c['banks']+=[dict(address=0x461960,before=image.read(0x461960,16).hex()),dict(address=0x46a0e8,before=image.read(0x46a0e8,4).hex())];c['name']='cache_composed_'+c['name'];rows.append(c)
 for seed in(rows[0],rows[20],rows[40]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_moving_shape_process.hpp"','#include "halo2/native_tu15_moving_shape_process.hpp"\n#include "halo2/native_tu15_contact_cache.hpp"\n#include "halo2/native_tu15_contact_shape.hpp"')
 text=text.replace('struct Fault{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool native_private(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=0x1000u;}struct Fault{};')
 text=text.replace('if(log_reads)reads.push_back','if(log_reads&&!native_private(p,b.size()))reads.push_back').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!native_private(p,b.size()))raw_stores.push_back')
 text=text.replace('const auto fault=c.value("fault",J{});','before.banks.emplace(PRIVATE,std::vector<std::byte>(0x1000u,std::byte{0xa5}));const auto fault=c.value("fault",J{});')
 begin=text.index('   services.initialize_cache=');end=text.index('\n   services.process_convex=',begin)
 body='''   services.initialize_cache=[&](auto owner,auto cache,auto first,auto second,auto transform,auto&state){
    SourcePhysicsContactRecordObserver child_observer;if(observer)child_observer=[&](const auto&event){if(!native_private(event.address,event.written.size()))observer(event);};
    NativeTu15ContactCacheServices native;
    native.shape_center=[&](auto shape,auto target,auto output,auto&){native_tu15_contact_shape_center(m,shape,target,output,child_observer);};
    native.support_vertex=[&](auto shape,auto target,auto direction,auto output,auto&fp){const NativeTu15ContactShapeScratch child{PRIVATE+0x904u,0,nullptr},scratch{PRIVATE+0x900u,0,&child};native_tu15_contact_shape_support(m,shape,target,direction,output,scratch,fp,0,child_observer);};
    native.vertex_encoding=[&](auto,auto target,auto&){check(target.identity==0x1cb030u,"Unknown cache encoding owner");return native_tu15_contact_shape_vertex_encoding();};
    if(owner==0x309060u)native_tu15_initialize_convex_contact_cache(m,cache,first,second,transform,{PRIVATE},state,native,child_observer);
    else{check(owner==0x309150u,"Unknown cache initializer owner");native_tu15_initialize_triangle_contact_cache(m,cache,first,second,transform,{PRIVATE},state,native,child_observer);}
   };'''
 return text[:begin]+body+text[end:]
def main():
 base.OWNERS=(*base.OWNERS,0x309060,0x309150,0x2e4690,0x2e47b0,0x2e5da0,0x2e5db0,0x2e8d70,0x2e8d20,0x1cb030);base.BOUNDARIES=(0x2f8b60,);base.cases=cases
 source=inspect.getsource(base.main);source=source.replace("shutil.copyfile(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(driver_source())")
 source=source.replace("units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')","units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp',*EXTRA)")
 # The cache-class mutation selects a real different initializer, observed at
 # persistent cache writes rather than at a supplied callback return.
 source=source.replace("remaining=['Returned cache and2F8B60 child closure, parent epilogue'","native_cache_composition=True,scope=__doc__,remaining=['Returned2F8B60 slow/simplex/publication closure'")
 environment=dict(base.__dict__,EXTRA=EXTRA,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
