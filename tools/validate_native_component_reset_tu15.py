#!/usr/bin/env python3
"""Native1D8470 reference/property reset using real original helper bodies."""
import inspect,json
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x1d8470,0x31c450,0x31c5e0,0x283450,0x2e4d60)
BOUNDARIES=(0x149e00,0x2e3250)
BACKEND='unchanged-tu15-native-component-reset-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=BOUNDARIES
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[190,55,28,18,1];')
 # Keep the exact authenticated sizes rather than assuming older inventory.
 counts=[len(complete_body(_Code(image,'.text'),owner).code)for owner in OWNERS];text=text.replace('[190,55,28,18,1]',repr(counts));env=dict(support.__dict__);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[]
 for pattern in range(25):
  for control in range(4):
   banks={0x1000000:bytearray(0x10000),0x556588:bytearray(1),0x5565ac:bytearray(4),0x48c938:bytearray(4),0x80061000:bytearray(0x1000)}
   def put(p,value,width=4):
    lo=next(lo for lo,b in banks.items()if lo<=p and p+width<=lo+len(b));banks[lo][p-lo:p-lo+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
   c=0x1000100;put(0x48c938,0x1008000);put(0x1008000,0x1008100);put(0x1008114,0x149e00);put(c+0x7c,0x1003000);put(c+0x70,0x1004000)
   bodies=(0x1002000,0x1002200,0x1002400,0x1002600)
   for n,b in enumerate(bodies):
    put(b,0x1007000);put(b+6,[0,1,2,0xffff][control],2);put(b+0x30,0x1005000+n*0x100);put(b+0x34,2);put(b+0x38,4)
    put(0x1005000+n*0x100,0x2001);put(0x1005004+n*0x100,0x12340001);put(0x1005008+n*0x100,0x2002);put(0x100500c+n*0x100,3)
   put(0x1007000,0x2e4d60);put(c+4,0xfeedbeef);put(c+0x18,0x1234,2);put(c+0x1c,1,1)
   put(0x1003008,bodies[0]);put(0x1004000+0x40,bodies[1]);put(0x1004050,0x80000000)
   if pattern in(1,5,7,9,11,13):put(c+0x80,1)
   if pattern in(2,5,6,8,10,12,13):put(c+0x74,1)
   if pattern in(3,5,7,9,11,13):put(c+0x98,bodies[2])
   if pattern in(4,5,8,10,12,13):put(c+0x9c,bodies[3])
   if pattern==6:put(bodies[1]+0x34,0)
   if pattern==7:put(c+0x80,0xffffffff)
   if pattern==8:put(c+0x74,0xffffffff)
   if pattern in(9,10):
    put(0x80061000,0x1007000);put(0x80061006,1,2);put(c+0x98,0x80061000)
   if pattern==11:put(c+0x80,2);put(0x1003014,bodies[0])
   if pattern==12:put(0x1004050,0);put(0x1004048,0x1009000)
   if pattern==14:
    put(c+0x74,1);put(bodies[1]+0x34,3);put(0x1005100,0x2009);put(0x1005104,123);put(0x1005108,0x2001);put(0x100510c,77);put(0x1005110,0x2002);put(0x1005114,9)
   if pattern==15:
    put(c+0x74,1);put(bodies[1]+0x34,3);put(0x1005100,0x2001);put(0x1005104,77);put(0x1005108,0x2001);put(0x100510c,88);put(0x1005110,0x2002);put(0x1005114,9)
   if pattern in(16,17,18,19):
    high=0x80061010;put(high,0x1007000);put(high+6,1,2);put(c+0x98,high);put(0x556588,1 if pattern in(16,18)else 0,1);put(0x5565ac,16 if pattern in(16,17)else 0xffffffff)
   if pattern in(20,21,22):
    high=0x80061000;put(high,0x1007000);put(high+6,1,2)
    if pattern==20:put(c+0x80,1);put(0x1003008,high)
    if pattern==21:
     put(c+0x74,1);put(0x1004040,high);put(high+0x30,0x1006000);put(high+0x34,2);put(high+0x38,4);put(0x1006000,0x2001);put(0x1006004,11);put(0x1006008,0x2002);put(0x100600c,3)
    if pattern==22:put(c+0x9c,high)
   if pattern==23:
    banks[0x80060000]=bytearray(0x1000);high=0x80060000;put(high,0x1007000);put(high+6,1,2);put(c+0x74,1);put(0x1004040,high);put(high+0x30,0x1006000);put(high+0x34,2);put(high+0x38,4);put(0x1006000,0x2001);put(0x1006004,11);put(0x1006008,0x2002);put(0x100600c,3)
   if pattern==24:put(c+0x74,1);put(bodies[1]+0x34,1);put(0x1005100,0x2009);put(0x1005104,77)
   n=len(rows);rows.append(dict(name=f'reset_{pattern}_{control}',entry=0x1d8470,receiver=0,arguments=[c],banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=base.physical.STOP,boundary=None))
 for seed in (rows[20],rows[40]):x=dict(seed);x['name']='fault_seed_'+seed['name'];rows.append(x)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+'''  def boundary(c,actual):
   owner=actual['actual_stop']
   if owner==physical.STOP:return None
   assert owner in(0x149e00,0x2e3250),hex(owner)
   gp,args=actual['boundary_gp'],actual['boundary_stack'];return dict(owner=owner,receiver=gp[1],data=args[1],bytes=args[2],memory_class=args[3])
'''+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_component_reset.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_component_reset.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})',body_counts_cpp).replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15ComponentResetServices services;').replace('NativeTu15ContactPressureStats stats;','NativeTu15ComponentResetStats stats;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','native_tu15_reset_physics_component(m,c.at("arguments")[0],{0x2000000u,0x2000004u},fp,0,services,observer,&stats);');begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+native_boundaries+driver[end:];driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!(p>=0x2000000u&&p<0x2001000u))reads.push_back').replace('auto m=before;m.log_reads','auto m=before;m.banks[0x2000000u].resize(0x1000u);m.log_reads').replace('stores.push_back(store);','if(store.address>=0x2000000u&&store.address<0x2001000u)return;stores.push_back(store);').replace('if(fault_case)stores=m.raw_stores;','if(fault_case){stores.clear();for(const auto&store:m.raw_stores)if(!(store.address>=0x2000000u&&store.address<0x2001000u))stores.push_back(store);}');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  (project/'negative.py')",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('refs_stride','offset+=12u','offset+=16u'),('clear_flags','component+4u,0u','component+4u,1u'),('property_key','remove(0x2002u','remove(0x2003u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  for name in('native_tu15_body_properties.cpp','native_tu15_array_growth.cpp','native_tu15_island_activation.cpp'):shutil.copyfile(ROOT/'src/physics'/name,project/name)
  (project/'types.cpp').write_text('')'''+text[end:]
 text=text.replace("+'.cpp types.cpp)","+'.cpp types.cpp native_tu15_body_properties.cpp native_tu15_array_growth.cpp native_tu15_island_activation.cpp)")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp'","ROOT/'include/halo2/native_tu15_component_reset.hpp'")
 filtering="rmw={s['instruction']for s in meta['sites']if any(o['read']and o['write']for o in s['operands'])};excluded=[c['name']for c in prefix if c.get('fault',{}).get('access')=='write'and c['stop_pc']in rmw];prefix=[c for c in prefix if c['name']not in excluded];rows+=physical.physical(image,meta,prefix,scratch)";text=text.replace('rows+=physical.physical(image,meta,prefix,scratch)',filtering)
 counts=[len(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),owner).code)for owner in OWNERS]
 native='''   services.allocation.release=[&](const auto&request,auto&){const auto&b=c.at("boundary");check(b.at("owner")==request.owner&&b.at("receiver")==request.receiver&&b.at("data")==request.data&&b.at("bytes")==request.bytes&&b.at("memory_class")==request.memory_class,name+" actual native reset release arguments differ");boundary(request.owner);};
'''
 env=dict(base.__dict__,__file__=__file__,body_counts_cpp='J::array({'+','.join(map(str,counts))+'})',native_boundaries=native);exec(text,env);code=env['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());executed=support.parent.EXECUTED;report['body_coverage']={hex(owner):dict(total=len(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),owner).code),executed=len(set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),owner).code)&executed),unexecuted=sorted(set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),owner).code)-executed))for owner in OWNERS};report['source_hashes'].update({str(p.relative_to(ROOT)):base.sha(p)for p in(ROOT/'src/physics/native_tu15_body_properties.cpp',ROOT/'src/physics/native_tu15_array_growth.cpp',ROOT/'src/physics/native_tu15_island_activation.cpp')});report.update(excluded_preinstruction_RMW_write_fault_phase=True,remaining=['Genuine actual component/reset child entry and sameproduction builder integration','Selected real allocator release beyond firstchild prefix, owned body destructor classes and lifetime closure','Mutable/aliasing callback/error-unwind profiles outside qualified domain']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
