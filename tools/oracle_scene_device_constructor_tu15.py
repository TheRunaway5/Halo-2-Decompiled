#!/usr/bin/env python3
"""Execute unchanged TU1.5 DEVI/MACH constructors over declared input banks."""
import argparse
import hashlib
import json
import random
import struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *


def group_cases(image, code):
    """Whole original group allocation and post-birth, with no service hooks."""
    entries = [0x10c790]*64+[0x10c180]*16+[0x10cae0]*8+[0x11e0b0]*32+[0x11e4f0]*32+[0x1d3770]*32
    owned = (0x10c790, 0x10c180, 0x10cae0, 0x16e6a0, 0x10d790, 0x10d870, 0x11e0b0, 0x11e4f0, 0x1d3770)
    sites = set().union(*(set(complete_body(code, p).code) for p in owned))
    rng = random.Random(0x10c790); result=[]; covered=set()
    registers = [UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
        UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]
    for n,entry in enumerate(entries):
        cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x800000)
        for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
        stack,pool,headers,body,stop=0x600f0000,0x61000000,0x61001000,0x62000000,0x60300000
        groups,rows,bits,scenario,placement=0x63000000,0x63001000,0x63002000,0x64000000,0x65000000
        for address,size in [(stack-4096,8192),(pool,0x10000),(body,4096),(stop,4096),(groups,0x3000),(scenario,0x2000),(placement,4096)]:cpu.mem_map(address,size)
        datum=(rng.randrange(1,65535)<<16)|(n%4)
        pb=bytearray(0x100);struct.pack_into('<I',pb,0x44,headers)
        initial=bytearray(rng.randbytes(0x240));struct.pack_into('<H',initial,0x12a,0xffff);struct.pack_into('<I',initial,0x68,0xffffffff)
        gb=bytearray(0x4c);struct.pack_into('<II',gb,0x20,32,12)
        struct.pack_into('<III',gb,0x34,2,2,2);struct.pack_into('<H',gb,0x40,0xfffe if n%2 else 0x8000)
        struct.pack_into('<II',gb,0x44,rows,bits)
        rb=bytearray(rng.randbytes(32*12))
        for i in range(2):struct.pack_into('<HHff',rb,i*12,0x9000+i,0 if (n+i)%3 else 1,.25+i*.5,.25+i*.5)
        sc=bytearray(0xa8);struct.pack_into('<II',sc,0xa0,2,scenario+0x1000)
        pl=bytearray(rng.randbytes(0x60));struct.pack_into('<hhI',pl,0x34,-1 if n%4<2 else 0,-1 if n%4%2==0 else 1,n%32)
        sb=bytearray(rng.randbytes(128));struct.pack_into('<III',sb,0,stop,datum,placement)
        declared=[(0x4ecbd0,struct.pack('<I',pool)),(0x4ecbf8,struct.pack('<I',groups)),(0x4ecc20,struct.pack('<I',scenario)),
            (0x46a0d0,image.read(0x46a0d0,4)),(stack-256,rng.randbytes(256)),(stack,sb),(pool,pb),
            (headers+(datum&65535)*12+8,struct.pack('<I',body)),(body,initial),(groups,gb),(rows,rb),(bits,struct.pack('<I',3)),(scenario,sc),(placement,pl)]
        gp=[rng.getrandbits(32) for _ in registers];gp[4]=stack;gp[6]=body
        if entry==0x10c180:
            index=(-1,0,1,2)[n%4];gp[0]=(gp[0]&0xffff0000)|(index&65535)
            if n%3==0:struct.pack_into('<H',rb,2,4)
        if entry==0x10cae0:
            count=n%8;struct.pack_into('<II',sc,0xa0,count,scenario+0x1000)
            struct.pack_into('<III',gb,0x34,0,0,0)
            declared=[(p,struct.pack('<I',0) if p==bits else b) for p,b in declared]
            data=bytearray(count*0x28)
            for i in range(count):struct.pack_into('<fI',data,i*0x28+0x20,i/8.,i%2)
            if data:declared.append((scenario+0x1000,data))
        for p,b in declared:cpu.mem_write(p,bytes(b))
        xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)];flags=0x202|((n&1)*0x8d5)
        for r,v in zip(registers,gp):cpu.reg_write(r,v)
        for i,w in enumerate(xmm):cpu.reg_write(UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*w),'little'))
        cpu.reg_write(UC_X86_REG_EFLAGS,flags);trace=[];stores=[]
        def instruction(uc,pc,size,_):
            assert pc in sites,hex(pc)
            assert bytes(uc.mem_read(pc,size))==image.read(pc,size)
            trace.append(pc);covered.add(pc)
        def store(uc,_,address,size,value,__):
            stores.append(dict(instruction=uc.reg_read(UC_X86_REG_EIP),address=address,before=bytes(uc.mem_read(address,size)).hex(),written=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
        cpu.hook_add(UC_HOOK_CODE,instruction);cpu.hook_add(UC_HOOK_MEM_WRITE,store)
        cpu.emu_start(entry,stop,count=10000);assert cpu.reg_read(UC_X86_REG_EIP)==stop
        result.append(dict(entry=entry,regions=[dict(address=p,bytes=bytes(b).hex()) for p,b in declared],input=dict(gp=gp,xmm=xmm,eflags=flags),
            expected=dict(gp=[cpu.reg_read(r) for r in registers],xmm=[list(struct.unpack('<4I',cpu.reg_read(UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
                eflags=cpu.reg_read(UC_X86_REG_EFLAGS),body=bytes(cpu.mem_read(body,len(initial))).hex(),stores=stores,trace=trace)))
    for entry in (0x11e0b0,0x11e4f0,0x10cae0):assert set(complete_body(code,entry).code)<=covered
    print(f'Original group/post-birth helpers: {len(result)} cases, {len(covered)} distinct instructions, no replaced services')
    return result


def machine_bank_cases(image, code):
    """Complete original BSP getter and machine-reference publication bodies."""
    entries = [0x202ab0]*8 + [0x2184a0]*48
    all_sites = {p: set(complete_body(code, p).code) for p in set(entries)}
    covered = {p: set() for p in all_sites}
    rng = random.Random(0x2184a0)
    registers = [UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
        UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]
    result = []
    for n, entry in enumerate(entries):
        cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        cpu.mem_map(0x10000, 0x800000)
        for section in image.sections:
            cpu.mem_write(section.va, image.section_bytes(section))
        stack, stop, pool, body = 0x600f0000, 0x60300000, 0x61000000, 0x62000000
        bsp, bank, rows, refs, records = 0x63000000, 0x63001000, 0x63002000, 0x63003000, 0x64000000
        for address, size in ((stack-4096,8192), (stop,4096), (pool,8192),
                (body,4096), (bsp,0x4000), (records,4096)):
            cpu.mem_map(address, size)
        datum = (rng.randrange(1,65535)<<16) | (n%8)
        initial = bytearray(rng.randbytes(0x240))
        sb = bytearray(rng.randbytes(128)); struct.pack_into('<I',sb,0,stop)
        pb = bytearray(rng.randbytes(0x100)); struct.pack_into('<I',pb,0x44,pool+4096)
        bb = bytearray(rng.randbytes(0xd0))
        struct.pack_into('<iI',bb,0xc4,(-1,0,1,2,-2147483648,2147483647,0,1)[n%8],bank)
        bank_bytes = bytearray(rng.randbytes(0x38)); struct.pack_into('<II',bank_bytes,0x30,2,rows)
        row_bytes = bytearray(rng.randbytes(2*0x1c))
        # Include every early return, both accepted object types, and a
        # multi-reference loop. These are explicit inputs, not live captures.
        variant = n%12
        initial[0xaa] = (0,6,7,7,7,7,7,7,6,6,7,7)[variant]
        index = (-1,-1,-1,2,0,0,0,1,0,1,0,1)[variant]
        struct.pack_into('<h',initial,0x130,index)
        struct.pack_into('<h',initial,0x1d4,index)
        for i in range(2):
            row_bytes[i*0x1c] = 0 if variant == 4 else 1
            count = 0 if variant == 5 else (-1 if variant == 6 else (1 if variant == 7 else 4))
            struct.pack_into('<iI',row_bytes,i*0x1c+0x14,count,refs)
        ref_bytes = b''.join(struct.pack('<hBB',i*2+1,(n+i)&255,(n*3+i)&255) for i in range(4))
        regions = [(stack-64,rng.randbytes(64)), (stack,sb), (pool,pb),
            (pool+4096+(datum&65535)*12+8,struct.pack('<I',body)), (body,initial),
            (0x4ecbd0,struct.pack('<I',pool)), (0x4ecc18,struct.pack('<I',bsp)),
            (0x505e78,struct.pack('<I',records)), (bsp,bb), (bank,bank_bytes),
            (rows,row_bytes), (refs,ref_bytes), (records,rng.randbytes(256))]
        for p,b in regions: cpu.mem_write(p,bytes(b))
        gp = [rng.getrandbits(32) for _ in registers]; gp[0]=bank; gp[2]=datum; gp[4]=stack
        xmm = [[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
        flags = 0x202 | ((n&1)*0x8d5)
        for register,value in zip(registers,gp): cpu.reg_write(register,value)
        for i,words in enumerate(xmm): cpu.reg_write(UC_X86_REG_XMM0+i,int.from_bytes(struct.pack('<4I',*words),'little'))
        cpu.reg_write(UC_X86_REG_EFLAGS,flags)
        trace, stores = [], []
        def instruction(uc,pc,size,_):
            assert pc in all_sites[entry],hex(pc)
            assert bytes(uc.mem_read(pc,size)) == image.read(pc,size)
            trace.append(pc); covered[entry].add(pc)
        def store(uc,_,address,size,value,__):
            stores.append(dict(instruction=uc.reg_read(UC_X86_REG_EIP),address=address,
                before=bytes(uc.mem_read(address,size)).hex(),written=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
        cpu.hook_add(UC_HOOK_CODE,instruction); cpu.hook_add(UC_HOOK_MEM_WRITE,store)
        cpu.emu_start(entry,stop,count=10000)
        assert cpu.reg_read(UC_X86_REG_EIP)==stop
        result.append(dict(entry=entry,regions=[dict(address=p,bytes=bytes(b).hex()) for p,b in regions],
            input=dict(gp=gp,xmm=xmm,eflags=flags),expected=dict(gp=[cpu.reg_read(r) for r in registers],
                xmm=[list(struct.unpack('<4I',cpu.reg_read(UC_X86_REG_XMM0+i).to_bytes(16,'little'))) for i in range(8)],
                eflags=cpu.reg_read(UC_X86_REG_EFLAGS),body=bytes(cpu.mem_read(body,len(initial))).hex(),stores=stores,trace=trace)))
    assert covered == all_sites, {hex(p):sorted(all_sites[p]-covered[p]) for p in all_sites}
    print('Original machine bank helpers: 56 cases, all 73 instructions, no replaced services')
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    assert not a.output.exists()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    all_sites = {p: set(complete_body(code, p).code) for p in (0x10b5c0, 0x117870, 0x10c0b0, 0x118120, 0xa9be0)}
    assert [len(s) for s in all_sites.values()] == [56, 34, 63, 69, 2]
    covered = {p: set() for p in all_sites}
    rng = random.Random(0x10b5c0)
    registers = [UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
        UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]
    rows = []
    for number in range(160):
        entry = (0x10b5c0, 0x117870, 0x10c0b0, 0x118120, 0xa9be0)[number % 5]
        sites = all_sites[entry]
        index = (0, 1, 17, 2047)[(number // 5) % 4]
        datum = (rng.randrange(1, 65536) << 16) | index
        stack, pool, headers, body, stop = 0x600f0000, 0x61000000, 0x61001000, 0x62000000, 0x60300000
        cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        cpu.mem_map(0x10000, 0x800000)
        for section in image.sections:
            cpu.mem_write(section.va, image.section_bytes(section))
        for address, size in ((stack-4096, 8192), (pool, 0x10000), (body, 4096), (stop, 4096)):
            cpu.mem_map(address, size)
        pool_bytes = bytearray(rng.randbytes(0x100))
        struct.pack_into('<I', pool_bytes, 0x44, headers)
        stack_bytes = bytearray(rng.randbytes(128))
        struct.pack_into('<II', stack_bytes, 0, stop, datum)
        initial_body = bytearray(rng.randbytes(0x240))
        struct.pack_into('<I', initial_body, 0, 0xee55000a)
        regions = [(0x4ecbd0, struct.pack('<I', pool)), (0x46a0d0, image.read(0x46a0d0, 4)),
            (stack-64, rng.randbytes(64)), (stack, stack_bytes), (pool, pool_bytes), (headers + index * 12 + 8, struct.pack('<I', body)), (body, initial_body)]
        if entry == 0x118120:
            cpu.mem_map(0x65000000, 0x6000)
            bsp = bytearray(rng.randbytes(0x228));portal_bank = bytearray(rng.randbytes(0x20));portals = bytearray(rng.randbytes(0x90))
            struct.pack_into('<I', bsp, 0x220, 0x65001000)
            struct.pack_into('<I', bsp, 0x60, 0x65003000)
            struct.pack_into('<I', portal_bank, 12, 0x65002000)
            struct.pack_into('<4h', portals, 0, 0, 1, 0, 0)
            struct.pack_into('<2h', portals, 36, 2, 3)
            variant = (number // 5) % 6
            struct.pack_into('<hH', initial_body, 0x1da, 0, (0, 0xffff, 2, 2, 2, 2)[variant])
            struct.pack_into('<II', stack_bytes, 8, 0x65004000, 0x65005000)
            mask = (0, 0, 0, 1, 2, 8)[variant]
            regions.extend([(0x4ecc18, struct.pack('<I', 0x65000000)), (0x65000000, bsp), (0x65001000, portal_bank),
                (0x65002000, struct.pack('<2h', 0, 1)), (0x65003000, portals), (0x65004000, struct.pack('<I', mask)), (0x65005000, b'\xa5')])
        if entry in (0x117870, 0x10c0b0):
            cpu.mem_map(0x63000000, 4096)
            cpu.mem_map(0x64000000, 0x3000)
            definition = bytearray(rng.randbytes(0x140))
            definition[0x11e] = (definition[0x11e] & ~4) | (((number // 5) & 1) * 4)
            if entry == 0x10c0b0:
                variant = (number // 5) % 4
                struct.pack_into('<I', definition, 0x38, 0xffffffff if variant == 0 else 0xee55000b)
                model = bytearray(rng.randbytes(0x100))
                struct.pack_into('<I', model, 4, 0xffffffff if variant == 1 else 0xee55000c)
                struct.pack_into('<H', initial_body, 0x12a, 0xffff if variant == 2 else 0)
                struct.pack_into('<I', initial_body, 0x18c, 0xffffffff)
                struct.pack_into('<I', initial_body, 0x1ac, 0xffffffff)
                regions.extend([(0x63000000 + 11 * 16 + 8, struct.pack('<I', 0x64001000)),
                    (0x63000000 + 12 * 16 + 8, struct.pack('<I', 0x64002000)),
                    (0x64001000, model), (0x64002000, rng.randbytes(128))])
            regions.extend([(0x4f0414, struct.pack('<I', 0x63000000)),
                (0x63000000 + 10 * 16 + 8, struct.pack('<I', 0x64000000)), (0x64000000, definition)])
        for address, data in regions:
            cpu.mem_write(address, bytes(data))
        gp = [rng.getrandbits(32) for _ in registers]
        gp[4] = stack
        xmm = [[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
        for register, value in zip(registers, gp):
            cpu.reg_write(register, value)
        for index, words in enumerate(xmm):
            cpu.reg_write(UC_X86_REG_XMM0 + index, int.from_bytes(struct.pack('<4I', *words), 'little'))
        flags = 0x202 | ((number & 1) * 0x8d5)
        cpu.reg_write(UC_X86_REG_EFLAGS, flags)
        trace, stores = [], []
        def instruction(uc, pc, size, _):
            assert pc in sites, hex(pc)
            assert bytes(uc.mem_read(pc, size)) == image.read(pc, size)
            trace.append(pc)
        def store(uc, _, address, size, value, __):
            stores.append(dict(instruction=uc.reg_read(UC_X86_REG_EIP), address=address,
                before=bytes(uc.mem_read(address, size)).hex(), written=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))
        cpu.hook_add(UC_HOOK_CODE, instruction)
        cpu.hook_add(UC_HOOK_MEM_WRITE, store)
        cpu.emu_start(entry, stop, count=1000)
        assert cpu.reg_read(UC_X86_REG_EIP) == stop and set(trace) <= sites
        covered[entry].update(trace)
        rows.append(dict(entry=entry, regions=[dict(address=p, bytes=bytes(b).hex()) for p, b in regions],
            input=dict(gp=gp, xmm=xmm, eflags=flags),
            expected=dict(gp=[cpu.reg_read(r) for r in registers],
                xmm=[list(struct.unpack('<4I', cpu.reg_read(UC_X86_REG_XMM0 + i).to_bytes(16, 'little'))) for i in range(8)],
                eflags=cpu.reg_read(UC_X86_REG_EFLAGS), body=bytes(cpu.mem_read(body, len(initial_body))).hex(), stores=stores, trace=trace)))
    assert all(covered[p] == all_sites[p] for p in (0x10b5c0, 0x117870, 0x118120, 0xa9be0))
    assert 0x10c145 not in covered[0x10c0b0] and 0x10c16f not in covered[0x10c0b0]
    rows.extend(group_cases(image,code))
    rows.extend(machine_bank_cases(image,code))
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(dict(reference='OG Xbox TU1.5', xbe_sha256=NEW_PIN,
        boundary='Unchanged original DEVI/MACH birth, portal, fresh pose, collision-type, device-group allocation/resolution and map groups, CTRL/LIFI post-birth, fresh channel lookup, BSP machine bank lookup and collision-reference publication; declared input banks. No replaced instructions or services. Live animation and remaining machine physics branches are not claimed by these cases; not full gameplay.',
        oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), cases=rows), indent=2)+'\n')
    print(f'PASS {len(rows)} unchanged original DEVI/MACH birth, portal, collision-type and fresh pose-change cases, {sum(len(r["expected"]["trace"]) for r in rows)} instructions, all 161 birth/portal/type CFG instructions and {len(covered[0x10c0b0])}/63 pose-change instructions covered, no external services')


if __name__ == '__main__':
    main()
