#!/usr/bin/env python3
"""Independent byte/repeat/fixture audit for real original broadphase ownership."""
import argparse,json
from pathlib import Path
from oracle_mp_power_broadphase_1_1 import ROOT,OUT,verify,sha,PIN
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/audit.json');a=ap.parse_args();count=verify()
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
 p=OUT/'final/measured/report.json';repeat=OUT/'final/repeat/report.json';assert p.read_bytes()==repeat.read_bytes();r=json.loads(p.read_text());rows=0
 for q in r['instruction_listings']:
  left,right=p.parent/q['file'],repeat.parent/q['file'];assert left.read_bytes()==right.read_bytes()and sha(left)==q['sha256']
  for line in left.read_text().splitlines():
   address,raw,*_=line.split();raw=bytes.fromhex(raw);assert old.read(int(address,16),len(raw))==raw;rows+=1
 executed=ROOT/'analysis/multiplayer/sandbox-source/power-module-schedule/frozen/measured/instructions/executed-original.asm';sites={}
 for line in executed.read_text().splitlines():
  address,raw,*_=line.split();address=int(address,16);raw=bytes.fromhex(raw);assert old.read(address,len(raw))==raw;sites[address]=raw
 observed={int(q['instruction'],16)for k in('reads','stores')for q in r[k]};assert observed<=sites.keys(),[hex(x)for x in observed-sites.keys()]
 cp=OUT/'final/later-correspondence/complete-report.json';c=json.loads(cp.read_text());oc,nc=_Code(old,'.text'),_Code(new,'.text')
 maps={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in c['routines']}
 for q in c['routines']:
  left=int(q['disc_1_1_entry'],16);right=maps[left]
  li=[oc.at(x)for x in sorted(_trace(oc,left,set(maps)).code)];ri=[nc.at(x)for x in sorted(_trace(nc,right,set(maps.values())).code)]
  assert full_tokens(li,old)==full_tokens(ri,new)
  for k,ins in(('disc_listing',li),('later_listing',ri)):
   dst=cp.parent/'instructions'/q[k]['file'];assert sha(dst)==q[k]['sha256'];assert dst.read_text()==''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins)
  for x in q['paired_image_memory_operands']:
   assert old.read(int(x['old_address'],16),x['width']).hex()==x['old_hex']
   assert new.read(int(x['later_address'],16),x['width']).hex()==x['later_hex']
 for name in('fixtures.tsv','report.json'):assert(OUT/'final/exported-owner'/name).read_bytes()==(OUT/'final/exported-repeat'/name).read_bytes()
 e=json.loads((OUT/'final/exported-owner/report.json').read_text());pairs=[]
 for call in r['calls']:
  if call['entry']!='002e8a00':continue
  stores=[x for x in r['stores']if call['order']<x['order']<call['return_order']and x['instruction']in('002e8e45','002e8e48','002e8e4f')]
  assert len(stores)==3
  assert stores[2]['written_hex']=='01000000'
  pairs.append(dict(call_id=call['id'],phase=call['phase'],handle=call['stack16'][0],pair_bank=stores[0]['address'],first_handle=stores[0]['written_hex'],second_handle=stores[1]['written_hex'],actual_source_pair_count=1))
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,
  original_report_sha256=sha(p),whole_original_repeat_byte_identical=True,calls=len(r['calls']),states=len(r['states']),reads=len(r['reads']),stores=len(r['stores']),
  complete_listings=len(r['instruction_listings']),original_listing_rows_audited=rows,original_executed_read_store_sites_audited=len(observed),prior_executed_listing_sha256=sha(executed),
  later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,
  fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,
  native_fixture_calls=len(e['cases']),original_pair_manager_boundary_calls=sum(x['stopped_at']is not None for x in e['cases']),actual_constructed_pairs=pairs,
  comparison_scope='Complete original retained allocator/backend/axis/packed-index/body/scratch/pair buffers and ordered writes. Two canister3144D0 cases stop at measured actual pair-manager entry. Semantic stack outputs compare ordered site/address/written bytes; before bytes of those temporary aliases are retained in source evidence but excluded from native store-before comparison. Other internal stack/FS plumbing is source-only. No solver/contact output supplied.')
 frozen_write(a.output,encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
