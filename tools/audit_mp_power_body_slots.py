#!/usr/bin/env python3
"""Audit unchanged original slot/world bytes, independent repeats and pins."""
import argparse,json
from pathlib import Path
from oracle_mp_power_body_slots_1_1 import ROOT,OUT,verify,sha,PIN
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/audit.json');args=ap.parse_args();count=verify()
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
 p=OUT/'final/measured/report.json';repeat=OUT/'final/repeat/report.json';assert p.read_bytes()==repeat.read_bytes();r=json.loads(p.read_text());rows=0
 for q in r['instruction_listings']:
  a,b=p.parent/q['file'],repeat.parent/q['file'];assert a.read_bytes()==b.read_bytes()and sha(a)==q['sha256']
  for line in a.read_text().splitlines():
   address,raw,*_=line.split();code=bytes.fromhex(raw);assert old.read(int(address,16),len(code))==code;rows+=1
 # Complete prior executed setup+factory+world bytes own every new observed
 # read/store EIP. Each byte is checked again against the original image.
 executed=ROOT/'analysis/multiplayer/sandbox-source/power-module-schedule/frozen/measured/instructions/executed-original.asm';sites={}
 for line in executed.read_text().splitlines():
  a,b,*_=line.split();a=int(a,16);code=bytes.fromhex(b);assert old.read(a,len(code))==code;sites[a]=code
 observed={int(q['instruction'],16)for key in('reads','stores')for q in r[key]}
 assert observed<=sites.keys(),[f'{a:08x}'for a in observed-sites.keys()]
 correspondence=OUT/'final/later-correspondence/complete-report.json';c=json.loads(correspondence.read_text());maps={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in c['routines']};oc,nc=_Code(old,'.text'),_Code(new,'.text')
 for q in c['routines']:
  a=int(q['disc_1_1_entry'],16);b=maps[a];oi=[oc.at(p)for p in sorted(_trace(oc,a,set(maps)).code)];ni=[nc.at(p)for p in sorted(_trace(nc,b,set(maps.values())).code)]
  assert full_tokens(oi,old)==full_tokens(ni,new)
  for key,ins in(('disc_listing',oi),('later_listing',ni)):
   p=correspondence.parent/'instructions'/q[key]['file'];assert sha(p)==q[key]['sha256'];assert p.read_text()==''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins)
 for name in('fixtures.tsv','report.json'):
  assert (OUT/'final/exported-owner'/name).read_bytes()==(OUT/'final/exported-repeat'/name).read_bytes()
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=PIN,prior_files_intact=count,
  original_report_sha256=sha(OUT/'final/measured/report.json'),whole_original_repeat_byte_identical=True,
  calls=len(r['calls']),states=len(r['states']),reads=len(r['reads']),stores=len(r['stores']),
  complete_listings=len(r['instruction_listings']),original_listing_rows_audited=rows,
  original_executed_read_store_sites_audited=len(observed),prior_executed_listing_sha256=sha(executed),
  later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,later_gameplay_execution_claimed=False,
  fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False)
 frozen_write(args.output,encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
