#!/usr/bin/env python3
"""Compare native object-deletion owners with unchanged physical TU1.5.

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
OWNERS=(0xba570,0xbbd10,0xbbd50,0xbaba0,0xc10c0,0x15dc40,0xbd980,0x10fd90,0x10e4b0,0xb1c90,0x10df30,0xbbbc0,0xe7160,0x10f300,0x2f30f0,0xb9390)
BACKEND='unchanged-tu15-native-object-deletion-v1'
BASE,POOL,ROWS,OBJECT,STATE,MODE,PLAYERS,TRACK,SCHEDULE,CLOCK,REFS,TAGS,DEPENDENTS=0x1000000,0x1000100,0x1001000,0x1003000,0x1000800,0x1000600,0x1007000,0x1000900,0x1000a00,0x1000b00,0x1000c00,0x1000d00,0x100b000
DATUM=0x12340002
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
    ops.append(dict(base=registers.get(base,-1),index=registers.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ)or i.mnemonic=='test',write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  pins[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[60,19,22,32,29,18,70,27,120,22,31,61,18,45,3,26],counts
 return dict(sites=sites,body_sites=counts,listing_sha256=pins,boundaries=[0xc0f60,0xba910,0x1d30a0,0x1ce340,0x1d9f10,0xb7e50,0x1f5620,0xe3e20,0xb7f00],regions=[])
def cases(image):
 rows=[]
 def make(name,entry=0xba570,header=0,flags=0,typ=0,parent=0xffffffff,tracking=0xffff,player=0xff,mode=1,reference=0,schedule=False,enabled=False,ticks=0xffffffff,ancestor_flags=0,tag_flags=0,dependent=False,actor=False,projectile_index=0xffff,projectile_slot=0,controller=0):
  banks={BASE:bytearray(0x12000)}
  for p,n in[(0x4ecbd0,4),(0x4ecc20,4),(0x4f3218,4),(0x4f63b8,4),(0x4eaba0,4),(0x4eabc4,4),(0x51df54,4),(0x51daf4,4),(0x4f0414,4),(0x51daf0,4),(0x46e468,1),(0x52be70,8)]:banks[p]=bytearray(n)
  def put(p,v,width=4):
   lo=next(lo for lo,b in banks.items()if lo<=p and p+width<=lo+len(b));banks[lo][p-lo:p-lo+width]=(v&((1<<(width*8))-1)).to_bytes(width,'little')
  for p,v in[(0x4ecbd0,POOL),(0x4ecc20,STATE),(0x4f3218,MODE),(0x4f63b8,PLAYERS),(0x4eaba0,TRACK),(0x4eabc4,SCHEDULE),(0x51df54,CLOCK),(0x51daf4,REFS),(0x4f0414,TAGS),(0x51daf0,DEPENDENTS),(0x52be70,BASE+0x11000),(0x52be74,controller)]:put(p,v)
  put(POOL+0x44,ROWS);put(POOL+0x24,12);put(POOL+0x38,3);put(POOL+0x48,BASE+0xf00);put(BASE+0xf00,7);put(STATE+0x48,5);put(STATE+0x54,BASE+0x10000);put(MODE+0xc,mode,1);put(CLOCK+8,100);put(TAGS+8,BASE+0xa000);put(BASE+0xa002,tag_flags,1)
  for k in range(3):
   obj=OBJECT+k*0x1000;put(ROWS+k*12,0x1234,2);put(ROWS+k*12+8,obj);put(obj+0x14,0xffffffff);put(obj+0x1c,0xffffffff);put(obj+0x1a,0xffff,2);put(obj+0xac,0xffff,2);put(obj+0xaf,0xff,1);put(obj+0xd4,0xffffffff)
  target=OBJECT+0x2000;put(ROWS+26,header,1);put(ROWS+27,typ,1);put(target+4,flags|(0x4000 if schedule else 0));put(target+0x14,parent);put(target+0xac,tracking,2);put(target+0xaf,player,1);put(target+0xc0,reference,2);put(target+0xd4,4 if actor else 0xffffffff)
  put(target+0x1a,projectile_index,2);put(BASE+0x10058,projectile_slot,2)
  put(ROWS+14,ancestor_flags,1);put(SCHEDULE+4,3,2);put(SCHEDULE+8,DATUM);put(target+0x20,120)
  for k in range(5):put(TRACK+k*4,DATUM if k%2==0 else 0x12340001)
  for k in range(32):put(REFS+0x280+k*4,DATUM if k%3==0 else 0x12340001)
  if dependent:put(DEPENDENTS+0x1d80,1);put(DEPENDENTS+0x1d00,DATUM);put(OBJECT+0xc0,2,2);put(OBJECT+0xb8,DATUM);put(ROWS+2,1,1);put(ROWS+3,2,1)
  args=[DATUM]if entry==0xba570 else[player]if entry==0x15dc40 else[int(enabled),ticks]if entry==0xbd980 else[]
  regs={'ecx':DATUM}if entry==0xbbd10 else{'edi':DATUM}if entry==0xc10c0 else{'edx':DATUM}if entry==0x10fd90 else{}
  n=len(rows);rows.append(dict(name=name,entry=entry,receiver=0,minimum_input=DATUM,entry_registers=regs,arguments=args,datum=DATUM,player=player if player<128 else player-256,enabled=enabled,ticks=ticks,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=physical.STOP,boundary=None))
 for header in(0,1,0x10,0x11,0x40,0x50,0x51):
  for flags in(0,1,0x200):make(f'parent_header_{header:x}_flags_{flags:x}',header=header,flags=flags)
 for entry in(0xbbd10,0xbbd50):
  for parent in(0xffffffff,0x12340001):
   for header in(0,0x10,0x40,0x50):make(f'predicate_{entry:x}_{parent:x}_{header:x}',entry,header=header,parent=parent,ancestor_flags=0x40)
 for tracking in(0xffff,0,1,0x7fff):make('tracking_'+str(tracking),0xc10c0,tracking=tracking)
 for mode in(1,4):
  for player in(0,1,3):make(f'player_{mode}_{player}',0x15dc40,mode=mode,player=player)
 for enabled in(False,True):
  for schedule in(False,True):
   for flags in(0,0x8000):
    for ticks in(0,10,0xffffffff,0x7fffffff):make(f'schedule_{enabled}_{schedule}_{flags}_{ticks}',0xbd980,header=1,enabled=enabled,schedule=schedule,flags=flags,ticks=ticks)
 for reference in(0,0x200,0xffff):make('references_'+str(reference),0x10fd90,reference=reference)
 for typ in(6,38):
  for slot in(0,1):make(f'projectile_{typ}_{slot}',typ=typ,projectile_index=0,projectile_slot=slot)
 make('projectile_actual_controller',typ=6,projectile_index=0,projectile_slot=1,controller=BASE+0x11000)
 for tracking in(0xffff,0):make('full_parent_tracking_'+str(tracking),tracking=tracking,header=0x11,reference=0x200,schedule=True,player=1)
 make('full_dependents_no_other',tag_flags=0x40)
 make('full_dependents_other',tag_flags=0x40,dependent=True)
 make('actual_actor_unregister',actor=True)
 make('fault_seed_parent',header=0x11,tracking=0,schedule=True,reference=0x200,player=1)
 make('fault_seed_schedule',entry=0xbd980,header=1,enabled=True)
 make('fault_seed_dependent',tag_flags=0x40,dependent=True)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],full_positive_collection_qualified=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(c,actual):
   owner=actual['actual_stop']
   if owner==physical.STOP:return None
   gp,args=actual['boundary_gp'],actual['boundary_stack']
   if owner==0xc0f60:return dict(owner=owner,arguments=args[1:6])
   if owner==0xba910:c['spatial_packet']=gp[7];return dict(owner=owner,datum=gp[2],packet=gp[7])
   if owner==0x1ce340:return dict(owner=owner,datum=gp[0])
   if owner==0x1d9f10:return dict(owner=owner,component=gp[6])
   if owner==0xb7e50:return dict(owner=owner,arguments=args[1:3])
   if owner==0x1f5620:return dict(owner=owner,datum=args[1],packet=gp[1])
   if owner==0xe3e20:return dict(owner=owner,datum=gp[0])
   if owner==0xb7f00:return dict(owner=owner,actor=gp[0])
   raise AssertionError(hex(owner))
  raw_boundary=boundary
  def labelled_boundary(c,actual):
   result=raw_boundary(c,actual)
   if result is not None:result['owner_label']=f'{result["owner"]:X}'
   return result
  physical.boundary=labelled_boundary;rows=physical.physical(image,meta,cases(image),scratch);prefix=[];code=_Code(image,'.text');atomic_write_faults=set()
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    seen=set();occurrences={}
    for e in seed[category]:
     key=e['address'],e['bytes']if category=='reads'else len(e['written'])//2;occurrences[key]=occurrences.get(key,0)+1
     if e['instruction']in seen:continue
     instruction=code.at(e['instruction'])
     if category=='writes'and any(op.type==X86_OP_MEM and op.access&CS_AC_READ and op.access&CS_AC_WRITE for op in instruction.operands):
      # A stop BEFORE an atomic RMW cannot stand for a separately injected
      # memory.write fault AFTER its read. Its real read-boundary and
      # completed-store observer prefixes are compared; atomic write-fault
      # execution remains an explicit original/native memory-service gap.
      atomic_write_faults.add(e['instruction']);continue
     seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(e['instruction']),stop_pc=e['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefix.append(c)
   seen=set()
   for n,e in enumerate(seed['writes']):
    if e['instruction']in seen:continue
    seen.add(e['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(n),stop_pc=e['instruction']+code.at(e['instruction']).size,observer_fault=n+1);prefix.append(c)
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'fixture.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=ROOT/'src/physics/native_tu15_object_deletion.cpp';text=source.read_text();(project/'native.cpp').write_text(text);shutil.copyfile(ROOT/'tests/native_tu15_object_deletion_tests.cpp',project/'driver.cpp');variants={}
  for name,needle,replacement in [('deleting_flag','header+2u,1)|0x10u','header+2u,1)|0x20u'),('tracking','0xc1108u,entries+offset*4u,0xffffffffu','0xc1108u,entries+offset*4u,0u'),('references','n<32','n<31')]:
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'activation.cpp')
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.stdout,r.stderr)\nprint(r.stderr[-4096:])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeMemoryPressure LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+='add_executable(pressure_'+name+' $<TARGET_OBJECTS:driver> '+name+'.cpp activation.cpp)\ntarget_include_directories(pressure_'+name+' PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(pressure_'+name+' PRIVATE cxx_std_20)\ntarget_compile_options(pressure_'+name+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=(''if name=='native'else'"'+sys.executable+'" "'+str(project/'negative.py')+'" ')+'$<TARGET_FILE:pressure_'+name+'> "'+str(fixture)+'"';cmake+='add_test(NAME pressure_'+name+' COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- '+command+')\n'
  cmake+='add_test(NAME pressure_production_binding COMMAND \"'+sys.executable+'\" \"'+str(ROOT/'tools/run_test.py')+'\" -- $<TARGET_FILE:pressure_native> \"'+str(fixture)+'\" --production-binding)\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--jobs','1','--test-jobs','1','--filter','^pressure_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   for name in('native',*variants):cmd+=['--target','pressure_'+name]
   result=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=result.returncode,command=cmd))
  files=[source,ROOT/'include/halo2/native_tu15_object_deletion.hpp',ROOT/'include/halo2/native_tu15_object_deletion_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp',ROOT/'include/halo2/native_tu15_activation_services.hpp',ROOT/'tests/native_tu15_object_deletion_tests.cpp',Path(__file__),ROOT/'tools/validate_native_island_agent_phase_tu15.py',ROOT/'tools/observe_native_constraint_step_tu15.cpp'];report.update(passed=all(r['exit_code']==0 for r in results),source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],compiler_results=results,cases=len(rows),full_returns=sum(c['actual_stop']==physical.STOP and not c.get('fault')and not c.get('observer_fault')for c in rows),actual_first_child_boundaries=sum(c['boundary']is not None for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),negative_controls=list(variants),instruction_patches=[],supplied_callee_results=False,decoder_access_rule='TEST memory operands are reads even when Capstoneaccess0; actual original instructions unchanged',unqualified_atomic_rmw_write_fault_sites=sorted(atomic_write_faults),remaining=['Returned selected detach/spatial/controller/AI/player/physics service closures','Actual live frame integration and whole original frame comparison','Atomic RMW write-fault execution, nonempty x87 and exceptional child-lifetime domains'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
