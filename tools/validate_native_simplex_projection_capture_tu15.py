#!/usr/bin/env python3
"""Qualify exact projection capture routing, guarded DATA, contexts and refusal."""
import argparse,hashlib,json,os,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-simplex-projection-input-capture-v1482-stage-20261103'
COHORT=ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-accepted-contact-original-v1482-20261103'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH'])
 m=json.loads((STAGE/'source-pins.json').read_text());ready=json.loads((COHORT/'ready.json').read_text())
 assert ready['iteration']==1 and sha(COHORT/'source-manifest.json')==m['base_source_manifest_sha256']
 original=Path(ready['source']);source=owner/'context';source.mkdir()
 assert sha(STAGE/'source-pins.json')=='5d830840c8ed2ffcdf02c5164cbf4742a304b5acf4863ff7c99a98c8c344cce0'
 pmap=json.loads((COHORT/'source-manifest.json').read_text())['source_files_sha256']
 for n,h in pmap.items():
  f=original/n;assert f.is_file() and not f.is_symlink() and sha(f)==h,n
  q=source/n;q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(f,q)
 for n,h in m['source_sha256'].items():
  f=source/n;f.parent.mkdir(parents=True,exist_ok=True)
  assert not f.exists() if n not in pmap else sha(f)==pmap[n]==m['base_sha256'][n]
  shutil.copyfile(STAGE/'staged'/n,f);assert sha(f)==h
 driver=ROOT/'tools/observe_native_simplex_projection_capture_tu15.cpp'
 r=dict(passed=False,scope=__doc__,owned_scratch=str(owner),stage_manifest_sha256=sha(STAGE/'source-pins.json'),driver_sha256=sha(driver),models=[],contexts=[],retained_large_artifacts=[])
 try:
  flags=json.loads((ROOT/'analysis/physics/native-seated-look-input-capture-host-context-20261102-second/report.json').read_text())['compiler_results'][0]['command'][1:-2]
  hostflags=[('-I'+str(source/'include')if x.startswith('-I')else x)for x in flags]
  common=['-std=c++20','-fno-fast-math','-ffp-contract=off','-frounding-math','-isystem','/usr/include/SDL2']
  units=['native_tu15_convex_simplex_solver.cpp','native_tu15_convex_simplex_single.cpp','native_tu15_solver_hull_query.cpp','native_tu15_solver_hull_single_vertex.cpp','native_tu15_solver_hull_support.cpp','native_tu15_contact_features.cpp']
  for cc in ('g++','clang++'):
   exe=owner/('capture-'+cc);cmd=[cc,'-O2','-ffunction-sections','-fdata-sections',*common,'-I'+str(source/'include'),'-I'+str(source/'tests'),str(driver),*[str(source/'src/physics'/n)for n in units],'-Wl,--gc-sections','-pthread','-o',str(exe)]
   x=subprocess.run(cmd,capture_output=True,text=True);(out/(cc+'-model-build.log')).write_text(x.stdout+x.stderr);assert x.returncode==0,x.stderr[-3000:]
   modelroot=owner/cc;modelroot.mkdir();x=subprocess.run([str(exe)],env={**os.environ,'HALO2_TEST_SCRATCH':str(modelroot)},capture_output=True,text=True);(out/(cc+'-model.log')).write_text(x.stdout+x.stderr);assert x.returncode==0,x.stderr
   val=json.loads(x.stdout);assert val['passed'];r['models'].append(dict(compiler=cc,**val))
   for name,path in [('Host','tests/physics_multiplayer_acceptance.cpp'),('World','src/physics/native_tu15_world_integration_residuals.cpp'),('Solver','src/physics/native_tu15_convex_simplex_solver.cpp'),('Live','src/physics_live_world_tu15.cpp')]:
    opts=hostflags if name=='Host' else common
    cmd=[cc,'-I'+str(source/'include'),'-I'+str(source/'src'),'-I'+str(source/'src/physics'),*opts,'-fsyntax-only',str(source/path)]
    x=subprocess.run(cmd,capture_output=True,text=True);(out/(cc+'-'+name+'.log')).write_text(x.stdout+x.stderr);r['contexts'].append(dict(compiler=cc,path=path,exit_code=x.returncode,command=cmd));assert x.returncode==0,x.stderr[-4000:]
  assert all(sha(source/n)==sha(STAGE/'staged'/n)==h for n,h in m['source_sha256'].items())
  r['passed']=True
 finally:
  r['allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in owner.rglob('*')if f.is_file())
  (out/'report.json').write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
