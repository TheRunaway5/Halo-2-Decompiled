#!/usr/bin/env python3
"""Audit source narrowphase owners, original-only continuation inputs and independent repeat."""
import argparse,json,collections
from pathlib import Path
from oracle_mp_power_narrowphase_owners_1_1 import ROOT,OUT,verify,sha,PIN
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/audit.json');a=ap.parse_args();count=verify()
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 p=OUT/'final/measured/report.json';repeat=OUT/'final/repeat/report.json';assert p.read_bytes()==repeat.read_bytes();r=json.loads(p.read_text());rows=0
 for q in r['instruction_listings']:
  left,right=p.parent/q['file'],repeat.parent/q['file'];assert left.read_bytes()==right.read_bytes()and sha(left)==q['sha256']
  for line in left.read_text().splitlines():
   at,raw,*_=line.split();assert old.read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw);rows+=1
 observed={int(q['instruction'],16)for k in('reads','stores')for q in r[k]};listing=[]
 for site in sorted(observed):
  i=oc.at(site);assert old.read(site,len(i.bytes))==i.bytes;listing.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
 frozen_write(a.output.parent/'observed-read-store-instructions.asm',''.join(listing).encode())
 cp=OUT/'final/later-correspondence/complete-report.json';c=json.loads(cp.read_text());maps={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in c['routines']}
 for q in c['routines']:
  left=int(q['disc_1_1_entry'],16);right=maps[left]
  li=[oc.at(x)for x in sorted(_trace(oc,left,set(maps)).code)];ri=[nc.at(x)for x in sorted(_trace(nc,right,set(maps.values())).code)];assert full_tokens(li,old)==full_tokens(ri,new)
  for k,ins in(('disc_listing',li),('later_listing',ri)):
   dst=cp.parent/'instructions'/q[k]['file'];assert sha(dst)==q[k]['sha256'];assert dst.read_text()==''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins)
  for x in q['paired_image_memory_operands']:
   assert old.read(int(x['old_address'],16),x['width']).hex()==x['old_hex'];assert new.read(int(x['later_address'],16),x['width']).hex()==x['later_hex']
 for name in('fixtures.tsv','report.json'):assert(OUT/'final/exported-owner'/name).read_bytes()==(OUT/'final/exported-repeat'/name).read_bytes()
 e=json.loads((OUT/'final/exported-owner/report.json').read_text());assert not e['native_expected_data_used']and not e['contact_callback_outputs_exported']
 separation=[]
 for q in r['calls']:
  if q['entry']=='002f4bd0':
   separation.append(dict(call_id=q['id'],agent=q['registers']['ecx'],arguments=q['stack16'][:5],returned_eax=q['returned_eax'],packets=q['packets']))
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,original_report_sha256=sha(p),whole_original_repeat_byte_identical=True,calls=len(r['calls']),states=len(r['states']),reads=len(r['reads']),stores=len(r['stores']),complete_root_listings=len(r['instruction_listings']),original_root_listing_rows_audited=rows,original_descendant_read_store_instruction_sites_audited=len(observed),later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,native_fixture_cases=len(e['cases']),native_complete_calls=sum(q['complete_original_call']for q in e['cases']),native_complete_continuations=sum(q['complete_original_continuation']for q in e['cases']),native_required_stops=sum(q['required_original_owner']is not None for q in e['cases']),native_stores=sum(q['stores']for q in e['cases']),native_compared_buffer_bytes=sum(q['input_bytes']for q in e['cases']),actual_source_separation_calls=separation,actual_executed_contact_packet_store_counts=dict(collections.Counter(q['instruction']for q in r['stores']if q['instruction']in('002f4e22','002f4e5d','002f4e44','002f4e55'))),remaining_live_gate='Complete30AE40 GJK with genuine support/retrieve/simplex owners, complete300870 contact batching and actual contact-manager/solver delivery. Native original-output continuations consume declared completed source state; none is a contact replay callback or proof of live collisions. Profiling RDTSC capacity, positive motion capability/refresh and cached positive separation are not executed in this suite.')
 frozen_write(a.output,encoded(result));print(json.dumps({k:v for k,v in result.items()if k!='actual_source_separation_calls'}))
if __name__=='__main__':main()
