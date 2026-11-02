#!/usr/bin/env python3
"""Whole328870 six-word schema header with strict alias/store/fault phases."""
import inspect,json
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x328870,)
BACKEND='unchanged-tu15-native-joint-constraint-schema-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[21];');environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 for pattern in range(8):
  for control in range(4):
   data=bytearray((n*37+17)&255 for n in range(0x1000));banks={0x1000000:data};builder=0x1000100;input=0x1000200;schema=0x1000300
   if pattern==1:schema=input
   if pattern==2:schema=builder
   if pattern==3:builder=input
   if pattern==4:schema=builder-16
   if pattern==5:schema=input-12
   if pattern==6:input=builder+8
   if pattern==7:schema=0x80061000;banks[schema]=bytearray(64)
   def put(at,value):
    lo=next(p for p,b in banks.items()if p<=at and at+4<=p+len(b));banks[lo][at-lo:at-lo+4]=(value&0xffffffff).to_bytes(4,'little')
   put(input,0x1004000+control*16);put(input+4,0x1005000+control*16);put(builder,0x1006000+control*16);put(builder+4,schema)
   n=len(rows);rows.append(dict(name=f'header_{pattern}_{control}',entry=0x328870,receiver=0,arguments=[input,builder,(0x1007000,0,0xffffffff,0x80000000)[control],(8,0,0xffffffff,0x80000000)[control]],banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=base.physical.STOP,boundary=None))
 for seed in(rows[0],rows[8],rows[12]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_joint_constraint_schema.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_joint_constraint_schema.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({21})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','native_tu15_begin_joint_constraint_schemas(m,{c.at("arguments")[0]},{c.at("arguments")[1]},c.at("arguments")[2],c.at("arguments")[3],observer);');begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('header_kind','schema,0x11801u','schema,0x11800u'),('runtime_size','schema+20u,runtime_bytes','schema+20u,runtime'),('publication','schema+24u','schema+20u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_joint_constraint_schema.hpp'")
 environment=dict(base.__dict__,__file__=__file__);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());sites=set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),0x328870).code);report.update(physically_executed_parent_sites=len(sites&support.parent.EXECUTED),unexecuted_parent_sites=sorted(sites-support.parent.EXECUTED),scope=__doc__,remaining=['Native31D930 parent and full Jacobian row/helper composition','Original formal/control-stack aliases excluded by immutable native value ABI; no GP reconstruction','Actual31D930 input and visible native route']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
