#!/usr/bin/env python3
"""Direct real2E5DB0/Sphere support, all phases including native FSTP faults."""
import copy,inspect,struct,json,sys
from pathlib import Path
import validate_native_moving_shape_process_tu15 as base
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-sweep-support-fault-phase-stage-20261102/staged'
def cases(image):
 rows=[]
 def make(sweep,direction,precision=2,rounding=0,alias=0):
  first=0x1000000;data=bytearray((n*17+3)&255 for n in range(0x1000));shape=first+0x100;child=first+0x200;table=first+0x300;input=first+0x400;output=first+0x500
  if alias==1:output=shape+0x10
  if alias==2:output=input
  if alias==3:output=shape+0x30
  def put(p,n):struct.pack_into('<I',data,p-first,n)
  put(shape+0x30,child);put(child,table);put(table+0x30,0x2e8d20)
  for lane,n in enumerate(sweep):put(shape+0x10+lane*4,n)
  for lane,n in enumerate(direction):put(input+lane*4,n)
  n=len(rows);rows.append(dict(name=f'sweep_{n}_alias{alias}',entry=0x2e5db0,receiver=shape,arguments=[input,output],relative=0,banks=[dict(address=first,before=data.hex()),dict(address=0x46a0e8,before=image.read(0x46a0e8,4).hex())],before_fp=[0x7f|(precision<<8)|(rounding<<10),((n%8)<<11)|0x453f,0x1f80|(rounding<<13)],stop_pc=base.physical.STOP,boundary=None))
 patterns=[((0x3f800000,0,0,0),(0x3f800000,0,0,0)),((0x3f800000,0x3e000001,0xbe800001,0x3f000001),(0xbf800000,0x3e800001,0,0xbf000001)),((0,0,0,0),(0,0,0,0))]
 for value in(0x80000000,1,0x80000001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345):patterns.append(((value,0x3f000001,0xbe800001,value),(0x3f800000,0x3e800001,0xbf000001,value)))
 for sweep,direction in patterns:
  for precision in(0,2,3):
   for rounding in range(4):make(sweep,direction,precision,rounding)
 for alias in(1,2,3):make(*patterns[0],alias=alias)
 for seed in(rows[0],rows[12],rows[24]):c=copy.deepcopy(seed);c['name']='fault_seed_'+c['name'];rows.append(c)
 return rows
def driver_source():
 text=(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp').read_text().replace('#include "halo2/native_tu15_moving_shape_process.hpp"','#include "halo2/native_tu15_contact_shape.hpp"')
 text=text.replace('fixture.at("body_sites")[0]==274','fixture.at("body_sites")==J::array({43,9})')
 text=text.replace('struct Fault{};','constexpr std::uint32_t PRIVATE=0x6000000u;bool native_private(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=16;}struct Fault{};')
 text=text.replace('if(log_reads)reads.push_back','if(log_reads&&!native_private(p,b.size()))reads.push_back').replace('if(log_writes)raw_stores.push_back','if(log_writes&&!native_private(p,b.size()))raw_stores.push_back')
 text=text.replace('const auto fault=c.value("fault",J{});','before.banks.emplace(PRIVATE,std::vector<std::byte>(16,std::byte{0xa5}));const auto fault=c.value("fault",J{});')
 begin=text.index('   services.classify_shape=');end=text.index('   const auto cw=',begin);text=text[:begin]+text[end:]
 old='native_tu15_process_moving_shape_contact(m,{c.at("receiver")},{c.at("arguments")[0]},{c.at("arguments")[1]},{c.at("arguments")[2]},c.at("arguments")[3],{c.at("relative")},fp,0,services,observer,&stats);'
 new='SourcePhysicsContactRecordObserver source_observer;if(observer)source_observer=[&](const auto&event){if(!native_private(event.address,event.written.size()))observer(event);};const NativeTu15ContactShapeScratch child{PRIVATE+4u,0,nullptr},scratch{PRIVATE,0,&child};native_tu15_contact_shape_support(m,{c.at("receiver")},{0x2e5db0u},c.at("arguments")[0],c.at("arguments")[1],scratch,fp,0,source_observer);'
 assert text.count(old)==1;text=text.replace(old,new).replace('NativeTu15MovingShapeProcessServices services;','').replace('NativeTu15MovingShapeProcessStats stats;','');return text
def main():
 base.OWNERS=(0x2e5db0,0x2e8d20);base.BOUNDARIES=();base.cases=cases
 meta=inspect.getsource(base.metadata).replace('counts[0]==274','counts==[43,9]');env=dict(base.__dict__);exec(meta,env);base.metadata=env['metadata']
 source=inspect.getsource(base.main);begin=source.index('  def boundary(c,actual):');end=source.index('  physical.boundary=boundary',begin);source=source[:begin]+"  def boundary(c,actual):\n   assert actual['actual_stop']==physical.STOP;return None\n"+source[end:]
 source=source.replace("source=ROOT/'src/physics/native_tu15_moving_shape_process.cpp'","source=STAGE/'src/physics/native_tu15_contact_shape.cpp'").replace("shutil.copyfile(ROOT/'tests/native_tu15_moving_shape_process_tests.cpp',project/'driver.cpp')","(project/'driver.cpp').write_text(driver_source())")
 begin=source.index('  for name,needle,replacement in(');end=source.index('\n  (project/\'negative.py\')',begin);source=source[:begin]+'''  old_load='asm volatile("flds %0"::"m"(value):"memory","st");'+chr(10)+'        const auto zero=m.word(0x46a0e8u);'
  early_load='const auto zero=m.word(0x46a0e8u);'+chr(10)+'        asm volatile("flds %0"::"m"(value):"memory","st");'
  rollback='asm volatile("frstor %0"::"m"(before):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");throw;'
  for name,needle,replacement in(('early_zero_read',old_load,early_load),('fstp_no_rollback',rollback,'throw;'),('sweep_branch','if(!(relation&0x4100u))','if(relation&0x4100u)'),('wrong_vector_sum','output,add(vertex,sweep)','output,subtract(vertex,sweep)')):
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
'''+source[end:]
 begin=source.index("  (project/'negative.py').write_text(");end=source.index('\n  units=',begin)
 negative='''import json,pathlib,subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
name=pathlib.Path(sys.argv[1]).name
assert r.returncode!=0,(r.stdout,r.stderr)
if name=='moving_early_zero_read':assert 'reads_0x2e5dfb complete moving-shape FP differs' in r.stderr,r.stderr
if name=='moving_fstp_no_rollback':assert 'writes_0x2e5e14 complete moving-shape FP differs' in r.stderr,r.stderr
out=pathlib.Path(OUTPUT)/'mutation-diagnostics';out.mkdir(exist_ok=True)
(out/(pathlib.Path(sys.argv[1]).parent.parent.name+'-'+name+'.json')).write_text(json.dumps(dict(rejected=True,exit_code=r.returncode,diagnostic=r.stderr[-4096:]),indent=2)+'\\n')
print(r.stderr[-4096:])
'''
 source=source[:begin]+"  (project/'negative.py').write_text("+repr(negative)+".replace('OUTPUT',repr(str(a.output))))"+source[end:]
 source=source.replace("units=('native_tu15_contact_query.cpp','native_tu15_contact_agent_factory.cpp','native_tu15_activation_services.cpp','native_tu15_inverse_geometry.cpp','native_tu15_geometry.cpp')","units=()")
 source=source.replace("source,ROOT/'include/halo2/native_tu15_moving_shape_process.hpp'","source,ROOT/'include/halo2/native_tu15_contact_shape.hpp'")
 source=source.replace('complete_body(code,0x2e5320)','complete_body(code,0x2e5db0)').replace("remaining=['Returned cache and2F8B60 child closure, parent epilogue'","scope=__doc__,remaining=['Wider non-sweep support families'")
 source=source.replace('negative_controls=list(variants),','negative_controls=list(variants),critical_fault_cases=[c["name"]for c in rows if c.get("fault")and c["stop_pc"]in(0x2e5dfb,0x2e5e14)],')
 environment=dict(base.__dict__,STAGE=STAGE,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
