#!/usr/bin/env python3
"""Export observed full retained124980/13F020 buffers and original stores."""
import argparse,hashlib,json
from pathlib import Path
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def memory(s):
 m=dict(s['runtime']);m['004eb118']=s['resource_globals1c_hex'];m['004ed9f0']=s['physical_arena34_hex']
 return m
def save(p,raw):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==raw
 else:p.write_bytes(raw)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();d=json.loads(a.source.read_text())
 lines=['# source-animation-capacity-v1','# source-report-sha256 '+sha(a.source)];counts=[]
 for c in d['resource_calls']:
  if c['entry']not in('00124980','0013f020')or'complete_state_before'not in c:continue
  before=memory(c['complete_state_before']);after=memory(c['complete_state_after']);entry=int(c['entry'],16)
  args=[int(c['stack'][0],16)]if entry==0x124980 else[c['registers']['eax'],int(c['stack'][0],16)]
  lines.append(f'CASE {c["id"]} {entry:x} '+' '.join(f'{v:x}'for v in args))
  for name,mem in [('BEFORE',before),('AFTER',after)]:
   for base,raw in sorted(mem.items()):lines.append(name+' '+base+' '+raw)
  events=[]
  for q in d['resource_stores']:
   if c['id']not in q['resource_calls']:continue
   addr=int(q['address'],16);n=q['size'];ip=int(q['instruction'],16)
   if not any(int(base,16)<=addr and addr+n<=int(base,16)+len(raw)//2 for base,raw in before.items()):continue
   assert n==4
   events.append((q['order'],f'EVENT STORE {ip:x} {addr:x} {int.from_bytes(bytes.fromhex(q["before"]),"little"):x} {int.from_bytes(bytes.fromhex(q["written"]),"little"):x}'))
  for q in d['applied_access']:
   if entry!=0x124980 or q['phase']!=c['phase']:continue
   events.append((q['order'],'EVENT PROTECT '+' '.join(q['args'])))
   for base,raw in sorted(memory(q['state_before']).items()):lines.append('AT_PROTECT '+base+' '+raw)
  lines.extend(v for _,v in sorted(events));lines.append('ENDCASE');counts.append(dict(entry=c['entry'],id=c['id'],events=len(events)))
 assert len(counts)==2
 save(a.output,('\n'.join(lines)+'\n').encode());print(json.dumps(dict(cases=counts,fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
