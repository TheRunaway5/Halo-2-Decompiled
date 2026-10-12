#!/usr/bin/env python3
"""Export complete original transition buffers/stores; imported bytes stay external."""
import argparse,json
from pathlib import Path
from oracle_mp_unit_constructors_1_1 import sha,save
OWNERS={0xbb240,0x1cefe0,0x1dce20,0x1dcc30,0xb8250}
CASES=OWNERS-{0xb8250}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads(a.report.read_text());tags=json.loads((a.report.parent.parent/'literal-tags.json').read_text());calls=r['calls']
 lines=['# literal-unit-local-transition-v1','# report-sha256 '+sha(a.report)]
 for d,t in sorted(tags['tags'].items()):lines.append(f'TAG {d} {t["address"]} {t["raw_hex"]}')
 words={0x465030:None,0x465048:None}
 for q in r['reads']:
  at=int(q['address'],16)
  if at in words:words[at]=int.from_bytes(bytes.fromhex(q['raw_hex']),'little')
 assert all(v is not None for v in words.values())
 lines.append('CONSTANTS '+' '.join(f'{v:x}'for v in words.values()))
 counts={}
 for c in calls:
  e=int(c['entry'],16)
  if e not in CASES or'after'not in c:continue
  counts[e]=counts.get(e,0)+1;regs=c['registers'];st=[int(x,16)for x in c['stack']]
  args={0xbb240:[st[0]],0x1cefe0:[regs['eax'],st[1]],0x1dce20:[],0x1dcc30:[st[0],regs['ebx']&255]}[e]
  result=int(c['return_cpu']['xmm128'][0],16)&0xffffffff if e==0x1dce20 else int(c['return_eax'],16)&255 if e==0x1cefe0 else 0
  lines.append('CASE '+str(c['id'])+' '+' '.join(f'{v:x}'for v in(e,c.get('unit',0xffffffff),c['pool']['capacity'],result,len(args),*args)))
  clock=c['clock_before'];assert c['clock_after']==clock
  lines.append(f'CLOCK {clock["raw0c_hex"]} {clock["fpcw"]} {clock["mxcsr"]}')
  if'unit'in c:
   base=int.from_bytes(bytes.fromhex(c['before']['H'])[8:12],'little')
   for name,key in(('M','manager'),('S','saved_locals'),('L','current_locals'),('C','counter')):
    if key in c:lines.append(f'LOCATION {name} {c[key]-base:x}')
  for name,v in c['before'].items():lines.append('BEFORE '+name+' '+v)
  for name,v in c['after'].items():lines.append('AFTER '+name+' '+v)
  if e==0xbb240:
   for obj in c['objects_before']:lines.append('OBJECTBEFORE '+obj['datum']+' '+obj['body_hex'])
   for obj in c['objects_after']:lines.append('OBJECTAFTER '+obj['datum']+' '+obj['body_hex'])
  children=[v for v in calls if c['id']in v['ancestors']]
  selected={v['id']for v in[c]+children if int(v['entry'],16)in OWNERS}
  clock_count=sum(q['instruction']in('001dcc31','001dce33','001dce4b')and any(i in selected for i in q['calls'])for q in r['reads'])
  lines.append(f'CLOCKCOUNT {clock_count:x}')
  for q in r['stores']:
   if c['id']not in q['calls']:continue
   label=q['labels'].get(str(c['id']))
   if label and label['buffer']in('POOL','U','M','S','C'):
    lines.append(f'STORE {q["instruction"]} {label["buffer"]} {label["offset"]:x} {q["size"]:x} {int.from_bytes(bytes.fromhex(q["before"]),"little"):x} {int.from_bytes(bytes.fromhex(q["written"]),"little"):x}')
  lines.append('ENDCASE')
 save(a.output,('\n'.join(lines)+'\n').encode());print(json.dumps(dict(cases=sum(counts.values()),owners={f'{e:08x}':n for e,n in counts.items()},fixture=str(a.output),sha256=sha(a.output))))
if __name__=='__main__':main()
