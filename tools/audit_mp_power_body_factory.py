#!/usr/bin/env python3
"""Audit frozen original factory/shape repetitions and instruction provenance."""
import argparse,json
from pathlib import Path
from oracle_mp_power_body_factory_1_1 import ROOT,OUT,verify,sha,MANIFEST_PIN
from oracle_mp_item_current_producers_1_1 import encoded,frozen_write
from oracle_smg_firing_1_1 import Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens
from oracle_smg_firing_1_1 import _Code,_trace

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'final/audit.json');args=ap.parse_args();count=verify();old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
 assert sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')==PIN and sha(NEW_DEFAULT)==NEW_PIN
 stats={};instructions=0
 for name,left,right in(('factory','measured','repeat'),('shape','shape-measured','shape-repeat')):
  lp,rp=OUT/'final'/left/'report.json',OUT/'final'/right/'report.json';assert lp.read_bytes()==rp.read_bytes()
  r=json.loads(lp.read_text());lists=r.get('instruction_listings',r.get('listings'))
  for q in lists:
   a,b=lp.parent/q['file'],rp.parent/q['file'];assert a.read_bytes()==b.read_bytes() and sha(a)==q['sha256']
   for line in a.read_text().splitlines():
    p,raw,*_=line.split();code=bytes.fromhex(raw);assert old.read(int(p,16),len(code))==code;instructions+=1
  stats[name]=dict(report_sha256=sha(lp),calls=len(r.get('calls',[r.get('call')])),reads=len(r['reads']),stores=len(r['stores']),listings=len(lists),repeated_identically=True)
 correspondence=OUT/'final/later-correspondence/complete-report.json';r=json.loads(correspondence.read_text());maps={int(q['disc_1_1_entry'],16):int(q['later_entry'],16)for q in r['routines']};oc,nc=_Code(old,'.text'),_Code(new,'.text')
 for q in r['routines']:
  a=int(q['disc_1_1_entry'],16);b=maps[a]
  oi=[oc.at(p)for p in sorted(_trace(oc,a,set(maps)).code)];ni=[nc.at(p)for p in sorted(_trace(nc,b,set(maps.values())).code)]
  assert full_tokens(oi,old)==full_tokens(ni,new)
  for key,ins,image in(('disc_listing',oi,old),('later_listing',ni,new)):
   p=correspondence.parent/'instructions'/q[key]['file'];assert sha(p)==q[key]['sha256']
   assert p.read_text()==''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins)
 for name in('fixtures.tsv','report.json'):
  assert (OUT/'final/exported-owner'/name).read_bytes()==(OUT/'final/exported-repeat'/name).read_bytes(),name
 result=dict(schema=1,audit_tool_sha256=sha(__file__),prior_manifest_sha256=MANIFEST_PIN,prior_files_intact=count,source=stats,original_instruction_rows_audited=instructions,later_complete_correspondence_routines=len(maps),later_owned_xbe_sha256=NEW_PIN,fixtures_repeated_identically=True,new_guest_fields_or_success_results_supplied=False,later_gameplay_execution_claimed=False)
 frozen_write(args.output,encoded(result));print(json.dumps(result))
if __name__=='__main__':main()
