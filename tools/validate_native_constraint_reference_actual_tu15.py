#!/usr/bin/env python3
"""Direct actual22E1B0 formal boundary; no fabricated GP or runtime profile."""
import argparse,json,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_reference_before_reference import ConstraintReferenceBeforeInput,build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 units=['constraint_reference_release','constraint_body_slots','constraint_scalar_cache'];files=[*[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in units],*[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in units],ROOT/'tools/observe_native_constraint_reference_actual_tu15.cpp',ROOT/'tools/build_native_tu15_constraint_reference_before_reference.py',ROOT/'tools/observe_native_activation_agents_tu15.cpp',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files}
 try:
  source=ConstraintReferenceBeforeInput(a.archive,a.archive_sha256,scratch/'before');actual=source.data['input'];instance=actual['instance'];refs=source.word(instance+8,2);flag=source.word(instance+0x10,1);census=dict(source.receipt,actual_input=actual,reference_count=refs,runtime_flag=flag,runtime_selected=refs==1 and flag==1,source_physics_fields_overridden=False);save(a.output/'input-census.json',census);print(json.dumps(census));assert not census['runtime_selected'],'Actual runtime-selected domain needs its separately qualified native service; no parent replay'
  reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Actual selected22E1B0 typed input; ordinary originalEAXinstance/ESIfullDatum marshalled independently, no capturedGP';save(a.output/'reference.json',reference)
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');body=complete_body(code,0x22e1b0);metadata={}
  for pc in sorted(body.code):
   i=code.at(pc);operands=[]
   if i.mnemonic!='lea':
    for op in i.operands:
     if op.type!=X86_OP_MEM or op.mem.segment:continue
     operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
   metadata[str(pc)]=dict(size=i.size,memory=operands)
  save(scratch/'metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for n in units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{n}.cpp',project/(n+'.cpp'))
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_constraint_reference_release.hpp"\n'+prefix+(ROOT/'tools/observe_native_constraint_reference_actual_tu15.cpp').read_text());text=(project/'constraint_reference_release.cpp').read_text();needle='word(memory,instance+8u,2)-1u';assert text.count(needle)==1;(project/'wrong_decrement.cpp').write_text(text.replace(needle,'word(memory,instance+8u,2)-2u'));(project/'negative.py').write_text('import json,subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True);j=json.load(open(sys.argv[-1]));assert r.returncode!=0 and not j.get("error")and any(not c["passed"]for c in j["cases"]),(r.returncode,j)\n')
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(actualReference LANGUAGES CXX)\ninclude(CTest)\nadd_library(driver OBJECT driver.cpp)\nadd_library(children OBJECT constraint_body_slots.cpp constraint_scalar_cache.cpp)\n'
  for target in('driver','children'):cmake+=f'target_include_directories({target} PRIVATE "{project}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
  for variant in('native','wrong_decrement'):
   sourcefile='constraint_reference_release.cpp'if variant=='native'else'wrong_decrement.cpp';cmake+=f'add_executable(reference_{variant} $<TARGET_OBJECTS:driver> $<TARGET_OBJECTS:children> {sourcefile})\ntarget_include_directories(reference_{variant} PRIVATE "{project}/include")\ntarget_compile_features(reference_{variant} PRIVATE cxx_std_20)\n';negative=''if variant=='native'else f'"{sys.executable}" "{project}/negative.py" ';cmake+=f'add_test(NAME reference_{variant} COMMAND "{sys.executable}" "{ROOT}/tools/run_test.py" -- {negative}$<TARGET_FILE:reference_{variant}> "{reference["elf"]}" "{source.path}/capture-native-before.json" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/{variant}.json")\n'
  (project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','reference_native','--target','reference_wrong_decrement','--filter','^reference_','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native.json','--retain-report','wrong_decrement.json'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==h for f,h in pins.items()),source_hashes=pins,compiler_results=results,reference=reference,input_census=census,source_sites=len(body.code),runtime_domain_qualified=False,scope='Actual selected22E1B0 input fast reference decrement; capturedFP plus declaredrounding controls; completebanks/strictoperandorder/FP/RET and rejectedmutation, no runtimeeligibleprofileguess')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
