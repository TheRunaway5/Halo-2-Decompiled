#!/usr/bin/env python3
"""Audit unchanged grouping instructions, repeats, exports and prior freezes."""
import argparse,json
from pathlib import Path
from oracle_mp_power_body_grouping_1_1 import ROOT,OUT,verify,sha,PIN
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
 observed={int(q['instruction'],16)for k in('reads','stores','math')for q in r[k]};assert observed<=sites.keys(),[hex(x)for x in observed-sites.keys()]
 cp=OUT/'final/later-correspondence/complete-report.json';c=json.loads(cp.read_text());oc,nc=_Code(old,'.text'),_Code(new,'.text')
 maps={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in c['routines']}
 for q in c['routines']:
  left=int(q['disc_1_1_entry'],16);right=maps[left]
  li=[oc.at(x)for x in sorted(_trace(oc,left,set(maps)).code)];ri=[nc.at(x)for x in sorted(_trace(nc,right,set(maps.values())).code)]
  assert full_tokens(li,old)==full_tokens(ri,new)
  for k,ins in(('disc_listing',li),('later_listing',ri)):
   dst=cp.parent/'instructions'/q[k]['file'];assert sha(dst)==q[k]['sha256'];assert dst.read_text()==''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins)
 for name in('fixtures.tsv','report.json'):assert(OUT/'final/exported-owner'/name).read_bytes()==(OUT/'final/exported-repeat'/name).read_bytes()
 e=json.loads((OUT/'final/exported-owner/report.json').read_text())
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,
  original_report_sha256=sha(p),whole_original_repeat_byte_identical=True,calls=len(r['calls']),states=len(r['states']),
  reads=len(r['reads']),stores=len(r['stores']),math_snapshots=len(r['math']),complete_listings=len(r['instruction_listings']),
  original_listing_rows_audited=rows,original_executed_read_store_math_sites_audited=len(observed),prior_executed_listing_sha256=sha(executed),
  later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,
  fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,
  temporary_abi_alias_deltas=sum(len(x['temporary_abi_alias_deltas'])for x in e['cases']),
  complete_original_factory_call_packets=sum(len(x['complete_factory_call_words'])for x in e['cases']),
  comparison_scope='Retained complete buffers and ordered instruction/address/written-byte stores; retained store-before exact. Temporary ABI stack store-before retained in original report, excluded from native equality after source-lifetime stack reuse.')
 frozen_write(a.output,encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
