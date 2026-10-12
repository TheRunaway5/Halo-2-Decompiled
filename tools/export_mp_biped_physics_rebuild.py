#!/usr/bin/env python3
"""Export natural1DA570 admission, retaining real lower-owner side effects.

The native test replays the independently executed grouping owner's writes
only at its required service boundary. This validates1DA570 orchestration;
it does not claim a native grouping constructor or live Havok integration.
"""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN,Xbe,_Code,_trace

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def spans(bank):
 addresses=sorted(bank);start=last=addresses[0];raw=bytearray([bank[start]])
 for p in addresses[1:]:
  if p!=last+1:yield start,bytes(raw);start=p;raw=bytearray()
  raw.append(bank[p]);last=p
 yield start,bytes(raw)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();j=json.loads(a.report.read_text())
 assert j['tool_sha256']==sha(ROOT/'tools/oracle_mp_death_physics_ticks_1_1.py')
 assert j['source_sha256']==PIN and j['cache_sha256']==CACHE_PIN and not j['instruction_patches'] and not j['new_supplied_guest_fields_or_results']
 assert j['failure']is None and j['frozen_prior_byte_identical']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');code=_Code(image,'.text')
 initial={};current={};rows=[];compared=0
 calls=j['rebuild_calls'];assert len(j['rebuild_roots'])==1
 root=next(c for c in calls if c['id']==j['rebuild_roots'][0]);assert root['entry']=='001da570'
 child=next(c for c in calls if c['entry']=='001d9330'and c['caller']=='001da622')
 assert root['order']<child['order']<child['return_order']<root['return_order']
 stack=lambda p:0x2000000<=p<0x2200000
 events=sorted([(r['order'],'R',r)for r in j['rebuild_reads']]+[(r['order'],'W',r)for r in j['rebuild_stores']])
 for order,kind,r in events:
  if not(root['order']<order<root['return_order']):continue
  at=int(r['address'],16)
  if stack(at):continue
  raw=bytes.fromhex(r['raw_hex']if kind=='R'else r['before_hex'])
  for i,b in enumerate(raw):
   p=at+i
   if p in current:assert current[p]==b,(kind,r['instruction'],hex(p))
   else:initial[p]=current[p]=b
  if kind=='W':
   for i,b in enumerate(bytes.fromhex(r['written_hex'])):current[at+i]=b
 rows.append('C '+root['registers']['edi']+' '+child['returned_eax'])
 for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
 for p,raw in spans(current):rows.append(f'A {p:08x} {raw.hex()}');compared+=len(raw)
 stores=[r for r in j['rebuild_stores']if child['order']<r['order']<child['return_order']and not stack(int(r['address'],16))]
 for r in stores:rows.append('S '+r['address']+' '+r['before_hex']+' '+r['written_hex'])
 own=[r for r in j['rebuild_stores']if root['order']<r['order']<root['return_order']and not(child['order']<r['order']<child['return_order'])and not stack(int(r['address'],16))]
 for r in own:rows.append('W '+r['instruction']+' '+r['address']+' '+r['before_hex']+' '+r['written_hex'])
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
 listings={}
 for entry in (0x1da570,0xe23f0,0x1c70d0):
  body=[code.at(p)for p in sorted(_trace(code,entry,set()).code)]
  raw=''.join(f'{r.address:08x} {r.bytes.hex()} {r.mnemonic} {r.op_str}\n'for r in body)
  listings[f'{entry:08x}']=hashlib.sha256(raw.encode()).hexdigest()
 a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(tool_sha256=sha(__file__),source_sha256=PIN,
  cache_sha256=CACHE_PIN,source_report_sha256=sha(a.report),fixture_sha256=sha(a.output),instruction_hashes=listings,
  compared_bytes=compared,native_own_stores=len(own),replayed_original_grouping_stores=len(stores),boundaries=__doc__),indent=2)+'\n')
 print(json.dumps(dict(compared_bytes=compared,native_own_stores=len(own),source_grouping_stores=len(stores))))
if __name__=='__main__':main()
