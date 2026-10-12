#!/usr/bin/env python3
"""Audit and freeze original callback comparisons and isolated native kernels."""
import argparse,json
from pathlib import Path
from oracle_mp_grenade_callbacks_1_1 import ROOT,OUT,motion,PINS
from oracle_smg_firing_1_1 import Xbe,PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();assert not a.output.exists()
 for name,h in PINS.items():assert motion.digest(ROOT/name)==h,name
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');files={};rows=0
 for path in (OUT/'measured').rglob('*'):
  if path.is_file():assert path.read_bytes()==(OUT/'repeat'/path.relative_to(OUT/'measured')).read_bytes(),path
 for folder in ('measured','repeat'):
  report=json.loads((OUT/folder/'report.json').read_text());assert report['tool_sha256']==motion.digest(ROOT/'tools/oracle_mp_grenade_callbacks_1_1.py')
  assert not report['new_supplied_service_returns'] and not report['instruction_patches'] and len(report['cases'])==15
  for listing in report['instructions']:
   path=OUT/folder/listing['file'];assert motion.digest(path)==listing['sha256']
   for line in path.read_text().splitlines():
    address,data,_=line.split(' ',2);raw=bytes.fromhex(data);assert image.read(int(address,16),len(raw))==raw;rows+=1
  for path in (OUT/folder).rglob('*'):
   if path.is_file():files[str(path.relative_to(ROOT))]=motion.digest(path)
 previous=json.loads((ROOT/'analysis/multiplayer/integration/grenade-bounded-phases/manifest.json').read_text())
 prior={**previous['source_files'],**previous['verified_prior_pins']}
 for name,h in prior.items():assert motion.digest(ROOT/name)==h,name
 for name in ['include/halo2/unit_grenade.hpp','src/unit_grenade.cpp','tests/unit_grenade_tests.cpp','docs/MP_GRENADE_NATIVE_CALLBACK_RESEARCH.md',
  'tools/oracle_mp_grenade_callbacks_1_1.py','tools/export_mp_grenade_callback_fixtures.py','tools/freeze_mp_grenade_callbacks_1_1.py']:files[name]=motion.digest(ROOT/name)
 value=dict(schema=1,source_files=files,verified_prior_pins=prior,source_instruction_rows=rows,original_xbe_sha256=PIN,
  original_report_sha256=motion.digest(OUT/'measured/report.json'),fixture_sha256=motion.digest(OUT/'measured/fixtures.tsv'),
  native_gameplay_update_applied=False,isolated_native_module=True,
  scope='Original direct callback composition and source-created constructor request boundaries only. Earlier begin remains unreturned; no completed action admission, allocator/attachment result, trajectory/fuse/explosion, hardware/graphics success or live Q integration is supplied or claimed.')
 motion.frozen_write(a.output,motion.encoded(value));print(json.dumps(dict(manifest_sha256=motion.digest(a.output),instruction_rows=rows,new_files=len(files),prior_pins=len(prior))))
if __name__=='__main__':main()
