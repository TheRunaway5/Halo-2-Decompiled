#!/usr/bin/env python3
"""Whole native31D930 with active Host Vec4 linear emitter and all rows."""
import copy,inspect
from pathlib import Path
import validate_native_joint_constraint_build_prefix_tu15 as prefix
import validate_native_contact_pressure_tu15 as base
import validate_native_dirty_island_objects_composed_tu15 as support
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-linear-jacobian-host-stage-20261102/staged'
OWNERS=(*prefix.OWNERS,0x3294e0,0x328a00)
BACKEND='unchanged-tu15-native-joint-build-whole-v1'
def metadata(image):
 text=inspect.getsource(prefix.metadata).replace('(0x3294e0,)','()').replace('207,21,104,48,110','207,21,104,48,110,155,129');env=dict(prefix.__dict__,OWNERS=OWNERS);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[]
 for seed in prefix.cases(image):
  if seed['name'].startswith('fault_seed'):continue
  for coefficient in(0,0x3f000000,0x7fc12345):
   c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);raw[c['receiver']+0xd0-0x1000000:c['receiver']+0xd4-0x1000000]=coefficient.to_bytes(4,'little');c['banks'][0]['before']=raw.hex();c['banks'].append(dict(address=0x46e400,before=image.read(0x46e400,48).hex()));c['name']=c['name'].replace('prefix','whole')+'_limit'+str(coefficient);rows.append(c)
 for seed in rows[:8]:
  for value in(0x40000000,0x7f812345):
   c=copy.deepcopy(seed);raw=bytearray.fromhex(c['banks'][0]['before']);offset=0x1002600+0xbc-0x1000000
   raw[offset:offset+4]=value.to_bytes(4,'little');c['banks'][0]['before']=raw.hex();c['name']+='_pointW'+str(value);rows.append(c)
 for seed in rows[:]:
  for precision in(0,3):
   c=copy.deepcopy(seed);c['before_fp'][0]=(c['before_fp'][0]&~0x300)|(precision<<8);c['name']+='_precision'+str(precision);rows.append(c)
 for seed in(rows[0],rows[40],rows[80]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=prefix.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_joint_constraint_build.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_joint_constraint_build.hpp"\\n#include "halo2/native_tu15_linear_jacobian_host.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({207,21,104,48,110,155,129})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15JointConstraintBuildServices services;').replace('NativeTu15ContactPressureStats stats;','NativeTu15JointConstraintBuildStats stats;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+SERVICES+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('limit_parity','&1u)==0u','&1u)!=0u'),('linear_points','services.linear(words(first[0]),words(second[0])','services.linear(words(second[0]),words(first[0])'),('translation','first[0]=add(first[0],first_position)','first[0]=first[0]')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 units=('native_tu15_linear_jacobian_emitter.cpp','native_tu15_joint_constraint_schema.cpp','native_tu15_joint_constraint_angular_row.cpp','native_tu15_joint_constraint_motor.cpp','native_tu15_joint_constraint_limits.cpp','native_tu15_constraint_angle.cpp')
 copy="(project/'types.cpp').write_text('');[shutil.copyfile((STAGE/'src/physics'/name)if name=='native_tu15_linear_jacobian_emitter.cpp'else(ROOT/'src/physics'/name),project/name)for name in"+repr((*units,'native_tu15_joint_constraint_numeric_internal.hpp'))+"]"
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')",copy).replace("+'.cpp types.cpp)","+'.cpp types.cpp "+' '.join(units)+")")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_joint_constraint_build.hpp',ROOT/'include/halo2/native_tu15_linear_jacobian_host.hpp'")
 calls='''native_tu15_build_joint_constraint(m,c.at("receiver"),{c.at("arguments")[0]},{c.at("arguments")[1]},fp,0,services,observer,&stats);'''
 services='''   services.linear=[&](const auto&first,const auto&second,auto input,auto packet,auto depth){native_tu15_emit_world_linear_jacobians_active(m,first,second,input.identity,packet.identity,depth,observer);};
'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,SERVICES=services,STAGE=STAGE);exec(text,env);rc=env['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=base.json.loads(path.read_text());code=_Code(base.Xbe(base.NEW_DEFAULT),'.text');report.update(parent_executed_sites=len(set(complete_body(code,0x31d930).code)&support.parent.EXECUTED),parent_unexecuted_sites=sorted(set(complete_body(code,0x31d930).code)-support.parent.EXECUTED),scope=__doc__,actual_input_and_live_route_proved=False);base.save(path,report);return rc
if __name__=='__main__':raise SystemExit(main())
