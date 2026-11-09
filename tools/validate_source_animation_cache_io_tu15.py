#!/usr/bin/env python3
"""Qualify extraction of the existing synchronous World animation I/O policy."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FILES=('include/halo2/source_animation_cache_io_tu15.hpp','tests/source_animation_cache_io_tu15_tests.cpp','src/physics/physics_live_world_tu15.cpp','tools/validate_source_animation_cache_io_tu15.py')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--world-syntax',action='store_true');args=p.parse_args()
 if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}
 report=dict(passed=False,scratch_directory=str(scratch),retained_large_artifacts=[])
 try:
  binary=scratch/'animation-io';command=['c++','-std=c++20','-O2','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/source_animation_cache_io_tu15_tests.cpp'),str(ROOT/'src/assets/cache.cpp'),'-o',str(binary)]
  if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
  subprocess.run(command,check=True);result=subprocess.run([str(binary)],capture_output=True,text=True)
  if result.returncode:raise ValueError('Animation cache I/O policy regression failed '+result.stdout+result.stderr)
  tests=json.loads(result.stdout)
  syntax=['c++','-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'src/physics'),'-I/usr/include/SDL2','-D_GNU_SOURCE=1','-D_REENTRANT',str(ROOT/'src/physics/physics_live_world_tu15.cpp')]
  if args.world_syntax:subprocess.run(syntax,check=True)
  drift=[name for name in FILES if pins[name]!=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()]
  if drift:raise ValueError('Animation I/O policy proof source drift '+str(drift))
  report.update(passed=True,regression=tests,compile_command=command,world_syntax_command=syntax if args.world_syntax else None,source_sha256=pins,proof_source_drift=drift,scope='Exact previous World policy versus shared helper with declared backed sector/request owners; production SourceRuntimeResources/World receipt algorithms unchanged. Not Xbox async scheduling or live real-asset acceptance.')
  print(result.stdout,end='')
 except BaseException as error:report['error']=str(error);raise
 finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())
