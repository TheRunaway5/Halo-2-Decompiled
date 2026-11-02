#!/usr/bin/env python3
"""Same native1F6D10 query binding, two MOPP query kinds and visibility owners.

Compare complete original returns and persistent ordered DATA/FP, with no
supplied child outcomes. The native call-local cache is initialized by its
native setup and read-guarded through traversal. Private cache/CPU frame bytes
are not retained outputs or inputs; their raw observer/address-fault domain
is explicitly unqualified.
"""
import inspect,json,struct
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
BACKEND='unchanged-tu15-native-collision-key-query-binding-v1'
OWNERS=(0x1f6d10,0x1f6fa0,0x1f76a0,0x186e70,0x2e9670,0x30a1e0,0x309940,0x2e9640,0x30a020,0x2e4550)
P=((base.physical.SP-4)&~15)-0xf4-12
def metadata(image):
 code=_Code(image,'.text');registers={name:n for n,name in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];pins={}
 for entry in OWNERS:
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   ins=code.at(pc);listing.append(f'{pc:x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}\n');ops=[]
   for op in ins.operands:
    if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue
    assert not op.mem.segment or ins.reg_name(op.mem.segment)=='es'
    access=CS_AC_READ if ins.mnemonic in('test','cmp')else CS_AC_WRITE if ins.mnemonic in('fst','fstp','fistp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),ins.mnemonic,ins.op_str)
    ops.append(dict(base=registers.get(ins.reg_name(op.mem.base),-1),index=registers.get(ins.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  pins[hex(entry)]=base.hashlib.sha256(''.join(listing).encode()).hexdigest()
 return dict(sites=sites,body_sites=counts,listing_sha256=pins,boundaries=[],regions=[])
def cases(image):
 rows=[]
 for instance in(False,True):
  for scene_count in(0,1,8192,8193):
   for visible in(False,True):
    for rounding in range(4):
     data=bytearray(0x9000)
     def put(p,v,n=4):data[p-0x1000000:p-0x1000000+n]=(v&((1<<(8*n))-1)).to_bytes(n,'little')
     shape,bounds,keys=0x1000100,0x1000200,0x1000300
     put(shape+16,0x1000800);put(keys,0x1001000);put(keys+4,0);put(keys+8,0x80000020)
     for k in range(3):put(bounds+k*4,0);put(bounds+16+k*4,0x41200000)
     for code in(0x1000800,0x1003800):put(code+12,0x47800000)
     primary=[11,0x40 if instance else 0x20,0,0,0,0x10,128,0,1,0x30,0x31]
     secondary=[0x10,128,0,1,0x30,0x31]
     data[0x830:0x830+len(primary)]=bytes(primary);data[0x3830:0x3830+len(secondary)]=bytes(secondary)
     put(0x1002000+0x144,0x1002200);put(0x1002000+0x13c,0x1002400)
     put(0x1002400+0x98,scene_count);put(0x1002400+0x9c,0x1003600);put(0x1002400+0xb4,0x1003400)
     put(0x1003450,0x461e24);put(0x1003460,0x1003800)
     for slot in range(2):
      at=0x1002200+slot*0x58
      for k in range(12):put(at+4+k*4,0x3f800000 if k in(0,4,8)else 0)
      put(at+0x34,0,2)
      put(0x1003200+slot*8+4,8,1);put(0x1003200+slot*8+5,0,1)
      put(0x1003600+slot*8+4,8,1);put(0x1003600+slot*8+5,0,1)
     put(0x1003000+0x2c,0x1003200);put(0x1005001,1 if visible else 0)
     for slot in range(2):put(0x1005201+slot,1 if visible else 0)
     globals=[(0x4ecc10,0x1003000,4),(0x4ecc18,0x1002000,4),(0x4f9b50,0x1005000,4),(0x474c24,0,2)]
     banks=[dict(address=0x1000000,before=data.hex()),dict(address=P+0x20,before=bytes([0xa5 if rounding&1 else 0x5a])*0xe0),dict(address=0x461950,before=image.read(0x461950,32).hex()),dict(address=0x461e24,before=image.read(0x461e24,52).hex()),dict(address=0x46a0cc,before=image.read(0x46a0cc,8).hex()),*[dict(address=p,before=v.to_bytes(n,'little').hex())for p,v,n in globals]]
     # The source workspace preimage is explicit retained matrix DATA, with
     # no return addresses, saved GP state or cache/control locals included.
     for b in banks:
      if isinstance(b['before'],bytes):b['before']=b['before'].hex()
     n=len(rows);rows.append(dict(name=f'query_instance{int(instance)}_surfaces{scene_count}_visible{int(visible)}_round{rounding}',entry=0x1f6d10,receiver=shape,arguments=[bounds,keys],matrices=P,native_cache=0x7000000,banks=sorted(banks,key=lambda b:b['address']),before_fp=[(0x7f,0x27f,0x37f)[n%3]|(rounding<<10),((n%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None))
 for index in(0,4,32,36):c=dict(rows[index]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def main():
 inputs=[Path(__file__),ROOT/'tools/validate_native_contact_pressure_tu15.py',ROOT/'tests/native_tu15_contact_pressure_tests.cpp',ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp']
 for name in('collision_key_query','collision_component','collision_surface_shape','inverse_geometry','mopp_aabb_setup','mopp_aabb_traversal','mopp_oriented_aabb_setup','array_growth'):
  inputs.extend([ROOT/f'src/physics/native_tu15_{name}.cpp',ROOT/f'include/halo2/native_tu15_{name}.hpp'])
 inputs.extend([ROOT/'include/halo2/native_tu15_collision_key_query_binding.hpp',ROOT/'include/halo2/native_tu15_mopp_aabb_private_cache.hpp'])
 input_pins={str(p.relative_to(ROOT)):base.sha(p)for p in inputs}
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical
 base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases
 text=inspect.getsource(base.main);begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin)
 text=text[:begin]+'''  def boundary(c,actual):
   assert actual['actual_stop']==physical.STOP,hex(actual['actual_stop']);return None
'''+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_collision_key_query.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text()
  driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_collision_key_query_binding.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({'+','.join(str(n)for n in meta['body_sites'])+'})').replace('auto services=native_tu15_contact_pressure_native_services();','NativeTu15ArrayGrowthServices services;').replace('NativeTu15ContactPressureStats stats;','NativeTu15CollisionKeyQueryBindingActivity stats;').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS)
  begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('wrong_shift','value=m.word(destination+4u)','value=m.word(destination)'),('wrong_count','const auto count=m.word(array+4u)-1u','const auto count=m.word(array+4u)-2u'),('wrong_instance_key','((child|0xffffa000u)<<16u)|instance_index','((child|0xffffa000u)<<16u)|(instance_index^1u)')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 setup="shutil.copyfile(ROOT/'src/physics/native_tu15_collision_component.cpp',project/'types.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',project/'native_tu15_joint_constraint_numeric_internal.hpp')"
 for name in('collision_surface_shape','inverse_geometry','mopp_aabb_setup','mopp_aabb_traversal','mopp_oriented_aabb_setup','array_growth'):
  setup+=f";shutil.copyfile(ROOT/'src/physics/native_tu15_{name}.cpp',project/'{name}.cpp')"
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')",setup)
 text=text.replace("+'.cpp types.cpp)","+'.cpp types.cpp collision_surface_shape.cpp inverse_geometry.cpp mopp_aabb_setup.cpp mopp_aabb_traversal.cpp mopp_oriented_aabb_setup.cpp array_growth.cpp)")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_collision_key_query.hpp',ROOT/'include/halo2/native_tu15_collision_key_query_binding.hpp',ROOT/'include/halo2/native_tu15_mopp_aabb_private_cache.hpp'")
 calls='native_tu15_query_bound_collision_keys(m,c.at("receiver"),c.at("arguments")[0],c.at("arguments")[1],{{c.at("matrices")},c.at("native_cache")},fp,0,services,{observer},&stats);'
 env=dict(base.__dict__,__file__=__file__,CALLS=calls);exec(text,env);result=env['main']()
 path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';r=json.loads(path.read_text());code=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(code,p).code)for p in OWNERS));unchanged=all(base.sha(ROOT/p)==value for p,value in input_pins.items());r['source_hashes'].update(input_pins);r.update(scope=__doc__,all_pinned_inputs_unchanged=unchanged,physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),parent_executed_sites=len(set(complete_body(code,0x1f6d10).code)&support.parent.EXECUTED),production_native_only_closure=False,remaining=['Positive growth/allocator composition','Actual retained input,2F36F0/304350 closure and live delivery','Raw private-cache/preimage observer or address-fault contract, unknown instance owners, wider alias/FP domains']);r['passed']=bool(r['passed']and unchanged);base.save(path,r);return result if unchanged else 1
if __name__=='__main__':raise SystemExit(main())
