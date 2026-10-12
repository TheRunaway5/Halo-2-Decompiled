#!/usr/bin/env python3
"""Replay unchanged336F0/33670 on captured menu object/parent inputs.
Host pointer relocation only; no source service is replaced. This is an
object-context producer replay, not a live shader-draw capture claim.
"""
from pathlib import Path
import json,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from oracle_multiplayer_hud_positions import Retail,PIN
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_XMM0
ROOT=Path(__file__).resolve().parents[1];D=ROOT/'analysis/frontend/customization';capture=ROOT/'analysis/frontend/profile-regression/original-model-customization-elite-camouflage.json'
r=Retail();source=json.loads(capture.read_text());objects={}
for o in source['objects']:
 objects[int(o['handle'],16)]=bytes.fromhex(o['object_header_hex'])
 w=o['held_weapon'];objects[int(w['handle'],16)]=bytes.fromhex(w['object_header_hex'])
high=max(h&65535 for h in objects)+1;pool=r.alloc(bytes(0x48));table=r.alloc(bytes(high*12));r.w(pool+0x24,12);r.w(pool+0x38,high);r.w(pool+0x44,table);r.w(0x4e0300,pool);ctx=r.alloc(bytes(0x80));r.w(0x485a58,ctx)
for h,b in objects.items():
 p=r.alloc(b);header=struct.pack('<HBBII',h>>16,0,0 if len(b)>0x130 else 2,0,p);r.g.mem_write(table+(h&65535)*12,header)
rows=[]
for h,b in objects.items():
 r.w(ctx,h);r.run(0x336f0,registers=[(UC_X86_REG_EAX,37)]);result=r.g.reg_read(UC_X86_REG_XMM0)&0xffffffff
 rows.append(dict(handle=f'{h:08x}',parent=f'{struct.unpack_from("<I",b,0x14)[0]:08x}',flag_aa=b[0xaa],result_word=f'{result:08x}'))
assert len(rows)==20 and all(x['result_word']=='00000000'for x in rows)
report=dict(method=__doc__,xbe_sha256=PIN,fixture=str(capture.relative_to(ROOT)),source='336F0 index37 →33850→33670',rows=rows)
(D/'frontend-menu-weapon-extern37-original.json').write_text(json.dumps(report,indent=2)+'\n');print('20 captured menu contexts replay unchanged original extern37: all exact00000000')
