#!/usr/bin/env python3
"""Full original pool/body and ordered114F00->1CEC20 calls; no fields filled."""
import argparse,json,struct
from pathlib import Path
from oracle_mp_unit_constructors_1_1 import sha,save

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=json.loads((a.source/'owning/report.json').read_text());x=json.loads((a.source/'extension.json').read_text());calls=r['calls'];lines=['# literal-original-unit-subchannel-caller-v1','# source-report-sha256 '+sha(a.source/'owning/report.json')]
 for d,t in sorted(x['literal_tags'].items()):
  if t['group']=='jmad':lines.append('TAG '+d+' '+t['address']+' '+t['raw_hex'])
 from oracle_mp_title_update_display import Xbe,OLD
 lines.append('ONE '+Xbe(OLD).read(0x465030,4)[::-1].hex());count=0
 for c in calls:
  if c['entry']!='00114f00':continue
  assert c['before']==c['after'];lines.append(f'CASE {c["id"]} {c["unit"]:x} {c["pool"]["capacity"]:x}')
  for kind,states in [('BEFORE',c['before']),('AFTER',c['after'])]:
   for k,v in states.items():
    if k in('POOL','H','U','R','G'):lines.append(kind+' '+k+' '+v)
  unit_base=int.from_bytes(bytes.fromhex(c['before']['H'])[8:12],'little')
  nested=[q for q in calls if c['id']in q['ancestors']and q['entry']=='001cec20'];assert len(nested)==3
  for q in nested:
   m=q['manager']-unit_base;ch=q['channel']-unit_base;reg=q['registers'];returned=int(q['return_eax'],16)&255
   assert q['before']==q['after']and returned==0
   payload=f'{q["caller"]} {c["unit"]:x} {m:x} {ch:x} {reg["ecx"]:x} {reg["edx"]:x}'
   # Caller address is the return site; instruction call is return-site minus5.
   payload=f'{int(q["caller"],16)-5:x}'+payload[8:]
   lines.append('CALL '+payload+' 0 0');lines.append('CALL '+payload+f' 1 {returned:x}')
  lines.append('ENDCASE');count+=1
 assert count==2;save(a.output,('\n'.join(lines)+'\n').encode());print(json.dumps(dict(cases=count,sha256=sha(a.output))))
if __name__=='__main__':main()
