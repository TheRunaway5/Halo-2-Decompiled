#!/usr/bin/env python3
"""Export literal original classifier buffers/stores; never native expectations."""
import argparse, hashlib, json
from pathlib import Path
from oracle_mp_unit_locomotion_1_1 import ROOT,OUT,FROZEN,digest,encoded,frozen_write

AUTHORITY={
 'tools/oracle_mp_unit_locomotion_1_1.py':'ec102f6366669797c040c804978d1d35fa343afb811abfeee906a6e5cfb7a614',
 'tools/oracle_mp_unit_locomotion_audit_1_1.py':'6c1d3ef774314d06df770863c93a2ea9946facda910c20dee3d15d8e92b08580',
 'analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/report.json':'70b1fc431af6636358fadc942677f2351ed8382f893f58f15729ecfabfe8e6f0',
 'analysis/multiplayer/integration/inventory-source/unit-locomotion-measured/audit.json':'31f268fdabaf95e7262030b201544a7ab9bcb42b9e3e8c446ee5c45bdb381b0c'}
EXPORT=ROOT/'analysis/multiplayer/integration/unit-locomotion-fixtures'

def verify_authority():
 for path,h in {**FROZEN,**AUTHORITY}.items():assert digest(ROOT/path)==h,path
 assert (OUT/'report.json').read_bytes()==(OUT/'reproduction/report.json').read_bytes()
 assert (OUT/'audit.json').read_bytes()==(OUT/'reproduction/audit.json').read_bytes()
 d=json.loads((OUT/'report.json').read_text())
 for q in d['instructions']:
  assert digest(OUT/q['file'])==q['sha256']
  assert (OUT/q['file']).read_bytes()==(OUT/'reproduction'/q['file']).read_bytes()
 return d

def export(out):
 d=verify_authority();rows=[f'M {len(d["cases"])}']
 for c in d['cases']:
  pointer=int(c['result_pointer'],16)
  stores=';'.join(f'{s["instruction"]},{int(s["address"],16)-pointer:x},{s["size"]},{int.from_bytes(bytes.fromhex(s["written_hex"]),"little"):08x}'for s in c['stores'])or'-'
  rows.append(' '.join(['C',str(c['id']),c['route'],c['request_before_hex'],c['result_before_hex'],c['result_after_hex'],
   c['physical_before_hex'],str(len(c['stores'])),stores]))
 frozen_write(out/'fixtures.tsv',('\n'.join(rows)+'\n').encode())
 frozen_write(out/'source-owner.json',encoded({k:d[k]for k in('source_owner','source_packet_call','jump_table','prior_owner_correlation','supplied_inputs','boundaries')}))
 frozen_write(out/'schema.json',encoded(dict(schema=1,format='Whitespace-delimited literal hexadecimal buffers. No floating-point text conversion.',
  metadata_row=['M','case_count'],case_row=['C','global_case_id','route','request_before_154_hex','classification_before_70_hex','classification_after_70_hex',
   'physical_before_10_hex','store_count','stores_or_dash'],
  store_record='semicolon-separated original_instruction_hex,output_offset_hex,width_decimal,little_endian_value_hex',
  routes=['leaf (original1F52F0)','wrapper (original1E94E0 state1 flag20 clear)','wrapper_flag20 (original1E94E0 flag20 set)'],
  original_cases=len(d['cases']),vectors=196,nonneutral_request_vectors_explicit=True,live_input_producer=False,
  expected_values='Only original measured packet bytes and stores. The source report retains instruction CPU operands/flags/x87 and complete unchanged source-owner state.')))
 payloads={p.name:dict(sha256=digest(p),bytes=p.stat().st_size)for p in(out/'fixtures.tsv',out/'source-owner.json',out/'schema.json')}
 manifest=dict(schema=1,exporter_sha256=digest(__file__),source_pins={**FROZEN,**AUTHORITY},payloads=payloads,
  original_report=str((OUT/'report.json').relative_to(ROOT)),original_report_sha256=digest(OUT/'report.json'),
  independent_original_repeat_byte_identical=True,native_implementation_expected_values=False)
 frozen_write(out/'manifest.json',encoded(manifest));return manifest

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-dir',type=Path,default=EXPORT);a=p.parse_args()
 m=export(a.output_dir);print(json.dumps(dict(output=str(a.output_dir),manifest_sha256=digest(a.output_dir/'manifest.json'),payloads=m['payloads'])))
if __name__=='__main__':main()
