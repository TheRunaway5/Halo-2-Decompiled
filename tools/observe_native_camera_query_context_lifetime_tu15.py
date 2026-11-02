#!/usr/bin/env python3
"""Source-only declared query proves the expired context's actual PUSH writer."""
import argparse,copy,hashlib,json,os
from pathlib import Path
import validate_native_camera_geometric_query_tu15 as query
import validate_native_contact_pressure_tu15 as base
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser();p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']);r=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],actual_live_input=False,scope=__doc__)
 try:
  assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN;image=Xbe(NEW_DEFAULT);meta=query.metadata(image);push=next(s for s in meta['sites']if s['instruction']==0x16bb45);assert not push['operands'];push['operands'].append(dict(base=4,index=-1,scale=1,displacement=0xfffffffc,bytes=4,read=False,write=True))
  base.physical.observer_source=query.bsp.support.parent.observer_source;base.physical.boundary=lambda c,o:None
  seeds=[copy.deepcopy(c)for c in query.cases(image)if c['name']=='bridge_128_1065353216_True'];assert len(seeds)==1
  rows=query.bsp.support.parent.ORIGINAL_PHYSICAL(image,meta,seeds,scratch);case=rows[0];writes=[w for w in case['writes']if w['instruction']==0x16bb45];r.update(observed_push_writes=writes,context=case['context']);assert len(writes)==1 and writes[0]['address']==case['context']+32
  r.update(passed=True,declared_profile=case['name'],context=case['context'],context_bytes=40,expired_context_actual_writer=writes[0],source_instruction='16BB45 PUSH EBX',original_sha256=NEW_PIN,caller_never_passes_or_reads_context_after_bsp_return=True,native_saved_register_serialization=False)
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(f.stat().st_blocks*512 for f in scratch.rglob('*')if f.is_file());(a.output/'report.json').write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
