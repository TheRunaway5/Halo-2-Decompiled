#!/usr/bin/env python3
"""Frozen canonical LiveWorld and acceptance contexts for BSP bootstrap."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-bsp-object-association-host-stage-20261102/staged'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(owner),retained_large_artifacts=[],scope=__doc__)
 try:
  project=owner/'source';project.mkdir();inputs={}
  # Copy code headers only. Large reference fixtures and caches are not
  # dependencies of syntax checks and never enter this disposable snapshot.
  for tree in('include','src','tests'):
   for p in(ROOT/tree).rglob('*'):
    if not p.is_file()or(tree!='include'and p.suffix not in('.h','.hpp','.hh','.hxx','.inc','.inl','.def','.ipp','.tpp')):continue
    n=p.relative_to(ROOT);dst=project/n;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,dst);inputs[str(n)]=sha(dst)
  staged={str(p.relative_to(STAGE)):sha(p)for p in STAGE.rglob('*')if p.is_file()}
  for n in staged:
   dst=project/n;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(STAGE/n,dst);inputs[n]=sha(dst)
  old=json.loads((ROOT/'analysis/physics/native-seated-look-input-capture-host-context-20261102-second/report.json').read_text())['compiler_results'][0]['command'][1:-2];hostflags=[('-I'+str(project/'include')if x.startswith('-I')else x)for x in old];results=[]
  for cc in('g++','clang++'):
   for name,relative,flags in [('Host','tests/physics_multiplayer_acceptance.cpp',hostflags),('LiveWorld','src/physics/physics_live_world_tu15.cpp',['-std=c++20','-O0','-fno-fast-math','-ffp-contract=off','-frounding-math','-isystem','/usr/include/SDL2'])]:
    cmd=[cc,'-I'+str(project/'include'),'-I'+str(project/'src'),'-I'+str(project/'src/physics'),*flags,'-fsyntax-only',str(project/relative)];run=subprocess.run(cmd,capture_output=True,text=True);(a.output/(cc+'-'+name+'.log')).write_text(run.stdout+run.stderr);results.append(dict(compiler=cc,translation_unit=name,exit_code=run.returncode,command=cmd));assert run.returncode==0,run.stderr[-5000:]
  assert all(sha(STAGE/n)==h for n,h in staged.items())and all(sha(project/n)==h for n,h in inputs.items());r.update(passed=True,results=results,stage_source_sha256=staged,frozen_context_sha256=inputs,source_inputs_unchanged=True,canonical_source_edited=False,active_mp_source_edited=False,live_claim=False)
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2)+'\n')
if __name__=='__main__':main()
