#!/usr/bin/env python3
"""Source2E5320 fullreturns through native2F8B60 real fast separation.

Only declared coherent incoming geometry/cache values are varied. The child
computes its separation and empty cleanup itself; no solver outcome, contact
or callback return is supplied. Slow/cached-refresh closure remains separate.
"""
import copy,inspect,struct
from pathlib import Path
import validate_native_moving_shape_process_tu15 as base
ROOT=Path(__file__).resolve().parents[1]
EXTRA=('native_tu15_convex_contact_process.cpp','native_tu15_convex_contact_query.cpp','native_tu15_agent_cleanup_children.cpp','native_tu15_agent_cleanup.cpp','native_tu15_array_growth.cpp','native_tu15_array_allocator.cpp')
ORIGINAL_CASES=base.cases
def cases(image):
 rows=[]
 seeds=[c for c in ORIGINAL_CASES(image)if (c['name'].startswith('types_')and'_vel_(0, 0)_cache_0'in c['name'])or c['name']=='property_parent_rejected']
 for seed in seeds:
  for distance in(0x3fc00000,0x40400000,0x41200000):
   for delta in(0xbe800000,0,0x3e800000):
    for rounding in range(4):
     c=copy.deepcopy(seed);raw=bytearray.fromhex(next(b['before']for b in c['banks']if b['address']==base.BASE))
     def put(p,n):struct.pack_into('<I',raw,p-base.BASE,n)
     for offset,value in enumerate((0x3f800000,0,0,distance)):put(base.AGENT+0x20+offset*4,value)
     put(base.AGENT+0x18,0)
     for transform in(base.TRANSFORM_A,base.TRANSFORM_B):
      for offset,value in enumerate((0,0,0,0x3f000000,delta if transform==base.TRANSFORM_B else 0,0,0,0x3f000000)):put(transform+offset*4,value)
     next(b for b in c['banks']if b['address']==base.BASE)['before']=raw.hex();c['banks'].append(dict(address=0x46a0e8,before=image.read(0x46a0e8,4).hex()));c['before_fp'][0]=(c['before_fp'][0]&~0xc00)|(rounding<<10);c['before_fp'][2]=(c['before_fp'][2]&~0x6000)|(rounding<<13);c['name']='full_fast_'+str(len(rows))+'_'+seed['name'];rows.append(c)
 for seed in(rows[0],rows[72],rows[-1]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp').read_text();text=text.replace('#include "halo2/native_tu15_moving_shape_process.hpp"','#include "halo2/native_tu15_moving_shape_process.hpp"\n#include "halo2/native_tu15_convex_contact_process.hpp"')
 text=text.replace('struct Fault{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool native_private(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=0x1000u;}struct Fault{};')
 text=text.replace('if(log_reads)reads.push_back','if(log_reads&&!native_private(p,b.size()))reads.push_back').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!native_private(p,b.size()))raw_stores.push_back')
 text=text.replace('const auto fault=c.value("fault",J{});','before.banks.emplace(PRIVATE,std::vector<std::byte>(0x1000u,std::byte{0xa5}));const auto fault=c.value("fault",J{});')
 begin=text.index('   services.process_convex=');end=text.index('\n   const auto cw=',begin)
 body='''   services.process_convex=[&](auto agent,auto first,auto second,auto config,auto output,auto&state){
    NativeTu15ConvexProcessServices convex;convex.geometry.solve_simplex=[&](auto,auto,auto,auto,auto,auto,auto&)->std::uint32_t{throw std::runtime_error("Unqualified slow simplex selected in full fast case");};
    SourcePhysicsContactRecordObserver child_observer;if(observer)child_observer=[&](const auto&event){if(!native_private(event.address,event.written.size()))observer(event);};
    NativeTu15ConvexProcessStats child_stats;native_tu15_process_convex_contact_agent(m,{agent.identity},first,second,config,{output},{PRIVATE,PRIVATE+0x300u},state,0,convex,child_observer,&child_stats);
   };'''
 return text[:begin]+body+text[end:]
def main():
 base.OWNERS=(*base.OWNERS,0x2f8b60,0x30ca60);base.cases=cases
 base.BOUNDARIES=(0x309060,0x309150)
 source=inspect.getsource(base.main)
 source=source.replace("   owner=actual['actual_stop'];assert owner in BOUNDARIES,hex(owner)","   owner=actual['actual_stop'];\n   if owner==physical.STOP:return None\n   assert owner in BOUNDARIES,hex(owner)")
 source=source.replace("shutil.copyfile(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(driver_source())")
 begin=source.index("  for name,needle,replacement in(");end=source.index('\n   assert needle in text;',begin)
 source=source[:begin]+'''  for name,needle,replacement in(('wrong_completed_mask','child.identity+0xf0u,motion_state','child.identity+0xf0u,motion_state^1u'),('wrong_restored_setting','m.word(context.identity+0xcu),old_setting,1','m.word(context.identity+0xcu),old_setting^1u,1'),('wrong_clear','shape+0x10u,_mm_setzero_ps()','shape+0x10u,broadcast(0x3f800000u)')):'''+source[end:]
 source=source.replace("units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')","units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp',*EXTRA)")
 source=source.replace("remaining=['Returned cache and2F8B60 child closure, parent epilogue'","full_returns=sum(c['actual_stop']==physical.STOP and not c.get('fault')and not c.get('observer_fault')for c in rows),scope=__doc__,remaining=['Moving/cache-refresh2F8B60 slow/simplex/publication path'")
 environment=dict(base.__dict__,EXTRA=EXTRA,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
