#!/usr/bin/env python3
"""Export unchanged original body placement and physical transform owners.

Natural admission and explicit inherited-placement API captures are separate.
No native output, supplied collision outcome or completed pose is used.
"""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import sha,spans
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
SELECT={'000e3270','001d3970','001d4610','001d46e0','00142f20','00144620','001d47c0','001d4a00'}
def main(select=SELECT,boundaries=__doc__,exporter=__file__):
 ap=argparse.ArgumentParser(description=boundaries);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();j=json.loads(a.report.read_text())
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN and j['failure']is None and not j['instruction_patches']
 assert j['frozen_prior_byte_identical']
 if 'rebuild_calls'in j:
  assert j['tool_sha256']==sha(Path(__file__).with_name('oracle_mp_death_physics_ticks_1_1.py'));cs=j['rebuild_calls'];reads=j['rebuild_reads'];stores=j['rebuild_stores']
 else:
  assert j['oracle_sha256']==sha(Path(__file__).with_name('oracle_mp_biped_body_placement_1_1.py'))
  assert j['natural_admission_snapshot_byte_identical']and j['supplied_caller_owned_anchor_bytes_only'];cs=j['calls'];reads=j['reads'];stores=j['stores']
 for listing in j['instructions']:assert sha(a.report.parent/listing['file'])==listing['sha256']
 events=sorted([(q['order'],'R',q)for q in reads]+[(q['order'],'W',q)for q in stores]);orders=[q[0]for q in events];rows=[];audit=[]
 for c in cs:
  if c['entry']not in select:continue
  entry=int(c['entry'],16);regs={k:int(v,16)for k,v in c['registers'].items()};args=[int(v,16)for v in c['stack16']]
  def owned(q):
   at=int(q['address'],16)
   if entry==0x1d3970:return any(p<=at and at+q['size']<=p+12 for p in(regs['edi'],regs['esi']))
   if entry==0x142f20:return args[0]<=at and at+q['size']<=args[0]+52
   if entry==0x144620:return regs['eax']<=at and at+q['size']<=regs['eax']+36
   return not(0x2000000<=at<0x2200000)
  initial={};current={};writes=[]
  es=events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
  for _,kind,q in es:
   at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
   for i,v in enumerate(raw):
    if at+i not in current:initial[at+i]=current[at+i]=v
    assert current[at+i]==v
   if kind=='W':
    if owned(q):writes.append(q)
    for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[at+i]=v
  children=[q for q in cs if c['order']<q['order']<c['return_order']]
  query=next((q for q in children if q['entry']=='0016a9c0'),None)
  point_sp=int(query['entry_esp'],16)if query else 0
  local=next((int(q['address'],16)for _,k,q in es if k=='W'and q['instruction']in('00143ef8','00143f4f')),0)
  rows.append(' '.join(['C',str(c['id']),c['entry'],c['entry_esp']]+[c['registers'][k]for k in('eax','ecx','edx','ebx','esi','edi')]+[c['fpcw'],c['mxcsr'],c['returned_eax']]+c['stack16']))
  rows.append('S '+' '.join(f'{v:08x}'for v in(point_sp-8 if point_sp else 0,point_sp-4 if point_sp else 0,local)))
  for at,raw in spans(initial):rows.append(f'M {at:08x} {raw.hex()}')
  output={}
  for q in writes:
   for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
  for at,raw in spans(output):rows.append(f'A {at:08x} {raw.hex()}')
  for q in writes:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
  audit.append(dict(id=c['id'],entry=c['entry'],input_bytes=len(initial),output_bytes=len(output),stores=len(writes)))
 a.output.write_text('\n'.join(rows)+'\n');a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),fixture_sha256=sha(a.output),exporter_sha256=sha(exporter),extraction_sha256=sha(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,native_expected_data_used=False,boundaries=boundaries),indent=2)+'\n');print(json.dumps(dict(cases=len(audit),stores=sum(q['stores']for q in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
