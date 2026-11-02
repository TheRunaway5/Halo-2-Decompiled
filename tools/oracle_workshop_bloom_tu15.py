"""Capture authored Workshop bloom controls from unchanged TU1.5 routines.

Only GPU/SDK submissions and HS argument/completion boundaries are intercepted.
All filter programs, blend controls, radii, and override writes are original
outputs. No runtime assets or original code are installed by this offline test.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def capture():
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    for at, size in ((0x10000,0x600000),(0x2100000,0x100000),(0x2300000,0x100000),(0x3000000,4096)):
        g.mem_map(at,size)
    base = struct.unpack_from('<I',image.data,0x104)[0]
    count, table = struct.unpack_from('<II',image.data,0x11c)
    for index in range(count):
        _, at, _, offset, size = struct.unpack_from('<5I',image.data,table-base+index*56)
        if size:
            g.mem_write(at,image.data[offset:offset+size])
    def put(at,*words):
        g.mem_write(at,struct.pack('<'+'I'*len(words),*words))
    def bits(value):
        return struct.unpack('<I',struct.pack('<f',value))[0]
    def word(at):
        return struct.unpack('<I',g.mem_read(at,4))[0]
    put(0x411568,0x2350000); put(0x2350000,0x23c0000,0x23f0000)
    put(0x515ec4,0x2100000); put(0x51df50,0x2101000); put(0x4920d4,0xffffffff)
    stubs={0x14ba0:8,0x14f40:0,0x15760:4,0x222b0:0,0x1ccd0:0,0x4044b0:0,0x34a80:44}
    programs, draws, blends, calls = [], [], [], []
    parsed = True
    def hook(guest,at,size,user):
        sp=guest.reg_read(UC_X86_REG_ESP)
        if at==0x212010:
            calls.append(at)
            guest.reg_write(UC_X86_REG_EAX,0x2103000 if parsed else 0)
            guest.reg_write(UC_X86_REG_ESP,sp+20)
            guest.reg_write(UC_X86_REG_EIP,word(sp)); return
        if at==0x211da0:
            calls.append(at)
            guest.reg_write(UC_X86_REG_ESP,sp+4)
            guest.reg_write(UC_X86_REG_EIP,word(sp)); return
        if at==0xbaba0:
            calls.append(at)
            guest.reg_write(UC_X86_REG_ESP,sp+4)
            guest.reg_write(UC_X86_REG_EIP,word(sp)); return
        if at not in stubs:
            return
        if at in (0x1ccd0,0x4044b0):
            programs.append(list(struct.unpack('<60I',guest.mem_read(0x491788,240))))
        if at==0x34a80:
            words=struct.unpack('<11I',guest.mem_read(sp+4,44)); draws.append(list(words))
            guest.reg_write(UC_X86_REG_EAX,words[0])
        if at==0x222b0:
            blends.append([guest.reg_read(UC_X86_REG_ESI),guest.reg_read(UC_X86_REG_EDI)])
        guest.reg_write(UC_X86_REG_ESP,sp+4+stubs[at])
        guest.reg_write(UC_X86_REG_EIP,word(sp))
    g.hook_add(UC_HOOK_CODE,hook)
    def run(entry,arguments):
        put(0x21fff00,0x3000000,*arguments)
        for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI):
            g.reg_write(reg,0)
        g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_EFLAGS,2)
        g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x27f)
        g.emu_start(entry,0x3000000,count=1000000)
        assert g.reg_read(UC_X86_REG_EIP)==0x3000000
    setters=[]
    rng=random.Random(0x2b6550)
    for opcode,entry,offset,width in ((891,0x2b63c0,4,4),(894,0x2b64b0,16,4),(896,0x2b6550,24,1)):
        for parsed in (False,True):
            for value in (0,1,0xffffff00,0xffffffff,bits(7),bits(.6)):
                before=rng.randbytes(36);g.mem_write(0x2100000,before);put(0x2103000,value)
                calls.clear();run(entry,[opcode,0x12340001,0x23450002])
                setters.append(dict(opcode=opcode,entry=entry,parsed=parsed,value=value,before=before.hex(),
                    after=bytes(g.mem_read(0x2100000,36)).hex(),calls=calls.copy(),
                    esp=g.reg_read(UC_X86_REG_ESP),registers=[g.reg_read(reg) for reg in
                        (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)]))
    collision=[]
    put(0x4ecbd0,0x2104000);put(0x2104044,0x2105000);put(0x2105020,0x2106000)
    for object_ in (0xffffffff,0x12340002):
        for enabled in (0,1,0xffffff00,0xffffffff):
            for flags in (0,0xffffffff,0x12345678,0x100000):
                put(0x2106004,flags);calls.clear()
                put(0x21fff00,0x3000000,enabled)
                for reg in (UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI):g.reg_write(reg,0)
                g.reg_write(UC_X86_REG_EAX,object_);g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_EFLAGS,2)
                g.emu_start(0xbe130,0x3000000,count=10000)
                assert g.reg_read(UC_X86_REG_EIP)==0x3000000
                collision.append(dict(object=object_,enabled=enabled,before=flags,after=word(0x2106004),calls=calls.copy(),
                    registers=[g.reg_read(reg) for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)]))
    fixtures=[]
    values=[(-1,-1), (7,.6), (0,0), (.5,1), (1,0), (2.25,.25), (4,1.2), (-2,-.2)]
    for cinematic in (False,True):
        for enabled in (False,True):
            for silver in (False,True):
                for amount,box in values:
                    override=bytearray(36);override[0]=enabled;override[24]=silver
                    for offset in (4,8,12,16,20,28,32):struct.pack_into('<f',override,offset,-1)
                    struct.pack_into('<f',override,4,amount);struct.pack_into('<f',override,16,box)
                    g.mem_write(0x2100000,bytes(override));g.mem_write(0x2101005,bytes([cinematic]));g.mem_write(0x492294,b'\0')
                    programs.clear();draws.clear();blends.clear()
                    run(0x214d0,[0x407f7cee,bits(.9),bits(.375),bits(.25),0,1,0])
                    fixtures.append(dict(cinematic=cinematic,enabled=enabled,silver=silver,
                        blur=bits(amount),box=bits(box),programs=programs.copy(),
                        radii=[row[6] for row in draws[:-1]],blends=blends.copy()))
    return dict(xbe_sha256=NEW_PIN,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        boundary=__doc__,setters=setters,collision=collision,cases=fixtures,framebuffer_parity_verified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert args.output.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve())
    report=capture();args.output.write_text(json.dumps(report,separators=(',',':'))+'\n')
    print(json.dumps(dict(setters=len(report['setters']),filters=len(report['cases']),bytes=args.output.stat().st_size)))
