#!/usr/bin/env python3
"""Syntax-only context for the isolated, still-unqualified B8F40 draft."""
from pathlib import Path
import hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-object-tick-stage-20261102/staged'
OUT=ROOT/'analysis/physics/native-object-tick-context-20261102-first'
def main():
 OUT.mkdir(exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,syntax_only=True,parent_physically_qualified=False,owned_scratch=str(owner),retained_large_artifacts=[],results=[])
 try:
  for cc in('g++','clang++'):
   command=[cc,'-std=c++20','-O0','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(STAGE/'include'),'-I'+str(ROOT/'include'),'-fsyntax-only',str(STAGE/'src/physics/native_tu15_object_tick.cpp')]
   run=subprocess.run(command,capture_output=True,text=True);(OUT/(cc+'.log')).write_text(run.stdout+run.stderr);r['results'].append(dict(compiler=cc,exit_code=run.returncode,command=command));assert run.returncode==0,run.stderr
  r.update(passed=True,source_sha256={str(p.relative_to(STAGE)):hashlib.sha256(p.read_bytes()).hexdigest()for p in STAGE.rglob('*')if p.is_file()},scope='Both compilers accept isolated semantic parent draft. No original comparison, callback closure, binding or live assertion.')
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file());(OUT/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
