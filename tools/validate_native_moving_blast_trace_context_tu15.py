#!/usr/bin/env python3
"""Full actual acceptance Host TU syntax for the exact narrow trace delta."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-moving-blast-retained-trace-stage-20261102/staged'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source-root',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();base=a.source_root.resolve();a.output.mkdir(parents=True,exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(owner),retained_large_artifacts=[],scope=__doc__)
 try:
  tests=owner/'tests';shutil.copytree(base/'tests',tests);host=STAGE/'tests/physics_multiplayer_acceptance.cpp';shutil.copyfile(host,tests/host.name)
  old=json.loads((ROOT/'analysis/physics/native-seated-look-input-capture-host-context-20261102-second/report.json').read_text())['compiler_results'][0]['command'][1:-2];flags=[('-I'+str(base/'include')if x.startswith('-I')else x)for x in old];results=[]
  pins={str(p.relative_to(base)):sha(p)for p in base.rglob('*')if p.is_file()};hostpin=sha(host)
  for cc in('g++','clang++'):
   cmd=[cc,'-I'+str(base/'include'),'-I'+str(base/'src'),'-I'+str(base/'src/physics'),*flags,'-fsyntax-only',str(tests/host.name)];run=subprocess.run(cmd,capture_output=True,text=True);(a.output/(cc+'.log')).write_text(run.stdout+run.stderr);results.append(dict(compiler=cc,exit_code=run.returncode,command=cmd));assert run.returncode==0,run.stderr[-5000:]
  assert hostpin==sha(host)and all(sha(base/n)==h for n,h in pins.items());r.update(passed=True,results=results,base_source_sha256=pins,staged_host_sha256=hostpin,source_inputs_unchanged=True,linked_or_live_claim=False)
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
