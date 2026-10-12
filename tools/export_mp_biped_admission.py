#!/usr/bin/env python3
"""Export original biped admission/placement owner comparisons.

Every input and expected output comes from unchanged original execution.
Declared lower-owner service records are fixture-only dependency boundaries,
never a live provider, final native pose, allocator result or admission default.
Body placement, maintenance, resource gates, activation, object removal and
controller disposal, world removal and insertion compose natively; records
supply original caller identities.
"""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import spans,sha,grouping_scratch
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
SELECT={'00183650','000dd9a0','0013a710','00110ee0','001f0e20','000bacc0','000e3190','000e2ce0','000e2520'}
LOWER={'000baa80','000b9730','001d4c50','001d3bb0','001d2b10','001d4f30','000e3270','001e13c0','001d4a00','000e30b0'}
NATIVE={'000e3270','001d4a00','000e30b0','001e13c0','000baa80','000b9730','001d3bb0','001d2b10','001d4c50','001d4f30'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();j=json.loads(a.report.read_text());cs=j['rebuild_calls']
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
 def within(c):return events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
 rows=[];audit=[]
 for c in cs:
  if c['entry']not in SELECT:continue
  entry=int(c['entry'],16);regs={k:int(v,16)for k,v in c['registers'].items()};args=[int(v,16)for v in c['stack16']]
  def owned(q):
   p=int(q['address'],16)
   if entry==0x183650:return args[0]<=p and p+q['size']<=args[0]+0x105
   if entry==0xdd9a0:return any(at<=p and p+q['size']<=at+4 for at in(args[0],regs['ebx']))
   if entry==0xbacc0:return regs['esi']<=p and p+q['size']<=regs['esi']+12
   if entry==0x1f0e20:return any(at<=p and p+q['size']<=at+n for at,n in((args[0],4),(regs['esi'],12)))
   return not(0x2000000<=p<0x2200000) and int(q['instruction'],16)not in fs
  initial={};current={};writes=[]
  for _,kind,q in within(c):
   p=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
   for i,v in enumerate(raw):
    if p+i not in current:initial[p+i]=current[p+i]=v
    assert current[p+i]==v
   if kind=='W':
    if owned(q):writes.append(q)
    for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[p+i]=v
  children=[q for q in cs if c['order']<q['order']<c['return_order']]
  packet=next((int(q['entry_esp'],16)-16 for q in children if q['entry']=='003190e0'),0)
  # Real caller-local packet identities, recovered from their actual calls.
  rebuild=next((q for q in[c]+children if q['entry']=='000e2ce0'),None)
  place=next((q for q in[c]+children if q['entry']=='000e3190'),None)
  def written(site):return next((int(q['address'],16)for _,kind,q in within(c)if kind=='W'and q['instruction']==site),0)
  anchor=int(rebuild['entry_esp'],16)-12 if rebuild else 0
  frame=int(place['entry_esp'],16) if place else 0
  recursive=next((q for q in children if q['entry']=='000dd9a0'),None)
  first=int(recursive['stack16'][0],16)if recursive else 0
  second=int(recursive['registers']['ebx'],16)if recursive else 0
  child_result=written('000e3ad1')
  node_slots=frame-0x108 if frame else 0;visited=frame-0x110 if frame else 0;result_word=frame-0x10c if frame else 0
  rows.append(' '.join(['C',str(c['id']),c['entry']]+[c['registers'][k]for k in('eax','ecx','ebx','esi','edi')]+[c['fpcw'],c['mxcsr'],c['returned_eax']]+c['stack16']))
  rows.append('S '+' '.join(f'{x:08x}'for x in(packet,anchor,node_slots,visited,result_word,first,second,child_result,written('001f0e66'),written('001f0e6d'),written('000e2c03'))))
  for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
  output={}
  for q in writes:
   for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
  for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
  for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
  # Ancestor services own their complete subtree. Do not supply both the
  # owner and its children; native-owned parents expose only actual lower calls.
  plans=[];end=0
  for q in children:
   if q['entry']not in LOWER or q['order']<end:continue
   if q['entry']not in{'000e30b0','000b9730','000baa80'}:end=q['return_order']
   plans.append(q)
   query=next((s for s in cs if q['order']<s['order']<q['return_order']and s['entry']=='0016a9c0'),None)if q['entry']=='000e3270'else None
   point_sp=int(query['entry_esp'],16)if query else 0
   local=next((int(s['address'],16)for _,k,s in within(q)if k=='W'and s['instruction']in('00143ef8','00143f4f')),0)if query else 0
   placement=[]
   if q['entry']=='000e30b0':
    pc=next(s for s in cs if q['order']<s['order']<q['return_order']and s['entry']=='000e3190');ps=int(pc['entry_esp'],16)
    pair=next(s for s in cs if pc['order']<s['order']<pc['return_order']and s['entry']=='000dd9a0')
    cr=next(int(s['address'],16)for _,k,s in within(pc)if k=='W'and s['instruction']=='000e3ad1')
    placement=[ps-0x108,ps-0x110,ps-0x10c,int(pair['stack16'][0],16),int(pair['registers']['ebx'],16),cr]
   rows.append(' '.join(['P',q['entry']]+[q['registers'][k]for k in('eax','ecx','ebx','edx','esi','edi')]+[q['returned_eax']]+q['stack16']+[q['entry_esp']]+[f'{x:08x}'for x in(point_sp-8 if point_sp else 0,point_sp-4 if point_sp else 0,local)]+[f'{x:08x}'for x in placement]))
   if q['entry']=='001d2b10':
    group=next(s for s in cs if q['order']<s['order']<q['return_order']and s['entry']=='001d9330')
    nested=[s for s in cs if group['order']<s['order']<group['return_order']]
    stored=[s for _,k,s in within(group)if k=='W']
    rows.append('G '+' '.join(f'{x:08x}'for x in grouping_scratch(group,nested,stored)))
    for s in j['math']:
     if group['order']<s['order']<group['return_order']:rows.append('Q '+s['operand_word']+' '+s['result_word'])
   if q['entry']in NATIVE:continue
   for _,kind,s in within(q):
    if kind=='W' and not(0x2000000<=int(s['address'],16)<0x2200000) and int(s['instruction'],16)not in fs:
     rows.append('L '+s['instruction']+' '+s['address']+' '+s['before_hex']+' '+s['written_hex'])
  audit.append(dict(id=c['id'],entry=c['entry'],input_bytes=len(initial),output_bytes=len(output),stores=len(writes),fixture_only_lower_owners=[q['entry']for q in plans if q['entry']not in NATIVE],native_composed_owners=[q['entry']for q in plans if q['entry']in NATIVE]))
 a.output.write_text('\n'.join(rows)+'\n');a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,boundaries=__doc__,native_expected_data_used=False),indent=2)+'\n');print(json.dumps(dict(cases=len(audit),stores=sum(c['stores']for c in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
