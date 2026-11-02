"""Capture ai_force_active with unchanged TU1.5 squad/group traversal.

The effect, all iterators and the whole 20A160 squad activation frame execute
without hooks, including its awareness query and activation/deactivation
helpers. HS argument parsing and completion are declared boundaries only for
the evaluator cases. Empty actor lists bound the frame without substituting
awareness or activation results; full scene AI/progression remain unverified.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

REGS=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,
      UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)
STACK=0x21ffe00
SQUADS=0x2101000
GROUPS=0x2102000


def capture():
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    g=Uc(UC_ARCH_X86,UC_MODE_32)
    for at,size in ((0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)):
        g.mem_map(at,size)
    base=struct.unpack_from('<I',image.data,0x104)[0]
    count,table=struct.unpack_from('<II',image.data,0x11c)
    for index in range(count):
        _,at,_,offset,size=struct.unpack_from('<5I',image.data,table-base+index*56)
        if size:g.mem_write(at,image.data[offset:offset+size])
    def put(at,*words):g.mem_write(at,struct.pack('<'+'I'*len(words),*words))
    def word(at):return struct.unpack('<I',g.mem_read(at,4))[0]
    calls=[]
    parsed=True
    def hook(guest,at,size,user):
        if at not in (0x212010,0x211da0):return
        sp=guest.reg_read(UC_X86_REG_ESP)
        calls.append(dict(target=at,registers=[guest.reg_read(r) for r in REGS],
                          stack=bytes(guest.mem_read(sp,24)).hex()))
        pop=0
        if at==0x212010:
            guest.reg_write(UC_X86_REG_EAX,0x2103000 if parsed else 0);pop=16
        guest.reg_write(UC_X86_REG_ESP,sp+4+pop)
        guest.reg_write(UC_X86_REG_EIP,word(sp))
    g.hook_add(UC_HOOK_CODE,hook)
    put(0x5020a8,0x2100000);put(0x2100014,0xffffffff)
    put(0x52be7c,0x2100400);put(0x2100444,SQUADS)
    put(0x52be80,0x2100500);put(0x2100544,GROUPS)
    # Root0 has child1 (which has child3) then sibling2. Group4 is
    # deliberately outside the subtree. Every branch owns distinct squads.
    groups=bytearray(5*0x38)
    for index,(child,head,sibling,parent) in enumerate(((1,0,-1,-1),(3,1,2,0),(-1,3,-1,0),(-1,4,-1,1),(-1,6,-1,-1))):
        struct.pack_into('<4I',groups,index*0x38+4,*[v&0xffffffff for v in (child,head,sibling,parent)])
    g.mem_write(GROUPS,bytes(groups))
    rng=random.Random(0x27f760)
    initial_registers=[0,0x11223344,0x55667788,0x99aabbcc,0x21fff00,0x12345678,0x23456789,0x3456789a]
    def run(entry,reference,enabled):
        calls.clear();stack=rng.randbytes(512);g.mem_write(STACK,stack)
        put(0x21fff00,0x3000000,enabled)
        regs=initial_registers.copy();regs[0]=reference
        for r,value in zip(REGS,regs):g.reg_write(r,value)
        g.reg_write(UC_X86_REG_EFLAGS,2);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x27f)
        g.emu_start(entry,0x3000000,count=100000)
        assert g.reg_read(UC_X86_REG_EIP)==0x3000000
        return dict(registers_before=regs,stack_before=stack.hex(),
                    registers=[g.reg_read(r) for r in REGS],stack_after=bytes(g.mem_read(STACK,512)).hex(),calls=calls.copy())
    effects=[]
    for active in (False,True):
        for reference in (0xffffffff,0,2,0x12340002,0x40000000,0x40000001,0x40000004,0x80000000,0xc0000002):
            for enabled in (0,1,0xffffff00,0xffffffff):
                before=bytearray(rng.randbytes(8*0x98))
                for index,next_ in enumerate((-1,2,-1,-1,5,-1,7,-1)):
                    struct.pack_into('<h',before,index*0x98+0x74,next_)
                g.mem_write(SQUADS,bytes(before));g.mem_write(0x2100001,bytes([active]))
                result=run(0x27f760,reference,enabled)
                assert not result['calls'], 'effect called a substituted iterator'
                effects.append(dict(active=active,reference=reference,enabled=enabled,before=before.hex(),
                                    after=bytes(g.mem_read(SQUADS,len(before))).hex(),**result))
    wrappers=[]
    for parsed in (False,True):
        for reference in (0xffffffff,2,0x40000000):
            for enabled in (0,1):
                before=bytearray.fromhex(effects[0]['before'])
                g.mem_write(SQUADS,bytes(before));g.mem_write(0x2100001,b'\1')
                put(0x2103000,reference,enabled)
                # The evaluator has three incoming HS arguments, not the
                # effect's single boolean. Preserve the same private stack.
                stack=rng.randbytes(512);g.mem_write(STACK,stack)
                put(0x21fff00,0x3000000,317,0x12340001,0x23450002)
                regs=initial_registers.copy()
                for r,value in zip(REGS,regs):g.reg_write(r,value)
                calls.clear();g.reg_write(UC_X86_REG_EFLAGS,2)
                g.emu_start(0x2af4c0,0x3000000,count=100000)
                assert g.reg_read(UC_X86_REG_EIP)==0x3000000
                wrappers.append(dict(parsed=parsed,reference=reference,enabled=enabled,before=before.hex(),
                    after=bytes(g.mem_read(SQUADS,len(before))).hex(),registers_before=regs,
                    stack_before=stack.hex(),stack_after=bytes(g.mem_read(STACK,512)).hex(),
                    registers=[g.reg_read(r) for r in REGS],calls=calls.copy()))
    # Whole squad activation frame, one allocated row, no actors. Forced
    # squads must reset idle time and refresh activity even with no awareness.
    put(0x51df54,0x2104000);g.mem_write(0x2104002,struct.pack('<h',30));put(0x2104008,1234)
    put(0x4f3218,0x2108000);put(0x4ecc18,0x2109000)
    put(0x5020c8,0x2100700)
    put(0x2100420,8,0x98);put(0x2100438,1);put(0x2100448,0x2100600);put(0x2100600,1)
    consumers=[]
    for force in (False,True):
        for already_active in (False,True):
            for attached in (False,True):
                before=bytearray(0x98)
                struct.pack_into('<H',before,0,0x9000)
                struct.pack_into('<H',before,2,(0x200 if force else 0)|(0x80 if already_active else 0))
                struct.pack_into('<h',before,0x28,22);struct.pack_into('<h',before,0x2a,-1)
                struct.pack_into('<I',before,0x68,0xffffffff)
                struct.pack_into('<I',before,0x78,0 if attached else 0xffffffff)
                struct.pack_into('<h',before,0x7c,5)
                g.mem_write(SQUADS,bytes(before));g.mem_write(0x2100001,b'\1');g.mem_write(0x474c24,b'\0\0')
                result=run(0x20a160,0,0)
                consumers.append(dict(force=force,already_active=already_active,attached=attached,
                    before=before.hex(),after=bytes(g.mem_read(SQUADS,len(before))).hex(),**result))
    return dict(xbe_sha256=NEW_PIN,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        boundary=__doc__,groups=groups.hex(),activity_scale=image.read(0x46a100,4).hex(),
        effects=effects,wrappers=wrappers,consumers=consumers,full_campaign_verified=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert args.output.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve())
    report=capture();args.output.write_text(json.dumps(report,separators=(',',':'))+'\n')
    print(json.dumps(dict(effects=len(report['effects']),wrappers=len(report['wrappers']),
                         consumers=len(report['consumers']),bytes=args.output.stat().st_size)))
