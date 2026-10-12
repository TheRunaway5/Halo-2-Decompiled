#!/usr/bin/env python3
"""Freeze original20 executed shadow inputs/banks/DebugVertex outputs.

The authored payload is selected by the captured helper opcode histogram and
literal input declaration; class2/3 share identical authored bytes. This is
not a source caster/receiver admission or live native stream producer.
"""
from pathlib import Path
import json,re,struct,hashlib,collections
ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'analysis/multiplayer-shading/original-xemu/shadow-gpu-audit'
OUT=ROOT/'tests/fixtures'
records=json.loads((SRC/'original20-executed-shadow-draws.json').read_text())['records']
authored=json.loads((SRC/'authored-shadow-assets-21.json').read_text())['maps'][11]['programs']
CLASS={7713:2,7721:2,7729:1,7737:4,7742:4,7750:5,7758:1,7988:0,7993:0,7998:0,8003:0}
MAC=['','MOV','MUL','ADD','MAD','DP3','DPH','DP4','DST','MIN','MAX','SLT','SGE','ARL']
ILU=['','MOV','RCP','RCC','RSQ','EXP','LOG','LIT']
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def wordline(values):return ' '.join(f'{v:08X}'for v in values)
lines=['# Original20 shadow source banks/attributes/DebugVertex outputs; xemu conversion retained separately.']
proof=[];samples=0
for rec in records:
 event=rec['event'];phase=0 if event<7900 else 1;ordinal=CLASS[event]
 variant=next(v for v in authored[phase]['variants']if v['class_index']==ordinal)
 code=(SRC/variant['file']).read_bytes();assert hashlib.sha256(code).hexdigest()==variant['sha256']
 name=f'retail_shadow_{"apply" if phase else "generate"}_class{ordinal}.vp';(OUT/name).write_bytes(code)
 asm=(SRC/f'original20-{event}-0.spvasm').read_text();main=asm[asm.index('  *oPos ='):asm.index('float4 decompress')if 'float4 decompress' in asm else asm.index('float clampAway')]
 captured=collections.Counter(re.findall(r'= _(MOV|MUL|ADD|MAD|DP3|DPH|DP4|RCC|RSQ|RCP)\(',main));source=collections.Counter()
 for offset in range(4,len(code),16):
  t=struct.unpack_from('<4I',code,offset);m=t[1]>>21&15;i=t[1]>>25&7
  if m and MAC[m]!='ARL':source[MAC[m]]+=1
  if i:source[ILU[i]]+=1
 assert source==captured,(event,source,captured)
 uniform=(SRC/f'original20-{event}-0-uniform0.bin').read_bytes();constants=struct.unpack_from('<768I',uniform)
 lines.append(f'B {event} {phase} {ordinal} '+wordline(struct.unpack_from('<2I',uniform,5336))+f' {struct.unpack_from("<I",uniform,3076)[0]:08X}')
 for c in range(192):lines.append(f'C {event} {c} '+wordline(constants[c*4:c*4+4]))
 inputs=re.findall(r'Input (?:float4|int)\* v(\d+)(_cmp)? :',asm)
 inline=struct.unpack('<'+'I'*((SRC/f'original20-{event}-0-uniform1.bin').stat().st_size//4),(SRC/f'original20-{event}-0-uniform1.bin').read_bytes())
 count=0
 for d in rec['debug']:
  assert d['valid'];a=[[0,0,0,0x3f800000]for _ in range(16)]
  assert len(inputs)==len(d['inputs'])
  for (v,packed),x in zip(inputs,d['inputs']):
   if packed:
    w=int(x['words'][0],16);out=[]
    for shift,n in ((0,11),(11,11),(22,10)):
     signed=(w>>shift)&((1<<n)-1)
     if signed>>(n-1):signed-=1<<n
     out.append(bits(signed/((1<<(n-1))-1)))
    a[int(v)]=[*out,0x3f800000]
   else:a[int(v)]=[int(w,16)for w in x['words']]
  for index,v in re.findall(r'PushConstants_var.inlineValue\[(\d+)\];[^;]+;\s+\*v(\d+) =',asm):a[int(v)]=list(inline[int(index)*4:int(index)*4+4])
  # Initial private output registers retain this order in each exported trace.
  ff=list(d['final'].values());expected=[int(w,16)for i in (0,1,2,7,8,9,10)for w in ff[i]['words']]
  lines.append(f'V {event} {d["index"]} '+wordline(sum(a,[])+expected));samples+=1;count+=1
 proof.append(dict(event=event,phase=phase,authored_class=ordinal,program=name,sha256=variant['sha256'],opcode_histogram=dict(source),samples=count))
(OUT/'retail_shadow_vertex.tsv').write_text('\n'.join(lines)+'\n')
result=dict(draws=len(proof),vertices=samples,records=proof,scope=__doc__)
(SRC/'original20-shadow-vertex-fixture-proof.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({'draws':len(proof),'vertices':samples,'compiled_payloads':len(set(x['sha256']for x in proof))}))
