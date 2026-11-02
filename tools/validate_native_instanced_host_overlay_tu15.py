#!/usr/bin/env python3
"""Requalify exact numeric public behavior on additive Host-key overlay."""
import argparse,hashlib,json,os,shutil,sys
from pathlib import Path
import validate_native_instanced_ray_callbacks_tu15 as prefix
import validate_native_instanced_collector_composed_tu15 as composed
import validate_native_camera_instanced_query_tu15 as instance
import validate_native_contact_pressure_tu15 as base
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--stage',required=True,type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);owned=Path(os.environ['HALO2_TEST_SCRATCH']);source=owned/'source';source.mkdir();report=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[],completed_phase_scratch=[])
 try:
  for name in('include','tests','tools'): (source/name).symlink_to(ROOT/name,target_is_directory=True)
  (source/'src/physics').mkdir(parents=True);(source/'src/rendering').symlink_to(ROOT/'src/rendering',target_is_directory=True)
  files=('native_tu15_instanced_ray_callbacks.cpp','native_tu15_camera_instanced_query.cpp','native_tu15_camera_query_leaves.cpp','native_tu15_camera_dynamic_query_leaves.cpp','native_tu15_bsp_segment_query.cpp','native_tu15_bsp_surface_query.cpp','native_tu15_joint_constraint_numeric_internal.hpp')
  for name in files:shutil.copyfile((a.stage/'staged/src/physics'/name)if name=='native_tu15_instanced_ray_callbacks.cpp'else ROOT/'src/physics'/name,source/'src/physics'/name)
  oldargs=sys.argv
  for module,label in((prefix,'numeric-prefix'),(composed,'numeric-fullchild')):
   phase=owned/label;phase.mkdir();os.environ['HALO2_TEST_SCRATCH']=str(phase)
   for target in(base,prefix,composed,instance):target.ROOT=source
   module.__file__=str(source/'tools'/Path(module.__file__).name);sys.argv=[module.__file__,'--output',str(a.output/label)]
   assert module.main()==0,label
   amount=sum(p.lstat().st_blocks*512 for p in phase.rglob('*')if p.is_file()and not p.is_symlink());shutil.rmtree(phase);report['completed_phase_scratch'].append(dict(owned_scratch=str(phase),allocated_bytes_before_cleanup=amount,removed_verified=not phase.exists()))
  os.environ['HALO2_TEST_SCRATCH']=str(owned)
  sys.argv=oldargs
  reports={str((a.output/label/'report.json').relative_to(ROOT)):sha(a.output/label/'report.json')for label in('numeric-prefix','numeric-fullchild')}
  report.update(passed=True,proofs=reports,public_numeric_unchanged=True,host_entry_lifetime_and_actual_caller_proof_required=True,overlay_sha256=sha(source/'src/physics/native_tu15_instanced_ray_callbacks.cpp'))
 except BaseException as e:report['error']=str(e)or repr(e);raise
 finally:report['temporary_allocated_bytes_before_cleanup']=sum(p.lstat().st_blocks*512 for p in owned.rglob('*')if p.is_file()and not p.is_symlink());(a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())
