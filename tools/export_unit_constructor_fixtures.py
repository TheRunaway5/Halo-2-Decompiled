#!/usr/bin/env python3
"""Export literal complete original constructor buffers and ordered writes.

Nested graph/reset/variant/heap outputs are explicit differential services.
This exporter generates neither a successful allocator nor gameplay defaults.
"""
import argparse, hashlib, json
from pathlib import Path

OWNERS={0x1ce8c0,0x1ce9b0,0x1ceac0,0xbd270,0xc52c0}
NESTED={0x1d0a60,0x18fd70,0x110a20,0x116680,0x110bf0}

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
 r=json.loads(a.report.read_text());lines=['# literal-source-unit-constructors-v1', '# original-report-sha256 '+hashlib.sha256(a.report.read_bytes()).hexdigest()]
 calls=r['calls'];count=0
 for c in calls:
  e=int(c['entry'],16)
  if e not in OWNERS or 'after'not in c:continue
  descendants=[d for d in calls if c['id'] in d['ancestors']]
  # Only boundaries not implemented by the complete native direct owners.
  leaves=[d for d in descendants if int(d['entry'],16) in NESTED and
          not any(int(calls[q]['entry'],16) in NESTED for q in d['ancestors'][len(c['ancestors'])+1:])]
  regs=c['registers'];stack=[int(s,16)for s in c['stack']]
  unit=c.get('unit',0xffffffff);graph=regs['edi']if e==0x1ceac0 else 0xffffffff;model=regs['edx']if e==0x1ceac0 else 0xffffffff
  params=[e,unit,graph,model,stack[0]&255 if e==0x1ceac0 else 0,c.get('component_field',0)&65535,c.get('component_size',0)&65535,c.get('alignment',0),c['pool']['capacity'],int(r['one_bits'],16),int(c['return_eax'],16)&255]
  lines.append('CASE '+str(c['id'])+' '+' '.join(f'{v:x}'for v in params))
  for k,v in c['before'].items():lines.append('BEFORE '+k+' '+v)
  for k,v in c['after'].items():lines.append('AFTER '+k+' '+v)
  meta=c['metadata']
  for k in ('graph','model'):
   if k in meta:lines.append('NODES '+k+' '+meta[k]['datum']+' '+f"{meta[k]['nodes']&0xffffffff:x}")
  if 'unit_definition' in meta:lines.append('DEF '+meta['unit_definition']['datum']+' '+meta['unit_definition']['raw1b8_hex'])
  if 'model_regions' in meta:lines.append('REGIONS '+meta['model_regions']['datum']+' '+(''.join(meta['model_regions']['record5c_hex'])or '-'))
  if 'game_type' in meta:lines.append('GAME '+f"{meta['game_type']:x}")
  for d in leaves:
   de=int(d['entry'],16);dr=d['registers'];ds=[int(x,16)for x in d['stack']]
   n={0x1d0a60:5,0x18fd70:3,0x110a20:1,0x116680:1,0x110bf0:7}[de]
   has_eax=de in(0x1d0a60,0x18fd70,0x116680,0x110bf0)
   lines.append('LEAF '+f'{de:x} {int(has_eax):x} {dr["eax"]if has_eax else 0:x} {int(d["return_eax"],16)&255:x} '+str(n)+' '+' '.join(f'{x:x}'for x in ds[:n]))
   for k,v in d['before'].items():lines.append('LBEFORE '+k+' '+v)
   for k,v in d['after'].items():lines.append('LAFTER '+k+' '+v)
   if 'growth_backing_after_hex'in d:lines.append('BACKING '+d['growth_backing_after_hex'])
   lines.append('ENDLEAF')
  implemented={e}
  if e==0x1ce8c0:implemented.add(0x1ce9b0)
  if e==0xc52c0:implemented.add(0xbd270)
  addresses={int(row.split()[0],16)for ent in implemented for row in(a.report.parent/'instructions'/f'{ent:08x}.asm').read_text().splitlines()}
  for s in r['stores']:
   if c['id']not in s['calls']or int(s['instruction'],16)not in addresses:continue
   label=s['labels'].get(str(c['id']))
   if label:lines.append(f'STORE {s["instruction"]} {label["buffer"]} {label["offset"]:x} {s["size"]:x} {int.from_bytes(bytes.fromhex(s["before"]),"little"):x} {int.from_bytes(bytes.fromhex(s["written"]),"little"):x}')
  lines.append('ENDCASE');count+=1
 raw=('\n'.join(lines)+'\n').encode();a.output.parent.mkdir(parents=True,exist_ok=True)
 if a.output.exists():assert a.output.read_bytes()==raw,'Immutable fixture differs'
 else:a.output.write_bytes(raw)
 print(json.dumps(dict(cases=count,fixture=str(a.output),sha256=hashlib.sha256(raw).hexdigest())))
if __name__=='__main__':main()
