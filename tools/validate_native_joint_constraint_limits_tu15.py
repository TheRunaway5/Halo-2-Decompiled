#!/usr/bin/env python3
"""Whole328A00 angular limit rows, source scalar/SIMD and alias/fault phases."""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x328a00,)
BACKEND='unchanged-tu15-native-joint-angular-limits-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[129];')
 text=text.replace("read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)","read=False if ins.mnemonic in('fst','fstp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp')")
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 import validate_native_joint_constraint_angular_row_tu15 as angular
 rows=angular.cases(image)
 for n,c in enumerate(rows):
  c['entry']=0x328a00;c['name']='limits_'+c['name'];data=bytearray.fromhex(c['banks'][0]['before']);descriptor=c['arguments'][0];axes=0x1000140;values=0x1000180
  axes_data=data[descriptor-0x1000000:descriptor-0x1000000+48];data[axes-0x1000000:axes-0x1000000+48]=axes_data
  def put(at,value):struct.pack_into('<I',data,at-0x1000000,value&0xffffffff)
  for lane,value in enumerate((axes,values,0x3f400000,(0,1,2,3,0xffffffff,0x80000001)[n%6])):put(descriptor+lane*4,value)
  for lane in range(6):put(values+lane*4,(0x3e800000,0xbe800000,0x3f000000,0xbf000000,0x3f800000,0xbf800000)[lane])
  put(c['arguments'][1]+8,0x3e800000);c['banks'][0]['before']=data.hex()
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_joint_constraint_limits.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_joint_constraint_limits.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({129})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);','native_tu15_emit_joint_constraint_limits(m,c.at("arguments")[0],{c.at("arguments")[1]},{c.at("arguments")[2]},fp,0,observer);');begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('jacobian_sign','axis=xor_bits(axis,mask);','axis=axis;'),('schema_kind','schema,0x40809u','schema,0x40808u'),('bias_scale','multiply_scalar(m.word(input+12u))','multiply_scalar(m.word(input+8u))')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_joint_constraint_limits.hpp',ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp'")
 environment=dict(base.__dict__,__file__=__file__);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());sites=set(complete_body(_Code(base.Xbe(base.NEW_DEFAULT),'.text'),0x328a00).code);report.update(physically_executed_parent_sites=len(sites&support.parent.EXECUTED),unexecuted_parent_sites=sorted(sites-support.parent.EXECUTED),preserved_x87_depth_physically_tested=0,scope=__doc__,remaining=['HostPOD active-child composition inside native31D930 parent','Actual31D930 input/production routing/live strict original','Nonempty x87/alias/error domains outside declaredprofiles remain open']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
