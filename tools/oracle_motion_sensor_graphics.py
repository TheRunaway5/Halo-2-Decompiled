#!/usr/bin/env python3
"""Capture unchanged radar graphics constructors at their device boundary.

Texture residency, device state and push-buffer allocation are explicit host
services. Pixel definitions, inline vertices, pass order, sprite radius and
final extent execute the untouched Xbox instructions. No driver is emulated.
"""
import argparse
import json
import struct
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from inspect_multiplayer_hud import HudCache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *


class Trace(Retail):
    def __init__(self):
        super().__init__()
        self.programs=[]; self.calls=[]
        self.buffer=self.alloc(bytes(0x10000));self.cursor=self.alloc(bytes(12))
        self.w(self.cursor,self.buffer);self.w(self.cursor+4,self.buffer+0x10000)
        self.w(0x407488,self.cursor)
        self.g.hook_add(UC_HOOK_CODE,self.graphics)

    def skip(self,n=0,value=0):
        sp=self.g.reg_read(UC_X86_REG_ESP)
        self.ret(value);self.g.reg_write(UC_X86_REG_ESP,sp+4+n)

    def graphics(self,g,pc,size,unused):
        sp=g.reg_read(UC_X86_REG_ESP)
        if pc==0x15180:
            self.programs.append(list(struct.unpack('<60I',g.mem_read(g.reg_read(UC_X86_REG_EDX),240))))
            self.calls.append(dict(entry=f'{pc:08x}',color_write_mask=self.r(0x4b8308)))
            self.skip()
        elif pc in (0x12360,0x12310): self.skip(value=1)
        elif pc in (0x16b10,0x14ac0,0x14390,0x1cc30,0x1cf50,0x400fa0,
                     0x14f60):self.skip()
        elif pc==0x222d0:
            self.calls.append(dict(entry=f'{pc:08x}',state=g.reg_read(UC_X86_REG_ESI),
                                   value=g.reg_read(UC_X86_REG_EDI)))
            self.skip()
        elif pc==0x224f0:
            self.calls.append(dict(entry=f'{pc:08x}',stage=g.reg_read(UC_X86_REG_EAX),
                                   state=g.reg_read(UC_X86_REG_EDX),value=g.reg_read(UC_X86_REG_ECX)))
            self.skip()
        elif pc==0x3f5ea0:
            # D3DDevice_SetTextureStageState_BorderColor: emits NV097 1B24.
            self.calls.append(dict(entry=f'{pc:08x}',stage=g.reg_read(UC_X86_REG_EAX),
                                   border_color=g.reg_read(UC_X86_REG_EBX)))
            self.skip()
        elif pc in (0x14bc0,):self.skip(8)
        elif pc==0x3f9fc0:
            self.calls.append(dict(entry=f'{pc:08x}',args=list(struct.unpack('<6I',g.mem_read(sp+4,24)))))
            self.skip(24)
        elif pc in (0x15720,):self.skip(8)
        elif pc in (0x1c710,0x3f53d0,0x3f59b0,0x3fb440):self.skip(4)
        elif pc==0x3f5cf0:
            self.calls.append(dict(entry=f'{pc:08x}',method=g.reg_read(UC_X86_REG_ECX),
                                   value=g.reg_read(UC_X86_REG_EDX)))
            self.skip()
        elif pc in (0x3fb280,0x3fb330):
            self.calls.append(dict(entry=f'{pc:08x}',args=list(struct.unpack('<3I',g.mem_read(sp+4,12)))))
            self.skip(12)
        elif pc==0x3fb2d0:
            self.calls.append(dict(entry=f'{pc:08x}',attribute=g.reg_read(UC_X86_REG_EDI),
                                   args=list(struct.unpack('<4I',g.mem_read(sp+4,16)))))
            self.skip(16)
        elif pc==0x3fb480:self.skip()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=__import__('pathlib').Path,default=ROOT/'analysis/multiplayer/hud-functionality/radar-graphics.json')
    args=ap.parse_args();c=HudCache(ROOT/'assets/disc/maps/lockout.map');r=Trace()
    gp=next(t for t in c.tags.values()if t['group']=='matg')['offset']
    interface=c.block(gp+0x110,0x98)[0]
    globals_=r.alloc(bytes(0x180));i=r.alloc(c.data[interface:interface+0x98])
    r.w(0x4e034c,globals_);r.w(globals_+0x110,1);r.w(globals_+0x114,i)
    for off in (0x38,0x40,0x60,0x68):
        t=c.tags[c.u(interface+off+4)];p=t['offset'];header=bytearray(c.data[p:p+0x4c])
        images=c.block(p+0x44,0x74)
        struct.pack_into('<I',header,0x48,r.alloc(b''.join(c.data[q:q+0x74]for q in images)))
        r.w(r.table+(int(t['datum'],16)&65535)*16+8,r.alloc(header))
    r.run(0x254200)
    initial_calls=r.calls.copy()
    initial_sampler=list(struct.unpack('<12I',r.g.mem_read(0x404dd0,48)))
    p=r.alloc(struct.pack('<2f',12,23));color=r.alloc(struct.pack('<3f',.25,.5,1))
    r.run(0x254490,argv=[p,0x3f800000,0x40000000,color,0])
    blip=bytes(r.g.mem_read(r.buffer,r.r(r.cursor)-r.buffer)).hex()
    blip_program=r.programs[-1]
    r.w(r.cursor,r.buffer);r.calls=[];r.programs=[]
    center=r.alloc(struct.pack('<2f',89,402))
    r.w(0x4e6950+0x70,0x3f800000);r.w(0x4b9ed8,0);r.w(0x4ba04c,1)
    r.run(0x2548f0,argv=[center,0x3f800000])
    assert len(r.programs)==3,len(r.programs)
    masks=[c['color_write_mask']for c in r.calls if c['entry']=='00015180']
    assert masks==[0x10101]*3,masks
    report=dict(xbe_sha256=PIN,instruction_patches=[],blip_program=blip_program,programs=r.programs,
        initial_helper_calls=initial_calls,initial_sampler_words=initial_sampler,
        center_offsets=list(struct.unpack('<6h',r.g.mem_read(0x47ff40,12))),
        hud_color_words=list(struct.unpack('<8I',r.g.mem_read(0x468c80,32))),
        blip_push_buffer_hex=blip,composite_push_buffer_hex=bytes(r.g.mem_read(r.buffer,r.r(r.cursor)-r.buffer)).hex(),
        composite_helper_calls=r.calls,
        boundary=__doc__)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print('Captured original blip, sweep, mask and final composition constructors:',args.output)


if __name__=='__main__':main()
