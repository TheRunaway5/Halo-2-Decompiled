#!/usr/bin/env python3
"""Export original serialized-to-resident Spartan model initialization."""
import argparse,json
from pathlib import Path
from export_mp_ragdoll_owners import sha,spans
from oracle_smg_firing_1_1 import PIN,CACHE_PIN

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();j=json.loads(a.report.read_text())
 assert j['oracle_sha256']==sha(Path(__file__).with_name('oracle_mp_spartan_shape_factory_1_1.py'))
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN
 assert not j['instruction_patches'] and not j['new_supplied_guest_fields_or_results']
 for e in j['instructions']:assert sha(a.report.parent/e['file'])==e['sha256']
 c=j['call'];base=int(c['model_pointer'],16);initial={base+i:v for i,v in enumerate(bytes.fromhex(c['raw_before_hex']))};current=dict(initial);writes=[]
 for _,kind,r in sorted([(r['order'],'R',r)for r in j['reads']]+[(r['order'],'W',r)for r in j['stores']]):
  p=int(r['address'],16)
  if 0x2000000<=p<0x2200000:continue
  raw=bytes.fromhex(r['raw_hex']if kind=='R'else r['before_hex'])
  for i,v in enumerate(raw):
   if p+i not in current:initial[p+i]=current[p+i]=v
   assert current[p+i]==v
  if kind=='W':
   writes.append(r)
   for i,v in enumerate(bytes.fromhex(r['written_hex'])):current[p+i]=v
 assert bytes(current[base+i]for i in range(c['model_size']))==bytes.fromhex(c['raw_after_hex'])
 rows=['C '+c['registers']['eax']+' '+c['stack16'][0]]
 for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
 for p,raw in spans(current):rows.append(f'A {p:08x} {raw.hex()}')
 for r in writes:rows.append('W '+r['instruction']+' '+r['address']+' '+r['before_hex']+' '+r['written_hex'])
 a.output.write_text('\n'.join(rows)+'\n');a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(source_report_sha256=sha(a.report),exporter_sha256=sha(__file__),fixture_sha256=sha(a.output),source_sha256=PIN,cache_sha256=CACHE_PIN,model_bytes=c['model_size'],retained_stores=len(writes),native_results_used=False),indent=2)+'\n');print(json.dumps(dict(model_bytes=c['model_size'],retained_stores=len(writes),fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
