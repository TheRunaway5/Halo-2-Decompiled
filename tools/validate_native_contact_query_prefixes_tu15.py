#!/usr/bin/env python3
"""Structured original2E4FE0 branches with native constants/body/inverse owners.

Cache/GJK dependencies stop at actual entry boundaries. No service result is
provided. Declared operand objects are component inputs, not map/runtime proof.
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
CHILD,FIRST,SECOND,CONTEXT,VALID,OUTPUT,SHAPE_A,SHAPE_B,TRANSFORM_A,TRANSFORM_B,BODY_A,BODY_B,MOTION_A,MOTION_B,PROP_A,PROP_B=(BASE+n for n in(0x100,0x300,0x400,0x500,0x600,0x700,0x900,0xa00,0xc00,0xd00,0xf00,0x1000,0x1100,0x1200,0x1400,0x1500))
SCRATCH=((physical.SP-4)&~15)-0x70
CLASS_TABLES={23:BASE+0x3100,24:BASE+0x3200,5:BASE+0x3300}
CLASS_OWNERS={23:0xa9520,24:0xab4b0,5:0x31dd70}
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,v):p.write_text(json.dumps(v,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');names={s:n for n,s in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];counts=[];listings={}
 for owner in(0x2e4fe0,0x2e4550,0x283380,*CLASS_OWNERS.values()):
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);ops=[];listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment
    ops.append(dict(base=names.get(i.reg_name(op.mem.base),-1),index=names.get(i.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   sites.append(dict(instruction=pc,operands=ops))
  listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts[0]==256 and counts[2:]==[68,2,2,2]
 return dict(sites=sites,body_sites=counts,listing_sha256=listings,regions=[],boundaries=[0x309060,0x309150,0x2f8630])
def cases():
 rows=[]
 def make(name,first_class=23,second_class=23,first_speed=0x40000000,second_speed=0x40000000,threshold=0x3f800000,eligible=True,cached=0,scale=0x3c888889,controls=None,chain=False):
  data=bytearray((n*11+3)&255 for n in range(0x6000))
  def put(p,v,size=4):data[p-BASE:p-BASE+size]=(v&((1<<(size*8))-1)).to_bytes(size,'little')
  for kind,table in CLASS_TABLES.items():put(table+0x14,CLASS_OWNERS[kind])
  put(SHAPE_A,CLASS_TABLES[first_class]);put(SHAPE_B,CLASS_TABLES[second_class]);put(SHAPE_A+0x20,threshold);put(SHAPE_B+0x20,threshold)
  put(FIRST,SHAPE_A);put(FIRST+8,TRANSFORM_A);put(FIRST+0xc,BASE+0x1700 if chain else 0);put(FIRST+0x20,BODY_A)
  put(SECOND,SHAPE_B);put(SECOND+8,TRANSFORM_B);put(SECOND+0xc,BASE+0x1800 if chain else 0);put(SECOND+0x20,BODY_B)
  put(BASE+0x1700+0xc,BASE+0x1900);put(BASE+0x1900+0xc,0);put(BASE+0x1900+0x20,BODY_A);put(BASE+0x1800+0xc,0);put(BASE+0x1800+0x20,BODY_B)
  for body,motion,props,speed in((BODY_A,MOTION_A,PROP_A,first_speed),(BODY_B,MOTION_B,PROP_B,second_speed)):
   put(body+0x30,props);put(body+0x34,1);put(props,0x2001);put(props+4,0xffffffff if eligible else 0);put(body+0x3c,motion)
   for n,v in enumerate((speed,0x3f000001,0x3e800001,0x80000000)):put(motion+0x40+n*4,v)
  for transform in(TRANSFORM_A,TRANSFORM_B):
   for column,values in enumerate(((0x3f800000,0,0,0),(0,0x3f800000,0,0),(0,0,0x3f800000,0),(0x3f000000,0x3f800000,0xbf800000,0x3f800000))):
    for lane,value in enumerate(values):put(transform+0x20+column*16+lane*4,value)
  put(CHILD+0xf0,cached);put(CONTEXT+0xc,BASE+0x2000);put(BASE+0x2000,1,1);put(CONTEXT+0x10,scale)
  put(BASE+0x3800+0x44,BASE+0x3900);put(BASE+0x3900+8,0);put(BASE+0x3b00+0x44,BASE+0x3c00);put(BASE+0x3c00+8,BASE+0x3d00);put(BASE+0x3d00+0xc0,6,2);put(BASE+0x3d00+0xb8,0xc001feed);put(BASE+0x4000+0x1d00,0xc001feed);put(BASE+0x4000+0x1d80,1)
  banks=[dict(address=BASE,before=data.hex()),dict(address=SCRATCH+0x18,before=bytes((n*13+7)&255 for n in range(0x58)).hex()),*[dict(address=p,before=v.to_bytes(4,'little').hex())for p,v in((0x52be5c,BASE+0x3800),(0x4ecbd0,BASE+0x3b00),(0x51daf0,BASE+0x4000))]]
  n=len(rows);rows.append(dict(name=name,entry=0x2e4fe0,receiver=CHILD,arguments=[VALID,FIRST,SECOND,CONTEXT,OUTPUT],child=CHILD,valid=VALID,first=FIRST,second=SECOND,context=CONTEXT,output=OUTPUT,scratch=SCRATCH,banks=sorted(banks,key=lambda b:b['address']),before_fp=controls or[0x27f|((n%4)<<10),(n%8)<<11,0x1f80|((n%4)<<13)],stop_pc=physical.STOP,boundary=None))
 for first_class in(23,24):
  for second_class in(23,24,5):
   for eligible in(False,True):
    for cached in(0,1,2,3,0xffffffff):make(f'classes_{first_class}_{second_class}_{eligible}_{cached:x}',first_class,second_class,eligible=eligible,cached=cached)
 for left,right in((0,0),(0x80000000,1),(0x3f000000,0x40000000),(0x7f812345,0x40000000),(0xffc54321,0x7fc12345),(0x7f800000,0xff800000)):
  for threshold in(0,0x3f800000,0x7fc2468a):make(f'vectors_{left:x}_{right:x}_{threshold:x}',first_speed=left,second_speed=right,threshold=threshold)
 for precision in(0,2,3):
  for rounding in range(4):
   for mode in(0,0x40,0x8000,0x8040):make(f'controls_{precision}_{rounding}_{mode:x}',controls=[0x7f|(precision<<8)|(rounding<<10),0x453f,0x1f80|(rounding<<13)|mode])
 make('parent_chains',chain=True);make('fault_seed_both_motion');make('fault_seed_no_motion',first_class=24,second_class=24,cached=0)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image)
  def boundary(c,o):
   target=o['actual_stop'];assert target in meta['boundaries'];return dict(owner=target,receiver=o['boundary_gp'][1],arguments=o['boundary_stack'][1:4]if target in(0x309060,0x309150)else o['boundary_stack'][1:6])
  physical.boundary=boundary;rows=physical.physical(image,meta,cases(),scratch);prefix=[];excluded=[];code=_Code(image,'.text')
  # Threshold reads occur while the original's private x87 value is live;
  # they need a separate resume owner and stay explicitly outside this proof.
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   for category in('reads','writes'):
    seen=set();occurrences={}
    for event in seed[category]:
     key=event['address'],event['bytes']if category=='reads'else len(event['written'])//2;occurrences[key]=occurrences.get(key,0)+1
     if event['instruction']in seen:continue
     seen.add(event['instruction'])
     if category=='reads'and event['instruction']in(0x2e5062,0x2e517e):excluded.append(dict(seed=seed['name'],instruction=event['instruction'],reason='Private live x87 threshold-read resume owner'));continue
     c=copy.deepcopy(seed);c.update(name=seed['name']+'_'+category+'_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key],access='read'if category=='reads'else'write'));prefix.append(c)
   seen=set()
   for ordinal,event in enumerate(seed['writes']):
    if event['instruction']in seen:continue
    seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_observer_'+hex(event['instruction']),stop_pc=event['instruction']+code.at(event['instruction']).size,observer_fault=ordinal+1);prefix.append(c)
  # Fault cases stop before the chosen source operation rather than at a callee.
  original_boundary=physical.boundary;physical.boundary=lambda c,o:None if c.get('fault')or c.get('observer_fault')else original_boundary(c,o)
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'prefix.json';save(fixture,dict(image_sha256=NEW_PIN,cases=rows));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_contact_query.cpp';test=ROOT/'tests/native_tu15_contact_query_prefix_tests.cpp';(project/'native.cpp').write_bytes(source.read_bytes());text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_second_state','m.word(p+0x20u)+2u','m.word(p+0x20u)+1u'),('wrong_first_compare','compare(measured,threshold)&0x4100u','compare(measured,threshold)&0x4000u'),('wrong_cache_selection','cache_class==5u?0x309150u:0x309060u','cache_class==5u?0x309060u:0x309150u')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);assert r.returncode!=0 and "query"in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Original query rejects branch mutation:",r.stderr.strip())\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_contact_query_prefix LANGUAGES CXX)
include(CTest)
add_library(driver OBJECT "{test}")
add_library(dependencies OBJECT "{ROOT}/src/physics/native_tu15_inverse_geometry.cpp" "{ROOT}/src/physics/native_tu15_geometry.cpp" "{ROOT}/src/physics/native_tu15_contact_agent_factory.cpp" "{ROOT}/src/physics/native_tu15_activation_services.cpp")
foreach(target driver dependencies)
 target_include_directories(${{target}} PRIVATE "{ROOT}/include")
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
'''
  for mode in('native',*variants):
   cmake+=f'add_executable(query_prefix_{mode} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {mode}.cpp)\ntarget_include_directories(query_prefix_{mode} PRIVATE "{ROOT}/include")\ntarget_compile_features(query_prefix_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(query_prefix_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME query_prefix_{mode} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:query_prefix_{mode}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','query_prefix_native',*sum((['--target','query_prefix_'+name]for name in variants),[]),'--filter','^query_prefix_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  files=[source,ROOT/'include/halo2/native_tu15_contact_query.hpp',test,Path(__file__),ROOT/'src/physics/native_tu15_activation_services.cpp',ROOT/'src/physics/native_tu15_contact_agent_factory.cpp'];pins={str(p.relative_to(ROOT)):sha(p)for p in files};report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,source_hashes=pins,original_sha256=NEW_PIN,body_sites=meta['body_sites'],listing_sha256=meta['listing_sha256'],physical_cases=len(rows),required_cache_entries=sum(bool(c['boundary']and c['boundary']['owner']in(0x309060,0x309150))for c in rows),required_convex_entries=sum(bool(c['boundary']and c['boundary']['owner']==0x2f8630)for c in rows),read_fault_prefixes=sum(c.get('fault',{}).get('access')=='read'for c in rows),store_fault_prefixes=sum(c.get('fault',{}).get('access')=='write'for c in rows),observer_prefixes=sum(bool(c.get('observer_fault'))for c in rows),ordered_reads=sum(len(c['reads'])for c in rows),ordered_stores=sum(len(c['writes'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),excluded=excluded,supplied_original_outcomes=False,instruction_patches=[],production_native_only_closure=False)
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
