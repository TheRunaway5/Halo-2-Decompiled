#!/usr/bin/env python3
"""Execute unchanged TU1.5 device setters and immediate group/field owners."""
import argparse
import hashlib
import itertools
import json
import random
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    entries = (0x10c1f0, 0x10c240, 0x10c420, 0x10c4e0)
    owners = (*entries, 0x10c290, 0x10ca30, 0xbcee0, 0xb9390, 0x10c5d0)
    sites = set().union(*(set(complete_body(code, p).code) for p in owners))
    api_sites = set().union(*(set(complete_body(code, p).code) for p in entries))
    assert len(api_sites) == 127
    combinations = list(itertools.product(entries, (False,True), (0,1,2,3), (-.25,0.,.75,1.25), (False,True)))
    assert len(combinations) == 256
    registers = (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
                 UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)
    rng = random.Random(0x11e180)
    cases, covered = [], set()
    for number, (entry, absent, flags, value, group_power) in enumerate(combinations):
        cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        cpu.mem_map(0x10000, 0x800000)
        for section in image.sections:
            cpu.mem_write(section.va, image.section_bytes(section))
        stack, stop, pool, body = 0x600f0000, 0x60300000, 0x61000000, 0x62000000
        groups, tags, tag = 0x63000000, 0x64000000, 0x64001000
        for address, size in ((stack-4096, 8192), (stop,4096), (pool,8192),
                              (body,4096), (groups,8192), (tags,8192)):
            cpu.mem_map(address, size)
        datum, group_datum = 0x90000000, 0x91000000
        initial = bytearray(rng.randbytes(0x240))
        struct.pack_into('<I', initial, 0, 0xa0000000)
        struct.pack_into('<I', initial, 0x14, 0xffffffff)
        struct.pack_into('<II', initial, 0xc, 0xffffffff, 0xffffffff)
        initial[0xaa] = 8
        struct.pack_into('<f', initial, 0x134, .875)
        struct.pack_into('<ff', initial, 0x140, .25, .125)
        struct.pack_into('<I', initial, 0xd4, 0xffffffff)
        struct.pack_into('<I', initial, 0x130, 0x91010001)
        struct.pack_into('<I', initial, 0x13c, group_datum)
        pb = bytearray(0x4c)
        struct.pack_into('<II', pb, 0x20, 2, 12)
        struct.pack_into('<I', pb, 0x38, 1)
        struct.pack_into('<I', pb, 0x44, pool+4096)
        headers = struct.pack('<HBBII', 0x9000, 0, 8, 0x240, body)
        gb = bytearray(0x4c)
        struct.pack_into('<II', gb, 0x20, 2, 12)
        struct.pack_into('<I', gb, 0x44, groups+4096)
        group_rows = struct.pack('<HHffHHff', 0x9100, flags, .25, 0., 0x9101, 4, 1., 1.)
        definition = bytearray(rng.randbytes(0x160))
        struct.pack_into('<hhf', definition, 0x11c, 0, 0, .75)
        for offset in (0x100, 0x108, 0x12c, 0x134, 0x13c):
            struct.pack_into('<I', definition, offset, 0xffffffff)
        sb = bytearray(rng.randbytes(128))
        struct.pack_into('<I', sb, 0, stop)
        scalar = struct.unpack('<I',struct.pack('<f',value))[0]
        target_group = 0xffffffff if absent else group_datum+int(group_power)*0x10001
        struct.pack_into('<I',sb,4,target_group if entry==0x10c4e0 else scalar)
        regions = [(0x4ecbd0, struct.pack('<I', pool)), (0x4ecbf8, struct.pack('<I', groups)),
                   (0x4f0414, struct.pack('<I', tags)), (0x46a0cc, image.read(0x46a0cc,8)),
                   (stack-256,rng.randbytes(256)),
                   (stack,sb), (pool,pb), (pool+4096,headers), (body,initial),
                   (groups,gb), (groups+4096,group_rows),
                   (tags,struct.pack('<4I',0x6374726c,0xa0000000,tag,0)), (tag,definition)]
        for address, data in regions:
            cpu.mem_write(address,bytes(data))
        gp = [rng.getrandbits(32) for _ in registers]
        gp[0], gp[4], gp[6], gp[7] = datum, stack, 0xffffffff if absent else datum, 0xffffffff if absent else datum
        xmm = [[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
        xmm[0][0] = scalar
        eflags = 0x202 | ((number&1)*0x8d5)
        for register, value in zip(registers,gp):
            cpu.reg_write(register,value)
        for i, words in enumerate(xmm):
            cpu.reg_write(UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*words),'little'))
        cpu.reg_write(UC_X86_REG_EFLAGS,eflags)
        trace, stores = [], []
        def instruction(uc, pc, size, _):
            assert pc in sites, hex(pc)
            assert bytes(uc.mem_read(pc,size)) == image.read(pc,size)
            trace.append(pc)
            covered.add(pc)
        def store(uc, _, address, size, value, __):
            stores.append(dict(instruction=uc.reg_read(UC_X86_REG_EIP),address=address,
                before=bytes(uc.mem_read(address,size)).hex(),
                written=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
        cpu.hook_add(UC_HOOK_CODE,instruction)
        cpu.hook_add(UC_HOOK_MEM_WRITE,store)
        cpu.emu_start(entry,stop,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop
        cases.append(dict(entry=entry,regions=[dict(address=p,bytes=bytes(b).hex()) for p,b in regions],
            input=dict(gp=gp,xmm=xmm,eflags=eflags),
            expected=dict(gp=[cpu.reg_read(r) for r in registers],
                xmm=[list(struct.unpack('<4I',cpu.reg_read(UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
                eflags=cpu.reg_read(UC_X86_REG_EFLAGS),body=bytes(cpu.mem_read(body,len(initial))).hex(),
                stores=stores,trace=trace)))
    assert api_sites <= covered, [hex(p) for p in sorted(api_sites-covered)]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(dict(reference='English TU1.5',xbe_sha256=NEW_PIN,
        suite='device_script',oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        boundary='Declared device/group inputs; unchanged setter/immediate and actual group/iterator/wake/reset owners. All127 API sites; linked effect definitions absent. Native script binding and live trajectories are separate validation.',
        api_sites_covered=sorted(api_sites&covered),cases=cases),indent=2)+'\n')
    print(f'PASS {len(cases)} original device setter cases, all127 API sites, '
          f'{sum(len(r["expected"]["trace"]) for r in cases)} instructions, '
          f'{sum(len(r["expected"]["stores"]) for r in cases)} ordered stores; no replaced services')


if __name__ == '__main__':
    main()
