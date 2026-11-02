#!/usr/bin/env python3
"""Compile both actual full Host/LiveWorld TUs for the frozen native look stage."""
import argparse,json,os,shlex,shutil,subprocess
from pathlib import Path
from prepare_native_look_pipeline_world_tu15 import ROOT,sha,save

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);p.add_argument('--base',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);stage=a.stage/'staged';owned=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[])
 try:
  tests=owned/'tests';tests.mkdir();shutil.copytree(a.base/'tests/helpers',tests/'helpers')
  for p in(a.base/'tests').iterdir():
   if p.is_file()and p.suffix in('.hpp','.h'):shutil.copyfile(p,tests/p.name)
  host='tests/physics_multiplayer_acceptance.cpp';live='src/physics/physics_live_world_tu15.cpp';shutil.copyfile(stage/host,tests/Path(host).name)
  actual_flags=json.loads((ROOT/'analysis/physics/native-seated-look-input-capture-host-context-20261102-second/report.json').read_text())['compiler_results'][0]['command'][1:-2]
  flags=[]
  for s in actual_flags:
   if s.startswith('-I'):flags.append('-I'+str(a.base/'include'))
   else:flags.append(s)
  results=[]
  for cc in('g++','clang++'):
   for kind,input,options in(('Host',tests/Path(host).name,flags),('LiveWorld',stage/live,['-std=c++20','-O0','-fno-fast-math','-ffp-contract=off','-frounding-math','-isystem','/usr/include/SDL2'])):
    command=[cc,'-I'+str(stage/'include'),'-I'+str(a.base/'include'),'-I'+str(a.base/'src'),*options,'-fsyntax-only',str(input)];r=subprocess.run(command,capture_output=True,text=True);(a.output/(cc+'-'+kind+'.log')).write_text(r.stdout+r.stderr);results.append(dict(compiler=cc,translation_unit=kind,exit_code=r.returncode,command=command));assert r.returncode==0,r.stderr[-4000:]
  report.update(passed=True,compiler_results=results,scope='Full actual Host/LiveWorld syntax contexts only. Native look original/actual/component/world-model qualification remain separately pinned; no live claim.',source_hashes={n:sha(stage/n)for n in(host,live,'include/halo2/native_tu15_physics_frame_residuals.hpp','include/halo2/native_tu15_look_pipeline_binding.hpp')})
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());save(a.output/'report.json',report)
 return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
