#!/usr/bin/env python3
"""Freeze unchanged constructor shield initialization and BCC20 model input.

D5B60 reads the actual bipd -> hlmt first damage-info maximum. The original
B7B40 initializer is executed from B81DF through B8235, before its next call.
Only supplied metadata pointers are relocated; no source instructions change.
"""
from pathlib import Path
import json, struct, sys
sys.path.insert(0, str(Path(__file__).resolve().parent))
from oracle_multiplayer_hud_positions import Retail, PIN
from audit_rendering import Cache, file_sha
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI
ROOT=Path(__file__).resolve().parents[1]
D=ROOT/'analysis/frontend/customization'
c=Cache(ROOT/'assets/disc/maps/mainmenu.map');r=Retail()
objects=r.alloc(bytes(0x50));table=r.alloc(bytes(12));obj=r.alloc(bytes(0x340))
r.w(objects+0x44,table);r.w(table+8,obj);r.w(0x4e0300,objects)
out=r.alloc(bytes(4));enabled=r.alloc(bytes(4))
rows=[];metadata=[]
live=ROOT/'analysis/frontend/profile-regression/original-model-customization-elite-camouflage.json'
snapshot=json.loads(live.read_text())
for record in snapshot['objects']:
    unit=int(record['definition'],16);_,at=c.tag(unit,'bipd')
    datum=c.u32(at+0x38);_,model=c.tag(datum,'hlmt')
    damage=c.block(model+0x60,0xf8)[0]
    definition=r.alloc(c.data[at:at+0x3c]);definition_model=r.alloc(c.data[model:model+0x68])
    info=r.alloc(c.data[damage:damage+0x90]);r.w(definition_model+0x64,info)
    r.w(r.table+(unit&65535)*16+8,definition)
    r.w(r.table+(datum&65535)*16+8,definition_model);r.w(obj,unit)
    selected=r.run(0xd5b60,registers=[(UC_X86_REG_ECX,0)])
    assert selected==info
    r.run(0xb81df,stop=0xb8235,registers=[(UC_X86_REG_EAX,selected),(UC_X86_REG_EDI,obj)])
    initialized=r.r(obj+0xf0)
    actual=struct.unpack_from('<I',bytes.fromhex(record['object_header_hex']),0xf0)[0]
    assert initialized==actual
    r.run(0xbcc20,argv=[0,0x0f000559,out,enabled])
    assert r.r(out)==initialized
    rows.append(f'0 {unit:08x} {initialized:08x}')
    metadata.append(dict(handle=record['handle'],unit=f'{unit:08x}',model=f'{datum:08x}',
        maximum_shield=f'{c.u32(damage+0x8c):08x}',initial_fraction=f'{initialized:08x}',
        live_fraction=f'{actual:08x}'))
# Includes both zero signs, tiny positive/negative, infinities and NaNs. The
# COMISS/JBE admission initializes one only for an ordered positive maximum.
maxima=[0,0x80000000,1,0x80000001,0x3f000000,0x3f800000,0x428c0000,
        0xbf800000,0x7f800000,0xff800000,0x7fc00000,0xffc00000]
for maximum in maxima:
    r.w(info+0x8c,maximum)
    r.run(0xb81df,stop=0xb8235,registers=[(UC_X86_REG_EAX,info),(UC_X86_REG_EDI,obj)])
    rows.append(f'1 {maximum:08x} {r.r(obj+0xf0):08x}')
for fraction in [0,0x80000000,0xbf800000,0x3e800000,0x3f000000,0x3f800000,0x40000000]:
    r.w(obj+0xf0,fraction);r.run(0xbcc20,argv=[0,0x0f000559,out,enabled])
    rows.append(f'2 {fraction:08x} {r.r(out):08x}')
path=D/'frontend-model-shield-original.tsv'
path.write_text('# B7B40/D5B60/BCC20 unchanged pinned XBE '+PIN+'\n'
    '# kind (0:unit,1:maximum,2:fraction) input_word result_word\n'+'\n'.join(rows)+'\n')
original=json.loads((D/'original-preview-shader-banks-elite.json').read_text())
# Source header+18 identifies template pass18, not another pass using the same
# shader-pass postprocess. This is the actual captured layer7 emitter.
_,shader=c.tag(0xe325017a,'shad');sb=c.block(shader+32,124)[0]
_,template=c.tag(c.u32(sb),'stem');tb=c.block(template+88,40)[0]
pass18=c.block(tb+16,10)[18]+c.magic
match=[row for row in original['rows'] if row['shader']=='e325017a' and
       struct.unpack_from('<I',bytes.fromhex(row['state_header_hex']),0x18)[0]==pass18]
assert len(match)==1
program=bytes.fromhex(match[0]['pixel_program_hex']);assert len(program)==240
(D/'frontend-model-elite-arms-emissive-live-program.bin').write_bytes(program)
report=dict(method=__doc__,xbe_sha256=PIN,mainmenu_sha256=file_sha(c.path),
    source_initializer=['000D5B60','000B81DF','000B8235'],input_routine='000BCC20',
    live_fixture=str(live.relative_to(ROOT)),objects=metadata,boundary_cases=len(maxima),
    input_cases=7,program_fixture='original-preview-shader-banks-elite.json',
    emitted_shader='e325017a',template_pass=18,
    limitations='Source bank and initial menu state proof; final native framebuffer comparison remains separate.')
(D/'frontend-model-shield-original.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'{len(metadata)} source constructor/live fractions, {len(maxima)} maxima and7 BCC20 inputs retained')
