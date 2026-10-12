#!/usr/bin/env python3
"""Export literal original point collision owners, with no supplied result."""
import argparse,bisect,json
from pathlib import Path
from export_mp_ragdoll_owners import sha,spans
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
SELECT={'0016a9c0','0016a7e0','0016a430','0014b9d0','00143e70'}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();j=json.loads(a.report.read_text())
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN and j['failure']is None and not j['instruction_patches']
 if 'rebuild_calls'in j:
  assert j['tool_sha256']==sha(Path(__file__).with_name('oracle_mp_death_physics_ticks_1_1.py'));assert j['frozen_prior_byte_identical'];cs=j['rebuild_calls'];reads=j['rebuild_reads'];stores=j['rebuild_stores']
 else:
  assert j['oracle_sha256']==sha(Path(__file__).with_name('oracle_mp_death_point_collision_1_1.py'))
  assert j['frozen_prior_byte_identical']and j['natural_admission_snapshot_byte_identical']and j['supplied_caller_owned_point_bytes_only'];cs=j['calls'];reads=j['reads'];stores=j['stores']
 for listing in j['instructions']:assert sha(a.report.parent/listing['file'])==listing['sha256']
 events=sorted([(q['order'],'R',q)for q in reads]+[(q['order'],'W',q)for q in stores]);orders=[q[0]for q in events];rows=[];audit=[]
 for c in cs:
  if c['entry']not in SELECT:continue
  initial={};current={};owned=[];entry=int(c['entry'],16);out=int(c['registers']['edx'],16)
  es=events[bisect.bisect_right(orders,c['order']):bisect.bisect_left(orders,c['return_order'])]
  for _,kind,q in es:
   at=int(q['address'],16);raw=bytes.fromhex(q['raw_hex']if kind=='R'else q['before_hex'])
   for i,v in enumerate(raw):
    if at+i not in current:initial[at+i]=current[at+i]=v
    assert current[at+i]==v
   if kind=='W':
    if entry==0x143e70 and out<=at and at+q['size']<=out+12:owned.append(q)
    elif not(0x2000000<=at<0x2200000):assert False,('Unexpected retained collision query write',q)
    for i,v in enumerate(bytes.fromhex(q['written_hex'])):current[at+i]=v
  # Source scratch tokens used by nested unmodified inverse/iterator owners.
  def written(site):return next((int(q['address'],16)for _,kind,q in es if kind=='W'and q['instruction']==site),0)
  local=written('00143ef8')or written('00143f4f')
  sp=int(c['entry_esp'],16)
  rows.append(' '.join(['C',str(c['id']),c['entry']]+[c['registers'][k]for k in('eax','ecx','edx','ebx','esi')]+[c['fpcw'],c['mxcsr'],c['returned_eax'],f'{sp-8:08x}',f'{sp-4:08x}',f'{local:08x}']+c['stack16']))
  for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
  output={}
  for q in owned:
   for i,v in enumerate(bytes.fromhex(q['written_hex'])):output[int(q['address'],16)+i]=v
  for p,raw in spans(output):rows.append(f'A {p:08x} {raw.hex()}')
  for q in owned:rows.append('W '+q['instruction']+' '+q['address']+' '+q['before_hex']+' '+q['written_hex'])
  audit.append(dict(id=c['id'],entry=c['entry'],input_bytes=len(initial),output_bytes=len(output),stores=len(owned)))
 a.output.write_text('\n'.join(rows)+'\n');a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),fixture_sha256=sha(a.output),exporter_sha256=sha(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,cases=audit,native_expected_data_used=False),indent=2)+'\n');print(json.dumps(dict(cases=len(audit),stores=sum(q['stores']for q in audit),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
