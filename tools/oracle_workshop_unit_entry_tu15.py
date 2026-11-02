"""Capture unit_set_enterable_by_player and its real seat-selection guard.

The setter, recursive seat enumeration CAF20, and whole CB090 selection body
execute unchanged. Authored seat flags decline the enumerated seat after the
player-entry gate, bounding this fixture before geometry/animation admission.
Only HS argument parsing and completion are intercepted for evaluator cases.
The complete2ABCA0 object_cannot_die evaluator also executes its inlined
vitality-flag write without an effect hook.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

REGS=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,
      UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)


def capture():
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)):g.mem_map(at,size)
    base=struct.unpack_from('<I',image.data,0x104)[0];count,table=struct.unpack_from('<II',image.data,0x11c)
    for index in range(count):
        _,at,_,offset,size=struct.unpack_from('<5I',image.data,table-base+index*56)
        if size:g.mem_write(at,image.data[offset:offset+size])
    def put(at,*words):g.mem_write(at,struct.pack('<'+'I'*len(words),*words))
    def word(at):return struct.unpack('<I',g.mem_read(at,4))[0]
    calls=[];trace=[];parsed=True
    def hook(guest,at,size,user):
        trace.append(at)
        if at not in (0x212010,0x211da0):return
        sp=guest.reg_read(UC_X86_REG_ESP)
        calls.append(dict(target=at,registers=[guest.reg_read(r) for r in REGS],stack=bytes(guest.mem_read(sp,24)).hex()))
        if at==0x212010:guest.reg_write(UC_X86_REG_EAX,0x2103000 if parsed else 0)
        guest.reg_write(UC_X86_REG_ESP,sp+(20 if at==0x212010 else 4));guest.reg_write(UC_X86_REG_EIP,word(sp))
    g.hook_add(UC_HOOK_CODE,hook)
    put(0x4ecbd0,0x2104000);put(0x2104044,0x2105000)
    put(0x2105014,0x2106000);put(0x2105020,0x2107000)
    g.mem_write(0x210501b,b'\1');put(0x2107000,3);put(0x2107010,0xffffffff)
    put(0x4f0414,0x2109000);put(0x2109038,0x2108000)
    put(0x21081c8,1,0x210a000);put(0x210a000,0x30000)
    rng=random.Random(0x11db00)
    def execute(entry,registers,arguments):
        stack=rng.randbytes(2048);g.mem_write(0x21ff800,stack);put(0x21fff00,0x3000000,*arguments)
        for reg,value in zip(REGS,registers):g.reg_write(reg,value)
        g.reg_write(UC_X86_REG_EFLAGS,2);g.reg_write(UC_X86_REG_FPCW,0x27f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        calls.clear();trace.clear();g.emu_start(entry,0x3000000,count=10000)
        assert g.reg_read(UC_X86_REG_EIP)==0x3000000
        return dict(entry=entry,registers_before=registers,arguments=arguments,stack_before=stack.hex(),
            stack_after=bytes(g.mem_read(0x21ff800,2048)).hex(),registers=[g.reg_read(r) for r in REGS],calls=calls.copy(),
            trace=trace.copy())
    effects=[]
    for object_ in (0xffffffff,0x23450002):
        for enabled in (0,1,0xffffff00,0xffffffff):
            for flags in (0,0xffffffff,0x1000,0x12345678):
                put(0x2107134,flags)
                regs=[object_,0x11223344,0x55667788,0x99aabbcc,0x21fff00,0x12345678,0x23456789,0x3456789a]
                result=execute(0x11db00,regs,[enabled])
                effects.append(dict(object=object_,enabled=enabled,before=flags,after=word(0x2107134),**result))
    wrappers=[]
    for parsed in (False,True):
        for object_ in (0xffffffff,0x23450002):
            for enabled in (0,1):
                put(0x2107134,0x12345678);put(0x2103000,object_,enabled)
                result=execute(0x2ad6e0,[0,0,0,0,0x21fff00,0,0,0],[213,0x12340001,0x23450002])
                wrappers.append(dict(parsed=parsed,object=object_,enabled=enabled,before=0x12345678,after=word(0x2107134),**result))
    immortality=[]
    for parsed in (False,True):
        for object_ in (0xffffffff,0x23450002):
            for enabled in (0,1,0xffffff00,0xffffffff):
                for flags in (0,0xff,0x40,0x12):
                    g.mem_write(0x210710b,bytes([flags]));put(0x2103000,object_,enabled)
                    result=execute(0x2abca0,[0,0,0,0,0x21fff00,0,0,0],[85,0x12340001,0x23450002])
                    immortality.append(dict(parsed=parsed,object=object_,enabled=enabled,before=flags,
                        after=g.mem_read(0x210710b,1)[0],**result))
    consumers=[]
    for player in (0xffffffff,0x34560000):
        for enabled in (False,True):
            # Run the actual setter first; its committed state is exactly
            # the state consumed by the original seat query, not a host flag.
            put(0x2107134,0x12345678);put(0x210613c,player)
            execute(0x11db00,[0x23450002,0,0,0,0x21fff00,0,0,0],[int(enabled)])
            before=word(0x2107134)
            result=execute(0xcb090,[0,0,0x23450002,0,0x21fff00,0,0,0],
                [0x12340001,0xffffffff,0x210b000,0x210b004,0,0])
            consumers.append(dict(player=player,enabled=enabled,before=before,after=word(0x2107134),**result))
    return dict(xbe_sha256=NEW_PIN,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        boundary=__doc__,effects=effects,wrappers=wrappers,immortality=immortality,
        consumers=consumers,successful_seat_entry_verified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    assert args.output.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve())
    report=capture();args.output.write_text(json.dumps(report,separators=(',',':'))+'\n')
    print(json.dumps(dict(effects=len(report['effects']),wrappers=len(report['wrappers']),
        immortality=len(report['immortality']),consumers=len(report['consumers']),bytes=args.output.stat().st_size)))
