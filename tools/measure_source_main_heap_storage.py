#!/usr/bin/env python3
"""Unchanged TU1.1 main storage, heap construction, allocation/free and pressure.

Only NtAllocateVirtualMemory is adapted to a real anonymous Linux mmap, with an
explicit32-bit host mirror. All source instructions are checked against the XBE;
no game instruction, heap bytes, allocation return or pressure result is patched.
Standalone entry GP/stack/FP controls are explicit experiment inputs. This is
retained storage-owner evidence, not complete startup, object admission or play.
"""
import argparse
import ctypes
import hashlib
import json
import mmap
import os
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, PIN

GP = [getattr(R, 'UC_X86_REG_' + s) for s in
      ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
RAW, STACK, SP, STOP = 0x1000000, 0x2000000, 0x200f000, 0x3000000
GLOBAL, GLOBAL_BYTES = 0x480d88, 0x34

def digest(raw):
    return hashlib.sha256(raw).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    a.output.mkdir(parents=True, exist_ok=False)
    (a.output / 'storage').mkdir()
    path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert digest(path.read_bytes()) == PIN
    image, executed, sequences = Xbe(path), set(), []
    code = _Code(image, '.text')
    def persist(raw):
        name = 'storage/' + digest(raw) + '.bin'
        p = a.output / name
        if not p.exists(): p.write_bytes(raw)
        return name
    for title in ('classes_and_reuse', 'page_exhaustion'):
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0, 0x810000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(STACK, 0x10000)
        g.mem_map(STOP, 4096)
        g.mem_map(0x80000000, 4096)
        stores, resources, backing, rows = [], [], [], []
        site, mapped = [0], [0]
        def u(p): return struct.unpack('<I', g.mem_read(p, 4))[0]
        def put(p, v): g.mem_write(p, struct.pack('<I', v))
        def hook(uc, p, n, user):
            if p == 0x800000b8:
                esp = uc.reg_read(R.UC_X86_REG_ESP)
                bp, zero, np, flags, protection = [u(esp+4+i*4) for i in range(5)]
                base, size = u(bp), u(np)
                assert (base, zero, size, flags, protection) == (0, 0, 0x106470, 0x101000, 4)
                page = os.sysconf('SC_PAGE_SIZE')
                mapped[0] = (size+page-1)//page*page
                raw = mmap.mmap(-1, mapped[0], prot=mmap.PROT_READ | mmap.PROT_WRITE)
                address = ctypes.addressof(ctypes.c_ubyte.from_buffer(raw))
                uc.mem_map_ptr(RAW, mapped[0], 3, address)
                backing.append(raw)
                resources.append(dict(request=[base, zero, size, flags, protection],
                    result=[0, RAW, mapped[0]], host_page_bytes=page,
                    initial_storage=persist(raw[:])))
                put(bp, RAW); put(np, mapped[0])
                uc.reg_write(R.UC_X86_REG_EAX, 0)
                uc.reg_write(R.UC_X86_REG_ESP, esp+24)
                uc.reg_write(R.UC_X86_REG_EIP, u(esp))
                return
            assert bytes(uc.mem_read(p, n)) == image.read(p, n), hex(p)
            site[0] = p; executed.add(p)
        def write(uc, access, p, n, value, user):
            if STACK <= p < STACK+0x10000: return
            assert RAW <= p < RAW+mapped[0] or GLOBAL <= p < GLOBAL+GLOBAL_BYTES, hex(p)
            stores.append(dict(instruction=site[0], address=p,
                before=bytes(uc.mem_read(p, n)).hex(),
                written=(value & ((1 << (8*n))-1)).to_bytes(n, 'little').hex()))
        g.hook_add(UC_HOOK_CODE, hook)
        g.hook_add(UC_HOOK_MEM_WRITE, write)
        def abi():
            return dict(gp=[g.reg_read(r) for r in GP],
                eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
                mxcsr=g.reg_read(R.UC_X86_REG_MXCSR))
        def run(entry, args=(), ecx=None, eax=None):
            values = [0x11223344, 0x22334455, 0x33445566, 0x44556677,
                      SP, 0x66778899, 0x778899aa, 0x8899aabb]
            if ecx is not None: values[1] = ecx
            if eax is not None: values[0] = eax
            for r, v in zip(GP, values): g.reg_write(r, v)
            g.reg_write(R.UC_X86_REG_EFLAGS, 2)
            g.reg_write(R.UC_X86_REG_FPCW, 0x27f)
            g.reg_write(R.UC_X86_REG_FPSW, 0)
            g.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
            g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
            for i in range(8): g.reg_write(getattr(R, f'UC_X86_REG_XMM{i}'), 0)
            g.mem_write(SP, struct.pack('<'+'I'*(1+len(args)), STOP, *args))
            before = abi()
            g.emu_start(entry, STOP, count=1000000)
            assert g.reg_read(R.UC_X86_REG_EIP) == STOP, hex(g.reg_read(R.UC_X86_REG_EIP))
            return before, abi()
        def step(kind, size=0, block=0):
            begin = len(stores)
            if kind == 'raw': entry, args, ecx = 0x1482a0, (), None
            elif kind == 'construct': entry, args, ecx = 0x148190, (), None
            elif kind == 'allocate': entry, args, ecx = 0x22e950, (size,), u(0x480da0)
            else: entry, args, ecx = 0x22eb20, (block, size), u(0x480da0)
            before, after = run(entry, args, ecx)
            row = dict(kind=kind, size=size, block=block, entry_abi=before,
                exit_abi=after, stores=stores[begin:],
                globals=bytes(g.mem_read(GLOBAL, GLOBAL_BYTES)).hex(),
                raw_storage=persist(bytes(g.mem_read(RAW, mapped[0]))))
            if kind != 'raw':
                # Both query bodies execute unchanged on the actually mutated heap.
                pool = u(0x480da0)
                run(0x22ecb0, eax=pool)
                row['statistics_bits'] = g.reg_read(R.UC_X86_REG_XMM0) & 0xffffffff
                row['statistics_mxcsr'] = g.reg_read(R.UC_X86_REG_MXCSR)
                _, pressure_abi = run(0x148740)
                row['pressure'] = pressure_abi['gp'][0]
                row['pressure_mxcsr'] = pressure_abi['mxcsr']
            rows.append(row)
            return after['gp'][0]
        try:
            initial_globals = bytes(g.mem_read(GLOBAL, GLOBAL_BYTES)).hex()
            step('raw'); step('construct')
            allocated = []
            sizes = [0,1,7,8,9,16,17,24,32,48,64,80,96,128,160,192,256,
                     384,512,513,1024,1025,2048,2049,4096,4097,8192]
            if title == 'page_exhaustion': sizes = [8192]*121
            for size in sizes:
                block = step('allocate', size)
                if block: allocated.append((block, size))
            if title == 'classes_and_reuse':
                for block, size in reversed(allocated): step('free', size, block)
                for size in sizes: step('allocate', size)
                step('construct')
            else:
                for block, size in reversed(allocated): step('free', size, block)
                step('allocate', 8192)
            assert len(resources) == 1
            sequences.append(dict(name=title, initial_globals=initial_globals,
                                  resources=resources, steps=rows))
        finally:
            del g
            for raw in backing: raw.close()
    fixture = a.output / 'fixture.json'
    fixture.write_text(json.dumps(dict(image_sha256=PIN, raw_address=RAW,
        global_address=GLOBAL, stack_address=STACK, stack_bytes=0x10000,
        stop=STOP, sequences=sequences), indent=2)+'\n')
    listing = a.output / 'executed-original.asm'
    listing.write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} '
        f'{code.at(p).mnemonic} {code.at(p).op_str}\n' for p in sorted(executed)))
    report = dict(source_sha256=PIN, oracle_sha256=digest(Path(__file__).read_bytes()),
        fixture_sha256=digest(fixture.read_bytes()),
        instruction_listing_sha256=digest(listing.read_bytes()),
        sequences=len(sequences), steps=sum(len(s['steps']) for s in sequences),
        stores=sum(len(r['stores']) for s in sequences for r in s['steps']),
        boundaries=__doc__, instruction_patches=[], supplied_gameplay_returns=[],
        native_expected_data_used=False, live_gameplay_proof=False,
        negative_kernel_branches_measured=False)
    (a.output / 'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))

if __name__ == '__main__': main()
