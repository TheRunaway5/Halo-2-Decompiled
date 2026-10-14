#!/usr/bin/env python3
"""Execute unchanged TU1.5 heap construction against native bootstrap receipts.

Offline only. The no-serialization cases need only the declared native virtual
memory adapter, which makes real Linux mmap/mprotect reservations. Game code
and callees are unchanged. Lock initialization and Xbox kernel parity are
outside this oracle; native lock construction has separate compatibility proof.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
GP = [getattr(R, 'UC_X86_REG_' + name) for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
GDT, STOP, RAW = 0x71000000, 0x73000000, 0x08000000


def segment(base, access):
    limit = 0xfffff
    return struct.pack('<II', (limit & 65535) | ((base & 65535) << 16),
                       ((base >> 16) & 255) | (access << 8) | (limit & 0xf0000) | (0xc << 20) | (base & 0xff000000))


def compare(image, row):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x7f0000)
    u = lambda data, at: struct.unpack_from('<I', data, at)[0]
    sections = []
    for i in range(u(image, 0x11c)):
        at = u(image, 0x120) - u(image, 0x104) + i * 56
        address, extent, offset, size = struct.unpack_from('<IIII', image, at + 4)
        assert size <= extent and offset + size <= len(image)
        data = image[offset:offset + size]
        cpu.mem_write(address, data)
        sections.append((address, data))
    cpu.mem_map(row['stack'], 0x100000)
    cpu.mem_map(row['fs'], 0x2000)
    cpu.mem_map(GDT, 0x1000)
    cpu.mem_map(STOP, 0x1000)
    cpu.mem_map(0x80000000, 0x1000)
    cpu.mem_write(GDT + 8, segment(0, 0x9b) + segment(0, 0x93) + segment(row['fs'], 0x93))
    cpu.reg_write(R.UC_X86_REG_GDTR, (0, GDT, 0xfff, 0))
    for reg, value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),
                       (R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):
        cpu.reg_write(reg, value)
    assert cpu.reg_read(R.UC_X86_REG_FS_BASE) == row['fs']
    read = lambda p: u(bytes(cpu.mem_read(p, 4)), 0)
    put = lambda p, value: cpu.mem_write(p, struct.pack('<I', value))
    put(row['fs'], 0xffffffff)
    if row['args'][5]:
        put(row['args'][5], 0x30)
    cpu.mem_write(row['sp'], struct.pack('<7I', STOP, *row['args']))
    for reg, value in zip(GP, row['initial_gp']):
        cpu.reg_write(reg, value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS, 2)
    cpu.reg_write(R.UC_X86_REG_FPCW, 0x27f)
    cpu.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
    libc = ctypes.CDLL(None, use_errno=True)
    libc.mmap.argtypes = [ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_long]
    libc.mmap.restype = ctypes.c_void_p
    libc.mprotect.argtypes = [ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int]
    libc.munmap.argtypes = [ctypes.c_void_p,ctypes.c_size_t]
    host, extent, active, instructions, stores, resources = None, 0, 0, 0, [], []

    def code(uc, address, size, user):
        nonlocal host, extent, active, instructions
        if address == 0x800000b8:
            sp = uc.reg_read(R.UC_X86_REG_ESP)
            bp, zero, np, flags, protection = (read(sp + 4 + i*4) for i in range(5))
            base, count = read(bp), read(np)
            assert zero == 0 and protection == 4 and count > 0 and count % 4096 == 0
            request = [bp,np,base,zero,count,flags,protection]
            if flags == 0x2000:
                assert host is None and base == 0
                host = libc.mmap(None,count,0,0x22,-1,0)
                if host == ctypes.c_void_p(-1).value:
                    raise OSError(ctypes.get_errno(), 'native reference reservation failed')
                extent, base = count, RAW
                uc.mem_map(base,count,0)
            else:
                assert flags == 0x1000 and host is not None and RAW <= base < base + count <= RAW + extent
                if libc.mprotect(host + base - RAW,count,3):
                    raise OSError(ctypes.get_errno(), 'native reference commit failed')
                uc.mem_protect(base,count,3)
                uc.mem_write(base,ctypes.string_at(host + base - RAW,count))
            resources.append(request + [base,count])
            put(bp,base);put(np,count)
            uc.reg_write(R.UC_X86_REG_EAX,0)
            uc.reg_write(R.UC_X86_REG_ESP,sp + 24)
            uc.reg_write(R.UC_X86_REG_EIP,read(sp))
            return
        assert address < 0x80000000, f'undeclared original platform boundary {address:x}'
        matches = [(p,data) for p,data in sections if p <= address < address + size <= p + len(data)]
        assert len(matches) == 1, f'original code owner missing at {address:x}'
        p, data = matches[0]
        assert bytes(uc.mem_read(address,size)) == data[address-p:address-p+size], 'original instruction changed'
        active, instructions = address, instructions + 1

    def write(uc, access, address, size, value, user):
        stores.append(dict(instruction=active,address=address,before=bytes(uc.mem_read(address,size)).hex(),
                           written=(value & ((1 << (size*8))-1)).to_bytes(size,'little').hex()))
    cpu.hook_add(UC_HOOK_CODE,code)
    cpu.hook_add(UC_HOOK_MEM_WRITE,write)
    try:
        cpu.emu_start(0x2de2ea,STOP,count=1000000)
        assert cpu.reg_read(R.UC_X86_REG_EIP) == STOP, 'original constructor did not return'
        assert [cpu.reg_read(reg) for reg in GP] == row['returned_gp'], 'returned general registers differ'
        assert (cpu.reg_read(R.UC_X86_REG_EFLAGS) ^ row['returned_eflags']) & 0xcd5 == 0, 'returned defined flags differ'
        assert resources == [resource[1:] for resource in row['resources']], 'native reserve/commit ownership differs'
        # Native diagnostics use canonical TU1.1 labels. Do not invent an
        # instruction-address mapping; compare every complete ordered store.
        assert [(s['address'],s['before'],s['written']) for s in stores] == [
            (s['address'],s['before'],s['written']) for s in row['stores']], 'ordered original game stores differ'
        for output in row['outputs']:
            expected = bytes.fromhex(output['hex'])
            assert bytes(cpu.mem_read(output['address'],len(expected))) == expected, 'retained original output differs'
        return dict(name=row['name'],passed=True,original_instructions=instructions,ordered_game_stores=len(stores),
                    committed_output_bytes=sum(len(bytes.fromhex(o['hex'])) for o in row['outputs']))
    finally:
        if host is not None:
            assert libc.munmap(host,extent) == 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-xbe',type=Path,required=True)
    parser.add_argument('--native-receipt',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    image = args.reference_xbe.read_bytes()
    assert hashlib.sha256(image).hexdigest() == PIN, 'requires authenticated English TU1.5 reference'
    receipt = json.loads(args.native_receipt.read_bytes())
    assert receipt['schema'] == 'H2_NATIVE_TU15_HEAP_BOOTSTRAP_V1'
    rows = [compare(image,row) for row in receipt['cases'] if row['args'][0] & 1]
    assert rows and all(row['passed'] for row in rows)
    report = dict(passed=True,reference_xbe_sha256=PIN,original_game_code_patched=False,
                  supplied_game_callee_results=False,native_platform_abi=True,xbox_kernel_parity=False,
                  world_startup_complete=False,returned_sse_x87_compared=False,rows=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(f"Unchanged TU1.5 heap constructor: {len(rows)} native-platform cases matched")


if __name__ == '__main__':
    main()
