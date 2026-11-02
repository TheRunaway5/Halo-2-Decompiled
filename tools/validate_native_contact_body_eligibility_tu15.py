#!/usr/bin/env python3
"""Unchanged physical283380 eligibility: complete integer paths/read faults."""
import argparse,copy,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
import validate_native_island_agent_phase_tu15 as physical
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
BASE=0x1000000
BODY,PROPS,COMPONENT_POOL,COMPONENTS,UNIT_POOL,UNITS,UNIT,ROSTER=(BASE+n for n in(0x100,0x300,0x500,0x600,0x900,0xa00,0xe00,0x1000))
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata(image):
 code=_Code(image,'.text');body=complete_body(code,0x283380);assert len(body.code)==68;names={s:n for n,s in enumerate(('eax','ecx','edx','ebx','esp','ebp','esi','edi'))};sites=[];listing=[]
 for pc in sorted(body.code):
  i=code.at(pc);ops=[];listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   assert not op.mem.segment
   ops.append(dict(base=names.get(i.reg_name(op.mem.base),-1),index=names.get(i.reg_name(op.mem.index),-1),scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
  sites.append(dict(instruction=pc,operands=ops))
 return dict(sites=sites,body_sites=68,listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest(),regions=[],boundaries=[])
def cases():
 rows=[]
 def make(name,properties=(),count=None,flags=0,parent=0x12345678,members=(),member_count=None,datum=0):
  data=bytearray(0x4000)
  def put(p,v,size=4):data[p-BASE:p-BASE+size]=(v&((1<<(size*8))-1)).to_bytes(size,'little')
  put(BODY+0x30,PROPS);put(BODY+0x34,len(properties)if count is None else count)
  for n,(key,value)in enumerate(properties):put(PROPS+n*8,key);put(PROPS+n*8+4,value)
  put(COMPONENT_POOL+0x44,COMPONENTS);put(UNIT_POOL+0x44,UNITS)
  for index in range(3):put(COMPONENTS+index*160+8,datum)
  put(UNITS+(datum&0xffff)*12+8,UNIT);put(UNIT+0xc0,flags,2);put(UNIT+0xb8,parent);put(ROSTER+0x1d80,len(members)if member_count is None else member_count)
  for n,value in enumerate(members):put(ROSTER+0x1d00+n*4,value)
  banks=[dict(address=BASE,before=data.hex()),* [dict(address=p,before=v.to_bytes(4,'little').hex())for p,v in((0x52be5c,COMPONENT_POOL),(0x4ecbd0,UNIT_POOL),(0x51daf0,ROSTER))]]
  n=len(rows);rows.append(dict(name=name,entry=0x283380,receiver=0,arguments=[BODY],body=BODY,banks=sorted(banks,key=lambda b:b['address']),before_fp=[0x27f|((n%4)<<10),(n%8)<<11,0x1f80|((n%4)<<13)],stop_pc=physical.STOP,boundary=None))
 for properties in((),((1,2),),((0x2001,0xffffffff),),((0x2001,0),),((1,2),(0x2001,1),(0x2001,0xffffffff)),((1,2),(3,4),(0x2001,0xabcd0002))):
  for flags in(0,1,2,4,6,0xfff9):
   for present in(False,True):make('properties_'+str(len(rows)),properties=properties,flags=flags,members=(4,0x12345678,7)if present else(4,6,7))
 for count in(0xffffffff,0x80000000):make('signed_property_count_'+str(count),properties=((0x2001,0xffffffff),),count=count,flags=6,members=(0x12345678,))
 for count in(0xffffffff,0x80000000,0):make('signed_roster_count_'+str(count),properties=((0x2001,0),),flags=6,members=(0x12345678,),member_count=count)
 make('parent_sentinel',flags=6,parent=0xffffffff)
 make('lower16_unit_datum',properties=((0x2001,0),),flags=6,members=(0x12345678,),datum=0xffff0042)
 make('fault_seed_default',flags=6,members=(4,6,0x12345678))
 make('fault_seed_property',properties=((4,5),(0x2001,0xabcd0002)),flags=4,members=(4,6,7))
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=metadata(image);physical.boundary=lambda c,o:None;rows=physical.physical(image,meta,cases(),scratch);prefix=[]
  for seed in(c for c in rows if c['name'].startswith('fault_seed')):
   seen=set();occurrences={}
   for event in seed['reads']:
    key=event['address'],event['bytes'];occurrences[key]=occurrences.get(key,0)+1
    if event['instruction']in seen:continue
    seen.add(event['instruction']);c=copy.deepcopy(seed);c.update(name=seed['name']+'_read_'+hex(event['instruction']),stop_pc=event['instruction'],fault=dict(address=key[0],bytes=key[1],occurrence=occurrences[key]));prefix.append(c)
  rows+=physical.physical(image,meta,prefix,scratch);fixture=scratch/'eligibility.json';save(fixture,dict(image_sha256=NEW_PIN,cases=rows));project=scratch/'project';project.mkdir();source=ROOT/'src/physics/native_tu15_contact_query.cpp';test=ROOT/'tests/native_tu15_contact_body_eligibility_tests.cpp';(project/'native.cpp').write_bytes(source.read_bytes());text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_flags','flags&6u','flags&2u'),('wrong_default','std::uint32_t property{}','std::uint32_t property{0xffffffffu}'),('wrong_membership','if(m.word(cursor)==parent)return false;','if(m.word(cursor)==parent)return true;')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True);assert r.returncode!=0 and any(x in r.stderr for x in("eligibility result", "eligibility ordered read")),(r.returncode,r.stdout,r.stderr)\nprint("Original eligibility rejects semantic mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_contact_body_eligibility LANGUAGES CXX)
include(CTest)
add_library(driver OBJECT "{test}")
add_library(dependencies OBJECT "{ROOT}/src/physics/native_tu15_inverse_geometry.cpp" "{ROOT}/src/physics/native_tu15_geometry.cpp")
foreach(target driver dependencies)
 target_include_directories(${{target}} PRIVATE "{ROOT}/include")
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
'''
  for mode in('native',*variants):
   cmake+=f'add_executable(eligibility_{mode} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {mode}.cpp)\ntarget_include_directories(eligibility_{mode} PRIVATE "{ROOT}/include")\ntarget_compile_features(eligibility_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(eligibility_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n';negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME eligibility_{mode} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:eligibility_{mode}> "{fixture}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','eligibility_native',*sum((['--target','eligibility_'+name]for name in variants),[]),'--filter','^eligibility_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1'];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  files=[source,ROOT/'include/halo2/native_tu15_contact_query.hpp',test,Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files};report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,original_sha256=NEW_PIN,source_hashes=pins,body_sites=68,listing_sha256=meta['listing_sha256'],original_cases=len(rows),complete_original_calls=len(rows)-len(prefix),read_fault_prefixes=len(prefix),ordered_reads=sum(len(c['reads'])for c in rows),original_instructions=sum(c['original_instructions']for c in rows),negative_controls=list(variants),supplied_original_outcomes=False,instruction_patches=[],production_native_only_closure=False)
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
