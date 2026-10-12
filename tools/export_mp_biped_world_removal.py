#!/usr/bin/env python3
"""Export native world-removal owners from unchanged ordinary moving death.

Component/body membership, broadphase endpoints and pairs, contact retirement,
listeners, island deletion, reference lifetime and allocator stores are computed
by the native owners. There are no supplied lower-owner result stores.
"""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import sha,spans
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN
SELECT={'001d4c50','00315d50','00314810','00313950','0031e570','002e8f00','002e6520','002e6340','00314690','0031c6d0','002ef640','0031f740'}
def load(path):
 ctx=json.loads(path.read_text())
 assert ctx['source_sha256']==PIN and ctx['cache_sha256']==CACHE_PIN
 assert ctx['oracle_sha256']==sha(Path(__file__).with_name('oracle_mp_death_removal_1_1.py'))
 assert ctx['entire_frozen_moving_death_byte_identical'] and not ctx['instruction_patches'] and not ctx['new_supplied_gameplay_or_physics_results']
 original=ROOT/ctx['source_report'];assert sha(original)==ctx['source_report_sha256']
 j=json.loads(original.read_text());assert j['oracle_sha256']==sha(Path(__file__).with_name('oracle_mp_spartan_death_physical_1_1.py'))
 return ctx,j,original
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();ctx,j,original=load(a.report)
 events=sorted([(q['order'],'R',q)for q in j['reads']]+[(q['order'],'W',q)for q in j['stores']]);orders=[q[0]for q in events];rows=[];audit=[];fs=set()
 for listing in j['instructions']:
  p=original.parent/listing['file'];assert sha(p)==listing['sha256'];fs.update(int(line.split()[0],16)for line in p.read_text().splitlines()if 'fs:[0]'in line)
 def within(c):return events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
 def retained(q):return not(0x2000000<=int(q['address'],16)<0x2200000)and int(q['instruction'],16)not in fs
 for index,c in enumerate(ctx['membership_calls']):
  if c['entry']not in SELECT:continue
  initial={};current={};writes=[]
  for _,kind,q in within(c):
   p=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
   for i,v in enumerate(raw):
    if p+i not in current:initial[p+i]=current[p+i]=v
    assert current[p+i]==v
   if kind=='W':
    if retained(q):writes.append(q)
    for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[p+i]=v
  rows.append(' '.join(['C',str(index),c['entry']]+[c['registers'][k]for k in('eax','ecx','ebx','esi','edi')]+[c['fpcw'],c['mxcsr'],c['returned_eax']]+c['stack16']))
  rows.append('S '+' '.join([c['entry_esp']]+['00000000']*10))
  for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
  out={}
  for q in writes:
   for i,v in enumerate(bytes.fromhex(q['written_hex'])):out[int(q['address'],16)+i]=v
  for p,raw in spans(out):rows.append(f'A {p:08x} {raw.hex()}')
  for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
  plans=[]
  for q in plans:
   rows.append(' '.join(['P',q['entry']]+[q['registers'][k]for k in('eax','ecx','ebx','edx','esi','edi')]+[q['returned_eax']]+q['stack16']+[q['entry_esp']]+['00000000']*3))
   for _,kind,s in within(q):
    if kind=='W'and retained(s):rows.append('L '+s['instruction']+' '+s['address']+' '+s['before_hex']+' '+s['written_hex'])
  audit.append(dict(id=index,entry=c['entry'],input_bytes=len(initial),output_bytes=len(out),stores=len(writes),fixture_only_lower_owners=[q['entry']for q in plans]))
 assert len(audit)==40
 a.output.write_text('\n'.join(rows)+'\n');a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_context_report_sha256=sha(a.report),source_report_sha256=sha(original),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n');print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
