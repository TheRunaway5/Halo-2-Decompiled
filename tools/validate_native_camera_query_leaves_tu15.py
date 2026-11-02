#!/usr/bin/env python3
"""Five source-owned camera query leaves, returns/FP/ordered DATA and faults."""
import inspect,json,struct
from pathlib import Path
import validate_native_dirty_island_objects_composed_tu15 as support
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import _Code
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x16b940,0x16e360,0xba970,0xba9e0,0x120fa0)
BACKEND='unchanged-tu15-native-camera-query-leaves-v1'
ROOT=Path(__file__).resolve().parents[1]
def metadata(image):
 support.OWNERS=OWNERS;support.BOUNDARIES=()
 text=inspect.getsource(support.metadata).replace('assert counts==[276,38,20];','assert counts==[29,30,36,33,76];')
 text=text.replace("if op.type!=X86_OP_MEM or ins.mnemonic=='lea':continue","if op.type!=X86_OP_MEM or ins.mnemonic in('lea','prefetcht0'):continue")
 environment=dict(support.__dict__);exec(text,environment);return environment['metadata'](image)
def cases(image):
 rows=[]
 def make(entry,flags=0,cluster=0,node=0xabcd0001,next=0xbcde0002,alias=0,geometry=None,rounding=0):
  data=bytearray((n*37+11)&255 for n in range(0x5000));root=0x1001000;pool=0x1000800;stride=32;body=0x1002000;iterator=0x1000300;payload=0x1000400
  if alias==1:payload=iterator
  if alias==2:payload=pool+0x24
  if alias==3:iterator=body+36
  if alias==4:payload=body+40
  def put(at,value):struct.pack_into('<I',data,at-0x1000000,value&0xffffffff)
  for n in range(-2,4):put(root+n*4,node)
  put(pool+0x44,body);put(pool+0x24,stride);put(iterator,node)
  for n in range(4):put(body+n*stride+4,0xdada0000+n);put(body+n*stride+8,next)
  banks=[dict(address=0x1000000,before=data.hex()),dict(address=0x4eabb0,before=struct.pack('<I',root).hex()),dict(address=0x4eabb4,before=struct.pack('<I',pool).hex())]
  c=dict(name=f'query_{len(rows)}_{entry:x}_alias{alias}',entry=entry,receiver=flags,arguments=[],entry_registers={},banks=banks,before_fp=[0x27f,((len(rows)%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None)
  if entry==0xba970:c.update(arguments=[cluster,payload],entry_registers=dict(ebx=iterator))
  elif entry==0xba9e0:c.update(arguments=[payload],entry_registers=dict(ebx=iterator))
  elif entry==0x120fa0:
   first,center,direction,radius=geometry
   for at,values in((0x1003000,first),(0x1003100,center),(0x1003200,direction)):
    for lane,value in enumerate(values):put(at+lane*4,value)
   c.update(receiver=0x1003100,entry_registers=dict(eax=0x1003000,edx=0x1003200),arguments=[radius]);c['banks'][0]['before']=data.hex()
  rows.append(c)
 for mask in range(64):
  for noise in(0,0xe07fffff):make(0x16b940,flags=(mask<<23)|noise)
 positions=(19,18,30,20,22,21)
 for mask in range(64):
  flags=sum(1<<position for lane,position in enumerate(positions)if mask&(1<<lane))
  for noise in(0,0x80000001):make(0x16e360,flags=flags|noise)
 for entry in(0xba970,0xba9e0):
  for cluster in(0,1,0xffff,0xfffffffe):
   for node in(0xffffffff,0xabcd0001):
    for next in(0xffffffff,0xbcde0002):make(entry,cluster=cluster,node=node,next=next)
  for alias in range(1,5):make(entry,alias=alias)
 z=(0,0,0);p=(0x40000000,0,0);geometries=[(z,p,(0xc0400000,0,0),0x3f800000),(z,p,(0x40400000,0,0),0x3f800000),(z,z,z,0x3f800000),(z,p,(0xbf800000,0x3f800000,0),0x3f800000),(z,p,(0xbf800000,0,0),0x3f800000),(z,p,z,0x3f800000)]
 for value in(0,0x80000000,1,0x80000001,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345,0xff854321):
  geometries.append(((value,0x3f000000,0xbe800000),(0x3f800000,value,0x40000000),(0xbf800000,0,value),value))
 for geometry in geometries:
  for rounding in range(4):make(0x120fa0,geometry=geometry,rounding=rounding)
 for seed in(rows[256],rows[268],rows[288],rows[-1],rows[-60]):c=dict(seed);c['name']='fault_seed_'+seed['name'];rows.append(c)
 return rows
def main():
 base.physical.observer_source=support.parent.observer_source;base.physical.physical=support.parent.physical;base.OWNERS=OWNERS;base.BACKEND=BACKEND;base.metadata=metadata;base.cases=cases;text=inspect.getsource(base.main)
 begin=text.index('  def boundary(c,actual):');end=text.index('  physical.boundary=boundary',begin);text=text[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+text[end:]
 text=text.replace("source=ROOT/'src/physics/native_tu15_contact_pressure.cpp'","source=ROOT/'src/physics/native_tu15_camera_query_leaves.cpp'")
 injected='''driver=(ROOT/'tests/native_tu15_contact_pressure_tests.cpp').read_text();driver=driver.replace('#include "halo2/native_tu15_contact_pressure.hpp"','#include "halo2/native_tu15_camera_query_leaves.hpp"').replace('#include "halo2/native_tu15_contact_pressure_binding.hpp"','').replace('unchanged-tu15-native-contact-pressure-v1',BACKEND).replace('J::array({220,2,2,2})','J::array({29,30,36,33,76})').replace('auto services=native_tu15_contact_pressure_native_services();','').replace('NativeTu15ContactPressureStats stats;','std::uint32_t result{};').replace('native_tu15_prune_contact_pressure(m,fp,services,observer,&stats);',CALLS);begin=driver.index('   services.select_candidate=');end=driver.index('   const auto cw=',begin);driver=driver[:begin]+driver[end:];driver=driver.replace('   check(failed==fault_case',RETURN_CHECK+'   check(failed==fault_case');(project/'driver.cpp').write_text(driver);variants={}'''
 text=text.replace("shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}",injected)
 begin=text.index('  for name,needle,replacement in [');end=text.index("\n  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp'",begin)
 text=text[:begin]+'''  for name,needle,replacement in [('bsp_mask','(flags>>23u)&0x3fu','(flags>>23u)&0x1fu'),('iterator_stride','(node&0xffffu)*stride','(node&0xffffu)*(stride+4u)'),('sphere_endpoint','return compare(discriminant,mul(endpoint,endpoint)).above();','return !compare(discriminant,mul(endpoint,endpoint)).above();')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+text[end:]
 text=text.replace("shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')","(project/'types.cpp').write_text('')")
 text=text.replace("ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp'","ROOT/'include/halo2/native_tu15_camera_query_leaves.hpp'")
 calls='''switch(c.at("entry").get<unsigned>()){
     case0x16b940:result=native_tu15_camera_bsp_query_flags(c.at("receiver"));break;
     case0x16e360:result=native_tu15_camera_object_query_flags(c.at("receiver"));break;
     case0xba970:result=native_tu15_begin_spatial_object_query(m,c.at("arguments")[0],c.at("entry_registers").at("ebx"),c.at("arguments")[1],observer);break;
     case0xba9e0:result=native_tu15_next_spatial_object_query(m,c.at("entry_registers").at("ebx"),c.at("arguments")[0],observer);break;
     case0x120fa0:result=native_tu15_segment_near_sphere(m,c.at("entry_registers").at("eax"),c.at("receiver"),c.at("entry_registers").at("edx"),c.at("arguments")[0],fp);break;
    }'''.replace('case0x','case 0x')
 check='''if(!fault_case&&!observer_failed){const auto mask=c.at("entry")==0x120fa0?0xffu:0xffffffffu;check((result&mask)==(c.at("boundary_gp")[0].get<std::uint32_t>()&mask),name+" scalar/AL return differs");}'''
 environment=dict(base.__dict__,__file__=__file__,CALLS=calls,RETURN_CHECK=check);exec(text,environment);code=environment['main']();path=Path(base.sys.argv[base.sys.argv.index('--output')+1])/'report.json';report=json.loads(path.read_text());codebook=_Code(base.Xbe(base.NEW_DEFAULT),'.text');sites=set().union(*(set(complete_body(codebook,owner).code)for owner in OWNERS));report.update(physically_executed_sites=len(sites&support.parent.EXECUTED),unexecuted_sites=sorted(sites-support.parent.EXECUTED),prefetch_policy='OriginalPREFETCHT0 unchanged/nonfaulting; no semantic memory read invented. Surrounding architectural pool stride/base reads remain strict.',sphere_return_contract='Source16C620 caller reads AL only; pure flag/iterator returns checked full32bits',scope=__doc__,remaining=['Actual16C620 query parent/BSP/object closure and real input/currentFP composition','Production native routing/typed service composition and visible camera/strictOriginal']);base.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
