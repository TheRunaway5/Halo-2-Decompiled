"""Capture whole TU1.5 27CBB0 ai_actors and its real list/iterator owners.

Only datum allocation, collection and HS parse/completion are service
boundaries. All linked-list writes, actor/group traversal and swarm-object
enumeration execute unchanged. Collection is a declared empty-bank boundary;
this fixture does not qualify game-wide HS garbage collection or scene AI.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def capture():
    x = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(x.data).hexdigest() == NEW_PIN
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    for at, size in [(0x10000, 0x600000), (0x2100000, 0x100000), (0x3000000, 4096)]:
        g.mem_map(at, size)
    base = struct.unpack_from('<I', x.data, 0x104)[0]
    count, table = struct.unpack_from('<II', x.data, 0x11c)
    for i in range(count):
        _, at, _, offset, size = struct.unpack_from('<5I', x.data, table-base+i*56)
        if size:
            g.mem_write(at, x.data[offset:offset+size])
    writes = []
    def put(at, value, width=4):
        writes.append([at, width, value & ((1 << (8*width))-1)])
        g.mem_write(at, int(value & ((1 << (8*width))-1)).to_bytes(width, 'little'))
    def word(at, width=4):
        return int.from_bytes(g.mem_read(at, width), 'little')
    allocation = [0, 0]
    exhausted = False
    parsed = True
    events = []
    def hook(guest, at, size, user):
        if at not in (0x16e6a0, 0x1e7180, 0x212010, 0x211da0):
            return
        sp = guest.reg_read(UC_X86_REG_ESP)
        events.append(at)
        pop = 0
        if at == 0x16e6a0:
            pool = guest.reg_read(UC_X86_REG_EDX)
            slot = 0 if pool == 0x2100600 else 1
            assert pool in (0x2100600, 0x2100700)
            result = 0xffffffff if (slot == 0 and exhausted) else ((0x7c00+slot) << 16) | allocation[slot]
            allocation[slot] += result != 0xffffffff
            guest.reg_write(UC_X86_REG_EAX, result)
        elif at == 0x212010:
            guest.reg_write(UC_X86_REG_EAX, 0x2100900 if parsed else 0)
            pop = 16
        guest.reg_write(UC_X86_REG_ESP, sp+4+pop)
        guest.reg_write(UC_X86_REG_EIP, word(sp))
    g.hook_add(UC_HOOK_CODE, hook)
    rows = []
    references = [0xffffffff, 0, 1, 3, 0x40000000, 0x40000001, 0x40000002,
                  0x80000000, 0x80000001, 0xc0000000, 0xc0000001, 0xc0000002]
    for active in (False, True):
        for exhausted in (False, True):
            for reference in references:
                g.mem_write(0x2100000, bytes(0x20000))
                writes.clear()
                for at, value in [(0x5020a8, 0x2100000), (0x52be7c, 0x2100100),
                                  (0x52be80, 0x2100200), (0x5020c8, 0x2100300),
                                  (0x510fa8, 0x2100400), (0x4ecbd0, 0x2100500),
                                  (0x5020b0, 0x2100600), (0x5020ac, 0x2100700),
                                  (0x4ecc20, 0x2100800), (0x2100144, 0x2101000),
                                  (0x2100244, 0x2102000), (0x2100344, 0x2103000),
                                  (0x2100324, 0x888), (0x2100444, 0x2106000),
                                  (0x2100544, 0x2107000), (0x2100644, 0x2108000),
                                  (0x2100744, 0x2109000), (0x2100724, 12),
                                  (0x2100960, 1), (0x2100964, 0x210a000),
                                  (0x210a048, 2), (0x210a04c, 0x210b000),
                                  (0x210b000, 100), (0x210b064, 101)]:
                    put(at, value)
                put(0x2100001, active, 1)
                for i, (child, head, sibling, parent) in enumerate([(1,0,-1,-1),(-1,1,2,0),(-1,3,-1,0)]):
                    for offset, value in zip((4,8,12,16), (child,head,sibling,parent)):
                        put(0x2102000+i*0x38+offset, value)
                for i, (head, next_) in enumerate([(0,-1),(1,2),(3,-1),(0xffffffff,-1)]):
                    put(0x2101000+i*0x98+0x68, head)
                    put(0x2101000+i*0x98+0x74, next_, 2)
                for i, (unit, swarm, next_, name) in enumerate([
                        (0x12340000,0xffffffff,0xffffffff,100),
                        (0xffffffff,0,2,101), (0x12340001,0xffffffff,0xffffffff,102),
                        (0x12340002,0xffffffff,0xffffffff,103)]):
                    at = 0x2103000+i*0x888
                    for offset,value in [(0,0x3456),(0x18,unit),(0x1c,swarm),(0x20,next_),(0x38,name)]:
                        put(at+offset,value,2 if offset == 0 else 4)
                put(0x2106018,0x12340003)
                for i in range(5):
                    put(0x2107000+i*12+8,0x210c000+i*0x200)
                # Swarm3 ->4 ->NONE, through the original relative extension.
                put(0x210c000+3*0x200+0x134,0)
                put(0x210c000+3*0x200+0x13a,0x160,2)
                put(0x210c000+3*0x200+0x16c,0x12340004)
                put(0x210c000+4*0x200+0x134,1)
                before = writes.copy()
                for wrapper, parsed in [(False,True),(True,True),(True,False)]:
                    allocation[:] = [0,0]
                    events.clear()
                    g.mem_write(0x2108000,bytes(0x2000))
                    put(0x2100900,reference)
                    g.mem_write(0x21ff000,bytes(4096))
                    args = [331,0x12340001,0x23450002] if wrapper else [reference]
                    g.mem_write(0x21fff00,struct.pack('<'+'I'*(len(args)+1),0x3000000,*args))
                    for reg in (UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,
                                UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI):
                        g.reg_write(reg,0)
                    g.reg_write(UC_X86_REG_ESP,0x21fff00)
                    g.reg_write(UC_X86_REG_EFLAGS,2)
                    g.emu_start(0x2af890 if wrapper else 0x27cbb0,0x3000000,count=100000)
                    assert g.reg_read(UC_X86_REG_EIP) == 0x3000000
                    header = 0x2108000
                    objects = []
                    next_ = word(header+8) if allocation[0] else 0xffffffff
                    while next_ != 0xffffffff:
                        at = 0x2109000+(next_&65535)*12
                        objects.append(word(at+4));next_=word(at+8)
                        assert len(objects)<128
                    rows.append(dict(reference=reference,active=active,exhausted=exhausted,
                        wrapper=wrapper,parsed=parsed,writes=before,
                        result=g.reg_read(UC_X86_REG_EAX),objects=objects,
                        count=word(header+6,2) if allocation[0] else 0,events=events.copy()))
    return dict(xbe_sha256=NEW_PIN,boundary=__doc__,
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        regions=[[0x2100000,0x20000]],cases=rows)


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    assert a.output.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve())
    report=capture();a.output.write_text(json.dumps(report,separators=(',',':'))+'\n')
    print(json.dumps(dict(cases=len(report['cases']),bytes=a.output.stat().st_size)))
