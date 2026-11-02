#!/usr/bin/env python3
"""Compile the exact readonly observer hook in its actual native World context."""
import argparse,os,subprocess
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--source-root',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve()/'staged';source=stage/'src/physics/native_tu15_world_integration_residuals.cpp';scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
 try:
  results=[]
  for compiler in('g++','clang++'):
   command=[compiler,'-std=c++20','-O0','-fsyntax-only','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(stage/'include'),'-I'+str(a.source_root/'include'),str(source)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(compiler.replace('+','x')+'.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=compiler,command=command,exit_code=r.returncode));assert r.returncode==0,r.stderr[-3000:]
  report.update(passed=True,compiler_results=results,stage_source_sha256=sha(source),source_hashes={str(p.relative_to(stage)):sha(p)for p in stage.rglob('*')if p.is_file()},scope='Actual complete World translation-unit contextual compile only; no physics execution or native admission')
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0
if __name__=='__main__':raise SystemExit(main())
