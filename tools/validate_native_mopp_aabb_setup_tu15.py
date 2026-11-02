#!/usr/bin/env python3
"""Native30A1E0/2E9670 to genuine309940: ordered DATA, FP and fault prefixes.

This does not supply a traversal result or admit a production MOPP query.
All compiler/reference products belong to the enclosing owned scratch.
"""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x30a1e0,0x2e9670)
BACKEND='unchanged-tu15-native-mopp-aabb-setup-prefix-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=(0x309940,)
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[100,15];')
 text=text.replace("read=bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)","read=False if ins.mnemonic in('fst','fstp','fistp')else bool(op.access&CS_AC_READ)or ins.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)or ins.mnemonic in('fst','fstp','fistp')")
 env=dict(support.__dict__);exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[]
 values=(0,0x80000000,0x3f800000,0xbf800000,0x3f000000,0x7fc12345,0x7f812345,0x7f800000,0xff800000,1,0x80000001,0x7f7fffff,0xff7fffff)
 for entry in OWNERS:
  for pattern in range(len(values)):
   for rounding in range(4):
    data=bytearray((n*37+11)&255 for n in range(0x3000));shape=0x1000100;cache=0x1000300 if entry==0x30a1e0 else base.physical.SP-0x50;code=0x1000800;bounds=0x1001000;keys=0x1001400
    def put(p,v):struct.pack_into('<I',data,p-0x1000000,v&0xffffffff)
    put(shape+16,code)
    for k in range(4):put(code+k*4,values[(pattern+k)%len(values)])
    for k in range(8):put(bounds+k*4,values[(pattern+2*k)%len(values)])
    banks=[dict(address=0x1000000,before=data.hex())]
    if entry==0x2e9670:banks.append(dict(address=cache,before=bytes((n*11+pattern)&255 for n in range(0x40)).hex()))
    n=len(rows);rows.append(dict(name=f'aabb_setup_{entry:x}_{pattern}_{rounding}',entry=entry,receiver=cache if entry==0x30a1e0 else shape,cache=cache,arguments=[code,bounds,keys]if entry==0x30a1e0 else[bounds,keys],banks=banks,before_fp=[(0x7f,0x27f,0x37f)[n%3]|(rounding<<10),((n%8)<<11)|0x4500|(n%32),0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None))
 for index in(0,4,20,52):
  c=dict(rows[index]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def main():
 original_observer=support.parent.observer_source
 def observer():
  text=original_observer();needle='{"boundary_packet",packet}'
  assert text.count(needle)==1
  return text.replace(needle,needle+',{ "aabb_variables",(std::uint32_t(regs.rip)==0x309940u?hex(memory(child.pid,stack.at(1).get<std::uint32_t>(),56)):std::string{})}')
 base.physical.observer_source=observer;base.physical.physical=support.parent.physical
 base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases
 text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin)
 text=text[:begin]+'''  def boundary(c,actual):
   if actual['actual_stop']==physical.STOP:return None
   assert actual['actual_stop']==0x309940
   words=struct.unpack('<14I',bytes.fromhex(actual['aabb_variables']))
   return dict(owner=0x309940,cache=actual['boundary_gp'][1],program=actual['boundary_stack'][2],upper=list(words[:3]),lower=list(words[4:7]),translation_and_properties=list(words[8:14]))
'''+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_mopp_aabb_setup.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text()
  driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_mopp_aabb_setup.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({100,15})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15MoppAabbSetupServices services;').replace('NativeTu15ContactPressureStats stats;','').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS)
  begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+NATIVE_SERVICES+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('no_minus_one','return integer-1u;','return integer;'),('wrong_scale','multiply_scalar(m.word(current+12u))','multiply_scalar(m.word(current+8u))'),('upper_high','cache+0x12u','cache+0x22u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('');shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_mopp_aabb_setup.hpp',ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp'")
 calls='''const auto a=c.at("arguments");if(c.at("entry")==0x30a1e0)native_tu15_prepare_mopp_aabb(m,c.at("cache"),a[0],a[1],a[2],fp,0,services,observer);else native_tu15_query_mopp_aabb_shape(m,c.at("receiver"),a[0],a[1],c.at("cache"),fp,0,services,observer);'''
 services='''   services.traverse=[&](auto cache,auto program,const auto&variables,auto&,auto depth){const auto&b=c.at("boundary");check(cache==b.at("cache")&&program==b.at("program")&&depth==0,name+" MOPP AABB boundary formals differ");check(J(variables.upper)==b.at("upper")&&J(variables.lower)==b.at("lower")&&J(variables.translation_and_properties)==b.at("translation_and_properties"),name+" native MOPP AABB variables differ");boundary(0x309940u);};
'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,NATIVE_SERVICES=services);exec(text,env);result=env['main']()
 path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';r=json.loads(path.read_text());code=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(code,p).code)for p in OWNERS));r.update(scope=__doc__,physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),boundary_policy='Only12 original initialized variable words compared; uninitialized alignment words0C/1C absent from native semantic interface. No child return supplied.',production_native_only_closure=False,remaining=['Native309940 traversal and query/publication closure','Positive actual input/service composition, native-only live integration','Nonempty incoming x87, unmasked exceptions and wider alias/lifetime ownership']);base.save(path,r);return result
if __name__=='__main__':raise SystemExit(main())
