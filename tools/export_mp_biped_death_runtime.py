#!/usr/bin/env python3
"""Export original natural-admission biped death constraint/gate owners.

First-touch original reads and store-before bytes are the only inputs.
Retained outputs and ordered stores are only from unmodified original calls.
Source temporary requirement packets are caller calculations, excluded from
retained output comparisons. No native values, body admission, pose or result
is used to generate this fixture.
"""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import spans,sha
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
SELECT={'000e5680','001f08e0','000e2430','001d6fc0'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();j=json.loads(a.report.read_text())
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
 assert j['frozen_prior_byte_identical'] and j['failure'] is None and j['capture_entry']=='000e2520'
 assert not j['instruction_patches'] and not j['new_supplied_guest_fields_or_results']
 assert j['tool_sha256']==sha(Path(__file__).with_name('oracle_mp_death_physics_ticks_1_1.py'))
 fs=set()
 for listing in j['instructions']:
  p=a.report.parent/listing['file'];assert sha(p)==listing['sha256']
  for line in p.read_text().splitlines():
   if 'fs:[0]'in line:fs.add(int(line.split()[0],16))
 events=sorted([(r['order'],'R',r)for r in j['rebuild_reads']]+[(r['order'],'W',r)for r in j['rebuild_stores']]);orders=[r[0]for r in events]
 rows=[];audit=[]
 for c in j['rebuild_calls']:
  if c['entry']not in SELECT:continue
  initial={};current={};writes=[]
  for _,kind,r in events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]:
   at=int(r['address'],16);raw=bytes.fromhex(r['raw_hex']if kind=='R'else r['before_hex'])
   for i,v in enumerate(raw):
    if at+i not in current:initial[at+i]=current[at+i]=v
    assert current[at+i]==v,(c['id'],kind,r)
   if kind=='W':
    if not(0x2000000<=at<0x2200000) and int(r['instruction'],16)not in fs:writes.append(r)
    for i,v in enumerate(bytes.fromhex(r['written_hex'])):current[at+i]=v
  output={}
  for r in writes:
   at=int(r['address'],16)
   for i,v in enumerate(bytes.fromhex(r['written_hex'])):output[at+i]=v
  children=[q for q in j['rebuild_calls']if c['order']<q['order']<c['return_order']]
  packet=next((int(q['entry_esp'],16)-0x10 for q in children if q['entry']=='003190e0'),0)
  rows.append(' '.join(['C',str(c['id']),c['entry']]+[c['registers'][k]for k in('eax','ecx')]+[c['fpcw'],c['mxcsr'],c['returned_eax'],f'{packet:08x}']+c['stack16']))
  for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
  for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
  for r in writes:rows.append('W '+r['instruction']+' '+r['address']+' '+r['before_hex']+' '+r['written_hex'])
  audit.append(dict(id=c['id'],entry=c['entry'],input_bytes=len(initial),output_bytes=len(output),stores=len(writes)))
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
 a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(exporter_sha256=sha(__file__),source_report_sha256=sha(a.report),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n')
 print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
