#!/usr/bin/env python3
"""Native2E4FE0 query with genuine remaining services and BEFORE-only inputs."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
from capstone import CS_AC_READ,CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def save(path,value):path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
def metadata(native_cache=False,native_shapes=False,native_convex=False):
 code=_Code(Xbe(NEW_DEFAULT),'.text');rows={}
 entries=(0x2e4fe0,0x2e4550,0x283380)+( (0x309060,0x309150,0x2e4690,0x2e47b0)if native_cache else())+((0x2e5da0,0x3077c0,0x2e7e10,0x306750,0x2e5db0,0x3077f0,0x1f6610,0x306790,0x2e7d90,0x1cb030)if native_shapes else())+((0x2f8630,0x2e4760,0x30d350)if native_convex else())
 for entry in entries:
  for pc in sorted(complete_body(code,entry).code):
   i=code.at(pc);ops=[]
   for op in i.operands:
    if op.type!=X86_OP_MEM or i.mnemonic=='lea'or pc in(0x3078a2,0x3078a9):continue
    assert not op.mem.segment
    access=CS_AC_READ if i.mnemonic in('test','cmp')else CS_AC_WRITE if i.mnemonic in('fst','fstp')else op.access
    assert access&(CS_AC_READ|CS_AC_WRITE),(hex(pc),i.mnemonic,i.op_str)
    ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(access&CS_AC_READ),write=bool(access&CS_AC_WRITE)))
   rows[str(pc)]=dict(memory=ops)
 return rows
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--context',action='append',choices=['gemini-column0','gemini-column1','elongation-crates']);p.add_argument('--native-cache',action='store_true');p.add_argument('--native-shapes',action='store_true');p.add_argument('--native-convex',action='store_true');p.add_argument('--variants',nargs='+',type=int,default=[0,1,2,3,4,5]);a=p.parse_args();a.native_shapes=a.native_shapes or a.native_convex;a.native_cache=a.native_cache or a.native_shapes;a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_contact_query.cpp';header=ROOT/'include/halo2/native_tu15_contact_query.hpp';fragment=ROOT/'tools/observe_native_contact_query_tu15.cpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';files=[source,header,fragment,bridge,ROOT/'src/physics/native_tu15_inverse_geometry.cpp',ROOT/'src/physics/native_tu15_geometry.cpp',Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files}
  if a.native_cache:
   for path in('src/physics/native_tu15_contact_cache.cpp','include/halo2/native_tu15_contact_cache.hpp'):pins[path]=sha(ROOT/path)
  if a.native_shapes:
   for path in('src/physics/native_tu15_contact_shape.cpp','include/halo2/native_tu15_contact_shape.hpp','src/physics/native_tu15_contact_agent_factory.cpp','include/halo2/native_tu15_contact_agent_factory.hpp','src/physics/native_tu15_contact_unwind_internal.hpp'):pins[path]=sha(ROOT/path)
  if a.native_convex:
   for path in('src/physics/native_tu15_convex_contact_query.cpp','include/halo2/native_tu15_convex_contact_query.hpp'):pins[path]=sha(ROOT/path)
  contexts=[];cases=[]
  for name in(a.context or['gemini-column0']):
   capture=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'/('iteration-04-'+name)/'original/required-failure-inputs';original=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(original)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(original/'original-hardware').exists(),r.stderr[-3000:];contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),original_reference_command=command,original_reference_raw_exit=r.returncode));cases.extend(dict(name=f'{name}_{ordinal}_{variant}',ordinal=ordinal,variant=variant,capture=capture,original=original)for ordinal in range(3 if name=='elongation-crates'else 2)for variant in a.variants)
  report['contexts']=contexts
  instructions=scratch/'instructions.json';save(instructions,metadata(a.native_cache,a.native_shapes,a.native_convex));project=scratch/'project';project.mkdir();shutil.copyfile(source,project/'native.cpp');shutil.copytree(ROOT/'include',project/'include');shared=bridge.read_text().split('int main(int argc,char**argv)')[0];(project/'observer.cpp').write_text('#include "halo2/native_tu15_contact_query.hpp"\n'+('#include "halo2/native_tu15_contact_cache.hpp"\n#define HALO2_NATIVE_QUERY_CACHE 1\n'if a.native_cache else'')+('#include \"halo2/native_tu15_contact_shape.hpp\"\n#include "halo2/native_tu15_contact_agent_factory.hpp"\n#define HALO2_NATIVE_QUERY_SHAPES 1\n'if a.native_shapes else'')+('#include \"halo2/native_tu15_convex_contact_query.hpp\"\n#define HALO2_NATIVE_QUERY_CONVEX 1\n'if a.native_convex else'')+shared+fragment.read_text());text=source.read_text();variants={}
  for name,needle,replacement in(('wrong_motion_state','query_child+0xf0u,final_state','query_child+0xf0u,final_state^1u'),('wrong_setting_restore','current_settings,restore,1','current_settings,restore^1u,1'),('wrong_valid_result','valid_result,valid,1','valid_result,valid^1u,1')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and not j["passed"],(r.returncode,j)\nprint("Unchanged original query rejects semantic mutation")\n')
  cmake=f'''cmake_minimum_required(VERSION 3.20)
project(native_contact_query LANGUAGES CXX)
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
  if a.native_cache:
   shutil.copyfile(ROOT/'src/physics/native_tu15_contact_cache.cpp',project/'cache.cpp');cmake+='target_sources(dependencies PRIVATE cache.cpp)\n'
  if a.native_shapes:
   shutil.copyfile(ROOT/'src/physics/native_tu15_contact_shape.cpp',project/'shape.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_agent_factory.cpp',project/'factory.cpp');shutil.copyfile(ROOT/'src/physics/native_tu15_contact_unwind_internal.hpp',project/'native_tu15_contact_unwind_internal.hpp');cmake+='target_sources(dependencies PRIVATE shape.cpp factory.cpp)\n'
  if a.native_convex:
   shutil.copyfile(ROOT/'src/physics/native_tu15_convex_contact_query.cpp',project/'convex.cpp');cmake+='target_sources(dependencies PRIVATE convex.cpp)\n'
  for mode in('native',*variants):
   cmake+=f'add_executable(query_{mode} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:dependencies> {mode}.cpp)\ntarget_include_directories(query_{mode} PRIVATE include)\ntarget_compile_features(query_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(query_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   selected=next((c for c in cases if c['ordinal']==0 and c['variant']==1),cases[0])
   for c in(cases if mode=='native'else[selected]):
    relative=f'evidence/{mode}_{c["name"]}.json';retain.append(relative);negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME query_{mode}_{c["name"]} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:query_{mode}> "{c["original"]}/original-hardware" "{c["capture"]}/capture.json" {c["ordinal"]} {c["variant"]} "{instructions}" "${{CMAKE_BINARY_DIR}}/{relative}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','query_native',*sum((['--target','query_'+name]for name in variants),[]),'--filter','^query_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,source_hashes=pins,sources_unchanged=unchanged,original_sha256=NEW_PIN,native_composed_owners=['2E4FE0','2E4550','283380']+(['309060','309150','2E4690','2E47B0']if a.native_cache else[])+(['2E5DA0','3077C0','2E7E10','306750','2E5DB0','3077F0','1F6610','306790','2E7D90','1CB030','0A9520','0AB4B0']if a.native_shapes else[])+(['2F8630','2E4760','30D350']if a.native_convex else[]),original_cases_per_compiler=len(cases),negative_controls=list(variants),declared_variants=a.variants,supplied_original_outcomes=False,captured_after_consumed=False,remaining_services=['30E930 simplex solver']if a.native_convex else['2F8630 GJK']if a.native_shapes else['shape class','shape center/support/encoding','2F8630 GJK']if a.native_cache else['shape class','309060/309150 cache initialization','2F8630 GJK'],production_native_only_closure=False,remaining=['fault/edge physical qualification','query+constructor source-data lifetime','full304350 closure and live integration'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
