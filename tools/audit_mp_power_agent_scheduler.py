#!/usr/bin/env python3
"""Audit unchanged original call tree, independent repeat and owned native export."""
import argparse,json
from pathlib import Path
from oracle_mp_power_agent_scheduler_1_1 import ROOT,OUT,verify,sha,PIN
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/audit.json');a=ap.parse_args();count=verify()
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
 p=OUT/'final/measured/report.json';repeat=OUT/'final/repeat/report.json';assert p.read_bytes()==repeat.read_bytes();r=json.loads(p.read_text());sites={};rows=0
 for q in r['instruction_listings']:
  left,right=p.parent/q['file'],repeat.parent/q['file'];assert left.read_bytes()==right.read_bytes()and sha(left)==q['sha256']
  for line in left.read_text().splitlines():
   address,raw,*_=line.split();address=int(address,16);raw=bytes.fromhex(raw);assert old.read(address,len(raw))==raw;sites[address]=raw;rows+=1
 observed={int(q['instruction'],16)for k in('reads','stores')for q in r[k]}
 prior=ROOT/'analysis/multiplayer/sandbox-source/power-module-schedule/frozen/measured/instructions/executed-original.asm'
 assert sha(prior)=='7e16d53697d3a21e3e9adceb0da4045cef4a2d0df0a1a064453da0955f1c83a3'
 tail=observed-sites.keys();extra=[]
 for line in prior.read_text().splitlines():
  address,raw,*_=line.split();address=int(address,16);raw=bytes.fromhex(raw)
  if address in tail:
   assert old.read(address,len(raw))==raw;sites[address]=raw;extra.append(line)
 assert observed<=sites.keys(),[hex(v)for v in observed-sites.keys()]
 frozen_write(a.output.parent/'audited-external-tail-instructions.asm',('\n'.join(extra)+'\n').encode())
 cp=OUT/'final/later-complete/complete-report.json';c=json.loads(cp.read_text());oc,nc=_Code(old,'.text'),_Code(new,'.text')
 maps={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in c['routines']}
 for q in c['routines']:
  left=int(q['disc_1_1_entry'],16);right=maps[left]
  li=[oc.at(x)for x in sorted(_trace(oc,left,set(maps)).code)];ri=[nc.at(x)for x in sorted(_trace(nc,right,set(maps.values())).code)]
  assert full_tokens(li,old)==full_tokens(ri,new)
  for k,ins in(('disc_listing',li),('later_listing',ri)):
   dst=cp.parent/'instructions'/q[k]['file'];assert sha(dst)==q[k]['sha256'];assert dst.read_text()==''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins)
  for x in q['paired_image_memory_operands']:
   assert old.read(int(x['old_address'],16),x['width']).hex()==x['old_hex'];assert new.read(int(x['later_address'],16),x['width']).hex()==x['later_hex']
 for name in('fixtures.tsv','report.json'):assert(OUT/'final/exported-owner'/name).read_bytes()==(OUT/'final/exported-repeat'/name).read_bytes()
 e=json.loads((OUT/'final/exported-owner/report.json').read_text());emitted=[]
 for call in r['calls']:
  if call['entry']!='0031f550':continue
  packet=next(q for q in call['packets']if q['name']=='emitted_descriptor_14');raw=bytes.fromhex(packet['before_hex'])
  emitted.append(dict(call_id=call['id'],manager=call['registers']['ecx'],arguments=call['stack16'][:4],output_descriptor=packet['pointer'],actual_emitted_count=int.from_bytes(raw[4:8],'little'),original_input_capacity=int.from_bytes(raw[8:12],'little')&0x7fffffff,complete_native_delivery_verified=True))
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,original_report_sha256=sha(p),whole_original_repeat_byte_identical=True,calls=len(r['calls']),states=len(r['states']),reads=len(r['reads']),stores=len(r['stores']),complete_listings=len(r['instruction_listings']),original_listing_rows_audited=rows,original_read_store_instruction_sites_audited=len(observed),external_tail_sites_audited=len(tail),prior_executed_listing_sha256=sha(prior),later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,native_fixture_calls=len(e['cases']),native_complete_calls=sum(q['complete_original_call']for q in e['cases']),native_required_stops=sum(not q['complete_original_call']for q in e['cases']),native_stores=sum(q['stores']for q in e['cases']),actual_emitted_manager_inputs=emitted,remaining_live_gate='31DB70 current broadphase update through actual2E68D0, actual300870 retained agent through its real shape/terrain/contact-manager/solver dependencies. Existing emitted rows and contact banks are inputs to the verified native delivery, never live replayed outputs.')
 frozen_write(a.output,encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
