#!/usr/bin/env python3
"""Compile the staged World point adapter against its explicit frozen headers."""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
def sha(p):return hashlib.file_digest(Path(p).open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--base',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage.resolve();base=a.base.resolve();source=stage/'src/physics/native_tu15_world_integration_residuals.cpp';pins=json.loads((stage.parent/'source-pins.json').read_text())['staged_hashes'];report=dict(passed=False,owned_scratch=str(scratch),stage=str(stage),base=str(base),source_sha256=sha(source),commands=[],results=[],retained_large_artifacts=[],scope='Actual staged World point-handler object and both stats headers under GCC/Clang; no linked/fullWorld/live claim.')
 try:
  assert all(sha(stage/name)==digest for name,digest in pins.items())
  for compiler in('g++','clang++'):
   command=[compiler,'-std=c++20','-O0','-g0','-I'+str(stage/'include'),'-I'+str(base/'include'),'-I'+str(base/'src/physics'),'-c',str(source),'-o',str(scratch/(compiler+'.o'))];report['commands'].append(command);result=subprocess.run(command,capture_output=True,text=True);report['results'].append(dict(compiler=compiler,exit_code=result.returncode,stdout=result.stdout[-8192:],stderr=result.stderr[-32768:]));assert result.returncode==0,result.stderr[-8192:]
  report['passed']=all(sha(stage/name)==digest for name,digest in pins.items())
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
