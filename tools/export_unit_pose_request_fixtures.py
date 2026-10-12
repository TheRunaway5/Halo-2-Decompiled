#!/usr/bin/env python3
"""Export complete owning pose buffers/output stores/call and resource order."""
import argparse,json
from pathlib import Path
from oracle_mp_unit_constructors_1_1 import sha,save
OWN=(0x114bd0,0x1cf4c0,0x1cf3c0,0x1cf360,0x1cef10,0x1cfb90,0x115af0)
BODY=(0x114bd0,0x115af0)
NEW=OWN+(0x1df670,0x1cee00,0x1cfde0)
def args(c):
 e=int(c['entry'],16);r=c['registers'];s=[int(q,16)for q in c['stack']]
 if e==0x1cf4c0:return[r['ecx'],s[2],r['eax'],0]
 if e==0x1cf3c0:return[s[0],r['edx'],r['edi'],0]
 if e==0x1cf360:return[s[0],s[1],r['edx'],0]
 if e==0x1cef10:return[r['ecx'],r['edx'],r['edi'],s[1]]
 if e==0x1cfb90:return[r['edx'],0,0,0]
 if e==0x115af0:return[r['eax'],0,0,0]
 if e==0x114bd0:return[s[0],0,0,0]
 if e==0x1df670:return[*s[2:5],r['eax']]
 if e==0x1cee00:return[s[1],s[2],r['ebx'],s[5]]
 if e==0x1cfde0:return s[:4]
 raise AssertionError(hex(e))
def result(c):
 e=int(c['entry'],16)
 if e==0x115af0:return None
 if e in(0x1cef10,0x1cee00,0x1cfde0):return int(c['return_eax'],16)&255
 assert'P'in c['after'];assert int(c['return_eax'],16)==c['packed_output']
 return int.from_bytes(bytes.fromhex(c['after']['P']),'little')
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--source',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args();r=json.loads((a.source/'owning/report.json').read_text());x=json.loads((a.source/'extension.json').read_text());cs=r['calls'];lines=['# literal-source-unit-pose-requests-v1','# source-report-sha256 '+sha(a.source/'owning/report.json')];stores=resources=calls=0;cases=0
 for d,t in sorted(x['literal_tags'].items()):
  if t['group']=='jmad':lines.append('TAG '+d+' '+t['address']+' '+t['raw_hex'])
 lines.append('ONE '+r['one_bits'])
 for c in cs:
  e=int(c['entry'],16)
  if e not in OWN:continue
  value=result(c)if e not in BODY else None
  lines.append(f'CASE {c["id"]} {e:x} '+('-'if value is None else f'{value:x}')+' '+' '.join(f'{v:x}'for v in args(c)))
  p=c['pool'];lines.append(f'POOL {p["capacity"]:x} {p["slot_bytes"]:x} {c.get("unit",0):x}')
  if 'K'in c['before']:lines.append(f'LOCATION H_K {(c["unit"]&65535)*12:x}')
  for key,ptr,n in [('M',c.get('manager'),0x90),('C',c.get('channel'),32)]:
   if not ptr:continue
   if'U'in c['before']:
    b=int.from_bytes(bytes.fromhex(c['before']['H'])[8:12],'little');off=ptr-b
    if 0<=off and off+n<=len(bytes.fromhex(c['before']['U'])):lines.append(f'LOCATION {key}_U {off:x}')
   if key=='C'and'M'in c['before']and c.get('manager'):
    off=ptr-c['manager']
    if 0<=off and off+32<=0x90:lines.append(f'LOCATION C_M {off:x}')
  clock=c['clock_before'];lines.append('CLOCK '+clock['raw_hex']+' '+clock['fpcw']+' '+clock['mxcsr'])
  for kind,state in [('BEFORE',c['before']),('AFTER',c['after'])]:
   for k,v in state.items():
    if k in('U','H','K','M','C','P','R','G'):lines.append(kind+' '+k+' '+v)
  events=[]
  for q in cs:
   if c['id']not in q['ancestors']:continue
   qe=int(q['entry'],16)
   if qe==0x124e40:
    payload=f'{q["registers"]["eax"]:x} {q["registers"]["ecx"]&255:x}';events.append((q['order'],'EVENT RESOURCE '+payload));resources+=1
   elif qe in NEW:
    # Only the owning calls represented by the new module's observer. Ignore
    # descendant1CFDE0 under external1D0A60, and other source-owner internals.
    site=int(q['caller'],16)-5
    if site not in(0x114d5e,0x114d78,0x114d93,0x114db6,0x114dd8,0x114df6,0x114e14,0x114e4f,0x114e6a,0x114e7b,0x114e9d,0x1cf4fd,0x1cf520,0x1cf437,0x1cef62,0x1cef86,0x1cf396,0x1cfbd8,0x1cfc03,0x1cfc32,0x115b53,0x115b7f):continue
    head=f'{site:x} {qe:x} '+' '.join(f'{v:x}'for v in args(q));v=result(q);events.extend([(q['order'],'EVENT ENTER '+head),(q['return_order'],'EVENT EXIT '+head+' '+('-'if v is None else f'{v:x}'))]);calls+=2
  for q in r['stores']:
   label=q['labels'].get(str(c['id']))
   if not label:continue
   k=label['buffer'];ip=int(q['instruction'],16)
   if e in BODY:
    if k!='U':continue
   elif e==0x1cef10:
    if k!='C':continue
   elif k!='P':continue
   payload=f'{ip:x} {k} {label["offset"]:x} {q["size"]:x} '+f'{int.from_bytes(bytes.fromhex(q["before"]),"little"):x} {int.from_bytes(bytes.fromhex(q["written"]),"little"):x}'
   events.append((q['order'],'EVENT STORE '+payload));stores+=1
  lines.extend(v for _,v in sorted(events));lines.append('ENDCASE');cases+=1
 assert cases==63
 save(a.output,('\n'.join(lines)+'\n').encode());print(json.dumps(dict(cases=cases,stores=stores,resources=resources,call_boundaries=calls,sha256=sha(a.output))))
if __name__=='__main__':main()
