#!/usr/bin/env python3
"""SAME native-only232D20 production helper with strict original DATA/FP prefixes."""
import inspect,json
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x232d20,0x232c80)
BOUNDARIES=(0x2e4d60,)
BACKEND='unchanged-tu15-native-main-heap-release-bound-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=BOUNDARIES
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[5,54];')
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(entry,count,index,*,duplicate=False,unknown=False,wrap=False):
  allocator=0x1000200;table=0x1000100;ledger=0x1001000;end=0x100
  pool_base=0xffff0000 if wrap else ledger
  if wrap:ledger=0x20000000;end=0x20010100
  banks={0x1000000:bytearray(0x4000)}
  if wrap:banks[ledger]=bytearray(0x200)
  def put(p,v):
   lo=next(lo for lo,b in banks.items()if lo<=p and p+4<=lo+len(b));banks[lo][p-lo:p-lo+4]=(v&0xffffffff).to_bytes(4,'little')
  put(allocator,table);put(table+0xc,0x2e4d60 if unknown else 0x232c80)
  put(allocator+0x18,pool_base);put(allocator+0x1c,end);put(allocator+0x20,count)
  for k in range(min(count,8)):
   put((pool_base+end-8-k*8)&0xffffffff,0x200 if duplicate else 0x200+k*0x20)
   put((pool_base+end-4-k*8)&0xffffffff,0x40+k)
  data=(pool_base+(0x200 if duplicate else 0x200+index*0x20))&0xffffffff
  if index<0:data=0xfedcba98
  n=len(rows);rows.append(dict(name=f'heap_{entry:x}_{count:x}_{index}_{duplicate}_{unknown}_{wrap}',entry=entry,receiver=allocator,arguments=[data,0xf0e0d0c0,0x87654321]if entry==0x232d20 else[data],allocator=allocator,data=data,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=base.physical.STOP,boundary=None))
 for entry in OWNERS:
  for count in(0,1,2,3,5,0xffffffff,0x80000000):
   for index in(-1,0,1,3):make(entry,count,index)
  for index in(0,1,3):make(entry,5,index,wrap=True)
  make(entry,5,1,duplicate=True)
 for count in(0,3):make(0x232d20,count,0,unknown=True)
 for seed in(rows[17],rows[49]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical
 base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin)
 text=text[:begin]+'''  def boundary(c,actual):
   owner=actual['actual_stop']
   if owner==physical.STOP:return None
   assert owner==0x2e4d60,hex(owner)
   gp,args=actual['boundary_gp'],actual['boundary_stack'];return dict(owner=owner,allocator=gp[1],data=args[1])
'''+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_main_heap_release.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_main_heap_release.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','#include "halo2/native_tu15_main_heap_release_binding.hpp"').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({5,54})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15MainHeapReleaseServices services;').replace('NativeTu15ContactPressureStats stats;','NativeTu15MainHeapReleaseStats stats;NativeTu15MainHeapReleaseBindingActivity activity;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','if(c.at("entry")==0x232d20u)native_tu15_release_bound_main_heap_allocation(m,{c.at("allocator"),0x232d20u,c.at("data"),c.at("arguments")[1],c.at("arguments")[2]},fp,0,observer,&activity);else native_tu15_remove_main_heap_allocation(m,c.at("allocator"),c.at("data"),observer,&stats);');begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+native_boundaries+driver[end:];driver=driver.replace('if(std::string(e.what())!="actual-island-owner-boundary")throw;','if(activity.kernel.unbound_services){check(!c.at("boundary").is_null()&&activity.kernel.unbound_owners.count(c.at("boundary").at("owner")),name+" native-only lifetime refusal differs");boundary_hit=true;}else if(std::string(e.what())!="actual-island-owner-boundary")throw;');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('search_stride','cursor-=8u','cursor-=4u'),('late_count','count-1u);break','count);break'),('second_word','source_base+current_end-12u','source_base+current_end-16u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_main_heap_release.hpp',ROOT/'include/halo2/native_tu15_main_heap_release_binding.hpp'")
 native='''   services.release=[&](auto allocator,auto owner,auto data,auto&,auto){const auto&b=c.at("boundary");check(b.at("allocator")==allocator&&b.at("data")==data,name+" actual main-heap lifetime arguments differ");boundary(owner);};
'''
 environment=dict(base.__dict__,__file__=__file__,native_boundaries=native);exec(text,environment);code=environment['main']()
 path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());executed=support.parent.EXECUTED;report['body_coverage']={hex(owner):dict(total=len(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),owner).code),executed=len(set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),owner).code)&executed),unexecuted=sorted(set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),owner).code)-executed))for owner in OWNERS};report.update(remaining=['Actual selected allocator/virtual+C backing and same-production World service closure','Mutable/aliasing callback and unrelated allocator lifetime classes outside qualified domain']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
