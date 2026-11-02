#!/usr/bin/env python3
"""Actual nativeinfo input association, integer leaf independent of FP depth."""
import argparse,os,shutil,subprocess,sys
from pathlib import Path
from build_native_tu15_constraint_info_before_reference import ConstraintInfoBeforeInput,build_constraint_reference,sha,save
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--archive',type=Path,required=True);p.add_argument('--archive-sha256',required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],actual_live_x87_depth=None)
 units=['constant_constraint_info','extended_constraint_info','contact_collector_info'];files=[*[ROOT/f'src/physics/native_tu15_{n}.cpp'for n in units],*[ROOT/f'include/halo2/native_tu15_{n}.hpp'for n in units],ROOT/'tools/observe_native_constraint_info_actual_tu15.cpp',ROOT/'tools/build_native_tu15_constraint_info_before_reference.py',Path(__file__)];pins={str(f.relative_to(ROOT)):sha(f)for f in files}
 try:
  source=ConstraintInfoBeforeInput(a.archive,a.archive_sha256,scratch/'before');reference=build_constraint_reference(source,scratch/'reference');reference['entry']='Actualnativeinfo typed formals, independentoriginalECXconstraint/stackinfo; emptyx87tags explicitlydeclared, actualdepth absentAPI';save(a.output/'reference.json',reference)
  from capstone import CS_AC_READ,CS_AC_WRITE
  from capstone.x86 import X86_OP_MEM
  assert sha(NEW_DEFAULT)==NEW_PIN;code=_Code(Xbe(NEW_DEFAULT),'.text');metadata={}
  for owner in(0x31d630,0x31f840,0x325e90):
   for pc in sorted(complete_body(code,owner).code):
    i=code.at(pc);operands=[]
    if i.mnemonic!='lea':
     for op in i.operands:
      if op.type!=X86_OP_MEM or op.mem.segment:continue
      operands.append(dict(base=i.reg_name(op.mem.base)or'',index=i.reg_name(op.mem.index)or'',scale=op.mem.scale,displacement=op.mem.disp&0xffffffff,bytes=op.size,read=bool(op.access&CS_AC_READ),write=bool(op.access&CS_AC_WRITE)))
    metadata[str(pc)]=dict(size=i.size,memory=operands)
  save(scratch/'metadata.json',metadata);project=scratch/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for unit in units:shutil.copyfile(ROOT/f'src/physics/native_tu15_{unit}.cpp',project/(unit+'.cpp'))
  prefix=(ROOT/'tools/observe_native_activation_agents_tu15.cpp').read_text().split('int main(int argc,char**argv)')[0];(project/'driver.cpp').write_text('#include "halo2/native_tu15_constant_constraint_info.hpp"\n#include "halo2/native_tu15_extended_constraint_info.hpp"\n'+prefix+(ROOT/'tools/observe_native_constraint_info_actual_tu15.cpp').read_text());cmake='cmake_minimum_required(VERSION 3.20)\nproject(actualInfo LANGUAGES CXX)\ninclude(CTest)\nadd_executable(actual_info driver.cpp '+' '.join(u+'.cpp'for u in units)+')\ntarget_include_directories(actual_info PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(actual_info PRIVATE cxx_std_20)\nadd_test(NAME actual_info COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:actual_info> "'+reference['elf']+'" "'+str(source.path/'capture-native-before.json')+'" "'+str(scratch/'metadata.json')+'" "${CMAKE_BINARY_DIR}/native.json")\n';(project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','actual_info','--filter','^actual_info$','--jobs','1','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--retain-report','native.json'];r=subprocess.run(command,check=False);results.append(dict(compiler=compiler,exit_code=r.returncode,command=command))
  report.update(passed=all(r['exit_code']==0 for r in results)and all(sha(ROOT/f)==h for f,h in pins.items()),source_hashes=pins,compiler_results=results,reference=reference,actual_typed_input=source.data['input'],metadata_boundary_label_is_not_owner_authority=True,scope='Actualtyped owner field/vtable validates31D630 evenwhere diagnosticboundarylabel says31F840. AllcapturedinputDATA/strictoperations/FPscalarpolicy/RET/typedreturn; no FPstackdepth/value capture claim or originalGP/results supplied.')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
