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
OWNERS=(0xc0f60,0xbbd10)
BACKEND='unchanged-tu15-native-object-detachment-v1'
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
 assert counts==[97,19],counts
 return dict(sites=sites,body_sites=counts,listing_sha256=pins,boundaries=[0xc5250,0xc4d30,0xc3750],regions=[])
def cases(image):
 import validate_native_object_deletion_tu15 as deletion
 baseline=deletion.cases(image)[0];rows=[]
 def make(name,first=1,second=0,third=0,fourth=0,flags=0,header=0,descriptors=0,kind=0,attachment=0xffffffff,tree=False):
  row=copy.deepcopy(baseline);banks={r['address']:bytearray.fromhex(r['before'])for r in row['banks']}
  def put(p,v,width=4):
   lo=next(lo for lo,b in banks.items()if lo<=p and p+width<=lo+len(b));banks[lo][p-lo:p-lo+width]=(v&((1<<(width*8))-1)).to_bytes(width,'little')
  for k in range(3):put(OBJECT+k*0x1000+0xc,0xffffffff);put(OBJECT+k*0x1000+0x10,0xffffffff)
  target=OBJECT+0x2000;put(target+4,flags);put(ROWS+26,header,1);put(target+0x11c,descriptors*8,2);put(target+0x11e,0x500,2)
  for k in range(3):put(target+0x500+k*8,kind,1);put(target+0x504+k*8,attachment)
  if tree:put(target+0x10,0x12340000);put(OBJECT+0xc,0x12340001);put(OBJECT+0x14,DATUM);put(OBJECT+0x1000+0x14,DATUM)
  row.update(name=name,entry=0xc0f60,receiver=0,minimum_input=0,entry_registers={},arguments=[DATUM,first,second,third,fourth],detach_attachments=first,delete_attachments=second,follow_siblings=third,current_only=fourth,banks=[dict(address=p,before=b.hex())for p,b in sorted(banks.items())]);rows.append(row)
 for fourth in(0,1,0x100,0x101):
  for flags in(0,1,0x10,0x40):
   for header in(0,0x10):make(f'guard_{fourth}_{flags}_{header}',fourth=fourth,flags=flags,header=header)
 for first in(0,1,0x100,0x101):
  for second in(0,1,0x100,0x101):
   for kind in(0,1):
    for attachment in(0xffffffff,0x12345678):make(f'attachment_{first}_{second}_{kind}_{attachment}',first,second,flags=0x10,descriptors=3,kind=kind,attachment=attachment)
 for first in(0,1,0x100,0x101):
  for second in(0,1,0x100,0x101):make(f'children_{first}_{second}',first,second,flags=0x40)
 make('native_tree',tree=True)
 make('native_tree_with_descriptors',flags=0x10,descriptors=3,tree=True)
 make('fault_seed_tree',tree=True)
 make('fault_seed_descriptors',flags=0x10,descriptors=3,tree=True)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],full_positive_collection_qualified=False)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(c,actual):
   owner=actual['actual_stop']
   if owner==physical.STOP:return None
   gp,args=actual['boundary_gp'],actual['boundary_stack']
   if owner==0xc5250:return dict(owner=owner,datum=gp[0])
   if owner==0xc4d30:return dict(owner=owner,datum=args[1])
   if owner==0xc3750:return dict(owner=owner,arguments=args[1:4])
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
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'fixture.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows));project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=ROOT/'src/physics/native_tu15_object_detachment.cpp';text=source.read_text();(project/'native.cpp').write_text(text);shutil.copyfile(ROOT/'tests/native_tu15_object_detachment_tests.cpp',project/'driver.cpp');variants={}
  for name,needle,replacement in [('byte_guard','request.current_only&255u','request.current_only'),('descriptor_stride','descriptor+=8u','descriptor+=4u'),('sibling','request.datum=sibling','request.datum=0xffffffffu')]:
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  shutil.copyfile(ROOT/'src/physics/native_tu15_activation_services.cpp',project/'activation.cpp')
  shutil.copyfile(ROOT/'src/physics/native_tu15_object_deletion.cpp',project/'deletion.cpp')
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0,(r.stdout,r.stderr)\nprint(r.stderr[-4096:])\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativeMemoryPressure LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\ntarget_include_directories(driver PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(driver PRIVATE cxx_std_20)\n'
  for name in('native',*variants):
   cmake+='add_executable(pressure_'+name+' $<TARGET_OBJECTS:driver> '+name+'.cpp activation.cpp deletion.cpp)\ntarget_include_directories(pressure_'+name+' PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(pressure_'+name+' PRIVATE cxx_std_20)\ntarget_compile_options(pressure_'+name+' PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=(''if name=='native'else'"'+sys.executable+'" "'+str(project/'negative.py')+'" ')+'$<TARGET_FILE:pressure_'+name+'> "'+str(fixture)+'"';cmake+='add_test(NAME pressure_'+name+' COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- '+command+')\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--jobs','1','--test-jobs','1','--filter','^pressure_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
   for name in('native',*variants):cmd+=['--target','pressure_'+name]
   result=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=result.returncode,command=cmd))
  files=[source,ROOT/'include/halo2/native_tu15_object_detachment.hpp',ROOT/'src/physics/native_tu15_object_deletion.cpp',ROOT/'include/halo2/native_tu15_object_deletion.hpp',ROOT/'src/physics/native_tu15_activation_services.cpp',ROOT/'include/halo2/native_tu15_activation_services.hpp',ROOT/'tests/native_tu15_object_detachment_tests.cpp',Path(__file__),ROOT/'tools/validate_native_island_agent_phase_tu15.py',ROOT/'tools/observe_native_constraint_step_tu15.cpp'];report.update(passed=all(r['exit_code']==0 for r in results),source_hashes={str(f.relative_to(ROOT)):sha(f)for f in files},original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],compiler_results=results,cases=len(rows),full_returns=sum(c['actual_stop']==physical.STOP and not c.get('fault')and not c.get('observer_fault')for c in rows),actual_first_child_boundaries=sum(c['boundary']is not None for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),negative_controls=list(variants),instruction_patches=[],supplied_callee_results=False,decoder_access_rule='TEST memory operands are reads even when Capstoneaccess0; actual original instructions unchanged',unqualified_atomic_rmw_write_fault_sites=sorted(atomic_write_faults),remaining=['Returned selected detach/spatial/controller/AI/player/physics service closures','Actual live frame integration and whole original frame comparison','Atomic RMW write-fault execution, nonempty x87 and exceptional child-lifetime domains'])
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
