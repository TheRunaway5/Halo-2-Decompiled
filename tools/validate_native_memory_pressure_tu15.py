#!/usr/bin/env python3
"""Compare native memory-pressure owners with unchanged physical TU1.5.

Full helper/low-pressure returns and genuine first object-service boundaries
are separate. No object collection outcome or old private register image is
supplied. Every compiler/original product belongs to HALO2_TEST_SCRATCH.
"""
import argparse,copy,hashlib,json,os,shutil,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
OWNERS=(0x1496f0,0x149ad0,0x149680,0x1498a0,0x2335b0)
BACKEND='unchanged-tu15-native-memory-pressure-v1'
POOL,CLOCK,EMERGENCY,SECOND=0x1000800,0x1000300,0x1001000,0x1001200
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');registers={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];pins={}
 for entry in OWNERS:
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment;base,index=i.reg_name(op.mem.base),i.reg_name(op.mem.index)
    ops.append(dict(base=registers.get(base,-1),index=registers.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  pins[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[123,60,32,16,16],counts
 return dict(sites=sites,body_sites=counts,listing_sha256=pins,boundaries=[0x1cd130,0x1ccbe0,0x1cb330,0x1cb220],regions=[])
def cases(image):
 rows=[]
 def make(name,entry,free_mb=16,used=0,flags=0,busy=0,alignment=0,allocator=EMERGENCY,primary=True,controls=None):
  banks={0x1000000:bytearray([0xa5])*0x4000,0x486058:bytearray(96),0x48b870:bytearray(8),0x51df44:bytearray(96),POOL+0xf0340:bytearray(0x90),0x46a134:bytearray(image.read(0x46a134,20)),0x456910:bytearray(image.read(0x456910,4))}
  def put(p,v,width=4):
   first=next(a for a,b in banks.items()if a<=p and p+width<=a+len(b));banks[first][p-first:p-first+width]=(v&((1<<(width*8))-1)).to_bytes(width,'little')
  put(0x486058,100);put(0x48605c,allocator-0x105410);put(0x48606c,POOL if primary else 0);put(0x486070,EMERGENCY if flags&1 else 0);put(0x486074,SECOND if flags&2 else 0);put(EMERGENCY+0x20,1);put(SECOND+0x20,1)
  put(0x48b873,busy,1);put(0x48b872,0x6a,1);put(0x51df44,0x1003000+alignment);put(0x51df54,CLOCK);put(CLOCK+8,105)
  put(POOL+0xf0340,used)
  for n in range(16):put(POOL+0x6c+n*4,16)
  put(POOL+0xf0384,int(free_mb*1024*1024/16))
  n=len(rows);rows.append(dict(name=name,entry=entry,receiver=0,arguments=[],minimum_input=POOL,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=controls or [0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=physical.STOP,boundary=None))
 for alignment in range(16):make('restore_alignment_'+str(alignment),0x149680,alignment=alignment)
 for address in(0,0x486070,0x51df44):make('restore_alias_or_zero_'+hex(address),0x149680,allocator=address,alignment=3)
 for free in(-2048,-1,0,1,2,4,8,16,32,128,2048):
  make('pool_free_'+str(free),0x2335b0,free_mb=free)
  make('level_free_'+str(free),0x149ad0,free_mb=free)
  make('frame_free_'+str(free),0x1496f0,free_mb=free)
 for used in(0,1,16777217,0x7fffffff,0x80000000,0xffffffff):
  for flags in range(4):make(f'level_used_{used:x}_flags_{flags}',0x149ad0,used=used,flags=flags)
 for flags in range(4):
  make('null_primary_'+str(flags),0x149ad0,flags=flags,primary=False)
  make('frame_null_primary_'+str(flags),0x1496f0,flags=flags,primary=False)
 for busy in(1,2,0xff):make('frame_reentrant_'+str(busy),0x1496f0,busy=busy)
 for rounding in range(4):
  for mode in(0,0x40,0x8000,0x8040):
   make(f'pool_controls_{rounding}_{mode:x}',0x2335b0,free_mb=1.000001,controls=[0x27f,0x453f,0x1f80|(rounding<<13)|mode])
   make(f'level_controls_{rounding}_{mode:x}',0x149ad0,used=16777217,controls=[0x27f,0x453f,0x1f80|(rounding<<13)|mode])
 make('batch_first_actual_owner',0x1498a0)
 for entry in(0x149680,0x149ad0,0x2335b0,0x1496f0):make('fault_seed_'+hex(entry),entry,free_mb=128,used=16777217,alignment=7)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],full_positive_collection_qualified=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  original_observer=physical.observer_source
  def observer():
   text=original_observer();needle='{"extended_result",';assert text.count(needle)==1;return text.replace(needle,'{"scalar_result",fp.xmm_space[0]},'+needle)
  physical.observer_source=observer
  def boundary(c,actual):
   owner=actual['actual_stop']
   if owner==physical.STOP:return None
   if owner==0x1ccbe0:
    gp,args=actual['boundary_gp'],actual['boundary_stack'];return dict(owner=owner,mode=gp[1],final_pass=bool(gp[0]&255),aggressive=bool(args[1]&255),recent_cleanup=bool(args[2]&255),excluded=args[3])
   return dict(owner=owner,arguments=actual['boundary_stack'][1:2]if owner==0x1cd130 else[])
  physical.boundary=boundary;rows=physical.physical(image,meta,cases(image),scratch);prefix=[];code=_Code(image,'.text')
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    seen=set();occurrences={}
    for e in seed[category]:
     key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;occurrences[key]=occurrences.get(key,0)+1
     if e['instruction']in seen:continue
     seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefix.append(c)
   seen=set()
   for n,e in enumerate(seed['writes']):
    if e['instruction']in seen:continue
    seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,observer_fault=n+1);prefix.append(c)
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'fixture.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=ROOT/'src/physics/native_tu15_memory_pressure.cpp';text=source.read_text();(project/'native.cpp').write_text(text);shutil.copyfile(ROOT/'tests/native_tu15_memory_pressure_tests.cpp',project/'driver.cpp');variants={}
  for name,needle,replacement in [('stride','cursor-0xf02dcu','cursor-0xf02d8u'),('alignment','padding=(0u-buffer)&15u','padding=buffer&15u'),('threshold','greater(threshold,occupied)','greater(occupied,threshold)'),('clear_busy','m.word(0x14988cu,0x48b873u,0,1)','m.word(0x14988cu,0x48b873u,1,1)')]:
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.stdout,r.stderr)\nprint(r.stderr[-4096:])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeMemoryPressure LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+='add_executable(pressure_'+name+' $<TARGET_OBJECTS:driver> '+name+'.cpp)\ntarget_include_directories(pressure_'+name+' PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(pressure_'+name+' PRIVATE cxx_std_20)\ntarget_compile_options(pressure_'+name+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=(''if name=='native'else'"'+sys.executable+'" "'+str(project/'negative.py')+'" ')+'$<TARGET_FILE:pressure_'+name+'> "'+str(fixture)+'"';cmake+='add_test(NAME pressure_'+name+' COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- '+command+')\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--jobs','1','--test-jobs','1','--filter','^pressure_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   for name in('native',*variants):cmd+=['--target','pressure_'+name]
   result=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=result.returncode,command=cmd))
  files=[source,ROOT/'include/halo2/native_tu15_memory_pressure.hpp',ROOT/'tests/native_tu15_memory_pressure_tests.cpp',Path(__file__),ROOT/'tools/validate_native_island_agent_phase_tu15.py',ROOT/'tools/observe_native_constraint_step_tu15.cpp'];report.update(passed=all(r['exit_code']==0 for r in results),source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],compiler_results=results,cases=len(rows),full_returns=sum(c['actual_stop']==physical.STOP and not c.get('fault')and not c.get('observer_fault')for c in rows),actual_first_child_boundaries=sum(c['boundary']is not None for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),negative_controls=list(variants),instruction_patches=[],supplied_callee_results=False,remaining=['Positive returned object-service/collection loops and reader audit','Actual live frame integration and whole original frame comparison','Unmasked selected SSE exception policies'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
