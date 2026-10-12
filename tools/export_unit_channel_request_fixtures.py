#!/usr/bin/env python3
"""Literal complete channel request/install buffers and actual provider inputs."""
import argparse,json
from pathlib import Path
from oracle_mp_unit_constructors_1_1 import sha,save
IMPLEMENTED={0x1cec20,0x1cee00,0x1c9e60};LOWER={0x1ceda0,0x1c9f50,0x1df670}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--source',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();r=json.loads((a.source/'owning/report.json').read_text());x=json.loads((a.source/'extension.json').read_text());calls=r['calls']
 lines=['# literal-unit-channel-request-v1','# original-report-sha256 '+sha(a.source/'owning/report.json')]
 for d,t in sorted(x['literal_tags'].items()):
  if t['group']=='jmad':lines.append('TAG '+d+' '+t['address']+' '+t['raw_hex'])
 for key,d in sorted(x['source_definitions'].items()):lines.append('DEF '+key.replace(':',' ')+' '+' '.join(d[n]or'-'for n in('raw','timing','sounds','effects','sound_refs','effect_refs')))
 # Immutable executable scalar words, observed from the mapped original image.
 from oracle_mp_title_update_display import Xbe,OLD
 image=Xbe(OLD);lines.append('CONSTANTS '+' '.join(image.read(p,4)[::-1].hex()for p in(0x465030,0x465048,0x46504c,0x4650c0,0x465028)))
 addresses={e:{int(q.split()[0],16)for q in(a.source/'owning/instructions'/f'{e:08x}.asm').read_text().splitlines()}for e in IMPLEMENTED}
 count=0
 for c in calls:
  e=int(c['entry'],16)
  if e not in(0x1cec20,0x1cee00)or'after'not in c:continue
  reg=c['registers'];st=[int(q,16)for q in c['stack']]
  args=[reg['ecx'],reg['edx']]if e==0x1cec20 else[st[1],st[2],reg['ebx'],*st[3:6]]
  lines.append('CASE '+str(c['id'])+' '+' '.join(f'{v:x}'for v in(e,int(c['return_eax'],16)&255,len(args),*args)))
  clock=c['clock_before'];lines.append('CLOCK '+clock['raw_hex']+' '+clock['fpcw']+' '+clock['mxcsr'])
  located=set()
  if'U'in c['before']:
   base=int.from_bytes(bytes.fromhex(c['before']['H'])[8:12],'little')
   for k,address,size in [('M',c['manager'],0x90),('C',c['channel'],32)]:
    if base<=address and address+size<=base+len(bytes.fromhex(c['before']['U'])):lines.append(f'LOCATION {k}_U {address-base:x}');located.add(k)
   if 'C' not in located and c['manager']<=c['channel']and c['channel']+32<=c['manager']+0x90:lines.append(f'LOCATION C_M {c["channel"]-c["manager"]:x}')
  elif c['manager']<=c['channel']and c['channel']+32<=c['manager']+0x90:lines.append(f'LOCATION C_M {c["channel"]-c["manager"]:x}')
  for prefix,states in(('BEFORE',c['before']),('AFTER',c['after'])):
   for k,v in states.items():
    if k in('U','M','C','R','G'):lines.append(prefix+' '+k+' '+v)
  descendants=[q for q in calls if c['id']in q['ancestors']]
  events=[]
  for q in descendants:
   if int(q['entry'],16)==0x124e40:
    payload=f'{q["registers"]["eax"]:x} '+f'{q["registers"]["ecx"]&255:x}'
    lines.append('RESOURCE '+payload);events.append((q['order'],'EVENT RESOURCE '+payload))
  for q in descendants:
   qe=int(q['entry'],16)
   if qe not in LOWER:continue
   if any(int(calls[z]['entry'],16)in LOWER for z in q['ancestors'][len(c['ancestors'])+1:]):continue
   events.extend([(q['order'],f'EVENT ENTER {qe:x}'),(q['return_order'],f'EVENT EXIT {qe:x}')])
   regs=q['registers'];qs=[int(v,16)for v in q['stack']]
   p=[qs[0]]if qe in(0x1ceda0,0x1c9f50)else[qs[0],regs['eax'],*qs[2:5]]
   lines.append('LOWER '+f'{qe:x} '+str(len(p))+' '+' '.join(f'{v:x}'for v in p))
   for prefix,states in(('LBEFORE',q['before']),('LAFTER',q['after'])):
    for k,v in states.items():
     if k in('U','M','C','R','G'):lines.append(prefix+' '+k+' '+v)
   # Actual variant output is observed by its independent immutable observer.
   if qe==0x1ceda0:
    # Ctor's packed stack output is retained in the lower's actual read/write
    # observer, independent of display string comparisons.
    writes=[z for z in r['stores']if q['id']in z['calls']and z['instruction']in('001cede4','001cedeb')and int(z['address'],16)==regs['esi']]
    assert len(writes)==1;lines.append('RESULT '+writes[0]['written'])
   lines.append('ENDLOWER')
  ids={q['id']for q in[c]+descendants if int(q['entry'],16)in IMPLEMENTED};ips=set().union(*(addresses[int(calls[q]['entry'],16)]for q in ids))
  for q in r['stores']:
   label=q['labels'].get(str(c['id']))
   if c['id']in q['calls']and int(q['instruction'],16)in ips and label and label['buffer']=='C':
    payload=f'{q["instruction"]} {label["offset"]:x} {q["size"]:x} {int.from_bytes(bytes.fromhex(q["before"]),"little"):x} {int.from_bytes(bytes.fromhex(q["written"]),"little"):x}'
    lines.append('STORE '+payload);events.append((q['order'],'EVENT STORE '+payload))
   if c['id']in q['calls']and q['instruction']in('001e10d1','001e10ef')and label and label['buffer']=='R':
    payload=f'{q["instruction"]} {label["offset"]//4:x} {int.from_bytes(bytes.fromhex(q["before"]),"little"):x} {int.from_bytes(bytes.fromhex(q["written"]),"little"):x}'
    events.append((q['order'],'EVENT RNG '+payload))
  lines.extend(v for _,v in sorted(events))
  lines.append('ENDCASE');count+=1
 save(a.output,('\n'.join(lines)+'\n').encode());print(json.dumps(dict(cases=count,sha256=sha(a.output))))
if __name__=='__main__':main()
