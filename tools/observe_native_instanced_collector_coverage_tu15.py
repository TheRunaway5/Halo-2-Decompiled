#!/usr/bin/env python3
"""Independent unchanged-source site census for returned collector fixtures."""
import argparse,hashlib,json,os
from pathlib import Path
import validate_native_instanced_collector_composed_tu15 as component
from oracle_smg_firing_1_1 import Xbe,_Code
from generate_physics_live_support_tu15 import complete_body
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);owned=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(owned),retained_large_artifacts=[])
 try:
  image=Xbe(component.base.NEW_DEFAULT);assert hashlib.sha256(Path(component.base.NEW_DEFAULT).read_bytes()).hexdigest()==component.base.NEW_PIN;cases=component.cases(image);digest=hashlib.sha256(json.dumps(cases,sort_keys=True,separators=(',',':')).encode()).hexdigest();meta=component.metadata(image)
  component.base.physical.observer_source=component.parent.observer_source
  component.base.physical.boundary=lambda case,actual:None
  rows=component.parent.support.parent.ORIGINAL_PHYSICAL(image,meta,cases,owned);executed={pc for row in rows for pc in row['executed_sites']};sites=set(complete_body(_Code(image,'.text'),0x24df20).code)
  r.update(passed=True,cases=len(rows),original_collector_executed_sites=len(sites&executed),original_collector_unexecuted_sites=sorted(sites-executed),declared_input_cases_sha256=digest,source_cpp_sha256=hashlib.sha256((ROOT/'src/physics/native_tu15_instanced_ray_callbacks.cpp').read_bytes()).hexdigest(),native_comparison_separate='native-instanced-collector-composed-tu15-20261102-third/report.json',scope=__doc__)
 except BaseException as e:r['error']=str(e)or repr(e);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
 return 0
if __name__=='__main__':raise SystemExit(main())
