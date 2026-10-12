#!/usr/bin/env python3
"""Export unchanged original cold biped physical construction comparisons."""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import sha,spans
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
SELECT={'000dfaf0','000e6f60','000e6fb0','001e8e60','001f3da0','000bacc0'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();j=json.loads(a.report.read_text())
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
 assert j['oracle_sha256']==sha(Path(__file__).with_name('oracle_mp_biped_physical_construction_1_1.py'))
 assert j['prior_runtime_correlation'] and not j['instruction_patches']and not j['new_supplied_gameplay_or_physics_results']
 for listing in j['instructions']:assert sha(a.report.parent/listing['file'])==listing['sha256']
 events=sorted([(q['order'],'R',q)for q in j['reads']]+[(q['order'],'W',q)for q in j['stores']]);orders=[q[0]for q in events];rows=[];audit=[]
 for c in j['calls']:
  if c['entry']not in SELECT:continue
  initial={};current={};writes=[]
  for _,kind,q in events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]:
   at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
   for i,v in enumerate(raw):
    if at+i not in current:initial[at+i]=current[at+i]=v
    assert current[at+i]==v
   if kind=='W':
    if not(0x2000000<=at<0x2200000):writes.append(q)
    for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[at+i]=v
  rows.append(' '.join(['C',str(c['id']),c['entry']]+[c['registers'][k]for k in('eax','ecx','ebx','esi','edi')]+[c['fpcw'],c['mxcsr'],c['returned_eax']]+c['stack16']))
  rows.append('S '+' '.join('00000000'for _ in range(11)))
  for at,raw in spans(initial):rows.append(f'M {at:08x} {raw.hex()}')
  output={}
  for q in writes:
   for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
  for at,raw in spans(output):rows.append(f'A {at:08x} {raw.hex()}')
  for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
  audit.append(dict(id=c['id'],entry=c['entry'],input_bytes=len(initial),output_bytes=len(output),stores=len(writes)))
 a.output.write_text('\n'.join(rows)+'\n');a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n');print(json.dumps(dict(cases=len(audit),stores=sum(q['stores']for q in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
