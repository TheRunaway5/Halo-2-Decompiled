#!/usr/bin/env python3
"""Whole16BB40 geometry selection and native1F7E00/1F8040, returns/FP/ordered DATA and faults."""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x16bb40,0x1f7e00,0x1f8040)
BACKEND='unchanged-tu15-native-camera-query-object-context-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[45,42,19];')
 text=text.replace("if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue","if op.type!=X86_OP_MEM or ins.mnemonic in('lea','prefetcht0'):continue")
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(entry,kind=4,model=2,collision=3,encoded=0x01020355,region_offset=0x100,node_offset=0x180,alias=0,index=0):
  data=bytearray((n*37+11)&255 for n in range(0x6000));packet=0x1000100;objects=0x1000400;headers=0x1001000;body=0x1002000;object_tag=0x1002800;model_tag=0x1002a00;collision_tag=0x1002c00;scenario=0x1002e00;instances=0x1003000;sections=0x1003400;regions=0x1003600;permutations=0x1003800;tags=0x1004000;output=0x1000600;context=0x1000700;datum=0xdddd0001
  if alias==1:output=body
  if alias==2:output=tags+16
  if alias==3:output=context
  def put(at,value,width=4):data[at-0x1000000:at-0x1000000+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
  put(objects+0x44,headers);put(headers+20,body);put(body,0xaaaa0001);put(body+0x11a,region_offset,2);put(body+0x116,node_offset,2)
  put(tags+16+8,object_tag);put(tags+32+8,model_tag);put(tags+48+8,collision_tag);put(object_tag+0x38,model);put(model_tag+12,collision)
  put(collision_tag+0x20,sections)
  for n in range(4):put(sections+n*12+8,regions)
  for n in range(4):put(regions+n*20+8,permutations+n*0x40)
  put(context+8,collision_tag);put(packet,kind);put(packet+0x40,datum);put(packet+0x48,encoded);put(packet+0x3c,index)
  put(scenario+0x144,instances);put(scenario+0x13c,0x1004800)
  for n in range(4):put(instances+n*0x58+0x34,(0,1,0xffff,0x8000)[n],2)
  c=dict(name=f'context_{len(rows)}_{entry:x}_kind{kind}_alias{alias}',entry=entry,receiver=0,arguments=[],entry_registers={},banks=[dict(address=0x1000000,before=data.hex()),*[dict(address=p,before=struct.pack('<I',v).hex())for p,v in((0x4ecbd0,objects),(0x4f0414,tags),(0x4ecc18,scenario),(0x4ecc10,0x12345678))]],before_fp=[0x27f|((len(rows)%4)<<10),((len(rows)%8)<<11)|0x453f,0x1f80|((len(rows)%4)<<13)],stop_pc=base.physical.STOP,boundary=None)
  if entry==0x16bb40:c['entry_registers']=dict(edi=packet)
  elif entry==0x1f7e00:c.update(arguments=[datum],entry_registers=dict(esi=output))
  else:c['arguments']=[context,encoded]
  rows.append(c)
 for kind in(0,1,2,3,4,5,0xffffffff):
  for index in range(4):
   for model,collision in((2,3),(0xffffffff,3),(2,0xffffffff)):make(0x16bb40,kind=kind,index=index,model=model,collision=collision)
 for region in(0,0x100,0xffff,0x8000):
  for node in(0,0x180,0xffff,0x8000):
   for alias in range(4):make(0x1f7e00,region_offset=region,node_offset=node,alias=alias)
 for model,collision in((0xffffffff,3),(2,0xffffffff)):make(0x1f7e00,model=model,collision=collision)
 for section in range(4):
  for region in range(4):
   for permutation in(0,1,255):make(0x1f8040,encoded=(section<<24)|(region<<16)|(permutation<<8)|0x55)
 for seed in(rows[48],rows[84],rows[148],rows[-1]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_camera_query_object_context.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_camera_query_object_context.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({45,42,19})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('model_reference','model_datum==0xffffffffu','model_datum!=0xffffffffu'),('region_stride','*0x44u','*0x40u'),('kind_selection','kind==3u','kind==2u')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_camera_query_object_context.hpp'")
 calls='''switch(c.at("entry").get<unsigned>()){
     case 0x16bb40:result=native_tu15_camera_query_collision_geometry(m,c.at("entry_registers").at("edi"));break;
     case 0x1f7e00:result=native_tu15_resolve_camera_collision_context(m,c.at("arguments")[0],c.at("entry_registers").at("esi"),observer);break;
     case 0x1f8040:result=native_tu15_camera_collision_region(m,c.at("arguments")[0],c.at("arguments")[1]);break;
    }'''
 check='''if(!fault_case&&!observer_failed){const auto mask=c.at("entry")==0x1f7e00?0xffu:0xffffffffu;check((result&mask)==(c.at("boundary_gp")[0].get<std::uint32_t>()&mask),name+" geometry/context return differs");}'''
 environment=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());codebook=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(codebook,owner).code)for owner in OWNERS));report.update(physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),prefetch_policy='OriginalPREFETCHT0 unchanged/nonfaulting; no semantic memory read invented. Surrounding architectural pool stride/base reads remain strict.',context_return_contract='Source16BB40 reads1F7E00 AL; geometry/region full32bits checked',scope=__doc__,remaining=['Actual16C620 query parent/BSP/object closure and real input/currentFP composition','Production native routing/typed service composition and visible camera/strictOriginal']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
