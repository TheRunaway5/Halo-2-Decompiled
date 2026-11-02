#!/usr/bin/env python3
"""Compile the actual Host translation unit with the staged readonly seat filter."""
import argparse,hashlib,json,os,shlex,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--base',required=True,type=Path);p.add_argument('--stage',required=True,type=Path);p.add_argument('--flags',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);owned=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[],native_admission_added=False)
 try:
  tests=owned/'tests';tests.mkdir();
  for file in a.base.joinpath('tests').iterdir():
   if file.is_file()and file.suffix in('.hpp','.h'):shutil.copyfile(file,tests/file.name)
  shutil.copytree(a.base/'tests/helpers',tests/'helpers');host='tests/physics_multiplayer_acceptance.cpp';helper='tests/helpers/native_seated_look_controller_input_capture_tu15.hpp';shutil.copyfile(a.stage/'staged'/host,tests/Path(host).name);shutil.copyfile(a.stage/'staged'/helper,tests/'helpers'/Path(helper).name)
  flags=[]
  for line in a.flags.read_text().splitlines():
   if line.startswith(('CXX_DEFINES =','CXX_INCLUDES =','CXX_FLAGS =')):flags+=shlex.split(line.split('=',1)[1])
  assert not any(x=='-I'+str(a.base/'tests')for x in flags)
  results=[]
  for cc in('g++','clang++'):
   command=[cc,*flags,'-fsyntax-only',str(tests/Path(host).name)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(cc+'.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=cc,exit_code=r.returncode,command=command));assert r.returncode==0,r.stderr[-3000:]
  report.update(passed=True,compiler_results=results,stage_source_hashes={n:sha(a.stage/'staged'/n)for n in(host,helper)},actual_cmake_flags_file_sha256=sha(a.flags),base_helper_sha256=sha(a.base/'tests/helpers/native_look_controller_input_capture_tu15.hpp'),no_test_include_search_path=True,scope='Full actual1476 Host TU syntax context on both compilers using actual CMake defines/includes/flags; no native admission, link/runtime or performance claim')
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
