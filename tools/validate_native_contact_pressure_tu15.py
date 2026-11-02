#!/usr/bin/env python3
"""Compare native contact-pressure owner with unchanged physical TU1.5.

Full source no-deletion returns and genuine first candidate-service boundaries
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
OWNERS=(0x1cce00,0x2e8cf0,0x31edf0,0x31ede0)
BACKEND='unchanged-tu15-native-contact-pressure-v1'
WORLD,ISLAND,ISLANDS,CONTACTS,BODY_A,BODY_B,SHAPE_A,SHAPE_B,BODIES=0x1000100,0x1000300,0x1000200,0x1001000,0x1003000,0x1003100,0x1003200,0x1003300,0x1003400
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
 assert counts==[220,2,2,2],counts
 return dict(sites=sites,body_sites=counts,listing_sha256=pins,boundaries=[0x1ccad0],regions=[])
def cases(image):
 rows=[]
 def make(name,contacts=0,islands=1,busy=0,world=WORLD,kind_a=0x2e8cf0,kind_b=0x2e8cf0,body_count=0,priority=0x80000000,first_type=1,second_type=1):
  banks={0x1000000:bytearray(0x6000),0x52be48:bytearray(4),0x48b870:bytearray(8)}
  def put(p,v,width=4):
   lo=next(lo for lo,b in banks.items()if lo<=p and p+width<=lo+len(b));banks[lo][p-lo:p-lo+width]=(v&((1<<(width*8))-1)).to_bytes(width,'little')
  put(0x52be48,world);put(0x48b873,busy,1);put(WORLD+8,ISLANDS);put(WORLD+12,islands)
  for k in range(3):put(ISLANDS+k*4,ISLAND+k*0x100);put(ISLAND+k*0x100+0x6c,contacts if k==0 else 24);put(ISLAND+k*0x100+0x68,CONTACTS)
  put(ISLAND+0x3c,BODIES);put(ISLAND+0x40,body_count);put(BODIES,BODY_A);put(BODY_A+0x8c,priority)
  for k in range(max(0,min(contacts,32))):put(CONTACTS+k*16,BODY_A+0x10);put(CONTACTS+k*16+4,BODY_B+0x10)
  put(BODY_A+0x18,first_type);put(BODY_A+0x20,BODY_A);put(BODY_B+0x18,second_type);put(BODY_B+0x20,BODY_B)
  put(BODY_A+0x3c,SHAPE_A);put(BODY_B+0x3c,SHAPE_B);put(SHAPE_A,0x1003500);put(SHAPE_B,0x1003600);put(0x1003518,kind_a);put(0x1003618,kind_b)
  n=len(rows);rows.append(dict(name=name,entry=0x1cce00,receiver=0,arguments=[],banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())],before_fp=[0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=physical.STOP,boundary=None))
 make('null_world',world=0)
 for busy in(1,2,255):make('reentrant_'+str(busy),contacts=25,busy=busy)
 for islands in(0,1,2,3,0xffffffff,0x80000000):
  for contacts in(0,1,23,24,0xffffffff,0x80000000):make(f'simple_{islands:x}_{contacts:x}',contacts,islands)
 for contacts in(25,26,30):
  for a in(0x2e8cf0,0x31edf0,0x31ede0):
   for b in(0x2e8cf0,0x31edf0,0x31ede0):make(f'dense_types_{contacts}_{a:x}_{b:x}',contacts,kind_a=a,kind_b=b)
 for priority in(0x80000000,0xffffffff,0,0x7fffffff):make(f'candidate_{priority:x}',25,body_count=1,priority=priority)
 make('fault_seed_simple',24,2)
 make('fault_seed_dense',25,kind_a=0x2e8cf0,kind_b=0x31edf0)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],full_positive_collection_qualified=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(c,actual):
   owner=actual['actual_stop']
   if owner==physical.STOP:return None
   assert owner==0x1ccad0,hex(owner)
   gp,args=actual['boundary_gp'],actual['boundary_stack'];return dict(owner=owner,body=gp[0],mode=gp[3],first_option=bool(args[1]&255),second_option=bool(args[2]&255),excluded=args[3])
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
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'fixture.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=ROOT/'src/physics/native_tu15_contact_pressure.cpp';text=source.read_text();(project/'native.cpp').write_text(text);shutil.copyfile(ROOT/'tests/native_tu15_contact_pressure_tests.cpp',project/'driver.cpp');variants={}
  for name,needle,replacement in [('threshold','signed32(effective)>24','signed32(effective)>23'),('stride','offset+=16u','offset+=12u'),('clear_busy','m.word(0x1cd0f1u,0x48b873u,0,1)','m.word(0x1cd0f1u,0x48b873u,1,1)')]:
   assert needle in text;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'types.cpp')
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.stdout,r.stderr)\nprint(r.stderr[-4096:])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeMemoryPressure LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+='add_executable(pressure_'+name+' $<TARGET_OBJECTS:driver> '+name+'.cpp types.cpp)\ntarget_include_directories(pressure_'+name+' PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(pressure_'+name+' PRIVATE cxx_std_20)\ntarget_compile_options(pressure_'+name+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=(''if name=='native'else'"'+sys.executable+'" "'+str(project/'negative.py')+'" ')+'$<TARGET_FILE:pressure_'+name+'> "'+str(fixture)+'"';cmake+='add_test(NAME pressure_'+name+' COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- '+command+')\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--jobs','1','--test-jobs','1','--filter','^pressure_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   for name in('native',*variants):cmd+=['--target','pressure_'+name]
   result=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=result.returncode,command=cmd))
  files=[source,ROOT/'include/halo2/native_tu15_contact_pressure.hpp',ROOT/'include/halo2/native_tu15_contact_pressure_binding.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp',ROOT/'tests/native_tu15_contact_pressure_tests.cpp',Path(__file__),ROOT/'tools/validate_native_island_agent_phase_tu15.py',ROOT/'tools/observe_native_constraint_step_tu15.cpp'];report.update(passed=all(r['exit_code']==0 for r in results),source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],compiler_results=results,cases=len(rows),full_returns=sum(c['actual_stop']==physical.STOP and not c.get('fault')and not c.get('observer_fault')for c in rows),actual_first_child_boundaries=sum(c['boundary']is not None for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),negative_controls=list(variants),instruction_patches=[],supplied_callee_results=False,remaining=['Returned positive candidate/deletion service closure and all deeper retained phases','Actual live frame integration and whole original frame comparison','Actual source deletion/cleanup lifetime and observers through native child closure'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
