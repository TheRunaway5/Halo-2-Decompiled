#!/usr/bin/env python3
"""Audit source GJK owners, emulator instruction estimates and independent repeat."""
import argparse,json,collections
from pathlib import Path
from oracle_mp_power_gjk_owners_1_1 import ROOT,OUT,verify,sha,PIN
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
 for left,right in(('exported-owner','exported-repeat'),('exported-composition','exported-composition-repeat')):
  for name in('fixtures.tsv','report.json'):assert(OUT/'final'/left/name).read_bytes()==(OUT/'final'/right/name).read_bytes()
 e=json.loads((OUT/'final/exported-owner/report.json').read_text());assert not e['native_expected_data_used']and not e['contact_callback_outputs_exported']
 actual=[];estimates=[]
 for q in r['calls']:
  if q['entry']=='0030ae40':
   actual.append(dict(call_id=q['id'],workspace=q['registers']['ecx'],arguments=q['stack16'][:5],returned_eax=q['returned_eax'],packets=q['packets']))
   writes=[x for x in r['stores']if q['order']<x['order']<q['return_order']]
   first=next(x for x in writes if x['instruction']=='0030bb1d'and int(x['address'],16)%16==0)
   output=next(x for x in writes if x['instruction']=='0030bb26')
   assert first['address']==output['address']
   estimates.append(dict(call_id=q['id'],input_order=first['order'],input_hex=first['written_hex'][:8],output_order=output['order'],output_hex=output['written_hex'],provenance='Unchanged original RSQRTSS bytes executed by Unicorn emulator; not Xbox hardware evidence.'))
 composition=json.loads((OUT/'final/exported-composition/report.json').read_text())
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,original_report_sha256=sha(p),whole_original_repeat_byte_identical=True,calls=len(r['calls']),states=len(r['states']),reads=len(r['reads']),stores=len(r['stores']),complete_root_listings=len(r['instruction_listings']),original_root_listing_rows_audited=rows,original_descendant_read_store_instruction_sites_audited=len(observed),later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,native_fixture_cases=len(e['cases'])+len(composition['cases']),native_complete_calls=len(e['cases'])+len(composition['cases']),native_stores=sum(q['stores']for v in(e,composition)for q in v['cases']),native_compared_buffer_bytes=sum(q['input_bytes']for v in(e,composition)for q in v['cases']),actual_source_gjk_calls=actual,original_emulator_rsqrtss_estimates=estimates,estimate_injection_test_only=True,xbox_hardware_or_contact_decision_parity_claimed=False,remaining_live_gate='Complete300870 packet batching/contact retention and actual300650/309210/309450 contact-manager/solver production. Source GJK fallback309D40/309AE0/30A370 and alternate shape virtuals remain required; they are listed and paired, unexecuted here. Native host-estimate results differ at the admitted canister child; no general hardware/contact-decision parity claim.')
 frozen_write(a.output,encoded(result));print(json.dumps({k:v for k,v in result.items()if k not in('actual_source_gjk_calls','original_emulator_rsqrtss_estimates')}))
if __name__=='__main__':main()
