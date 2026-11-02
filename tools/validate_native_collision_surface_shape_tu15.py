#!/usr/bin/env python3
"""Physical original1F5FE0 surface constructors from actual MCC before inputs."""
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
 code=_Code(Xbe(NEW_DEFAULT),'.text');body=complete_body(code,0x1f5fe0);assert len(body.code)==123;rows={};listing=[]
 for pc in sorted(body.code):
  i=code.at(pc);listing.append(f'{pc:x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n');ops=[]
  for op in i.operands:
   if op.type!=X86_OP_MEM or i.mnemonic=='lea':continue
   assert not op.mem.segment
   ops.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
  rows[str(pc)]=dict(memory=ops)
 return rows,hashlib.sha256(''.join(listing).encode()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);p.add_argument('--context',action='append',choices=['gemini-column0','gemini-column1','elongation-crates']);p.add_argument('--producer',type=int);p.add_argument('--compilers',nargs='+',default=['g++','clang++']);p.add_argument('--variants',nargs='+',type=int,default=[0,1,2]);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],declared_context_selection=a.context,declared_producer_selection=a.producer)
 try:
  assert sha(NEW_DEFAULT)==NEW_PIN;source=ROOT/'src/physics/native_tu15_collision_surface_shape.cpp';header=ROOT/'include/halo2/native_tu15_collision_surface_shape.hpp';fragment=ROOT/'tools/observe_native_collision_surface_shape_tu15.cpp';bridge=ROOT/'tools/observe_native_contact_producer_tu15.cpp';files=[source,header,fragment,bridge,Path(__file__)];pins={str(p.relative_to(ROOT)):sha(p)for p in files};meta,listing=metadata();instructions=scratch/'instructions.json';save(instructions,meta);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include');shutil.copyfile(source,project/'native.cpp');shared=bridge.read_text().split('int main(int argc,char**argv)')[0];driver=project/'observer.cpp';driver.write_text('#include "halo2/native_tu15_collision_surface_shape.hpp"\n'+shared+fragment.read_text());probe=scratch/'enumerator';compile=['g++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(project/'include'),str(driver),str(project/'native.cpp'),'-o',str(probe)];r=subprocess.run(compile,capture_output=True,text=True);assert r.returncode==0,r.stderr[-6000:];contexts=[];cases=[]
  base=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-world-lift-original-v1458'
  for name in(a.context or('gemini-column0','gemini-column1','elongation-crates')):
   capture=base/('iteration-04-'+name)/'original/required-failure-inputs';original=scratch/('original-'+name);command=[sys.executable,str(ROOT/'tools/replay_live_phantom_hardware_tu15.py'),'--native-frame-semantics','--return-breakpoints','--capture',str(capture),'--output',str(original)];r=subprocess.run(command,capture_output=True,text=True,timeout=180);assert(original/'original-hardware').exists(),r.stderr[-3000:];cohort=[]
   for producer in([a.producer]if a.producer is not None else range(3 if name=='elongation-crates'else 2)):
    output=a.output/(name+'-producer-'+str(producer)+'-constructors.json');cmd=[str(probe),str(original/'original-hardware'),str(capture/'capture.json'),str(producer),'list','0',str(instructions),str(output)];e=subprocess.run(cmd,capture_output=True,text=True,timeout=180);assert e.returncode==0,e.stderr[-3000:];entries=json.loads(output.read_text())['constructors'];cohort.append(dict(producer_ordinal=producer,constructors=entries,enumeration_report_sha256=sha(output)));cases.extend(dict(name=f'{name}_{producer}_{entry["ordinal"]}_{variant}',producer=producer,constructor=entry['ordinal'],variant=variant,placement_transform=entry['placement_transform'],elf=original/'original-hardware',capture=capture/'capture.json')for entry in entries for variant in a.variants)
   contexts.append(dict(name=name,capture=str(capture),capture_sha256=sha(capture/'capture.json'),reference_command=command,reference_raw_exit=r.returncode,enumerated_producers=cohort));print(name,sum(len(p['constructors'])for p in cohort),'genuine constructors',flush=True)
  assert cases;variants={};text=source.read_text()
  for name,needle,replacement in(('wrong_vertex_tag','tag_count|0x3f000000u','tag_count|0x3f800000u'),('wrong_encoded_key','p+8u,key);return shape;','p+8u,key^1u);return shape;'),('skip_placement_scale','if(needs_scale(scale,scalar(one)))','if(false&&needs_scale(scale,scalar(one)))')):
   assert text.count(needle)==1;variants[name]=text.replace(needle,replacement);(project/(name+'.cpp')).write_text(variants[name])
  selected=next((c for c in cases if c['placement_transform']and c['variant']==1),None)
  if selected is None:variants.pop('skip_placement_scale')
  (project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error") and not j["passed"],(r.returncode,j)\nprint("Unchanged original surface rejects native semantic mutation")\n')
  cmake='''cmake_minimum_required(VERSION 3.20)
project(native_collision_surface LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_library(driver OBJECT observer.cpp)
target_include_directories(driver PRIVATE include)
target_compile_features(driver PRIVATE cxx_std_20)
target_compile_options(driver PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(driver PRIVATE nlohmann_json::nlohmann_json)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")
''';retain=[]
  for mode in('native',*variants):
   cmake+=f'add_executable(surface_{mode} $<TARGET_OBJECTS:driver> {mode}.cpp)\ntarget_include_directories(surface_{mode} PRIVATE include)\ntarget_compile_features(surface_{mode} PRIVATE cxx_std_20)\ntarget_compile_options(surface_{mode} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
   for c in(cases if mode=='native'else[selected or cases[0]]):
    relative='evidence/'+mode+'_'+c['name']+'.json';retain.append(relative);negative=''if mode=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME surface_{mode}_{c["name"]} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:surface_{mode}> "{c["elf"]}" "{c["capture"]}" {c["producer"]} {c["constructor"]} {c["variant"]} "{instructions}" "${{CMAKE_BINARY_DIR}}/{relative}")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in a.compilers:
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','surface_native',*sum((['--target','surface_'+name]for name in variants),[]),'--filter','^surface_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','1','--test-jobs','1',*sum((['--retain-report',p]for p in retain),[])];r=subprocess.run(command);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  unchanged=all(sha(ROOT/p)==pin for p,pin in pins.items());report.update(passed=all(r['exit_code']==0 for r in results)and unchanged,compiler_results=results,contexts=contexts,source_hashes=pins,sources_unchanged=unchanged,original_sha256=NEW_PIN,body_sites=123,listing_sha256=listing,original_cases_per_compiler=len(cases),transformed_input_cases=sum(bool(c['placement_transform'])for c in cases),negative_controls=list(variants),declared_variants=a.variants,supplied_original_outcomes=False,captured_after_consumed=False,production_native_only_closure=False,remaining=['prefix/alias/fault qualification','complete resolver1F7C10 integration','constructor opaque-data lifetime and full304350 closure','visible multiplayer integration'])
 except BaseException as error:report['error']=str(error);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
