#!/usr/bin/env python3
"""Owned high-ID residency regression against an authenticated active cohort.

Reuses already completed, hash-pinned libraries read-only; it never builds or
mutates the supplied build tree. The native component project is disposable.
This is CPU Scene qualification, not GPU, live split-screen or delivery proof.
"""
import argparse,hashlib,json,os,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def quoted(s):return '"'+str(s).replace('\\','/').replace('"','\\"')+'"'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--prepared-build',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--expect-old-refusal',action='store_true');p.add_argument('--scene-candidate',type=Path);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);build=a.prepared_build.resolve(strict=True);source=build.parent/'source';assert source.is_dir()and build.is_relative_to(ROOT/'.local/test-scratch')
 link=build/'CMakeFiles/path_tracing_scene_tests.dir/link.txt';parts=shlex.split(link.read_text());libraries=[Path(x)if Path(x).is_absolute()else build/x for x in parts[parts.index('-o')+2:]if Path(x).name!='libhalo2_path_tracing.a'];assert libraries and all(p.is_file()for p in libraries)
 pins={str(p):sha(p)for p in libraries};pins[str(link)]=sha(link)
 a.report=a.report.resolve();a.report.mkdir(parents=True,exist_ok=False);project=scratch/'project';project.mkdir();test=ROOT/'tests/path_tracing/scene_tests.cpp';shutil.copyfile(test,project/'driver.cpp');pins[str(test)]=sha(test)
 units='driver.cpp'
 for name in ('scene.cpp','geometry.cpp','color_guide.cpp','emitter_sampling.cpp'):
  candidate=a.scene_candidate.resolve(strict=True)if name=='scene.cpp'and a.scene_candidate else source/'src/rendering/path_tracing'/name
  shutil.copyfile(candidate,project/name);pins[str(candidate)]=sha(candidate);units+=' '+name
 negative=project/'negative.py';negative.write_text('import subprocess,sys\nr=subprocess.run(sys.argv[1:],capture_output=True,text=True)\nassert r.returncode!=0 and "Path Tracing scene exceeds finite geometry/material/light limits" in r.stderr,(r.returncode,r.stdout,r.stderr)\nprint("Actual 77,668-slot preload reproduces old material-cap refusal")\n')
 cmake='cmake_minimum_required(VERSION 3.20)\nproject(material_namespace LANGUAGES CXX)\ninclude(CTest)\nadd_executable(material_namespace '+units+')\ntarget_include_directories(material_namespace PRIVATE '+quoted(source/'include')+' '+quoted(source/'src')+')\ntarget_compile_features(material_namespace PRIVATE cxx_std_20)\ntarget_compile_options(material_namespace PRIVATE -ffp-contract=off)\ntarget_link_libraries(material_namespace PRIVATE '+' '.join(quoted(p)for p in libraries)+')\n';prefix=(quoted(sys.executable)+' '+quoted(negative)+' ')if a.expect_old_refusal else'';cmake+='add_test(NAME material_namespace COMMAND '+prefix+'$<TARGET_FILE:material_namespace>)\n';(project/'CMakeLists.txt').write_text(cmake)
 report=dict(passed=False,scope=__doc__,old_refusal_control=a.expect_old_refusal,scene_candidate=str(a.scene_candidate)if a.scene_candidate else None,prepared_source=str(source),prepared_build=str(build),source_and_library_hashes=pins,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  results=[]
  for compiler in ('g++','clang++'):
   command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(a.report/compiler),'--target','material_namespace','--filter','^material_namespace$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,'--jobs','2','--test-jobs','1']
   r=subprocess.run(command,capture_output=True,text=True);(a.report/(compiler+'-runner.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,exit_code=r.returncode));assert r.returncode==0,r.stdout[-2500:]+r.stderr[-2500:]
  report.update(passed=True,results=results,dependencies_unchanged=all(sha(p)==h for p,h in pins.items()));assert report['dependencies_unchanged']
 except BaseException as e:report['error']=str(e);raise
 finally:(a.report/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(passed=report['passed'],old_refusal_control=a.expect_old_refusal)));return 0
if __name__=='__main__':raise SystemExit(main())
