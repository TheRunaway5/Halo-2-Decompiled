#!/usr/bin/env python3
"""Compare native1.1 packet outputs with original u5; explicitly adapt site IDs."""
import argparse,json,subprocess
from pathlib import Path
from oracle_mp_unit_locomotion_u5 import ROOT,OUT,INPUT_PINS,PINS,digest,encoded,frozen_write
AUTHORITY={'tools/oracle_mp_unit_locomotion_u5_audit.py':'80769c8767f637af939e589bce830a07a4bd29213f0cfbac467e64e80a3f82ba',
 'analysis/multiplayer/integration/inventory-source/unit-locomotion-u5/report.json':'4adb6786e9554be1395594a77a216680a80124bb782d24139ee1a74e4ded2d87'}
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer/integration/unit-locomotion-native/unit_locomotion_tests')
 p.add_argument('--output',type=Path,default=OUT/'native-normal.json');a=p.parse_args()
 for name,h in {**INPUT_PINS,**AUTHORITY}.items():assert digest(ROOT/name)==h,name
 for name,h in PINS.items():assert digest(name)==h,name
 d=json.loads((OUT/'report.json').read_text());audit=json.loads((OUT/'audit.json').read_text())
 assert digest(ROOT/'tools/oracle_mp_unit_locomotion_u5.py')==d['oracle_sha256']
 assert audit['report_sha256']==digest(OUT/'report.json')and audit['auditor_sha256']==AUTHORITY['tools/oracle_mp_unit_locomotion_u5_audit.py']
 for name in ['report.json','audit.json']+[q['file']for q in d['listings']]:assert (OUT/name).read_bytes()==(OUT/'reproduction'/name).read_bytes(),name
 reverse={int(r['u5'],16):int(r['old'],16)for r in d['instruction_mapping']};rows=[f'M {len(d["cases"])}']
 for c in d['cases']:
  op=int(c['result_pointer'],16)
  # Only trace-site labels are adapted to the immutable native1.1 API.
  # Inputs, full expected packets and store data/order come from original u5.
  stores=';'.join(f'{reverse[int(s["instruction"],16)]:08x},{int(s["address"],16)-op:x},{s["size"]},{int.from_bytes(bytes.fromhex(s["written_hex"]),"little"):08x}'for s in c['stores'])or'-'
  rows.append(' '.join(['C',str(c['global_id']),c['route'],c['request_before_hex'],c['result_before_hex'],c['result_after_hex'],
   c['physical_before_hex'],str(len(c['stores'])),stores]))
 fixture=OUT/'native-fixtures.tsv';frozen_write(fixture,('\n'.join(rows)+'\n').encode())
 mapping=dict(schema=1,source_report_sha256=digest(OUT/'report.json'),actual_u5_to_native1_1_store_sites=[dict(u5=f'{n:08x}',native1_1=f'{reverse[n]:08x}')for n in sorted({int(s['instruction'],16)for c in d['cases']for s in c['stores']})],
  scope='Trace instruction identities only. Expected full packets and store offset/width/data/order are original u5 outputs. This does not claim the native trace already uses u5 addresses.')
 frozen_write(OUT/'native-site-adapter.json',encoded(mapping))
 run=subprocess.run([str(a.binary.resolve()),str(fixture)],text=True,capture_output=True);print(run.stdout,end='');print(run.stderr,end='')
 r=dict(schema=1,passed=run.returncode==0,cases=len(d['cases']),ordered_stores=sum(len(c['stores'])for c in d['cases']),
  tool_sha256=digest(__file__),original_u5_report_sha256=digest(OUT/'report.json'),original_u5_audit_sha256=digest(OUT/'audit.json'),
  fixture_sha256=digest(fixture),instruction_site_adapter_sha256=digest(OUT/'native-site-adapter.json'),
  native_binary_sha256=digest(a.binary),native_sources={name:digest(ROOT/name)for name in('include/halo2/unit_locomotion.hpp','src/unit_locomotion.cpp','tests/unit_locomotion_tests.cpp')},
  complete_output70_and_store_data_order_compared=True,native_trace_is_1_1_address_space=True,actual_u5_addresses_separately_recorded=True,
  stdout=run.stdout,stderr=run.stderr,returncode=run.returncode,
  limits='Old original packets/physical-state/CPU are explicit inputs. No source-created u5 owner/controller/request/clock/pose/frame producer or CPU hardware parity is established. Other physical-state dispatches remain unavailable.')
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(r,indent=2)+'\n');raise SystemExit(run.returncode)
if __name__=='__main__':main()
