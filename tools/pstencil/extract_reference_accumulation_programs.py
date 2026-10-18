#!/usr/bin/env python3
"""Recover May2003 bump/texture builtin programs using unchanged original CPU code.

These are declared cached-binding fixtures, stopped before GPU submission.
They establish program construction, not live material selection, target alpha,
original texture contents, or a completed accumulation renderer.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from oracle_smg_firing_1_1 import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_EIP, UC_X86_REG_FPCW

REFERENCE = Path('/mnt/SSD4TB/Leaks/Halo 2/2003/game/halo2debug.xbe')
XBE_SHA256 = '5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18'
MAP_SHA256 = '1c40d851a19dd9077b1a5cc8d84319e00150678dbfed5d6713e7e786973cc4b1'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Refusing to replace retained reference evidence')
    if hashlib.sha256(REFERENCE.read_bytes()).hexdigest() != XBE_SHA256:
        raise RuntimeError('May2003 executable differs from the reference')
    cache = REFERENCE.parent / 'maps/ec_interior_test.map'
    raw_cache = cache.read_bytes()
    if hashlib.sha256(raw_cache).hexdigest() != MAP_SHA256:
        raise RuntimeError('Resident cache differs from the reference')
    inventory = ROOT / 'analysis/pstencil/2003-presentation/resident-material-passes/passes.json'
    selected = []
    for row in json.loads(inventory.read_text())['programs']:
        name = row['tag']['name'].rsplit('\\', 1)[-1]
        if name not in ('default_bump', 'anisotropic_bump', 'default_texture', 'default_texture_zwrite'):
            continue
        field = next(f for f in row['definition']['fields'] if f['name'] == 'hack pixel shader index*')
        offset = int(field['file_offset'], 16)
        builtin = struct.unpack_from('<H', raw_cache, offset)[0]
        if builtin != field['value'] or builtin != (1 if 'bump' in name else 3):
            raise RuntimeError('Primary resident builtin selection differs')
        selected.append(dict(name=name, builtin=builtin, cache_file_offset=offset))
    if len(selected) != 4:
        raise RuntimeError('Missing resident accumulation families')

    source = Xbe(REFERENCE)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    limit = max(s.va + s.vsize for s in source.sections)
    cpu.mem_map(0x10000, (limit - 0x10000 + 4095) & ~4095)
    for section in source.sections:
        cpu.mem_write(section.va, source.section_bytes(section))
    cpu.mem_map(0x2100000, 0x30000)
    executed = set()

    def word(address, value):
        cpu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def instruction(uc, pc, size, _):
        if pc in (0x1db50, 0x1d070):
            raise RuntimeError(f'Original constructor assertion at {pc:08x}')
        if bytes(uc.mem_read(pc, size)) != source.read(pc, size):
            raise RuntimeError(f'Original instruction changed at {pc:08x}')
        executed.add((pc, size))

    cpu.hook_add(UC_HOOK_CODE, instruction)
    # Explicit fixture pointers and already-cached stage bindings. They keep
    # bitmap/driver allocation outside this constructor arithmetic experiment.
    # No return value is supplied and no original instruction/service is patched.
    word(0x743a20, 0x2110000)
    word(0x7438a4, 0x2120000)
    word(0x21100b8, 0x1111)
    word(0x21100d8, 0x2222)
    rows = []
    values = (-2., -.1, 0., .1, .25, .5, .75, .9, 1., 2.)
    for builtin in (1, 3):
        inputs = [(1., 1.)] if builtin == 1 else itertools.product(values, repeat=2)
        for first, second in inputs:
            if builtin == 1:
                cached = ((0x8198e4, 0x1111), (0x8198e8, 3),
                          (0x819a94, 0x2222), (0x819a98, 3))
            else:
                cached = ((0x8198e4, 0x1111), (0x8198e8, 1),
                          (0x819974, 0x1111), (0x819978, 2),
                          (0x819a94, 0x1111), (0x819a98, 0))
            for address, value in cached:
                word(address, value)
            cpu.mem_write(0x48f494, struct.pack('<2f', first, second))
            stack = 0x2108000
            cpu.mem_write(stack, struct.pack('<5I', 0, builtin, 0, 0, 0))
            cpu.reg_write(UC_X86_REG_ESP, stack)
            cpu.reg_write(UC_X86_REG_FPCW, 0x23f)
            cpu.emu_start(0x964b0, 0x97bb4, count=100000)
            if cpu.reg_read(UC_X86_REG_EIP) != 0x97bb4:
                raise RuntimeError('Original constructor did not reach the GPU boundary')
            program = bytes(cpu.mem_read(cpu.reg_read(UC_X86_REG_EBP) - 0x104, 240))
            rows.append(dict(builtin=builtin, global_48f494=first, global_48f498=second,
                             program_words=list(struct.unpack('<60I', program)),
                             program_sha256=hashlib.sha256(program).hexdigest()))
    result = dict(scope=__doc__, original=str(REFERENCE), original_sha256=XBE_SHA256,
                  cache=str(cache), cache_sha256=MAP_SHA256, resident_selection=selected,
                  entry='000964b0', stop_before='00097bb4', fpcw='023f',
                  instruction_patches=[], replaced_services=[], fixtures=rows,
                  instructions=[dict(address=f'{pc:08x}', size=size,
                                     sha256=hashlib.sha256(source.read(pc, size)).hexdigest())
                                for pc, size in sorted(executed)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(fixtures=len(rows), unique_programs=len({r['program_sha256'] for r in rows}),
                          authenticated_instructions=len(executed), output=str(args.output))))


if __name__ == '__main__':
    main()
