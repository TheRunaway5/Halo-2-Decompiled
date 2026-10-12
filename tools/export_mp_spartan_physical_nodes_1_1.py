#!/usr/bin/env python3
"""Export independently executed physical-node inputs and complete output banks."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from audit_mp_spartan_death_physical_1_1 import segments,encoded,sha

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--directory',type=Path,required=True)
 ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();r=json.loads((a.directory/'report.json').read_text())
 assert r['oracle_sha256']==sha(ROOT/'tools/oracle_mp_spartan_physical_nodes_1_1.py')
 assert r['source_sha256']==PIN and not r['instruction_patches'] and not r['new_guest_inputs_or_results_supplied']
 assert len(r['cases'])==1 and r['cases'][0]['completed'] and len(r['stages'])==1 and not r['stages'][0]['used_services']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 for listing in r['instructions']:
  file=a.directory/listing['file'];assert sha(file)==listing['sha256']
  for line in file.read_text().splitlines():
   address,raw,*_=line.split(' ',2);assert image.read(int(address,16),len(bytes.fromhex(raw)))==bytes.fromhex(raw)
 initial={};current={};writes=0
 case=r['cases'][0];ob=int(case['unit'],16)
 # Retain the entire actual object, including all untouched current/local nodes.
 for _,(base,raw)in segments(r['stages'][0]['before']).items():
  if base==ob:
   for i,b in enumerate(raw):initial[base+i]=current[base+i]=b
 def stack(p):return 0x2000000<=p<0x2200000
 events=sorted([(x['order'],'R',x)for x in r['reads']]+[(x['order'],'W',x)for x in r['stores']])
 for _,kind,row in events:
  at=int(row['address'],16)
  if stack(at):continue
  raw=bytes.fromhex(row['raw_hex']if kind=='R'else row['before_hex'])
  for i,b in enumerate(raw):
   p=at+i
   if p in current:assert current[p]==b,(kind,row['instruction'],hex(p))
   else:initial[p]=current[p]=b
  if kind=='W':
   for i,b in enumerate(bytes.fromhex(row['written_hex'])):current[at+i]=b
   writes+=1
 # Native scratch belongs to the caller; it is deliberately independent of source stack addresses.
 for at,n in ((0x10000000,32),(0x10000100,64)):
  for i in range(n):initial[at+i]=0
 rows=[f"C {case['component']} {case['mask']} 10000000 10000100"]
 def spans(bank):
  addresses=sorted(bank);start=last=addresses[0];raw=bytearray([bank[start]])
  for p in addresses[1:]:
   if p!=last+1:yield start,bytes(raw);start=p;raw=bytearray()
   raw.append(bank[p]);last=p
  yield start,bytes(raw)
 for p,raw in spans(initial):rows.append(f'M {p:08x} {raw.hex()}')
 for p,raw in spans(current):rows.append(f'A {p:08x} {raw.hex()}')
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
 a.output.with_suffix('.provenance.json').write_bytes(encoded(dict(tool_sha256=sha(__file__),report_sha256=sha(a.directory/'report.json'),
  fixture_sha256=sha(a.output),source_sha256=PIN,initial_bytes=len(initial),compared_output_bytes=len(current),source_stores=writes,
  source_execution_replacements=[],natural_ragdoll_admission=False)))
 print(json.dumps(dict(initial_bytes=len(initial),compared_output_bytes=len(current),source_stores=writes)))
if __name__=='__main__':main()
