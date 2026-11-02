#!/usr/bin/env python3
"""Native2F8630/2E4760/30D350 with genuine30E930 and BEFORE-only inputs."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def save(p,j):p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def metadata():
 code=_Code(Xbe(NEW_DEFAULT),'.text');rows={};listings={};counts=[]
 for owner in(0x2f8630,0x2e4550,0x2e4760,0x30d350):
  body=complete_body(code,owner);counts.append(len(body.code));listing=[]
  for pc in sorted(body.code):
   i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
    assert not op.mem.segment
    access=CS_AC_READ if i.mnemonic in('test','cmp')else CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
    ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   rows[str(pc)]=dict(memory=ops)
  listings[hex(owner)]=hashlib.sha256(''.join(listing).encode()).hexdigest()
 assert counts[0]==200 and counts[2:]==[24,45]
 return rows,dict(body_sites=counts,listing_sha256=listings)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--context',action='append',choices=['gemini-column0','gemini-column1','elongation-crates']);p.add_argument('--producer',type=int);p.add_argument('--inventory-group',type=int,choices=[0,1,2]);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--variants',nargs='+',type=int,default=list(range(10)));a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],dependency_inventory_group=a.inventory_group)
 if a.inventory_group is not None:os.environ["HALO2_SIMPLEX_INVENTORY_GROUP"]=str(a.inventory_group)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_convex_contact_query.cpp';header=ROOT/'include/halo2/native_tu15_convex_contact_query.hpp';fragment=ROOT/'tools/observe_native_convex_contact_query_tu15.cpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';files=[source,header,fragment,bridge,ROOT/'src/physics/native_tu15_inverse_geometry.cpp',ROOT/'src/physics/native_tu15_geometry.cpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files};meta,authority=metadata();instructions=scratch/'instructions.json';save(instructions,meta);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'native.cpp');(project/'observer.cpp').write_text('#include "halo2/native_tu15_convex_contact_query.hpp"\n'+bridge.read_text().split('int main(int argc,char**argv)')[0]+fragment.read_text());contexts=[];cases=[];base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in(a.context or('gemini-column0','gemini-column1','elongation-crates')):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';original=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(original)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(original/'original-hardware').exists(),r.stderr[-3000:]
   for producer in([a.producer]if a.producer is not None else range(3 if name=='elongation-crates'else 2)):
    cases.extend(dict(name=f'{name}_{producer}_{variant}',producer=producer,variant=variant,elf=original/'original-hardware',capture=capture/'capture.json')for variant in a.variants)
   contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_raw_exit=r.returncode));print(name,'convex query source inputs ready',flush=True)
  text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_valid_result','m.word(0x2f8882u,valid_result,1,1)','m.word(0x2f8882u,valid_result,0,1)'),('wrong_fast_projection','reversed_minus(m.word(child.identity+0x2cu));','plus(m.word(child.identity+0x2cu));'),('wrong_result_distance','m.word(0x30d363u,output+0x20u,distance)','m.word(0x30d363u,output+0x20u,distance^1u)')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  if a.inventory_group is not None:variants.clear()
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and not j["passed"],(r.returncode,j)\nprint("Unchanged original convex query rejects native mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_convex_contact_query LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT observer.cpp)
add_library(dependencies OBJECT "{ROOT}/src/physics/native_tu15_inverse_geometry.cpp" "{ROOT}/src/physics/native_tu15_geometry.cpp")
foreach(target driver dependencies)
 target_include_directories(${{target}} PRIVATE include)
 target_compile_features(${{target}} PRIVATE cxx_std_20)
 target_compile_options(${{target}} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
endforeach()
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${{CMAKE_BINARY_DIR}}/evidence")
''';retain=[]
  for mode in('native',*variants):
   cmake+=f'add_executable(convex_{mode} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {mode}.cpp)\ntarget_include_directories(convex_{mode} PRIVATE include)\ntarget_compile_features(convex_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(convex_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   selected=next((c for c in cases if c['variant']==(3 if mode=='wrong_fast_projection'else 6)),cases[0])
   for c in(cases if mode=='native'else[selected]):
    relative='evidence/'+mode+'_'+c['name']+'.json';retain.append(relative);negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME convex_{mode}_{c["name"]} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:convex_{mode}> "{c["elf"]}" "{c["capture"]}" {c["producer"]} {c["variant"]} "{instructions}" "${{CMAKE_BINARY_DIR}}/{relative}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','convex_native',*sum((['--target','convex_'+name]for name in variants),[]),'--filter','^convex_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,source_hashes=pins,sources_unchanged=unchanged,original_sha256=NEW_PIN,authority=authority,original_cases_per_compiler=len(cases),negative_controls=list(variants),declared_variants=a.variants,supplied_original_outcomes=False,captured_after_consumed=False,production_native_only_closure=False,remaining=['30E930 and transitive native solver closure','profiling clock owner','fault/alias/observer-prefix qualification','wholequery/304350 and visible multiplayer integration'])
 except BaseException as error:report['error']=str(error)or repr(error);report['error_type']=type(error).__name__;raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
