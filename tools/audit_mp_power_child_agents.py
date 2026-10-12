#!/usr/bin/env python3
"""Audit retained child-cache/agent source, repeats and literal fixture ownership."""
import argparse,json,collections
from pathlib import Path
from oracle_mp_power_child_agents_1_1 import ROOT,OUT,verify,sha,PIN
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens
from export_mp_power_body_slots import full_snapshot

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/audit.json');a=ap.parse_args();count=verify()
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT);oc,nc=_Code(old,'.text'),_Code(new,'.text')
 p=OUT/'final/measured/report.json';repeat=OUT/'final/repeat/report.json';assert p.read_bytes()==repeat.read_bytes();r=json.loads(p.read_text());rows=0
 for q in r['instruction_listings']:
  left,right=p.parent/q['file'],repeat.parent/q['file'];assert left.read_bytes()==right.read_bytes()and sha(left)==q['sha256']
  for line in left.read_text().splitlines():
   at,raw,*_=line.split();assert old.read(int(at,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw);rows+=1
 # Descendant instruction authority includes unchanged already-frozen geometry,
 # allocator and real narrowphase/solver consumers, not just twelve new roots.
 observed={int(q['instruction'],16)for k in('reads','stores')for q in r[k]}
 listing=[]
 for site in sorted(observed):
  i=oc.at(site);assert old.read(site,len(i.bytes))==i.bytes
  listing.append(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
 frozen_write(a.output.parent/'observed-read-store-instructions.asm',''.join(listing).encode())
 cp=OUT/'final/later-correspondence/complete-report.json';c=json.loads(cp.read_text());maps={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in c['routines']}
 for q in c['routines']:
  left=int(q['disc_1_1_entry'],16);right=maps[left]
  li=[oc.at(x)for x in sorted(_trace(oc,left,set(maps)).code)];ri=[nc.at(x)for x in sorted(_trace(nc,right,set(maps.values())).code)]
  assert full_tokens(li,old)==full_tokens(ri,new)
  for k,ins in(('disc_listing',li),('later_listing',ri)):
   dst=cp.parent/'instructions'/q[k]['file'];assert sha(dst)==q[k]['sha256'];assert dst.read_text()==''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins)
  for x in q['paired_image_memory_operands']:
   assert old.read(int(x['old_address'],16),x['width']).hex()==x['old_hex'];assert new.read(int(x['later_address'],16),x['width']).hex()==x['later_hex']
 for name in('fixtures.tsv','report.json'):assert(OUT/'final/exported-owner'/name).read_bytes()==(OUT/'final/exported-repeat'/name).read_bytes()
 e=json.loads((OUT/'final/exported-owner/report.json').read_text());caches=[];constructors=[]
 def read_state(index,at,n):
  for base,b in full_snapshot(r['states'][index]):
   if base<=at and at+n<=base+len(b):return b[at-base:at-base+n]
  raise AssertionError((index,hex(at),n))
 for call in r['calls']:
  if call['entry']=='002efc30':
   packet=next(q for q in call['packets']if q['name']=='retained_agent_40');raw=bytes.fromhex(packet['after_hex']);ptr=int.from_bytes(raw[12:16],'little');n=int.from_bytes(raw[16:20],'little')
   bank=read_state(call['after_state'],ptr,n*12)if n else b''
   children=[dict(key=f'{int.from_bytes(bank[k:k+4],"little"):08x}',retained_middle_word=f'{int.from_bytes(bank[k+4:k+8],"little"):08x}',agent=f'{int.from_bytes(bank[k+8:k+12],"little"):08x}')for k in range(0,len(bank),12)]
   caches.append(dict(call_id=call['id'],phase=call['phase'],parent_agent=call['registers']['ecx'],arguments=call['stack16'][:4],packet=packet,rows=children))
  if call['entry']=='002f48a0':constructors.append(dict(call_id=call['id'],arguments=call['stack16'][:3],packets=call['packets'],returned_identity=call['returned_eax']))
 assert len(caches)==2 and caches[0]['rows'][0]['key']=='20000813'and caches[0]['rows'][1]['key']=='2000081d'and caches[1]['rows']==[]
 chain=[q for q in r['original_scheduler_call_tree']if q['entry']in('002e1430','002e14e0','002f4bd0','0030ae40')]
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,original_report_sha256=sha(p),whole_original_repeat_byte_identical=True,calls=len(r['calls']),states=len(r['states']),reads=len(r['reads']),stores=len(r['stores']),complete_root_listings=len(r['instruction_listings']),original_root_listing_rows_audited=rows,original_descendant_read_store_instruction_sites_audited=len(observed),later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,native_fixture_calls=len(e['cases']),native_complete_calls=sum(q['complete_original_call']for q in e['cases']),native_stores=sum(q['stores']for q in e['cases']),native_compared_buffer_bytes=sum(q['input_bytes']for q in e['cases']),actual_caches=caches,actual_constructors=constructors,actual_next_narrowphase_chain=chain,actual_capsule_endpoint_store_visits=dict(collections.Counter(q['instruction']for q in r['stores']if q['instruction']in('002e42f9','002e430f'))),remaining_live_gate='Complete300870 per-child processing, type table2E1430, shape-order wrapper2E14E0, real2F4BD0/30AE40 narrowphase and actual contact-manager/solver delivery. Native child caches are produced here, no source-created contact is replayed. Alternate source incremental cache mode4878B8!=0 is explicitly unavailable; retained warm merge/release branches are source-listed, not positively executed by this cold suite.')
 frozen_write(a.output,encoded(result));print(json.dumps({k:v for k,v in result.items()if k not in('actual_caches','actual_constructors','actual_next_narrowphase_chain')}))
if __name__=='__main__':main()
