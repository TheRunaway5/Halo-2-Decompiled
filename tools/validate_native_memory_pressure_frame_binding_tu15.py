#!/usr/bin/env python3
"""Build the exact staged native pressure Frame bindings in owned scratch."""
import argparse,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.file_digest(p.open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();project=scratch/'project';project.mkdir();report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],scope='Exact staged nativeGC production Frame binding, model and remaining residual failure prefix; nativeGC physical proof separate; no live claim')
 try:
  shutil.copytree(a.source_root/'include',project/'include');pins={}
  files=['tests/native_tu15_physics_frame_tests.cpp','src/physics/native_tu15_physics_frame.cpp','src/physics/native_tu15_physics_frame_residuals.cpp','src/physics/native_tu15_object_publication.cpp','src/physics/native_tu15_activation_services.cpp']
  for n in ['src/physics/native_tu15_memory_pressure.cpp','src/physics/native_tu15_contact_pressure.cpp']:
   if(a.stage/'staged'/n).is_file()or(a.source_root/n).is_file():files.append(n)
  for n in files:
   f=a.stage/'staged'/n
   if not f.is_file():f=a.source_root/n
   t=project/n;t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(f,t);pins[n]=sha(f)
  for f in(a.stage/'staged/include').rglob('*'):
   if f.is_file():t=project/'include'/f.relative_to(a.stage/'staged/include');t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(f,t);pins[str(t.relative_to(project))]=sha(f)
  cmake='cmake_minimum_required(VERSION 3.20)\nproject(NativePressureFrame LANGUAGES CXX)\ninclude(CTest)\nadd_executable(native_pressure_frame '+' '.join('"'+str(project/n)+'"'for n in files)+')\ntarget_include_directories(native_pressure_frame PRIVATE "'+str(project/'include')+'")\ntarget_compile_features(native_pressure_frame PRIVATE cxx_std_20)\ntarget_compile_options(native_pressure_frame PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\nadd_test(NAME native_pressure_frame_model COMMAND "'+sys.executable+'" "'+str(ROOT/'tools/run_test.py')+'" -- $<TARGET_FILE:native_pressure_frame>)\n';(project/'CMakeLists.txt').write_text(cmake);results=[]
  for compiler in('g++','clang++'):
   cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.output/compiler),'--target','native_pressure_frame','--filter','^native_pressure_frame_model$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler];r=subprocess.run(cmd);results.append(dict(compiler=compiler,exit_code=r.returncode,command=cmd))
  report.update(passed=all(r['exit_code']==0 for r in results),compiler_results=results,frozen_source_hashes=pins,stage=str(a.stage.resolve()),source_root=str(a.source_root.resolve()))
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
