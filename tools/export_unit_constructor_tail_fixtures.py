#!/usr/bin/env python3
"""Literal full source constructor-tail buffers/callbacks/stores, not gameplay defaults."""
import argparse,json
from pathlib import Path
from oracle_mp_unit_constructors_1_1 import sha,save
IMPLEMENTED={0x110a20,0x116680,0x110bf0,0x110800,0x1100e0,0x10f380,0x115030,0x16f510,0x1e0aa0,0x114f70}
LOWER={0x1d0a60,0x114f00,0x1cfa90,0xbb240,0x114bd0,0x1e13c0}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--operands',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.report.read_text());ops=json.loads(a.operands.read_text())['images'][0];calls=r['calls'];lines=['# literal-source-unit-constructor-tail-v1','# report-sha256 '+sha(a.report),'# operand-sha256 '+sha(a.operands)]
 # Full packet bytes stay ignored/external; no imported assets are embedded in
 # native source or tests. Source-created mutable body snapshots are test only.
 tags=json.loads((a.report.parent.parent/'literal-tags.json').read_text())
 for key,v in sorted(tags['tags'].items()):lines.append(f'TAG {key} {v["address"]} {v["raw_hex"]}')
 one=int(r['one_bits'],16);zero=0 # actual465048 operand below, not a runtime initializer
 source_reads={int(v['address'],16):v['raw_hex']for v in r['reads']if int(v['address'],16)==0x465048}
 assert source_reads.get(0x465048)=='00000000';zero=int.from_bytes(bytes.fromhex(source_reads[0x465048]),'little')
 lines.append('CONSTANTS '+' '.join(f'{v:x}'for v in(one,zero,ops['normal_blend_bits']['bits'],ops['special_blend_bits']['bits']))+' '+' '.join(f'{v:x}'for pair in ops['activity_pairs']['pairs']for v in pair))
 addresses={e:{int(line.split()[0],16)for line in(a.report.parent/'instructions'/f'{e:08x}.asm').read_text().splitlines()}for e in IMPLEMENTED}
 count=0
 for c in calls:
  e=int(c['entry'],16)
  if e not in IMPLEMENTED or 'after'not in c:continue
  regs=c['registers'];stack=[int(s,16)for s in c['stack']]
  args={0x110a20:[],0x110800:[],0x1100e0:[],0x10f380:[],0x115030:[],0x116680:[stack[0]],0x110bf0:[regs['eax']]+stack[1:7],0x16f510:[regs['ecx'],regs['edi']],0x1e0aa0:[regs['edx'],stack[1]],0x114f70:[regs['eax'],regs['ecx'],regs['edx']&255,stack[0]&255,stack[1]]}[e]
  result=int(c['return_cpu']['xmm128'][0],16)&0xffffffff if e==0x114f70 else int(c['return_eax'],16)&(0xff if e==0x110bf0 else 0xffffffff)
  lines.append('CASE '+str(c['id'])+' '+' '.join(f'{v:x}'for v in(e,c.get('unit',0xffffffff),c.get('pool',{}).get('capacity',0),result,len(args),*args)))
  for name,v in c['before'].items():lines.append('BEFORE '+name+' '+v)
  for name,v in c['after'].items():lines.append('AFTER '+name+' '+v)
  gate=[v for v in r['reads']if c['id']in v['calls']and int(v['instruction'],16)==0x110c75]
  if gate:assert len(gate)==1;lines.append('GATE '+gate[0]['raw_hex'])
  descendants=[v for v in calls if c['id']in v['ancestors']]
  leaves=[v for v in descendants if int(v['entry'],16)in LOWER and not any(int(calls[q]['entry'],16)in LOWER for q in v['ancestors'][len(c['ancestors'])+1:])]
  for d in leaves:
   de=int(d['entry'],16);dr=d['registers'];ds=[int(x,16)for x in d['stack']];du=d.get('unit',c.get('unit',0xffffffff))
   if de==0x1e13c0: packet=[dr['edx'],ds[0]];eax=0;has=0
   else:
    packet={0x1d0a60:ds[:5],0x114f00:[],0x1cfa90:[],0xbb240:ds[:1],0x114bd0:ds[:1]}[de]
    has=int(de in(0x1d0a60,0x114f00));eax=dr['eax']if has else 0
   lines.append('LEAF '+' '.join(f'{v:x}'for v in(de,du,eax,has,int(d['return_eax'],16)&255,len(packet),*packet)))
   for name,v in d['before'].items():lines.append('LBEFORE '+name+' '+v)
   for name,v in d['after'].items():lines.append('LAFTER '+name+' '+v)
   lines.append('ENDLEAF')
  implemented_calls={v['id']for v in[c]+descendants if int(v['entry'],16)in IMPLEMENTED and not any(int(calls[q]['entry'],16)in LOWER for q in v['ancestors'][len(c['ancestors'])+1:])}
  ips=set().union(*(addresses[int(calls[q]['entry'],16)]for q in implemented_calls))
  for st in r['stores']:
   if c['id']not in st['calls']or int(st['instruction'],16)not in ips:continue
   if any(q not in implemented_calls and q!=c['id'] and int(calls[q]['entry'],16)in LOWER for q in st['calls'][len(c['ancestors'])+1:]):continue
   label=st['labels'].get(str(c['id']))
   if label and label['buffer']in('U','P'):lines.append(f'STORE {st["instruction"]} {label["buffer"]} {label["offset"]:x} {st["size"]:x} {int.from_bytes(bytes.fromhex(st["before"]),"little"):x} {int.from_bytes(bytes.fromhex(st["written"]),"little"):x}')
  lines.append('ENDCASE');count+=1
 save(a.output,('\n'.join(lines)+'\n').encode());print(json.dumps(dict(cases=count,fixture=str(a.output),sha256=sha(a.output))))
if __name__=='__main__':main()
