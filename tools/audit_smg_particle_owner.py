#!/usr/bin/env python3
"""Replay observed buffers/stores and freeze focused native test evidence."""
import argparse,json,subprocess
from pathlib import Path
from oracle_smg_particle_owner_1_1 import ROOT,OUT,FROZEN,sha,save,encoded

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'audit.json');a=p.parse_args();source=json.loads((OUT/'report.json').read_text())
 for path,pin in FROZEN.items():assert sha(ROOT/path)==pin,path
 repeated=['report.json']+[q['file']for q in source['instructions']]
 for path in repeated:assert (OUT/path).read_bytes()==(OUT/'independent-repeat'/path).read_bytes(),path
 def replay(begin,end,address,raw):
  buf=bytearray.fromhex(raw);count=0
  for store in source['stores']:
   if not begin<store['order']<end:continue
   start=int(store['address'],16);value=bytes.fromhex(store['written_hex']);lo=max(start,address);hi=min(start+len(value),address+len(buf))
   if lo<hi:buf[lo-address:hi-address]=value[lo-start:hi-start];count+=1
  return buf.hex(),count
 intervals=[]
 for call in source['calls']:
  b,o=call['before'],call['after']
  for label in('emitter','cache','system','location','effect','clock','camera'):
   if b[label+'_hex']is None:continue
   assert b[label+'_address']==o[label+'_address'];raw,count=replay(call['order'],call['return_order'],int(b[label+'_address'],16),b[label+'_hex']);assert raw==o[label+'_hex'],(call['id'],label)
   intervals.append(dict(call_id=call['id'],buffer=label,bytes=len(raw)//2,overlapping_stores=count,full_output_equal=True))
  # The pointer is source-owned; it is read directly by both source owners.
  rng_read=next(q for q in source['reads']if call['order']<q['order']<call['return_order']and q['raw_hex']==b['seed8_hex'][8:]and q['size']==4)if call['entry']=='00249a20'else None
  if rng_read:
   address=int(rng_read['address'],16)-4;raw,count=replay(call['order'],call['return_order'],address,b['seed8_hex']);assert raw==o['seed8_hex'];intervals.append(dict(call_id=call['id'],buffer='seed8',bytes=8,overlapping_stores=count,full_output_equal=True))
  if 'checkpoints'in call:
   alloc,prefix=call['checkpoints'];raw,count=replay(alloc['order'],prefix['order'],int(alloc['actual_particle_address'],16),alloc['actual_particle40_hex']);assert raw==prefix['particle40_hex'];intervals.append(dict(call_id=call['id'],buffer='allocated_particle40_to_successful_prefix',bytes=64,overlapping_stores=count,full_output_equal=True))
 correspondence=OUT/'correspondence';c=json.loads((correspondence/'report.json').read_text());cr=['report.json']+[q['file']for row in c['owners']for q in row['listings']]
 for path in cr:assert (correspondence/path).read_bytes()==(OUT/'correspondence-independent-repeat'/path).read_bytes(),path
 command=['ctest','--test-dir',str(ROOT/'build'),'-R','original_(live_particle_creation|live_particle_emitter|particle_birth|particle_emitter)','--output-on-failure'];r=subprocess.run(command,capture_output=True,text=True);save(OUT/'validation/focused-regression.log',(r.stdout+r.stderr).encode());assert r.returncode==0
 normal=json.loads((OUT/'native-verified/report.json').read_text());sanitized=json.loads((OUT/'native-sanitized/report.json').read_text())
 for fixture,pin in normal['files'].items():assert sha(OUT/'native-verified'/fixture)==sha(OUT/'native-sanitized'/fixture)==pin
 assert all(q['exit_code']==0 for q in normal['native_results']+sanitized['native_results'])
 report=dict(schema='source-smg-particle-owner-audit-v1',tool_sha256=sha(__file__),original_report_sha256=sha(OUT/'report.json'),prior_pins_intact=len(FROZEN),byte_identical_original_files=len(repeated),byte_identical_static_correspondence_files=len(cr),full_buffer_replays=intervals,normal_native=normal,sanitized_native=sanitized,focused_regression=dict(command=command,exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr),native_source_pins={p:sha(ROOT/p)for p in('src/rendering/effects/particle_birth.cpp','src/rendering/effects/particle_creation.cpp','tests/particle_creation_tests.cpp','tests/particle_emitter_tests.cpp')},boundaries=['Original ABI calls and whole measured buffers only; no native-generated expected data.','Actual authored SMG angle zero, two particles and four emitter updates are executed; other initializer branches remain earlier separate evidence or listed-only.','Original finite clock and external local-view/fire request/unavailable audio/resource adapter boundaries are retained. Continuous expiry and render submission are unproved.'])
 save(a.output,encoded(report));print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),full_buffer_replays=len(intervals),original_files=len(repeated),prior_pins=len(FROZEN))))
if __name__=='__main__':main()
