#!/usr/bin/env python3
"""Independently repeat-check literal lifecycle buffers and source-store replay."""
import argparse,json
from pathlib import Path
from oracle_smg_effect_lifecycle_1_1 import ROOT,OUT,FROZEN
from oracle_smg_effect_owner_1_1 import sha,save,encoded

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'audit.json');a=p.parse_args();source=OUT/'measured';r=json.loads((source/'report.json').read_text())
 for file,pin in FROZEN.items():assert sha(ROOT/file)==pin,file
 repeated=['report.json']+[q['file']for q in r['instructions']]
 for file in repeated:assert (source/file).read_bytes()==(OUT/'independent-repeat'/file).read_bytes(),file
 def buffers(state):
  c=state['current'];out=[]
  def add(name,p,raw):
   if raw is not None:out.append((name,int(p,16),raw))
  for pool in c['pools']:
   label=pool['global_address'];add(label+'.header',pool['pointer'],pool.get('header4c_hex'));add(label+'.bitmap',pool.get('bitmap_pointer','0'),pool.get('bitmap_hex'))
  for pool in state['retained_ever_allocated_slots']:
   for slot in pool['slots']:add(pool['global_address']+'.slot'+str(slot['index']),slot['pointer'],slot['raw_hex'])
  for label,field in(('clock','raw24_hex'),('player','raw21c_hex'),('shared_seed','raw8_hex')):add(label,c[label]['pointer'],c[label][field])
  for owner in c['local_owner_storage']:add(owner['name'],owner['pointer'],owner['raw_hex'])
  for q in state['source_frame_globals']:add('global'+q['address'],q['address'],q['raw_hex'])
  return out
 replays=[]
 for stage in r['stages']:
  b=r['states'][stage['before_state']];o=r['states'][stage['after_state']];before=buffers(b);after=buffers(o);assert before==after
  calls=[q for q in r['calls']if q['phase']==stage['name']];lo=min(q['order']for q in calls);hi=max((q.get('return_order',lo)for q in calls))if stage['completion']=='complete_original_return'else max(q['order']for q in r['stores'])+1
  for name,address,raw in before:
   buf=bytearray.fromhex(raw);count=0
   for q in r['stores']:
    if not lo<q['order']<hi:continue
    p=int(q['address'],16);value=bytes.fromhex(q['written_hex']);left=max(p,address);right=min(p+len(value),address+len(buf))
    if left<right:buf[left-address:right-address]=value[left-p:right-p];count+=1
   assert buf.hex()==raw,(stage['name'],name)
   replays.append(dict(stage=stage['name'],buffer=name,bytes=len(buf),overlapping_stores=count,full_bytes_equal=True))
 assert r['stages'][0]['completion']=='complete_original_return'and not r['stages'][0]['services'];stop=r['stages'][1]['invalid'];assert stop['kernel_ordinal']==128 and r['stages'][1]['stopped_eip']=='80000080'
 assert len(r['outside_source_text'])==1 and r['outside_source_text'][0]['address']=='80000080'
 visited={q['entry']for q in r['calls']};assert not visited&{'00139640','00147dd0','001789d0','00176410','001764b0','0024a820','00249210'}
 result=dict(schema='source-smg-effect-lifecycle-audit-v1',tool_sha256=sha(__file__),original_report_sha256=sha(source/'report.json'),byte_identical_files=len(repeated),prior_pins_intact=len(FROZEN),full_buffer_replays=replays,fixed_effect_phase_supplied_services=0,outer_frame_first_unavailable_ordinal=128,unexecuted_particle_elapsed_and_aging_owners=True,source_state_inputs_added=0)
 save(a.output,encoded(result));print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),repeated_files=len(repeated),buffer_replays=len(replays),prior_pins=len(FROZEN))))
if __name__=='__main__':main()
