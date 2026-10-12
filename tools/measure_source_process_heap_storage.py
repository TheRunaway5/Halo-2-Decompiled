#!/usr/bin/env python3
"""Unchanged TU1.1 process-heap/SEH/copy execution with real split host storage.

Linux mmap(PROT_NONE) reserves the actual bank; mprotect commits only requested
pages, and Unicorn mirrors the same pages with matching permissions. Kernel184
is an explicit native resource adapter: success0, preserved other GP/flags, and
stdcall return. This is not a recovered Xbox kernel ABI or full game startup.
By default kernel291 remains a stop with its real caller frame. The optional
initializer mode runs separately pinned, compiled compatibility expressions,
with their own instruction/store trace. No game instruction, heap field,
TLS/callback or allocation outcome is patched/supplied.
"""
import argparse
import ctypes
import hashlib
import json
import os
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, PIN

GP = [getattr(R, 'UC_X86_REG_' + x) for x in
      ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
RAW, STACK, SP, STOP, DATA = 0x10000000, 0x2000000, 0x200f000, 0x3000000, 0x4000000
PARAM = DATA + 0x800

def sha(raw): return hashlib.sha256(raw).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--with-initializer', action='store_true')
    a = ap.parse_args(); a.output.mkdir(parents=True, exist_ok=False)
    (a.output / 'storage').mkdir()
    path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path.read_bytes()) == PIN and os.sysconf('SC_PAGE_SIZE') == 0x1000
    image = Xbe(path)
    initializer = None
    if a.with_initializer:
        directory = ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world'
        raw = (directory / 'report.json').read_bytes()
        assert sha(raw) == 'ad0fb0b253d9fbf5b8c40b79ec2a4c142271be284c3570656b8325a7e9b92c54'
        initializer = json.loads(raw)['compiled_compatibility_initializer']
        assert sha((directory / 'sources.json').read_bytes()) == initializer['source_manifest_sha256']
        for source in initializer['primary_sources']:
            assert sha((directory / source['saved']).read_bytes()) == source['sha256']
        initializer_bytes = (directory / 'initializer.bin').read_bytes()
        assert initializer_bytes.hex() == initializer['compiled_text_hex']
        assert sha(initializer_bytes) == initializer['compiled_text_sha256']
        assert sha((directory / 'initializer.c').read_bytes()) == initializer['generated_C_sha256']
    libc = ctypes.CDLL(None, use_errno=True)
    libc.mmap.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int,
                          ctypes.c_int, ctypes.c_int, ctypes.c_long]
    libc.mmap.restype = ctypes.c_void_p
    libc.mprotect.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]
    libc.munmap.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    def persist(raw):
        name = 'storage/' + sha(raw) + '.bin'; p = a.output / name
        if not p.exists(): p.write_bytes(raw)
        return name
    cases = []
    for name, flags, reserve, commit, lock, params in (
        ('startup_flags2_required_lock', 2, 0x100000, 0x1000, 0, PARAM),
        ('standalone_nolock', 1, 0x100000, 0x1000, 0, PARAM),
        ('standalone_null_parameters', 1, 0x100000, 0x1000, 0, 0),
        ('standalone_rounded_extents', 1, 0x12345, 0x1234, 0, PARAM),
        ('standalone_default_extents', 1, 0, 0, 0, PARAM),
        ('standalone_invalid_lock', 1, 0x100000, 0x1000, 0x12345678, PARAM)):
        cases.append((name, 0x2da83a, [flags, 0, reserve, commit, lock, params], None))
    if initializer:
        for name, reserve, commit, params, xapi in (
            ('startup_null_parameters', 0x100000, 0x1000, 0, None),
            ('startup_rounded_extents', 0x12345, 0x1234, PARAM, None),
            ('startup_default_extents', 0, 0, PARAM, None),
            ('explicit_xapi_bit8', 0x100000, 0x1000, PARAM, 8),
            ('explicit_xapi_bit8_null_parameters', 0x100000, 0x1000, 0, 8),
            ('explicit_xapi_bit8_rounded', 0x12345, 0x1234, PARAM, 8)):
            cases.append((name, 0x2da83a, [2, 0, reserve, commit, 0, params], xapi))
    # Complete copy-owner coverage: tiny/word/unrolled/REP paths, all byte
    # alignments and both overlap directions. These are explicit standalone
    # buffer inputs, not generated expected results or live heap state.
    for count in (0, 1, 2, 3, 4, 7, 8, 15, 16, 31, 32, 33, 63, 64, 127):
        for source in range(4):
            for destination in range(4):
                cases.append((f'copy_{count}_{source}_{destination}', 0x326e20,
                              [DATA + 0x200 + destination, DATA + source, count], None))
        for shift in (1, 2, 3, 4, 7, 15):
            cases.append((f'overlap_back_{count}_{shift}', 0x326e20,
                          [DATA + shift, DATA, count], None))
            cases.append((f'overlap_forward_{count}_{shift}', 0x326e20,
                          [DATA, DATA + shift, count], None))
    rows, executed = [], set()
    for name, entry, args, xapi in cases:
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0, 0x810000)
        for section in image.sections: g.mem_write(section.va, image.section_bytes(section))
        if xapi is not None: g.mem_write(0x566ef1, bytes([xapi]))
        if initializer:
            g.mem_map(0x9000000, 0x1000); g.mem_write(0x9000000, initializer_bytes)
        g.mem_map(STACK, 0x10000); g.mem_map(STOP, 0x1000)
        g.mem_map(DATA, 0x1000); g.mem_map(0x80000000, 0x1000)
        g.mem_write(0, struct.pack('<I', 0xffffffff))
        initial_data = bytearray((i * 37 + 19) & 255 for i in range(0x1000))
        initial_data[0x800:0x830] = struct.pack('<I', 0x30) + bytes(0x2c)
        g.mem_write(DATA, bytes(initial_data))
        stores, resources, boundaries = [], [], []
        platform_calls, platform_active, platform_pc = [], [False], [0]
        active, host, extent, committed = [0], [None], [0], set()
        def u(p): return struct.unpack('<I', g.mem_read(p, 4))[0]
        def put(p, v): g.mem_write(p, struct.pack('<I', v))
        def abi():
            return dict(gp=[g.reg_read(r) for r in GP],
                        eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
                        mxcsr=g.reg_read(R.UC_X86_REG_MXCSR))
        def hook(uc, p, n, user):
            if initializer and 0x9000000 <= p < 0x9000000 + len(initializer_bytes):
                assert platform_active[0]
                assert bytes(uc.mem_read(p, n)) == initializer_bytes[p-0x9000000:p-0x9000000+n]
                platform_pc[0] = p
                return
            if platform_active[0]:
                assert p == platform_calls[-1]['return']
                platform_calls[-1]['after'] = bytes(uc.mem_read(platform_calls[-1]['pointer'], 28)).hex()
                platform_active[0] = False
            if p == 0x800000b8:
                esp = uc.reg_read(R.UC_X86_REG_ESP)
                bp, zero, np, flags, protection = [u(esp+4+i*4) for i in range(5)]
                base, size = u(bp), u(np)
                assert zero == 0 and protection == 4 and size and size % 0x1000 == 0
                if flags == 0x2000:
                    assert base == 0 and host[0] is None
                    host[0] = libc.mmap(None, size, 0, 0x22, -1, 0)
                    if host[0] == ctypes.c_void_p(-1).value:
                        raise OSError(ctypes.get_errno(), 'real host reserve failed')
                    extent[0] = size
                    uc.mem_map_ptr(RAW, size, 0, host[0]); base = RAW
                    initial = None
                else:
                    assert flags == 0x1000 and host[0] is not None
                    assert base >= RAW and base + size <= RAW + extent[0]
                    assert base % 0x1000 == 0
                    if libc.mprotect(host[0] + base-RAW, size, 3):
                        raise OSError(ctypes.get_errno(), 'real host commit failed')
                    uc.mem_protect(base, size, 3)
                    initial = persist(ctypes.string_at(host[0] + base-RAW, size))
                    committed.update(range(base, base + size, 0x1000))
                resources.append(dict(instruction=active[0], request=[u(bp), zero, size, flags, protection],
                    cells=[bp, np], result=[0, base, size], initial_storage=initial))
                put(bp, base); put(np, size)
                uc.reg_write(R.UC_X86_REG_EAX, 0)
                uc.reg_write(R.UC_X86_REG_ESP, esp+24)
                uc.reg_write(R.UC_X86_REG_EIP, u(esp))
                return
            if p >= 0x80000000:
                assert p == 0x80000123, hex(p)
                if initializer:
                    esp = uc.reg_read(R.UC_X86_REG_ESP); pointer = u(esp+4)
                    platform_calls.append(dict(instruction=active[0], target=p, tail=False,
                        pointer=pointer, return_address=u(esp), **{'return': u(esp)},
                        before=bytes(uc.mem_read(pointer, 28)).hex(), stores=[]))
                    platform_active[0] = True
                    uc.reg_write(R.UC_X86_REG_EIP, int(initializer['entry'], 16))
                    return
                boundaries.append(dict(instruction=active[0], target=p, tail=False))
                uc.emu_stop(); return
            assert bytes(uc.mem_read(p, n)) == image.read(p, n), hex(p)
            active[0] = p; executed.add(p)
        def write(uc, access, p, n, value, user):
            selected = platform_calls[-1]['stores'] if platform_active[0] else stores
            selected.append(dict(instruction=platform_pc[0] if platform_active[0] else active[0], address=p,
                before=bytes(uc.mem_read(p, n)).hex(),
                written=(value & ((1 << (8*n))-1)).to_bytes(n, 'little').hex()))
        g.hook_add(UC_HOOK_CODE, hook); g.hook_add(UC_HOOK_MEM_WRITE, write)
        values = [0x11223344, 0x22334455, 0x33445566, 0x44556677,
                  SP, 0x66778899, 0x778899aa, 0x8899aabb]
        for reg, value in zip(GP, values): g.reg_write(reg, value)
        g.reg_write(R.UC_X86_REG_EFLAGS, 2)
        g.reg_write(R.UC_X86_REG_FPCW, 0x27f); g.reg_write(R.UC_X86_REG_FPSW, 0)
        g.reg_write(R.UC_X86_REG_FPTAG, 0xffff); g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        g.mem_write(SP, struct.pack('<' + 'I'*(1+len(args)), STOP, *args))
        before = abi()
        g.emu_start(entry, STOP, count=1000000)
        eip = g.reg_read(R.UC_X86_REG_EIP)
        assert eip == STOP or boundaries, hex(eip)
        heap = [dict(address=p, storage=persist(bytes(g.mem_read(p, 0x1000)))) for p in sorted(committed)]
        rows.append(dict(name=name, entry=entry, args=args, before=before, after=abi(),
            boundary=boundaries, resources=resources, stores=stores, heap=heap,
            platform_calls=platform_calls, initial_xapi_flag=xapi,
            initial_data=persist(bytes(initial_data)), final_data=persist(bytes(g.mem_read(DATA, 0x1000))),
            final_stack=persist(bytes(g.mem_read(SP-0x200, 0x300))),
            final_fs=bytes(g.mem_read(0, 4)).hex()))
        if host[0] is not None:
            g.mem_unmap(RAW, extent[0]); assert libc.munmap(host[0], extent[0]) == 0
    fixture = dict(schema=1, original_sha256=PIN, tool_sha256=sha(Path(__file__).read_bytes()),
        raw_address=RAW, stack_address=STACK, stack_pointer=SP, data_address=DATA,
        native_resource_abi='Linux resource success0; preserve other GP/flags; stdcall184 return; no Xbox kernel ABI claim',
        entry_context='Standalone GP/stack/empty x87/027F/1F80 and FS:[0]=FFFFFFFF; actual initial image mutable globals, not live startup state',
        instruction_patches=[], supplied_heap_lock_tls_or_callback_outcomes=False,
        compiled_compatibility_initializer=initializer,
        cases=rows, executed_instructions=[f'{p:08x}' for p in sorted(executed)])
    out = a.output / 'fixture.json'; out.write_text(json.dumps(fixture, indent=2, sort_keys=True)+'\n')
    print(json.dumps(dict(cases=len(rows), executed=len(executed), stores=sum(len(x['stores']) for x in rows),
                         fixture_sha256=sha(out.read_bytes()))))

if __name__ == '__main__': main()
