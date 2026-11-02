#!/usr/bin/env python3
"""Qualify the typed2F7A50 parent against unchanged physical TU1.5 code.

Closed recursive compound children execute the real original2F7A50 body in
the oracle and the native semantic body in the replacement. Noncompound
children stop at their genuine304350 entry; no contact outcome is supplied.
All compiler products and original operand banks belong to run_test.py.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE=0x1000000
AGENT,BODY,SECOND,CONFIG,OUTPUT=BASE+0x800,BASE+0x200,BASE+0x300,BASE+0x400,BASE+0x500
PROXY=physical.SP-16
BACKEND='unchanged-tu15-native-compound-process-v1'
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')

def metadata(image):
 code=_Code(image,'.text');body=complete_body(code,0x2f7a50);assert len(body.code)==40
 names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for pc in sorted(body.code):
  i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   assert not op.mem.segment
   base,index=i.reg_name(op.mem.base),i.reg_name(op.mem.index)
   ops.append(dict(base=names.get(base,-1),index=names.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
  sites.append(dict(instruction=pc,operands=ops))
 return dict(sites=sites,body_sites=40,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),regions=[],boundaries=[0x304350])

def cases():
 rows=[]
 def make(name,count,depth=1,boundary=False,controls=None):
  data=bytearray((n*37+11)&255 for n in range(0x4000))
  def put(p,n):struct.pack_into('<I',data,p-BASE,n&0xffffffff)
  for level in range(4):
   agent=AGENT+level*0x80;shape=BASE+0x1000+level*0x100;agents=BASE+0x2000+level*0x100;children=BASE+0x2800+level*0x100
   put(agent,BASE+0x3f00);put(agent+12,agents);put(agent+16,count if level==0 else (2 if level<depth else 0));put(shape+12,children)
   for index in range(8):put(agents+index*4,AGENT+(level+1)*0x80);put(children+index*8,shape+0x100)
  put(BASE+0x3f00+16,0x304350 if boundary else 0x2f7a50)
  put(BODY,BASE+0x1000);put(BODY+8,BASE+0x3500)
  # Every proxy is a retained data packet, independent of saved GP registers.
  banks=[dict(address=BASE,before=data.hex())]
  for level in range(depth+2):banks.append(dict(address=PROXY-level*52,before=bytes((n+level*13)&255 for n in range(16)).hex()))
  n=len(rows);rows.append(dict(name=name,entry=0x2f7a50,receiver=AGENT,arguments=[BODY,SECOND,CONFIG,OUTPUT],agent=AGENT,first=BODY,second=SECOND,configuration=CONFIG,output=OUTPUT,proxy=PROXY,banks=banks,before_fp=controls or [0x27f|((n%4)<<10),(n%8)<<11,0x1f80|((n%4)<<13)],stop_pc=physical.STOP,boundary=None))
 for count in(0,0xffffffff,0x80000000):make('empty_signed_'+hex(count),count)
 for count in(1,2,5,8):
  for depth in(1,2):make(f'recursive_{count}_{depth}',count,depth)
 for count in(1,3):make(f'genuine_producer_boundary_{count}',count,boundary=True)
 for precision in(0,2,3):
  for rounding in range(4):
   for mode in(0,0x40,0x8000,0x8040):make(f'controls_{precision}_{rounding}_{mode:x}',2,2,controls=[0x7f|(precision<<8)|(rounding<<10),0x453f,0x1f80|(rounding<<13)|mode])
 make('fault_seed_recursive',3,2);make('fault_seed_boundary',1,boundary=True)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(c,actual):
   if actual['actual_stop']==physical.STOP:return None
   assert actual['actual_stop']==0x304350
   return dict(owner=actual['actual_stop'],receiver=actual['boundary_gp'][1],arguments=actual['boundary_stack'][1:5])
  physical.boundary=boundary
  rows=physical.physical(image,meta,cases(),scratch);prefix=[];code=_Code(image,'.text')
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    seen=set();occurrences={}
    for event in seed[category]:
     key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2;occurrences[key]=occurrences.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefix.append(c)
   seen=set()
   for ordinal,event in enumerate(seed['writes']):
    if event['instruction']in seen:continue
    seen.add(event['instruction'])
    c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+str(ordinal),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=ordinal+1);prefix.append(c)
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'compound.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=40,cases=rows))
  project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_compound_process.cpp';header=ROOT/'include/halo2/native_tu15_compound_process.hpp';test=ROOT/'tests/native_tu15_compound_process_tests.cpp';text=source.read_text();(project/'native.cpp').write_text(text);variants={}
  for name,needle,replacement in(('wrong_proxy_parent','proxy.identity+12u,first.identity','proxy.identity+12u,second.identity'),('wrong_child_stride','shape_children+index*8u','shape_children+index*4u'),('eager_shape_children','const auto shape_children=m.word(compound_shape+12u);','const auto shape_children=m.word(compound_shape+16u);')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(x in r.stderr for x in ("compound", "Missing solver extent")),(r.returncode,r.stdout,r.stderr)\nprint("Original compound process rejects mutation:",r.stderr.strip())\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_compound_process LANGUAGES CXX)
include(CTest)
add_library(driver OBJECT "{test}")
target_include_directories(driver PRIVATE "{ROOT}/include")
target_compile_features(driver PRIVATE cxx_std_20)
'''
  for name in('native',*variants):
   cmake+=f'add_executable(compound_{name} $<TARGET_OBJECTS:driver> {name}.cpp)\ntarget_include_directories(compound_{name} PRIVATE "{ROOT}/include")\ntarget_compile_features(compound_{name} PRIVATE cxx_std_20)\n';command=(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:compound_{name}> "{fixture}"';cmake+=f'add_test(NAME compound_{name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','compound_native',*sum((['--target','compound_'+name]for name in variants),[]),'--filter','^compound_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  files=[source,header,test,Path(__file__),ROOT/'tools/observe_native_constraint_step_tu15.cpp',ROOT/'tools/validate_native_island_agent_phase_tu15.py'];pins={str(p.relative_to(ROOT)):sha(p)for p in files};report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_sha256=NEW_PIN,body_sites=40,listing_sha256=meta['listing_sha256'],original_cases=len(rows),complete_recursive_original_calls=sum(c['actual_stop']==physical.STOP for c in rows if not c.get('fault')and not c.get('observer_fault')),required_owner_entry_prefixes=sum(c['boundary']is not None for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_hashes=pins,negative_controls=list(variants),supplied_original_outcomes=False,instruction_patches=[],production_native_only_closure=False,remaining=['Real304350 return and production dependency context','Full live multiplayer integration','Incoming private control bytes are not semantic output'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
