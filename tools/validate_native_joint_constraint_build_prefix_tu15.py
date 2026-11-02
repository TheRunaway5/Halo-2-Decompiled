#!/usr/bin/env python3
"""Native31D930 headers/transforms/angular/motor through real3294E0 entry."""
import copy,inspect,json,struct
from pathlib import Path
import validate_native_joint_constraint_motor_tu15 as motor
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
OWNERS=(0x31d930,0x328870,0x3288b0,0x37c6b0,0x329100)
BACKEND='unchanged-tu15-native-joint-build-prefix-v1'
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=(0x3294e0,)
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[207,21,104,48,110];').replace("read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)","read=False if ins.mnemonic in('fst','fstp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp')")
 env=dict(support.__dict__);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[];seeds=motor.cases(image)
 for pattern in range(4):
  for previous in(0,0x41a00000,0xc1a00000,0x40490fdb,0xc0490fdb):
   for rounding in range(4):
    c=copy.deepcopy(seeds[pattern*4+rounding]);data=bytearray.fromhex(c['banks'][0]['before']);constraint=0x1002200;body_a=0x1002400;body_b=0x1002500;motion_a=0x1002600;motion_b=0x1002800
    def put(p,n):struct.pack_into('<I',data,p-0x1000000,n&0xffffffff)
    def vector(p,values):
     for lane,n in enumerate(values):put(p+lane*4,n)
    put(constraint+12,body_a);put(constraint+16,body_b);put(body_a+0x3c,motion_a);put(body_b+0x3c,motion_b)
    bases=((0x3f800000,0,0,0),(0,0x3f800000,0,0),(0,0,0x3f800000,0),(0x3f000000,0xbe800000,0x3e000000,0))
    for motion in(motion_a,motion_b):
     for n in range(3):vector(motion+0x80+n*16,bases[n])
     vector(motion+0xb0,bases[(pattern+1)%4])
    for row in range(4):vector(constraint+0x20+row*16,bases[(row+pattern)%4])
    for row in range(3):vector(constraint+0x60+row*16,bases[(row+pattern+2)%3])
    for off,n in((0xc0,0xbf800000),(0xc4,0x3f800000),(0xc8,0x3f000000),(0xcc,0x3e800000),(0xd0,0),(0xd4,previous)):put(constraint+off,n)
    c['banks'][0]['before']=data.hex();c.update(name=f'joint_prefix_{pattern}_{previous:x}_{rounding}',entry=0x31d930,receiver=constraint,arguments=c['arguments'][1:])
    for p,n in((0x41e90c,4),(0x41c4c8,8),(0x46a0e8,4),(0x4485cc,16)):
     if not any(b['address']<=p and p+n<=b['address']+len(bytes.fromhex(b['before']))for b in c['banks']):c['banks'].append(dict(address=p,before=image.read(p,n).hex()))
    rows.append(c)
 for seed in(rows[0],rows[20],rows[40]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def observer_source():
 text=support.parent.observer_source();needle='{"boundary_gp",gp(regs)}';replacement='{"linear_first",std::uint32_t(regs.rip)==0x3294e0u?hex(memory(child.pid,stack.at(1).get<std::uint32_t>(),16)):std::string{}},{"linear_second",std::uint32_t(regs.rip)==0x3294e0u?hex(memory(child.pid,stack.at(2).get<std::uint32_t>(),16)):std::string{}},'+needle
 assert text.count(needle)==1;return text.replace(needle,replacement).replace('"Original pose instruction fault"','(std::string("Original joint instruction fault PC=")+std::to_string(pc)).c_str()')
def main():
 base.physical.observer_source=observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+'''  def boundary(c,actual):
   if actual['actual_stop']==physical.STOP:return None
   assert actual['actual_stop']==0x3294e0
   return dict(owner=0x3294e0,first=actual['linear_first'],second=actual['linear_second'],input=actual['boundary_stack'][3],packet=actual['boundary_stack'][4])
'''+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_joint_constraint_build.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_joint_constraint_build.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({207,21,104,48,110})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15JointConstraintBuildServices services;').replace('NativeTu15ContactPressureStats stats;','NativeTu15JointConstraintBuildStats stats;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+SERVICES+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('translation','first[0]=add(first[0],first_position)','first[0]=first[0]'),('cross_order','"0"(product1),"x"(product0)','"0"(product0),"x"(product1)'),('unwrap_threshold','0x41e90cu','0x46a0e8u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 units=('native_tu15_joint_constraint_schema.cpp','native_tu15_joint_constraint_angular_row.cpp','native_tu15_joint_constraint_motor.cpp','native_tu15_joint_constraint_limits.cpp','native_tu15_constraint_angle.cpp')
 copy="(project/'types.cpp').write_text('');[shutil.copyfile(ROOT/'src/physics'/name,project/name)for name in"+repr((*units,'native_tu15_joint_constraint_numeric_internal.hpp'))+"]"
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')",copy).replace("+'.cpp types.cpp)","+'.cpp types.cpp "+' '.join(units)+")")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_joint_constraint_build.hpp'")
 calls='''native_tu15_build_joint_constraint(m,c.at("receiver"),{c.at("arguments")[0]},{c.at("arguments")[1]},fp,0,services,observer,&stats);'''
 services='''   services.linear=[&](const auto&first,const auto&second,auto input,auto packet,auto depth){const auto&b=c.at("boundary");check(hex(std::as_bytes(std::span(first)))==b.at("first").get<std::string>()&&hex(std::as_bytes(std::span(second)))==b.at("second").get<std::string>()&&input.identity==b.at("input")&&packet.identity==b.at("packet")&&depth==0,name+" transformed linear Vec4/formals differ");boundary(0x3294e0u);};
'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,SERVICES=services);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
