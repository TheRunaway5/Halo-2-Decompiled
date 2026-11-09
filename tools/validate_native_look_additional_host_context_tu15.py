#!/usr/bin/env python3
"""Both complete current LiveWorld syntax contexts on an owned header snapshot."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-look-additional-host-stage-20261102'
OUT=ROOT/'analysis/physics/native-look-additional-host-context-20261102-first'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 OUT.mkdir(exist_ok=False);owned=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[],results=[])
 try:
  project=owned/'project';project.mkdir();shutil.copytree(ROOT/'include',project/'include')
  for p in(ROOT/'src').rglob('*'):
   if p.is_file()and p.suffix in('.h','.hpp','.inc'):
    q=project/p.relative_to(ROOT);q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
  pins=json.loads((STAGE/'source-pins.json').read_text())['source_sha256']
  for name,pin in pins.items():
   p=STAGE/'staged'/name;assert sha(p)==pin,name;q=project/name;q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
  for cc in('g++','clang++'):
   command=[cc,'-std=c++20','-O0','-fno-fast-math','-ffp-contract=off','-frounding-math','-isystem','/usr/include/SDL2',
    '-I'+str(project/'include'),'-I'+str(project/'src'),'-I'+str(project/'src/physics'),'-fsyntax-only',str(project/'src/physics/physics_live_world_tu15.cpp')]
   run=subprocess.run(command,capture_output=True,text=True);(OUT/(cc+'-LiveWorld.log')).write_text(run.stdout+run.stderr);r['results'].append(dict(compiler=cc,exit_code=run.returncode,command=command));assert run.returncode==0,run.stderr[-6000:]
  r.update(passed=True,stage_source_sha256=pins,header_snapshot_sha256={str(p.relative_to(project)):sha(p)for p in project.rglob('*')if p.is_file()and p.suffix in('.h','.hpp','.inc')},
   scope='Both full current LiveWorld TU syntax on immutable owned current-header snapshot plus exact six-file stage. No linked/runtime/active1481 claim; acceptance fragments are separately compiled.')
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());(OUT/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
