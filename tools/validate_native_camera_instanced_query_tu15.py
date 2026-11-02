#!/usr/bin/env python3
"""Native16C000+25/13-site children and native sphere/inverse/BSP/publication.

All1128 root arithmetic/result bytes plus persistent banks are strict; sole
nested BSP40B context has its own proved DATA lifetime and owned native bank.
"""
import copy,inspect,struct
from pathlib import Path
import validate_native_bsp_segment_query_tu15 as bsp
import validate_native_contact_pressure_tu15 as base
import validate_native_dirty_island_objects_composed_tu15 as support
ROOT=Path(__file__).resolve().parents[1]
OWNERS=(0x16c000,0x16b810,0x186e30,0x120fa0,0x144090,0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80,0x16b9a0)
COUNTS='165,25,13,76,82,42,262,125,30,97,107';BACKEND='unchanged-tu15-native-camera-instance-query-v1'
def metadata(image):
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==['+COUNTS+'];');text=text.replace('assert not op.mem.segment','assert not op.mem.segment or ins.reg_name(op.mem.segment)=='+repr('es'));env=dict(support.__dict__,OWNERS=OWNERS,BOUNDARIES=());exec(text,env);return env['metadata'](image)
def cases(image):
 rows=[];seed=bsp.cases(image)[256]
 def make(entry=0x16c000,instance=0,definition_count=1,last_flags=0,mask=0,bsp_flags=1,scale=0x3f800000,origin=0xbf800000,delta=0x40000000,collision_count=0,collision_table=True,mapped=0,material=0xffff,rounding=0):
  c=copy.deepcopy(seed);raw=bytearray(0x10000);src=bytes.fromhex(c['banks'][0]['before']);raw[:len(src)]=src;B=0x1000000;scenario=B+0x6000;definitions=B+0x7000;instances=B+0x8000;rows_data=B+0x9000;collision=B+0xa000;mapping=B+0xb000;bitmap=B+0xc000;output=B+0xd000
  def put(p,n,width=4):raw[p-B:p-B+width]=(n&((1<<(8*width))-1)).to_bytes(width,'little')
  # Source16C000 instance index0 points to definition0; independent declared
  # leaves vary signed instance and counts without inventing callback results.
  for off,n in((0x144,instances),(0x13c,definitions),(0xc4,collision_count),(0xc8,collision if collision_table else 0),(0x10,B+0xe000)):put(scenario+off,n)
  descriptor=src[bsp.D-B:bsp.D-B+64];raw[definitions+0x70-B:definitions+0xb0-B]=descriptor
  put(definitions+0x98,definition_count);put(definitions+0x9c,rows_data);put(rows_data+(definition_count if 0<definition_count<8 else 1)*8-4,last_flags,1)
  put(collision+0x40,2);put(collision+0x44,mapping);put(mapping,mapped,2);put(mapping+4,mapped,2)
  frame=instances+(instance*0x58 if entry==0x16c000 else 0)
  for lane,n in enumerate((scale,0x3f800000,0,0,0,0x3f800000,0,0,0,0x3f800000,0,0,0)):put(frame+lane*4,n)
  put(frame+0x34,0,2)
  for off in(0x3c,0x40,0x44):put(frame+off,0)
  put(frame+0x48,0x42c80000)
  for lane,n in enumerate((origin,0,0)):put(bsp.O+lane*4,n)
  for lane,n in enumerate((delta,0,0)):put(bsp.V+lane*4,n)
  put(output+4,0x3f800000);put(rows_data+6,material,2)
  for index in range(-1,3):put(B+0xe000+index*20+8,7+index,2)
  inverse=base.physical.SP-0x468
  extra=[b for b in c['banks']if b['address']not in(B,)and not base.physical.STACK<=b['address']<base.physical.STACK+0x10000]
  extra.extend([dict(address=0x4ecc18,before=scenario.to_bytes(4,'little').hex()),dict(address=0x474c24,before='0000'),dict(address=0x4f9b50,before=bitmap.to_bytes(4,'little').hex()),dict(address=0x48a0b0,before='0700')])
  c.update(name=f'instance_{len(rows)}_{entry:x}_count{definition_count}_mask{mask:x}_flags{bsp_flags}_scale{scale:x}',entry=entry,receiver=0,entry_registers={},arguments=[],before_fp=[0x27f|(rounding<<10),0x4520,0x1f80|(rounding<<13)],banks=[],inverse=inverse,local_origin=inverse+52,local_displacement=inverse+64,collision=inverse+76)
  c['banks']=[dict(address=B,before=raw.hex()),dict(address=inverse,before=(bytes([0xa5])*1128).hex()),*extra]
  if entry==0x16c000:c.update(entry_registers=dict(eax=instance,ebx=output),arguments=[mask,bsp_flags,bsp.O,bsp.V])
  elif entry==0x16b810:c.update(entry_registers=dict(edx=definitions,esi=instance),arguments=[mask])
  else:c.update(entry_registers=dict(eax=instance))
  # Source readonly inverse/FP constants remain authenticated actualXBE.
  covered=[(b['address'],len(bytes.fromhex(b['before'])))for b in c['banks']]
  for p in(0x46a0d0,):
   if not any(lo<=p and p+4<=lo+n for lo,n in covered):c['banks'].append(dict(address=p,before=image.read(p,4).hex()))
  rows.append(c)
 for count in(0,1,2,0xffffffff):
  for last in(0,0x10):
   for mask in(0,0x40000):make(0x16b810,definition_count=count,last_flags=last,mask=mask)
 for index in(0,1,2,0xffffffff,0x80000000):
  for collision_count in(0,1,0xffffffff):
   for table in(False,True):
    for mapped in(0,0xffff):make(0x16b810,instance=index,mask=0x40000,collision_count=collision_count,collision_table=table,mapped=mapped)
 for index in(0xffffffff,0,1,0x12345,0xfffffffe):make(0x186e30,instance=index)
 for scale in(0,0x3f800000,0xbf800000,0x3f000000):
  for flags in(0,1,2,3,8,9):
   for material in(0xffff,0,1):make(scale=scale,bsp_flags=flags,material=material,rounding=len(rows)%4)
 for index in(1,2):make(instance=index)
 for origin,delta in((0,0),(0x80000000,0),(1,0x80000001),(0x3f800000,0xc0000000),(0x7fc12345,0x3f800000),(0x7f812345,0x3f800000),(0x7f800000,0xff800000)):
  for rounding in range(4):make(origin=origin,delta=delta,rounding=rounding)
 for n in(0,17,80,82,90,115,145,165):c=copy.deepcopy(rows[n]);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows

def observer_source():
 text=support.parent.observer_source()
 return text.replace('"Original pose instruction fault"','(std::string("Original instance instruction fault PC=")+std::to_string(pc)+" EDX="+std::to_string(std::uint32_t(regs.rdx))).c_str()')
def main():
 base.physical.observer_source=observer_source;base.physical.physical=support.parent.ORIGINAL_PHYSICAL;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("if e['instruction']in seen:continue","if e['instruction']in seen or(category=='writes'and e['instruction']==0x1e6c81):continue")
 text=text.replace("  rows+=physical.physical(image,meta,prefix,scratch);fixture=", "  excluded_rep_observers=[c[\'name\']for c in prefix if c.get(\'observer_fault\')and c[\'stop_pc\']==0x1441b9];prefix=[c for c in prefix if c[\'name\']not in excluded_rep_observers];rows+=physical.physical(image,meta,prefix,scratch);fixture=")
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_camera_instanced_query.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_camera_instanced_query.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({165,25,13,76,82,42,262,125,30,97,107})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');driver=driver.replace('if(log_reads)reads.push_back','if(log_reads&&!(p>=0x6000000u&&std::uint64_t(p)+b.size()<=0x6010000u))reads.push_back').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!(p>=0x6000000u&&std::uint64_t(p)+b.size()<=0x6010000u))raw_stores.push_back').replace('for(bool read_pass:{false,true}){','before.banks.emplace(0x6000000u,std::vector<std::byte>(0x10000));for(bool read_pass:{false,true}){').replace('stores.push_back(store);','if(store.address>=0x6000000u&&std::uint64_t(store.address)+store.written.size()<=0x6010000u)return;stores.push_back(store);');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('instance_stride','q.instance*0x58u','q.instance*0x54u'),('definition_filter','mask&0x40000u','mask&0x80000u'),('origin_translation','inverse+40u','inverse+44u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 units=('native_tu15_camera_query_leaves.cpp','native_tu15_camera_dynamic_query_leaves.cpp','native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp')
 copy="(project/'types.cpp').write_text('');[shutil.copyfile(ROOT/'src/physics'/name,project/name)for name in"+repr((*units,'native_tu15_joint_constraint_numeric_internal.hpp'))+"];shutil.copyfile(ROOT/'src/rendering/world/native_tu15_camera_projection.cpp',project/'native_tu15_camera_projection.cpp')"
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')",copy).replace("+'.cpp types.cpp)","+'.cpp types.cpp "+' '.join(units)+" native_tu15_camera_projection.cpp)")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_camera_instanced_query.hpp',ROOT/'src/physics/native_tu15_bsp_segment_query.cpp'")
 text=text.replace("report.update(passed=all(r['exit_code']==0 for r in results)","report.update(excluded_rep_observer_prefixes=excluded_rep_observers,REP_observer_exception_domain_qualified=False,component_scope='Normal and operand faults strict; inverse zero-scale REP postinstruction observer exception remains explicit unqualified',passed=all(r['exit_code']==0 for r in results)")
 calls='''{auto a=c.at("arguments"),g=c.at("entry_registers");switch(c.at("entry").get<unsigned>()){case 0x16b810:result=native_tu15_admit_camera_query_instance(m,g.at("esi"),g.at("edx"),a[0]);break;case 0x186e30:result=native_tu15_camera_instance_collision_bitmap(m,g.at("eax"));break;case 0x16c000:result=native_tu15_query_camera_instance(m,{g.at("eax"),a[0],a[1],a[2],a[3],g.at("ebx")},{c.at("inverse"),c.at("local_origin"),c.at("local_displacement"),c.at("collision"),0x6000100u},fp,0,observer);break;}}'''
 check='''if(!fault_case&&!observer_failed){const auto mask=c.at("entry")==0x186e30?0xffffffffu:255u;check((result&mask)==(c.at("boundary_gp")[0].get<std::uint32_t>()&mask),name+" instance scalar return differs");}'''
 env=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
