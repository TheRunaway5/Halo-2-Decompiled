#!/usr/bin/env python3
"""Literal complete1DF670/1CFA90 buffers, output stores and resource order."""
import argparse,json
from pathlib import Path
from oracle_mp_unit_constructors_1_1 import sha,save

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--source',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();r=json.loads((a.source/'owning/report.json').read_text());x=json.loads((a.source/'extension.json').read_text());calls=r['calls'];lines=['# literal-original-alternate-request-v1','# original-report-sha256 '+sha(a.source/'owning/report.json')];count=0;resources=0;stores=0
 for d,t in sorted(x['literal_tags'].items()):
  if t['group']=='jmad':lines.append('TAG '+d+' '+t['address']+' '+t['raw_hex'])
 ips={0x1df732,0x1df741,0x1df74f,0x1e014e,0x1e0162,0x1e0170}
 for c in calls:
  e=int(c['entry'],16)
  if e not in(0x1df670,0x1cfa90):continue
  assert'after'in c;reg=c['registers'];st=[int(q,16)for q in c['stack']]
  args=[st[0],*st[2:5],reg['eax']]if e==0x1df670 else[]
  if e==0x1df670:assert int(c['return_eax'],16)==st[1]
  lines.append(f'CASE {c["id"]} {e:x} '+f'{int(c["return_eax"],16)&255 if e==0x1cfa90 else 0:x} {len(args):x} '+' '.join(f'{q:x}'for q in args))
  if e==0x1cfa90 and'U'in c['before']:
   b=int.from_bytes(bytes.fromhex(c['before']['H'])[8:12],'little');off=c['manager']-b
   if 0<=off and off+0x90<=len(bytes.fromhex(c['before']['U'])):lines.append(f'LOCATION M_U {off:x}')
  for prefix,states in [('BEFORE',c['before']),('AFTER',c['after'])]:
   for k,v in states.items():
    if k in('P','O0','O1','O2','U','H','M','R','G'):lines.append(prefix+' '+k+' '+v)
  events=[];desc=[q for q in calls if c['id']in q['ancestors']]
  for q in desc:
   if q['entry']=='00124e40':
    payload=f'{q["registers"]["eax"]:x} {q["registers"]["ecx"]&255:x}';lines.append('RESOURCE '+payload);events.append((q['order'],'EVENT RESOURCE '+payload));resources+=1
   if e==0x1cfa90 and q['entry']=='001df670':
    qs=[int(z,16)for z in q['stack']];a2=[qs[0],*qs[2:5],q['registers']['eax']];site=int(q['caller'],16)-5;head=f'{site:x} '+' '.join(f'{z:x}'for z in a2)
    events.extend([(q['order'],'EVENT ENTER '+head),(q['return_order'],'EVENT EXIT '+head+' '+q['after']['P'])])
  if e==0x1df670:
   outputs={st[1]:('P',0)}|{v:(f'O{i}',i*4)for i,v in enumerate(st[5:8])if v}
   for q in r['stores']:
    if c['id']not in q['calls']or int(q['instruction'],16)not in ips:continue
    k,offset=outputs[int(q['address'],16)];before=int.from_bytes(bytes.fromhex(q['before']),'little');after=int.from_bytes(bytes.fromhex(q['written']),'little')
    payload=f'{q["instruction"]} {k} {offset:x} {q["size"]:x} {before:x} {after:x}'
    lines.append('STORE '+payload);events.append((q['order'],'EVENT STORE '+payload));stores+=1
  lines.extend(s for _,s in sorted(events));lines.append('ENDCASE');count+=1
 assert count==26;save(a.output,('\n'.join(lines)+'\n').encode());print(json.dumps(dict(cases=count,stores=stores,resources=resources,sha256=sha(a.output))))
if __name__=='__main__':main()
