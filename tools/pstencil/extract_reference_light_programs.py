#!/usr/bin/env python3
"""Execute original utility/light program construction on declared input rows.

This recovers complete builtin 6/7/10 programs before GPU submission. The
context selector fields and renderer globals are explicit fixtures, not a
captured original scene. No instructions or services are replaced.
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Refusing to replace retained reference evidence')
    reference = Path('/mnt/SSD4TB/Leaks/Halo 2/2003/game/halo2debug.xbe')
    executable_hash = hashlib.sha256(reference.read_bytes()).hexdigest()
    if executable_hash != '5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18':
        raise RuntimeError('Primary executable differs')
    cache = (reference.parent / 'maps/ec_interior_test.map').read_bytes()
    cache_hash = hashlib.sha256(cache).hexdigest()
    if cache_hash != '1c40d851a19dd9077b1a5cc8d84319e00150678dbfed5d6713e7e786973cc4b1':
        raise RuntimeError('Primary resident cache differs')
    inventory = ROOT / 'analysis/pstencil/2003-presentation/resident-material-passes/passes.json'
    selected = []
    for row in json.loads(inventory.read_text())['programs']:
        field = next(f for f in row['definition']['fields'] if f['name'] == 'hack pixel shader index*')
        offset = int(field['file_offset'], 16)
        builtin = struct.unpack_from('<H', cache, offset)[0]
        if builtin != field['value']:
            raise RuntimeError('Decoded resident builtin differs')
        if builtin in (6, 7, 10):
            selected.append(dict(pass_=row['tag'], builtin=builtin, cache_file_offset=offset))
    if {row['builtin'] for row in selected} != {6, 7, 10}:
        raise RuntimeError('Original utility/light resident selections missing')

    image = Xbe(reference)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    limit = max(s.va + s.vsize for s in image.sections)
    cpu.mem_map(0x10000, (limit - 0x10000 + 4095) & ~4095)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    cpu.mem_map(0x2100000, 0x30000)
    instructions = set()

    def instruction(uc, pc, size, _):
        if pc in (0x1db50, 0x1d070):
            raise RuntimeError(f'Original assertion at {pc:08x}')
        if bytes(uc.mem_read(pc, size)) != image.read(pc, size):
            raise RuntimeError(f'Original instruction changed at {pc:08x}')
        instructions.add((pc, size))

    cpu.hook_add(UC_HOOK_CODE, instruction)
    cpu.mem_write(0x7438a4, struct.pack('<I', 0x2120000))
    # Original 952C0 accepts context values 0..3 and tests equality with zero.
    # A4C50 supplies this from the live light geometry record, not a shader
    # lighting-model row. Keep literal context names pending its full crosswalk.
    fixtures = []
    for builtin, model, first, second, quality in itertools.product(
            (6, 10), range(4), range(4), range(4), (-1, 0, 1, 2)):
        fixtures.append(dict(builtin=builtin, context_7438a8=model,
                             context_u176=first, context_u178=second,
                             global_48f346=quality))
    for model, selector, global_selector in itertools.product(range(4), range(4), range(3)):
        fixtures.append(dict(builtin=7, context_7438a8=model, context_u0dc=selector,
                             global_7439dc=global_selector))
    rows = []
    for fixture in fixtures:
        cpu.mem_write(0x2120000, bytes(0x200))
        cpu.mem_write(0x7438a8, struct.pack('<H', fixture['context_7438a8']))
        for key, offset in (('context_u176', 0x176), ('context_u178', 0x178), ('context_u0dc', 0xdc)):
            cpu.mem_write(0x2120000 + offset, struct.pack('<H', fixture.get(key, 0)))
        cpu.mem_write(0x48f346, bytes([fixture.get('global_48f346', 0) & 255]))
        cpu.mem_write(0x7439dc, struct.pack('<I', fixture.get('global_7439dc', 0)))
        cpu.mem_write(0x7439bc, struct.pack('<3f', .2, .5, .8))
        cpu.mem_write(0x7439cc, struct.pack('<3f', .3, .2, .7))
        stack = 0x2108000
        cpu.mem_write(stack, struct.pack('<5I', 0, fixture['builtin'], 0, 0, 0))
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_FPCW, 0x23f)
        cpu.emu_start(0x964b0, 0x97bb4, count=100000)
        if cpu.reg_read(UC_X86_REG_EIP) != 0x97bb4:
            raise RuntimeError('Original constructor did not reach its GPU boundary')
        program = bytes(cpu.mem_read(cpu.reg_read(UC_X86_REG_EBP) - 0x104, 240))
        rows.append(dict(inputs=fixture, words=list(struct.unpack('<60I', program)),
                         sha256=hashlib.sha256(program).hexdigest()))
    result = dict(scope=__doc__, original_sha256=executable_hash, cache_sha256=cache_hash,
        entry='000964b0', stop_before='00097bb4', fpcw='023f',
        resident_selection=selected, instruction_patches=[], replaced_services=[],
        declared_colors={'007439bc': [.2, .5, .8], '007439cc': [.3, .2, .7]},
        fixtures=rows, instructions=[dict(address=f'{pc:08x}', size=size,
            sha256=hashlib.sha256(image.read(pc, size)).hexdigest()) for pc, size in sorted(instructions)],
        full_presentation_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(fixtures=len(rows), authenticated_instructions=len(instructions),
        unique_programs={str(b): len({r['sha256'] for r in rows if r['inputs']['builtin'] == b})
                         for b in (6, 7, 10)})))


if __name__ == '__main__':
    main()
