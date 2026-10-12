#!/usr/bin/env python3
"""Export complete original retained buffers; lower replay remains test-only."""
import argparse,hashlib,json
from pathlib import Path
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def word(raw,offset,n=4):return int.from_bytes(bytes.fromhex(raw)[offset:offset+n],'little')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.report.read_text());calls=r['calls'];reads=r['reads'];literal={}
 for row in reads:
  address=int(row['address'],16)
  if 'buffer'in row or 'tag'in row or 0x2000000<=address<0x2200000:continue
  for i,b in enumerate(bytes.fromhex(row['raw_hex'])):
   if address+i in literal:assert literal[address+i]==b,(hex(address+i),literal[address+i],b)
   literal[address+i]=b
 for tag in r['tags'].values():
  address=int(tag['address'],16)
  for i,b in enumerate(bytes.fromhex(tag['raw_hex'])):
   if address+i in literal:assert literal[address+i]==b
   literal[address+i]=b
 lines=['# original-generic-object-construction-v1','# source '+sha(a.report)]
 listing=(a.report.parent/'complete-generic-constructor.asm').read_text().splitlines()
 assert any(v.startswith('000b90f9 0f2fc8 comiss xmm1, xmm0')for v in listing)
 lines.append('COMISS 000b90f9 0f2fc8')
 addresses=sorted(literal);i=0
 while i<len(addresses):
  start=addresses[i];raw=bytearray([literal[start]]);i+=1
  while i<len(addresses)and addresses[i]==start+len(raw):raw.append(literal[addresses[i]]);i+=1
  lines.append(f'MEM {start:x} {raw.hex()}')
 def original(address,n):return bytes(literal[address+i]for i in range(n))
 pool=int.from_bytes(original(0x4e78d0,4),'little');world=int.from_bytes(original(0x4e58c4,4),'little')
 one=int.from_bytes(original(0x465030,4),'little');sentinel=int.from_bytes(original(0x46fb64,2),'little')
 lines.append(f'OPERANDS {one:x} {sentinel:x}')
 roots=[q for q in calls if not q['ancestors']];total=children=stores=0
 def buffers(prefix,q):
  nonlocal total
  values=q;heads=word(values['P'],0x44)
  for name,raw in values.items():
   if name=='P':at=pool
   elif name=='K':at=heads
   elif name=='W':at=world
   elif name=='D':at=root['descriptor']
   elif name=='S':at=root['scratch']
   else:assert name.startswith('U');at=word(values['K'],int(name[1:])*12+8)
   lines.append(f'{prefix} {name} {at:x} {raw}')
   if prefix=='AFTER':total+=len(raw)//2
 for root in roots:
  assert root['entry']=='000b8a30'
  lines.append(f'CASE {root["id"]:x} {root["descriptor"]:x} {root["scratch"]:x} {root["return_cpu"]["registers"]["eax"]} {root["entry_cpu"]["mxcsr"]} {root["return_cpu"]["mxcsr"]}')
  buffers('BEFORE',root['before'])
  children_rows=[v for v in calls if v['ancestors']==[root['id']]]
  for child in children_rows:
   regs=child['entry_cpu']['registers'];regwords=[regs[n]for n in ('eax','ecx','edx','ebx','esi','edi')]
   lines.append('CALL '+child['call_site']+' '+child['entry']+' '+child['return_cpu']['registers']['eax']+' '+' '.join(regwords+child['stack'][:5])+ ' '+child['entry_cpu']['xmm128'][0]+' '+child['entry_cpu']['mxcsr']+' '+child['return_cpu']['mxcsr']);buffers('CALLBEFORE',child['before']);buffers('CALLAFTER',child['after']);lines.append('ENDCALL');children+=1
  for row in r['stores']:
   if row['owner']==root['id'] and row.get('buffer')not in (None,'S'):
    lines.append(f'STORE {row["instruction"]} {row["address"]} {row["bytes"]:x} {int.from_bytes(bytes.fromhex(row["before"]),"little"):x} {int.from_bytes(bytes.fromhex(row["value_hex"]),"little"):x}');stores+=1
  buffers('AFTER',root['after']);lines.append('ENDCASE')
 data=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True)
 if a.output.exists():assert a.output.read_bytes()==data
 else:a.output.write_bytes(data)
 print(json.dumps(dict(cases=len(roots),lower_calls=children,retained_stores=stores,whole_output_bytes=total,sha256=sha(a.output))))
if __name__=='__main__':main()
