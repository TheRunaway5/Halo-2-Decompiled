#!/usr/bin/env python3
"""Qualify post-integration listener cohorts against unchanged physical TU1.5.

Real default callbacks execute; custom owners stop at their genuine entry.
No listener result, memory postimage or guest register result is supplied.
Compiler products and operand banks belong to run_test.py scratch.
"""
import argparse, copy, json, os, shutil, struct, subprocess, sys
from pathlib import Path
from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_activation_tu15 as activation
import validate_native_geometry_tu15 as reference
ROOT=reference.ROOT
BASE,WORLD,ISLAND,STEP,ARRAY,LISTENER,TABLE=0x1000000,0x1000100,0x1000500,0x1000600,0x1001000,0x1002000,0x1003000
ENTRIES={0x322820:55,0x322950:49,0x3161f0:2,0x8fdb0:1}
def metadata(image):
 code=reference._Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];hashes={}
 for entry,count in ENTRIES.items():
  body=reference.complete_body(code,entry);assert len(body.code)==count;listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment
    b,x=i.reg_name(op.mem.base),i.reg_name(op.mem.index)
    ops.append(dict(base=names.get(b,-1),index=names.get(x,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  hashes[hex(entry)]=reference.hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert image.read(0x8fdb0,3)==bytes.fromhex('c20800')
 return dict(sites=sites,body_sites=list(ENTRIES.values()),listing_sha256=hashes,stop=reference.STOP)
def cases(image):
 rows=[]
 def make(entry,pattern,final=0,count=None,seed=False):
  raw=bytearray((n*19+7)&255 for n in range(0x4000))
  def put(p,v):struct.pack_into('<I',raw,p-BASE,v&0xffffffff)
  array_offset,count_offset=(0x84,0x88)if entry==0x322820 else(0x9c,0xa0)
  put(WORLD+array_offset,ARRAY);put(WORLD+count_offset,len(pattern)if count is None else count)
  put(TABLE+4,0x8fdb0);put(TABLE+0x14,0x2e4d60)
  for n,kind in enumerate(pattern):
   put(ARRAY+n*4,LISTENER+n*0x10 if kind else 0)
   put(LISTENER+n*0x10,TABLE if kind==1 else TABLE+0x10)
  put(WORLD+0xc0,LISTENER+0x100 if final else 0);put(LISTENER+0x100,TABLE if final==1 else TABLE+0x10)
  n=len(rows);depth=(0,1,3,8)[n%4];live=[struct.pack('<QH',0x8000000000000000+(k<<58),0x3fff+k%3|(0x8000 if k%2 else 0)).hex()for k in range(depth)]
  gp=[0x5a010000+n*31+k*7 for k in range(8)];gp[1]=WORLD;gp[4]=reference.SP
  row=dict(id=n,name=('fault_seed_'if seed else'cohort_')+f'{entry:x}_{n}',entry=entry,world=WORLD,island=ISLAND,body=0,arguments=[WORLD,*([ISLAND]if entry==0x322950 else[]),STEP],gp=gp,before_fp=[0x7f|((0,2,3)[n%3]<<8)|((n%4)<<10),((n%8)<<11)|((n%4)<<8)|(n*7%64),0x1f80|((n%4)<<13)|((0,0x40,0x8000,0x8040)[n%4])],live_x87=live,banks=[dict(address=BASE,before=raw.hex()),dict(address=0x41d000,before=image.read(0x41d000,0x1000).hex()),dict(address=0x556000,before=bytes(0x1000).hex())],alignment_fault=False)
  if 2 in pattern or(entry==0x322820 and final==2):row['required_owner']=dict(owner=0x2e4d60,argument_count=2)
  rows.append(row)
 patterns=((),(0,),(1,),(0,1),(1,0),(0,1,0,1),(1,0,1,0),(0,0,0),(1,1,1))
 for entry in (0x322820,0x322950):
  for repeat in range(4):
   for pattern in patterns:
    for final in ((0,1)if entry==0x322820 else(0,)):make(entry,pattern,final)
  for count in (0xffffffff,0x80000001):make(entry,(),count=count)
  for pattern in ((2,),(0,1,2),(2,0,1)):make(entry,pattern)
  make(entry,(0,1,0,1),final=int(entry==0x322820),seed=True)
 make(0x322820,(0,1,0),final=2)
 return rows
def driver():
 text=activation.DRIVER.replace('J::array({45,57,18,98,37,1})','J::array({55,49,2,1})')
 begin=text.index('NativeTu15IslandActivationStats stats;');end=text.index('   Host host;',begin)
 text=text[:begin]+'''NativeTu15WorldCollisionListenerServices services;J required=nullptr;
   const auto stop=[&](auto listener,auto owner,auto subject,auto step){required=J{{"owner",owner.identity},{"receiver",listener.identity},{"arguments",J::array({subject.identity,step.identity})}};throw OwnerStop{};};
   services.notify=[&](auto listener,auto owner,auto world,auto step,auto&){stop(listener,owner,world,step);};
   services.notify_island=[&](auto listener,auto owner,auto island,auto step,auto&){stop(listener,owner,island,step);};
'''+text[end:]
 begin=text.index('switch(entry){');end=text.index('   }catch(const Prefix&)',begin)
 text=text[:begin]+'''switch(entry){
    case 0x322820:native_tu15_notify_world_step_listeners(m,world,{test.at("arguments").back()},fp,services,observer);break;
    case 0x322950:native_tu15_notify_island_step_listeners(m,world,island,{test.at("arguments").back()},fp,services,observer);break;
    default:throw std::runtime_error("Unexpected actual step-listener entry");}
'''+text[end:]
 start=text.index('   if(test.at("entry")==0x283450');end=text.index('\n',start);text=text[:start]+text[end:]
 return text.replace('activation','step-listener')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 source=ROOT/'src/physics/native_tu15_world_step_listeners.cpp';files=[source,ROOT/'include/halo2/native_tu15_world_step_listeners.hpp',ROOT/'tools/validate_native_island_activation_tu15.py',ROOT/'tools/validate_native_geometry_tu15.py',ROOT/'tests/native_tu15_shape_bounds_tests.cpp',ROOT/'tools/observe_native_geometry_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):reference.sha(f)for f in files};report=dict(passed=False,source_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);meta=metadata(image);owned=activation.prepare(image,meta,scratch);rows=reference.physical(owned,cases(image),'complete');faults=[]
  for seed in [c for c in rows if c['name'].startswith('fault_seed')]:
   for phase,events in (('before_read',seed['reads']),('before_write',seed['writes']),('after_write',seed['writes'])):
    for n,event in enumerate(events):
     c=copy.deepcopy(seed);c.update(id=len(rows)+len(faults),name=seed['name']+'_'+phase+'_'+str(n),fault=dict(phase=phase,ordinal=n));faults.append(c)
  rows+=reference.physical(owned,faults,'prefix');fixture=scratch/'listeners.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,body_sites=meta['body_sites'],cases=rows),separators=(',',':')))
  project=scratch/'project';project.mkdir();prefix=(ROOT/'tests/native_tu15_shape_bounds_tests.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_world_step_listeners.hpp"\n'+prefix+driver());shutil.copyfile(source,project/'native.cpp');text=source.read_text();variants={}
  for name,needle,replacement in (('wrong_compaction','m.word(current+cursor*4u+4u)','m.word(current+cursor*4u)'),('wrong_final_listener','world.identity+0xc0u','world.identity+0xc4u'),('wrong_count','count_offset)-1u;\n        m.word(count_site','count_offset);\n        m.word(count_site')):
   assert text.count(needle)==1,(name,text.count(needle));variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.returncode,r.stdout,r.stderr)\nprint("Original rejects mutation:",r.stderr.strip()[:300])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeWorldStepListeners LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'+f'target_include_directories(driver PRIVATE "{ROOT}/include")\n'
  for name in ('native',*variants):
   cmake+=f'add_executable(listeners_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(listeners_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(listeners_{name} PRIVATE cxx_std_20)\n'
   command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '+(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:listeners_{name}> "{fixture}"';cmake+=f'add_test(NAME listeners_{name} COMMAND {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in ('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','listeners_native',*sum((['--target','listeners_'+n]for n in variants),[]),'--filter','^listeners_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(reference.sha(ROOT/f)==v for f,v in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,original_cases=len(rows),complete_original_calls=sum(c['completed']for c in rows),required_owner_stops=sum(bool(c.get('required_owner'))and not c.get('fault')for c in rows),body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],ordered_original_reads=sum(len(c['reads'])for c in rows),ordered_original_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_snapshot_unchanged=unchanged,negative_controls=list(variants),image_sha256=reference.NEW_PIN,supplied_original_outcomes=False,instruction_patches=[],remaining=['Genuine custom callback return composition and mutable callback cohorts','Full visible multiplayer binding','Unmasked platform exceptions'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
