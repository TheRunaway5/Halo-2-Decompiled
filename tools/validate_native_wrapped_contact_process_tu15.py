#!/usr/bin/env python3
"""Qualify native2F6CA0 and geometry against unchanged physical TU1.5 code.

Closed recursive compound children execute the real original2F7A50 body in
the oracle and the native semantic body in the replacement. Noncompound
children stop at their genuine304350 entry; no contact outcome is supplied.
All compiler products and original operand banks belong to run_test.py.
"""
import argparse,copy,hashlib,json,os,struct,subprocess,sys,shutil
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

BASE=0x1000000
AGENT,BODY,SECOND,CONFIG,OUTPUT=BASE+0x800,BASE+0x200,BASE+0x300,BASE+0x400,BASE+0x500
PROXY=physical.SP-0x80
GEOMETRY=physical.SP-0x70
CHILD=AGENT+0x80
BACKEND='unchanged-tu15-native-wrapped-contact-process-v1'
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')

def metadata(image):
 code=_Code(image,'.text');names={n:i for i,n in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];hashes={};counts=[]
 for entry in (0x2f6ca0,0x2e44d0,0x2f7a50):
  body=complete_body(code,entry);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment
    base,index=i.reg_name(op.mem.base),i.reg_name(op.mem.index)
    ops.append(dict(base=names.get(base,-1),index=names.get(index,-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  hashes[hex(entry)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts==[46,40,40]
 return dict(sites=sites,body_sites=counts,listing_sha256=hashes,regions=[],boundaries=[])

def cases():
 import random
 rows=[]
 def make(name,count=2,depth=1,controls=None,changes=()):
  n=len(rows);rng=random.Random(n);data=bytearray().join(struct.pack('<f',rng.uniform(-8,8))for _ in range(0x4000//4))
  def put(p,n):struct.pack_into('<I',data,p-BASE,n&0xffffffff)
  put(AGENT+12,CHILD)
  for level in range(4):
   agent=CHILD+level*0x80;shape=BASE+0x1100+level*0x100;agents=BASE+0x2000+level*0x100;children=BASE+0x2800+level*0x100
   put(agent,BASE+0x3f00);put(agent+12,agents);put(agent+16,count if level==0 else (2 if level<depth else 0));put(shape+12,children)
   for index in range(8):put(agents+index*4,CHILD+(level+1)*0x80);put(children+index*8,shape+0x100)
  put(BASE+0x3f10,0x2f7a50);put(BODY,BASE+0x1000);put(BODY+4,0x19876543);put(BODY+8,BASE+0x3000);put(BASE+0x100c,BASE+0x1100)
  for address,value in changes:put(address,value)
  banks=[dict(address=BASE,before=data.hex()),dict(address=PROXY,before=bytes((k*7+n*13)&255 for k in range(112)).hex())]
  for level in range(depth+2):banks.append(dict(address=PROXY-52*(level+1),before=bytes((k+level*13+n)&255 for k in range(16)).hex()))
  rows.append(dict(name=name,entry=0x2f6ca0,receiver=AGENT,arguments=[BODY,SECOND,CONFIG,OUTPUT],agent=AGENT,first=BODY,second=SECOND,configuration=CONFIG,output=OUTPUT,proxy=PROXY,geometry=GEOMETRY,banks=banks,before_fp=controls or [0x27f|((n%4)<<10),((n%8)<<11)|0x453f,0x1f80|((n%4)<<13)],stop_pc=physical.STOP,boundary=None))
 for count in(0,0xffffffff,0x80000000):make('empty_signed_'+hex(count),count)
 for count in(1,2,5,8):
  for depth in(1,2):make(f'recursive_{count}_{depth}',count,depth)
 for precision in(0,2,3):
  for rounding in range(4):
   for mode in(0,0x40,0x8000,0x8040):make(f'controls_{precision}_{rounding}_{mode:x}',controls=[0x7f|(precision<<8)|(rounding<<10),0x453f,0x1f80|(rounding<<13)|mode])
 for p in(BASE+0x1010,BASE+0x102c,BASE+0x3020,BASE+0x304c):
  for bits in(0x80000000,1,0x7f800000,0xff800000,0x7fc12345,0x7f812345):make(f'edge_{p:x}_{bits:x}',changes=((p,bits),))
 make('fault_seed_recursive',3,2)
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--parent-prefix',action='store_true');p.add_argument('--case-filter');p.add_argument('--binding',action='store_true');a=p.parse_args();assert not a.binding or a.parent_prefix,'Binding proof currently requires the strict parent-prefix cohort';a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(c,actual):
   if actual['actual_stop']==physical.STOP:return None
   assert actual['actual_stop']==0x304350
   return dict(owner=actual['actual_stop'],receiver=actual['boundary_gp'][1],arguments=actual['boundary_stack'][1:5])
  physical.boundary=boundary
  declared=cases()
  if a.case_filter:declared=[c for c in declared if a.case_filter in c['name']];assert declared
  if a.parent_prefix:
   meta['boundaries']=[0x2f7a50]
   for case in declared:case['banks']=case['banks'][:2]
   def boundary(case,actual):
    assert actual['actual_stop']==0x2f7a50
    return dict(owner=actual['actual_stop'],receiver=actual['boundary_gp'][1],arguments=actual['boundary_stack'][1:5])
   physical.boundary=boundary
  rows=physical.physical(image,meta,declared,scratch);prefix=[];code=_Code(image,'.text')
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
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'compound.json';save(fixture,dict(image_sha256=NEW_PIN,required_backend=BACKEND,body_sites=meta['body_sites'],cases=rows))
  project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');source=ROOT/'src/physics/native_tu15_wrapped_contact_process.cpp';header=ROOT/'include/halo2/native_tu15_wrapped_contact_process.hpp';test=ROOT/'tests/native_tu15_wrapped_contact_process_tests.cpp';text=source.read_text();(project/'native.cpp').write_text(text);shutil.copyfile(test,project/'driver.cpp');
  if a.binding:
   driver=(project/'driver.cpp').read_text().replace('halo2/native_tu15_wrapped_contact_process.hpp','halo2/native_tu15_contact_process_binding.hpp')
   needle='native_tu15_process_wrapped_contact_agent(m,{c.at("agent")},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")},{{c.at("proxy")},c.at("geometry")},fp,services,observer,&stats);'
   assert driver.count(needle)==1
   replacement='NativeTu15ContactProcessBinding binding(m,{c.at("proxy"),112u},services,observer,nullptr,[&](const NativeTu15CompoundChildProcessCall&child,const auto&){if(child.owner.identity==0x2f7a50u){const auto&b=c.at("boundary");check(!b.is_null()&&b.at("owner")==child.owner.identity&&b.at("receiver")==child.receiver.identity&&b.at("arguments")==J::array({child.first.identity,child.second.identity,child.configuration.identity,child.output.identity}),name+" typed binding child input differs");boundary_hit=true;throw std::runtime_error("actual-island-owner-boundary");}});binding.process({{c.at("agent")},{0x2f6ca0u},{c.at("first")},{c.at("second")},{c.at("configuration")},{c.at("output")}},fp);'
   (project/'driver.cpp').write_text(driver.replace(needle,replacement))
  shutil.copyfile(ROOT/'src/physics/native_tu15_geometry.cpp',project/'geometry.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_compound_process.cpp',project/'compound.cpp');variants={}
  for name,needle,replacement in(('wrong_parent','scratch.descriptor.identity+12u,first.identity','scratch.descriptor.identity+12u,second.identity'),('wrong_geometry_basis','current_transform+32u,shape+16u','current_transform+16u,shape+16u'),('wrong_child_shape','m.word(shape+12u)','m.word(shape+8u)')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and any(x in r.stderr for x in ("compound", "wrapped-process", "Missing solver extent")),(r.returncode,r.stdout,r.stderr)\nprint("Original compound process rejects mutation:",r.stderr.strip())\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_compound_process LANGUAGES CXX)
include(CTest)
add_library(driver OBJECT "{project}/driver.cpp")
target_include_directories(driver PRIVATE "{project}/include")
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
'''
  for name in('native',*variants):
   cmake+=f'add_executable(compound_{name} $<TARGET_OBJECTS:driver> {name}.cpp geometry.cpp compound.cpp)\ntarget_include_directories(compound_{name} PRIVATE "{project}/include")\ntarget_compile_features(compound_{name} PRIVATE cxx_std_20)\ntarget_compile_options(compound_{name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';command=(''if name=='native'else f'"{sys.executable}" "{project}/negative.py" ')+f'$<TARGET_FILE:compound_{name}> "{fixture}"';cmake+=f'add_test(NAME compound_{name} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {command})\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','compound_native',*sum((['--target','compound_'+name]for name in variants),[]),'--filter','^compound_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  files=[source,header,test,ROOT/'include/halo2/native_tu15_contact_process_binding.hpp',ROOT/'include/halo2/native_tu15_geometry.hpp',ROOT/'include/halo2/native_tu15_compound_process.hpp',ROOT/'include/halo2/native_tu15_physics_frame.hpp',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'src/physics/native_tu15_geometry.cpp',ROOT/'src/physics/native_tu15_compound_process.cpp',Path(__file__),ROOT/'tools/observe_native_constraint_step_tu15.cpp',ROOT/'tools/validate_native_island_agent_phase_tu15.py'];pins={str(p.relative_to(ROOT)):sha(p)for p in files};report.update(passed=all(r['exit_code']==0 for r in results),parent_prefix_only=a.parent_prefix,raw_complete_composition_passed=False,compiled_source_snapshot=True,typed_binding_qualified=a.binding,compiler_results=results,original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],original_cases=len(rows),complete_recursive_original_calls=sum(c['actual_stop']==physical.STOP for c in rows if not c.get('fault')and not c.get('observer_fault')),required_owner_entry_prefixes=sum(c['boundary']is not None for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),source_hashes=pins,negative_controls=list(variants),supplied_original_outcomes=False,instruction_patches=[],production_native_only_closure=False,remaining=['Noncompound child processing and actual production caller context','Full visible multiplayer integration','Real live x87 input-depth/fault domains beyond the declared cases'])
 except BaseException as error:report['error']=str(error);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
