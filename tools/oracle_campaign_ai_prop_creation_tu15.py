#!/usr/bin/env python3
"""Execute original property creation with real classification/allocation/init.

Whole274450/274490 reuse273B70,274200,274020 and their unchanged original
lookup, allocator, team and constructor bodies. Declared pools exercise new,
recycled and full allocation; original counter/stamps cover wrap and duplicate
classification. No instruction patches, dependency returns or allocations are
supplied. Unexecuted dependency branches remain explicit.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from capstone.x86 import X86_OP_IMM
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP
from export_final_startup_storage_data_tu15 import regions

NEW = {0x274450, 0x274490}
ENTRIES = NEW | {0x273b70, 0x274200, 0x274020, 0xbcdf0, 0xbbe00, 0x16e6a0, 0x2660f0, 0x1e7810}
BASE, STACK, STOP = 0x2000000, 0x203f000, 0x3000000
OH, PH, CONTEXT, CLOCK = (BASE + p for p in (0x100, 0x200, 0x300, 0x400))
OBJECT_ROWS, UNIT, RECIPIENT, BITMAP, PROPS, DESCRIPTOR = (BASE + p for p in (0x4000, 0xb000, 0xc000, 0xd000, 0xe000, 0xf000))
COLD = ((0x4eabc8, 1), (0x4eabcc, 4), (0x4eabd0, 8192))
CELLS = (0x4ecbd0, 0x50eef4, 0x5020a8, 0x51df54, 0x474ce8)
BANKS = [[BASE, 0x40000]] + [list(r) for r in COLD] + [[p, 4] for p in CELLS]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    ap.add_argument('--admissible-pools', action='store_true',
                    help='Use bitmap-consistent live counts; retain the older declared-count reference unchanged')
    args = ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    bodies = {p: _trace(code, p, ENTRIES) for p in ENTRIES}
    sites = set().union(*(set(b.code) for b in bodies.values()))
    cases, covered, points, prefixes = [], set(), set(), []
    profiles = itertools.product(sorted(NEW), (0, 2047), ('new', 'recycle', 'full'), (0, 12),
                                 (0x1234, 0xfffe), ((0, 0), (0xffffffff, 0), (7, 8)), (0, 0xa5))
    specs = []
    for entry, index, pool, kind, salt, (epoch, stamp), poison in profiles:
        for priority in ((-1, 0, 3) if entry == 0x274490 else (0,)):
            specs.append(dict(entry=entry, index=index, pool=pool, kind=kind, salt=salt,
                              epoch=epoch, stamp=stamp, poison=poison, priority=priority))
    for spec in specs:
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x40000)
        g.mem_map(STOP, 4096)
        for at, n in BANKS:
            g.mem_write(at, bytes([spec['poison']]) * n)
        patches = []

        def raw(at, data):
            patches.append([at, data.hex()])
            g.mem_write(at, data)

        def put(at, value, n=4):
            raw(at, (value & ((1 << (n * 8)) - 1)).to_bytes(n, 'little'))

        index, kind = spec['index'], spec['kind']
        object_id = 0xabcd0000 | index
        allocation = dict(new=(0, 0, 0), recycle=(0, 3, 5), full=(3, 3, 7))[spec['pool']]
        for at, value in ((0x4ecbd0, OH), (OH + 0x20, 2048), (OH + 0x24, 12),
                          (OH + 0x38, 2048), (OH + 0x44, OBJECT_ROWS),
                          (OBJECT_ROWS + index * 12 + 8, UNIT), (0x50eef4, PH),
                          (PH + 0x20, 3), (PH + 0x24, 0xc4), (PH + 0x34, allocation[0]),
                          (PH + 0x38, allocation[1]),
                          (PH + 0x3c, allocation[2].bit_count() if args.admissible_pools else allocation[1]),
                          (PH + 0x44, PROPS), (PH + 0x48, BITMAP), (BITMAP, allocation[2]),
                          (0x5020a8, CONTEXT), (0x51df54, CLOCK), (CLOCK + 8, 100),
                          (UNIT + 0x14, 0xffffffff), (UNIT + 0x12c, 0xffffffff),
                          (UNIT + 0x134, 0), (UNIT + 0x13c, 0xffffffff),
                          (UNIT + 0x204, 0x93610005), (UNIT + 0x208, 0xffffffff),
                          (0x4eabcc, spec['epoch']), (0x4eabd0 + index * 4, spec['stamp']),
                          (DESCRIPTOR + 4, object_id), (STACK, STOP)):
            put(at, value)
        put(OBJECT_ROWS + index * 12, 0xabcd, 2)
        put(OBJECT_ROWS + index * 12 + 3, kind, 1)
        put(PH + 0x40, spec['salt'], 2)
        put(CONTEXT + 1, 0, 1)
        put(0x4eabc8, 0, 1)
        put(UNIT + 0xaa, kind, 1)
        put(UNIT + 0xb2, 0, 1)
        put(UNIT + 0x138, 0xffff, 2)
        put(UNIT + 0x12e, 0xffff, 2)
        put(UNIT + 0x13a, 0x200, 2)
        put(RECIPIENT + 0x12, 0xffff, 2)
        put(RECIPIENT + 0x24, 0, 2)
        put(RECIPIENT + 0x2c, 0x40800000)
        put(DESCRIPTOR, 4, 2)
        put(DESCRIPTOR + 8, spec['priority'], 2)
        raw(UNIT + 0x64, bytes(12))
        raw(RECIPIENT + 4, bytes(12))
        raw(0x474ce8, image.read(0x474ce8, 4))
        initial = [0x11223344, object_id, 0x33445566, 0x44556677, STACK, 0x55667788, RECIPIENT, 0x778899aa]
        if spec['entry'] == 0x274490:
            initial[3] = DESCRIPTOR
            put(STACK + 4, RECIPIENT)
        xmm = [[0x3f000000 + i * 4 + j for j in range(4)] for i in range(8)]
        for reg, value in zip(GP, initial):
            g.reg_write(reg, value)
        for i, lanes in enumerate(xmm):
            g.reg_write(R.UC_X86_REG_XMM0 + i, int.from_bytes(struct.pack('<4I', *lanes), 'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        g.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        g.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        trace, stores = [], []
        prefix = dependency = None

        def instruction(_g, pc, size, _u):
            nonlocal prefix, dependency
            assert pc in sites and bytes(g.mem_read(pc, size)) == image.read(pc, size), (spec, hex(pc))
            ins = code.at(pc)
            if ins.mnemonic == 'call' and (ins.operands[0].type != X86_OP_IMM or ins.operands[0].imm not in ENTRIES):
                prefix, dependency = pc, ins.operands[0].imm if ins.operands[0].type == X86_OP_IMM else 'indirect'
                g.emu_stop()
                return
            if not trace or trace[-1] != pc:
                trace.append(pc)
            covered.add(pc)

        def read(_g, _kind, at, n, _value, _u):
            if any(p <= at < at + n <= p + size for p, size in BANKS):
                return
            section = next((s for s in image.sections if s.va <= at < at + n <= s.va + s.vsize), None)
            type_field = any(0x47d632 + i * 16 <= at < at + n <= 0x47d63a + i * 16 for i in range(9))
            assert section and (section.name in ('.rdata', '.text') or type_field), ('Undeclared input', spec, hex(at), n)
            points.update(range(at, at + n))

        def store(_g, _kind, at, n, value, _u):
            assert any(p <= at < at + n <= p + size for p, size in BANKS), (spec, hex(at))
            stores.append([g.reg_read(R.UC_X86_REG_EIP), at, bytes(g.mem_read(at, n)).hex(),
                           (value & ((1 << (n * 8)) - 1)).to_bytes(n, 'little').hex()])

        g.hook_add(UC_HOOK_CODE, instruction)
        g.hook_add(UC_HOOK_MEM_READ, read)
        g.hook_add(UC_HOOK_MEM_WRITE, store)
        try:
            g.emu_start(spec['entry'], STOP, count=10000)
        except Exception as error:
            raise RuntimeError(f"Original creation {spec} PC={g.reg_read(R.UC_X86_REG_EIP):x}") from error
        assert g.reg_read(R.UC_X86_REG_EIP) == (prefix or STOP), spec
        if prefix:
            prefixes.append((spec['entry'], prefix, dependency))
        cases.append(dict(spec, prefix_end=prefix, dependency=dependency, patches=patches, gp=initial, xmm=xmm,
                          final_gp=[g.reg_read(reg) for reg in GP], final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                          final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0 + i).to_bytes(16, 'little'))) for i in range(8)],
                          trace=trace, stores=stores, outputs=[[at, bytes(g.mem_read(at, n)).hex()] for at, n in
                          ((0x4eabc8, 1), (0x4eabcc, 4), (0x4eabd0 + index * 4, 4), (PH, 0x4c),
                           (PROPS, 3 * 0xc4), (BITMAP, 4), (UNIT, 0x300), (RECIPIENT, 0x50), (STACK - 0x100, 0x108))]))
    assert all(set(bodies[p].code) <= covered for p in NEW), {hex(p): sorted(set(bodies[p].code) - covered) for p in NEW}
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, instruction_patches=[], supplied_dependency_returns=[],
                   native_component_banks=BANKS, cold=COLD, immutable_operands=[[p, image.read(p, n).hex()] for p, n in regions(points)], cases=cases)
    raw_bytes = (json.dumps(fixture, separators=(',', ':')) + '\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == raw_bytes, 'Preserve original property-creation reference'
    else:
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_bytes(raw_bytes)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, cases=len(cases), whole_cases=len(cases) - len(prefixes),
                  dependency_prefixes=len(prefixes), prefix_boundaries=sorted(set(prefixes), key=str),
                  entry_coverage={f'{p:08x}': dict(observed=len(covered & set(b.code)), static=len(b.code), unexecuted=sorted(set(b.code) - covered))
                                  for p, b in sorted(bodies.items())}, new_bodies=sorted(NEW), new_original_instruction_count=50,
                  fixture_bytes=len(raw_bytes), fixture_sha256=hashlib.sha256(raw_bytes).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), full_pool_reclamation_or_campaign_qualified=False)
    report['bitmap_consistent_live_counts'] = args.admissible_pools
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
