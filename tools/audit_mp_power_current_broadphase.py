#!/usr/bin/env python3
"""Audit unchanged current-broadphase source buffers, independent repeat and complete native export."""
import argparse,json
from pathlib import Path
from oracle_mp_power_current_broadphase_1_1 import ROOT,OUT,verify,sha,PIN
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
 cp=OUT/'final/later-correspondence/complete-report.json';c=json.loads(cp.read_text());oc,nc=_Code(old,'.text'),_Code(new,'.text')
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
 e=json.loads((OUT/'final/exported-owner/report.json').read_text());backend=[];agents=[]
 for name in('fixtures.tsv','report.json'):assert(OUT/'final/exported-schedule'/name).read_bytes()==(OUT/'final/exported-schedule-repeat'/name).read_bytes()
 composed=json.loads((OUT/'final/exported-schedule/report.json').read_text())
 assert len(composed['cases'])==2 and all(not q['complete_original_call']and q['required_original_owner']['entry']=='00300870'for q in composed['cases'])
 for call in r['calls']:
  if call['entry']=='002e68d0':
   output=[]
   for name in('added_descriptor_14','removed_descriptor_14'):
    packet=next(q for q in call['packets']if q['name']==name);before=bytes.fromhex(packet['before_hex']);after=bytes.fromhex(packet['after_hex'])
    output.append(dict(name=name,pointer=packet['pointer'],before_count=int.from_bytes(before[4:8],'little'),after_count=int.from_bytes(after[4:8],'little')))
   stores=[q for q in r['stores']if call['order']<q['order']<call['return_order']and not q['source_stack']]
   backend.append(dict(call_id=call['id'],receiver=call['registers']['ecx'],arguments=call['stack16'][:5],real_pair_delta=output,endpoint_stores=stores))
  if call['entry']=='00300870':
   tree=r['original_scheduler_call_tree'];ancestor=next(q for q in tree if q['entry']==call['entry']and q['entry_esp']==call['entry_esp']and q['registers']['ecx']==call['registers']['ecx'])
   child=next(q for q in tree if ancestor['order']<q['order']<ancestor['return_order'])
   agents.append(dict(call_id=call['id'],receiver=call['registers']['ecx'],arguments=call['stack16'][:4],complete_original_call=True,first_native_missing_owner=dict(entry=child['entry'],instruction=child['call_site'],receiver=child['registers']['ecx'],entry_esp=child['entry_esp'],arguments=child['stack16'][:4])))
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,original_report_sha256=sha(p),whole_original_repeat_byte_identical=True,calls=len(r['calls']),states=len(r['states']),reads=len(r['reads']),stores=len(r['stores']),complete_listings=len(r['instruction_listings']),original_listing_rows_audited=rows,original_read_store_instruction_sites_audited=len(observed),external_tail_sites_audited=len(tail),prior_executed_listing_sha256=sha(prior),later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,native_fixture_calls=len(e['cases'])+len(composed['cases']),native_complete_calls=sum(q['complete_original_call']for q in e['cases']),native_required_stops=len(composed['cases']),native_stores=sum(q['stores']for q in e['cases'])+sum(q['stores']for q in composed['cases']),composed_schedule_fixture_sha256=composed['fixture_sha256'],composed_schedule_source_only=True,actual_current_backend=backend,actual_retained_agents=agents,remaining_live_gate='Actual300870 retained agent through2EFC30 current relative transform/bounds and terrain child-shape enumeration, followed by real per-child contact-manager/solver production. No source-created contact is replayed in native runtime.')
 frozen_write(a.output,encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
