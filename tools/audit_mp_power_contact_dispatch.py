#!/usr/bin/env python3
"""Audit300870 source prefixes and raw packet-input byte ownership; no completion."""
import argparse,json
from pathlib import Path
from export_mp_power_contact_dispatch import ROOT,OUT,MANIFEST,PIN,verify,sha
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/audit.json');a=ap.parse_args()
 prior,batch=verify();p=MANIFEST.parent/'measured/report.json';r=json.loads(p.read_text());repeat=MANIFEST.parent/'repeat/report.json'
 assert p.read_bytes()==repeat.read_bytes()
 im=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 source_asm=MANIFEST.parent/'measured/00300870.asm';repeat_asm=MANIFEST.parent/'repeat/00300870.asm';assert source_asm.read_bytes()==repeat_asm.read_bytes()
 rows=0
 for line in source_asm.read_text().splitlines():
  site,raw,*_=line.split();assert im.read(int(site,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw);rows+=1
 correspondence=json.loads((MANIFEST.parent/'later-correspondence/complete-report.json').read_text())
 pair=next(q for q in correspondence['routines']if q['disc_1_1_entry']=='00300870');assert pair['later_entry']=='00304350'and pair['normalized_instruction_and_internal_branch_topology_equal']
 exports=OUT/'final/exported-owner';again=OUT/'final/exported-repeat'
 for name in('fixtures.tsv','report.json'):assert(exports/name).read_bytes()==(again/name).read_bytes()
 e=json.loads((exports/'report.json').read_text());assert not e['native_expected_data_used']
 canister=next(c for c in r['calls']if c['entry']=='00300870'and c['registers']['ecx']=='0601ade0')
 # This is a byte ancestry observation of the unchanged original execution.
 # Recorded values are diagnostic evidence, never a callback packet provider.
 packets=[]
 for copy in r['calls']:
  if copy['entry']!='00300100' or not(canister['order']<copy['order']<canister['return_order']):continue
  packet=next(q for q in copy['packets']if q['name']=='input_packet');base=int(packet['pointer'],16);raw=bytes.fromhex(packet['before_hex']);assert len(raw)==128
  last={}
  for w in r['stores']:
   if not(canister['order']<w['order']<copy['order']):continue
   at=int(w['address'],16);data=bytes.fromhex(w['written_hex'])
   for k,b in enumerate(data):
    if base<=at+k<base+128:last[at+k]=(w,b)
  ownership=[]
  for k,value in enumerate(raw):
   if base+k in last:
    w,b=last[base+k];assert b==value
    owner=dict(instruction=w['instruction'],order=w['order'],store_address=w['address'],store_size=w['size'])
   else:owner=dict(retained_before_dispatch=True)
   if ownership and ownership[-1]['owner']==owner:ownership[-1]['bytes_hex']+=f'{value:02x}';ownership[-1]['size']+=1
   else:ownership.append(dict(offset=f'{k:02x}',size=1,bytes_hex=f'{value:02x}',owner=owner))
  packets.append(dict(original_copy_call=copy['id'],source_input=packet['pointer'],bytes=128,before_hex=packet['before_hex'],byte_ancestry=ownership))
 failure=OUT/'draft/first-native-test.log';assert 'store 805' in failure.read_text()and '21fcd20' in failure.read_text().lower()
 result=dict(schema=1,audit_tool_sha256=sha(__file__),batch_manifest_sha256=PIN,prior_gjk_pins_intact=prior,batch_pins_intact=batch,
  whole_original_repeat_byte_identical=True,original_report_sha256=sha(p),original300870_complete_rows=rows,
  authenticated_u5_entry=pair['later_entry'],authenticated_u5_complete_body_equal=True,u5_gameplay_execution_claimed=False,
  source_only_exports_repeat_identically=True,source_fixture_sha256=sha(exports/'fixtures.tsv'),
  native_cases=len(e['cases']),native_complete_calls=sum(q['complete_original_call']for q in e['cases']),
  native_required_packet_frame_prefixes=sum(not q['complete_original_call']for q in e['cases']),
  owning_writes=sum(q['stores']for q in e['cases']),compared_bytes=sum(q['input_bytes']for q in e['cases']),
  raw_packet_inputs=packets,retained_failed_full_composition=sha(failure),
  captured_callback_completion_used=False,source_contacts_or_clock_or_admission_supplied=False,
  boundaries=['The300A07→300400 packet input requires actual preceding guest ABI/stale-frame ownership. Missing provider stops before append/count mutation.',
   'Only canister prefix through first separated child and complete player cold-empty query are native-verified. Positive dispatcher tail is translated but unverified.',
   '313800 real constraint/world insertion;322410 point/scalar/index bank;31E8B0/31C2E0 listener and solver producers remain required.',
   'Unicorn RSQRTSS observations are test-only emulator evidence. Actual host path is separate; Xbox/PentiumIII contact-decision parity is unverified.'])
 frozen_write(a.output,encoded(result));print(json.dumps({k:v for k,v in result.items()if k not in('raw_packet_inputs','boundaries')}))
if __name__=='__main__':main()
