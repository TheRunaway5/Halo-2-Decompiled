#!/usr/bin/env python3
"""Native1F6FA0 plus genuine native1F76A0/186E70, unchanged original proof."""
import inspect,json,struct,copy
from pathlib import Path
import validate_native_collision_component_visibility_tu15 as visibility
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
BACKEND='unchanged-tu15-native-collision-key-pruning-v1'
OWNERS=(0x1f6fa0,0x1f76a0,0x186e70)
def metadata(image):
 text=inspect.getsource(visibility.metadata).replace('for owner in(0x1f76a0,0x186e70):','for owner in(0x1f6fa0,0x1f76a0,0x186e70):').replace('assert counts==[82,27]','assert counts==[35,82,27]')
 env=dict(visibility.__dict__);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[];seeds=visibility.cases()
 for seed in seeds:
  if seed['entry']!=0x1f76a0:continue
  for mode in range(2):
   c=copy.deepcopy(seed);c['name']='keys_'+str(mode)+'_'+c['name'];c.update(entry=0x1f6fa0,receiver=0,arguments=[0x1000100],key_array=0x1005000,entry_registers={'esi':0x1005000})
   b=next(b for b in c['banks']if b['address']==0x1000000);data=bytearray.fromhex(b['before']);keys=[0x40000000,c['key'],c['key'],0x40000000]if mode else[c['key']]
   for k,value in enumerate((0x1005100,len(keys),0x80000020)):struct.pack_into('<I',data,0x5000+k*4,value)
   for k,value in enumerate(keys):struct.pack_into('<I',data,0x5100+k*4,value)
   b['before']=data.hex();rows.append(c)
 for count in(0,0xffffffff,0x80000000):
  c=copy.deepcopy(rows[0]);c['name']='signed_count_'+hex(count);b=next(b for b in c['banks']if b['address']==0x1000000);data=bytearray.fromhex(b['before']);struct.pack_into('<I',data,0x5004,count);b['before']=data.hex();rows.append(c)
 # Fault seeds retain real enabled/disabled visibility and shifting branches.
 for seed in (rows[2],rows[3],next(r for r in rows if 'object_3_'in r['name']),next(r for r in rows if 'key_5_63_False'in r['name'])):
  c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical
 base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases
 text=inspect.getsource(base.main);begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin)
 text=text[:begin]+'''  def boundary(c,actual):
   assert actual['actual_stop']==physical.STOP;return None
'''+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_collision_key_query.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text()
  driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_collision_key_query.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({35,82,27})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','native_tu15_prune_collision_keys(m,c.at("key_array"),observer);')
  begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('wrong_shift','value=m.word(destination+4u)','value=m.word(destination)'),('wrong_count','const auto count=m.word(array+4u)-1u','const auto count=m.word(array+4u)-2u'),('inverted_visibility','if(!native_tu15_collision_component_enabled(memory,key))','if(native_tu15_collision_component_enabled(memory,key))')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","shutil.copyfile(ROOT/'src/physics/native_tu15_collision_component.cpp',project/'types.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_collision_surface_shape.cpp',project/'surface.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_inverse_geometry.cpp',project/'inverse.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')")
 text=text.replace("+'.cpp types.cpp)","+'.cpp types.cpp surface.cpp inverse.cpp)")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_collision_key_query.hpp',ROOT/'src/physics/native_tu15_collision_component.cpp',ROOT/'src/physics/native_tu15_collision_surface_shape.cpp',ROOT/'src/physics/native_tu15_inverse_geometry.cpp',ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp'")
 env=dict(base.__dict__,__file__=__file__);exec(text,env);result=env['main']()
 path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';r=json.loads(path.read_text());code=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(code,p).code)for p in OWNERS));r.update(scope=__doc__,physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),production_native_only_closure=False,remaining=['Whole1F6D10 key query/native setup/traversal composition','Positive instance expansion and native virtual2C ownership','Actual native-only key-query/candidate service binding and visible multiplayer']);base.save(path,r);return result
if __name__=='__main__':raise SystemExit(main())
