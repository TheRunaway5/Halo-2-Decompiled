#!/usr/bin/env python3
"""Literal unchanged-source tree/selection packets and explicit leaf services."""
import argparse,hashlib,json,struct
from pathlib import Path

def words(raw):return struct.unpack('<'+'I'*(len(raw)//8),bytes.fromhex(raw))
def nums(values):return' '.join(f'{x:x}'for x in values)
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,'Immutable fixture differs'
 else:p.write_bytes(b)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('report',type=Path);p.add_argument('output',type=Path);a=p.parse_args();r=json.loads(a.report.read_text())
 lines=['# literal-animation-selection-v1','# original-report-sha256 '+hashlib.sha256(a.report.read_bytes()).hexdigest()]
 for g in r['graphs']:
  lines.append('GRAPH '+g['datum']+' '+g['pointer']);lines.append('MEM '+g['pointer']+' '+g['headerc4_hex'])
 for b in r['banks'].values():
  lines.append('MEM '+b['pointer']+' '+b['header8_hex'])
  if b['records_hex']:lines.append('MEM '+f'{words(b["header8_hex"])[1]:08x}'+' '+b['records_hex'])
 graphs={int(g['pointer'],16):int(g['datum'],16)for g in r['graphs']};defs={}
 for c in r['calls']:
  e=int(c['entry'],16);reg=c['registers'];s=[int(x,16)for x in c['stack']];ret=int(c['return_eax'],16)
  if e==0x1de890:
   d=c['definition_return'];key=(graphs[reg['edx']],s[0]);val=(d['frame_count']&65535,d['authored_flags'])
   if key in defs:assert defs[key]==val
   defs[key]=val
  elif e==0x1e0f50:lines.append('LOOKUP '+nums(s[:3]+[ret]))
  elif e==0x1cfce0:lines.append('NORMAL '+c['before_manager_hex']+' '+nums([reg['eax'],reg['ecx'],reg['edx'],s[1]])+' '+c['outputs_after']['tuple'])
  elif e==0x29c6a0:
   out=c['outputs_before'];after=c['outputs_after']
   lines.append('NEXT '+out['state']+' '+''.join(out[k]for k in('mode','weapon_class','weapon'))+' '+after['state']+' '+''.join(after[k]for k in('mode','weapon_class','weapon'))+' '+f'{ret:x}')
  elif e==0x1deb10:lines.append('TREE '+nums([reg['ecx'],reg['eax'],s[0],s[1],ret]))
 for(k,v)in sorted(defs.items()):lines.append('DEF '+nums(list(k)+list(v)))
 implemented=(0x1d0a60,0x1d0590,0x1c9e60,0x1cee00)
 addresses={int(row.split()[0],16)for ent in implemented for row in(a.report.parent/'instructions'/f'{ent:08x}.asm').read_text().splitlines()}
 cases=leaves=stores=0
 for c in r['calls']:
  e=int(c['entry'],16)
  if e not in(0x1d0a60,0x1d0590,0x1c9e60):continue
  reg=c['registers'];s=[int(x,16)for x in c['stack']];m=c['manager']
  if e==0x1d0a60:args=[s[0],s[1],s[2],reg['eax'],s[3],s[4]]
  elif e==0x1c9e60:args=[reg['eax']-m,reg['ecx'],reg['edx']]+s[:5]
  else:args=[]
  lines.append('CASE '+nums([c['id'],e,int(c['return_eax'],16)&255,len(args)]+args));lines.append('BEFORE '+c['before_manager_hex']);lines.append('AFTER '+c['after_manager_hex'])
  if e==0x1d0a60:
   for d in r['calls']:
    de=int(d['entry'],16)
    if c['id']not in d['ancestors']or de not in(0x1cfde0,0x1d0790,0x1d06c0,0x1ceda0,0x1c9f50,0x1d1a00,0x1d1b70):continue
    # Skip any nested service's descendants; record complete owning boundary.
    if any(int(r['calls'][q]['entry'],16)in(0x1cfde0,0x1d0790,0x1d06c0,0x1ceda0,0x1c9f50,0x1d1a00,0x1d1b70)for q in d['ancestors'][1:]):continue
    dr=d['registers'];ds=[int(x,16)for x in d['stack']];out=d['outputs_after'];off=0;outputs=[]
    if de==0x1cfde0:args=ds[:5];outputs=list(words(out['requested']))+list(words(out['resolved']))+list(words(out['packed']))+[int(d['return_eax'],16)&255]
    elif de==0x1d0790:args=ds[1:3];outputs=list(words(out['packed']))+[int(out['eligible'],16)]
    elif de==0x1d06c0:args=ds[1:4]+[dr['ecx']];outputs=list(words(out['packed']))+list(words(out['activity']))
    elif de==0x1ceda0:
     off=0x40 if int(d['caller'],16)==0x1d0d00 else 0;args=ds[:1];outputs=list(words(out['packed']))
    elif de==0x1c9f50:off=dr['eax']-m;args=ds[:1]
    elif de==0x1d1b70:args=[dr['eax'],dr['ecx'],dr['edx'],ds[1],ds[2]]
    else:args=[]
    lines.append('LEAF '+nums([de,off,len(args)]+args+[len(outputs)]+outputs));lines.append('LBEFORE '+d['before_manager_hex']);lines.append('LAFTER '+d['after_manager_hex']);lines.append('ENDLEAF');leaves+=1
  for st in r['stores']:
   if c['id']not in st['calls']or int(st['instruction'],16)not in addresses:continue
   label=st['manager_offsets'].get(str(c['id']))
   if label is not None:lines.append('STORE '+nums([int(st['instruction'],16),label,st['size'],int.from_bytes(bytes.fromhex(st['before']),'little'),int.from_bytes(bytes.fromhex(st['written']),'little')]));stores+=1
  lines.append('ENDCASE');cases+=1
 for name in('00465030','00465048','0046504c'):lines.append('CONSTANT '+name+' '+r['constant_words'][name])
 raw=('\n'.join(lines)+'\n').encode();save(a.output,raw)
 print(json.dumps(dict(cases=cases,explicit_leaf_boundaries=leaves,ordered_stores=stores,definitions=len(defs),fixture_sha256=hashlib.sha256(raw).hexdigest())))
if __name__=='__main__':main()
