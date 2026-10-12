#!/usr/bin/env python3
"""Export source death transition owners with native physical reconstruction.

E6050, E6DC0,1E8E60,1EFF60,1C70D0,1D4C50,315D50,314690,314810,31C6D0,
1D3BB0,1D2B10 and1D4F30 compose natively. Records supply original caller
identities and platform reciprocal-square-root observations, never lower-owner
effects, a live physics provider or a completed pose.
"""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import sha,spans
from export_mp_biped_controller import load,controller_scratch
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN
SELECT={'000e6050','000e6dc0','001e8e60'}
LOWER={'001d4c50','001d3bb0','001d2b10','001d4f30'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();context,j,original=load(a.report);cs=j['calls']
 floating={c['id']:c for c in context['contexts']};fs=set()
 for listing in j['instructions']:
  p=original.parent/listing['file'];assert sha(p)==listing['sha256']
  fs.update(int(line.split()[0],16)for line in p.read_text().splitlines()if 'fs:[0]'in line)
 events=sorted([(q['order'],'R',q)for q in j['reads']]+[(q['order'],'W',q)for q in j['stores']]);orders=[q[0]for q in events];rows=[];audit=[]
 def within(c):return events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
 def retained(q):return not(0x2000000<=int(q['address'],16)<0x2200000)and int(q['instruction'],16)not in fs
 for c in cs:
  if c['entry']not in SELECT:continue
  initial={};current={};writes=[]
  for _,kind,q in within(c):
   at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
   for i,v in enumerate(raw):
    if at+i not in current:initial[at+i]=current[at+i]=v
    assert current[at+i]==v
   if kind=='W':
    if retained(q):writes.append(q)
    for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[at+i]=v
  fp=floating[c['id']];assert fp['entry']==c['entry']
  rows.append(' '.join(['C',str(c['id']),c['entry']]+[c['registers'][k]for k in('eax','ecx','ebx','esi','edi')]+[fp['fpcw'],fp['mxcsr'],c['returned_eax']]+c['stack16']))
  # The transition's caller-owned vector is distinct from the retained unit.
  anchor=int(c['entry_esp'],16)-12 if c['entry']=='000e6050'else int(c['entry_esp'],16)-12
  rows.append('S '+' '.join(f'{v:08x}'for v in(0,anchor,0,0,0,0,0,0,0,0,0)))
  for at,raw in spans(initial):rows.append(f'M {at:08x} {raw.hex()}')
  output={}
  for q in writes:
   for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
  for at,raw in spans(output):rows.append(f'A {at:08x} {raw.hex()}')
  for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
  end=0;plans=[]
  for q in cs:
   if not(c['order']<q['order']<c['return_order'])or q['order']<end or q['entry']not in LOWER:continue
   end=q['return_order'];plans.append(q)
   rows.append(' '.join(['P',q['entry']]+[q['registers'][k]for k in('eax','ecx','ebx','edx','esi','edi')]+[q['returned_eax']]+q['stack16']+[q['entry_esp'],'00000000','00000000','00000000']))
   if q['entry']=='001d3bb0':continue
   if q['entry']in{'001d4c50','001d4f30'}:
    continue
   if q['entry']=='001d2b10':
    rows.append('J '+' '.join(f'{v:08x}'for v in controller_scratch(q,context['controller_calls'])))
    for s in context['math']:
     if q['order']<s['order']<q['return_order']:rows.append('Q '+s['operand_word']+' '+s['result_word'])
    continue
   for _,kind,s in within(q):
    if kind=='W'and retained(s):rows.append('L '+s['instruction']+' '+s['address']+' '+s['before_hex']+' '+s['written_hex'])
  audit.append(dict(id=c['id'],entry=c['entry'],input_bytes=len(initial),output_bytes=len(output),stores=len(writes),fixture_only_lower_owners=[q['entry']for q in plans if q['entry']not in LOWER],native_composed_owners=[q['entry']for q in plans if q['entry']in LOWER]))
 assert len(audit)==3
 a.output.write_text('\n'.join(rows)+'\n');a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_context_report_sha256=sha(a.report),source_report_sha256=sha(original),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n');print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
