#!/usr/bin/env python3
"""Execute whole original274E30 object/property owner lookup.

Declared object, relative-part and property-pool fields cover direct full-handle
matches, related-owner matches, rejected aliases and list exhaustion. Both
signed relative offsets and the null-result guard are exercised. No original
instructions, dependency returns, runtime actor records or results are supplied.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_campaign_ai_placement_settings_tu15 import GP

ENTRY = 0x274e30
BASE, LOW, OBJECT, STACK, STOP = 0x2000000, 0x4000, 0x6000, 0x200f000, 0x3000000
OH, PH, LH, OBJECT_ROWS, PROPS, LISTS = (BASE + p for p in (0x100, 0x200, 0x300, 0x1000, 0x3000, 0x5000))
CELLS = (0x4ecbd0, 0x50eef4, 0x50eef8)
BANKS = [[BASE, 0x10000], [LOW, 0x4000]] + [[p, 4] for p in CELLS]
CHAINS = ('empty', 'direct_first', 'direct_last', 'related_first', 'related_last',
          'flag_zero', 'related_mismatch', 'wrong_object_salt', 'exhaust', 'invalid_related_owner')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    body = _trace(code, ENTRY, {ENTRY})
    assert len(body.code) == 56 and not body.calls and not body.tail_jumps and not body.tables
    cases, covered, returns = [], set(), set()
    profiles = itertools.product((0, 12, 255), (0, 0xffffffff), (0x200, -0x200, -OBJECT), CHAINS, (0, 3), (0, 0xa5))
    for kind, root, offset, chain, index, poison in profiles:
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x10000)
        g.mem_map(LOW, 0x4000)
        g.mem_map(STOP, 4096)
        for at, n in BANKS:
            g.mem_write(at, bytes([poison]) * n)
        patches = []

        def put(at, value, n=4):
            raw = (value & ((1 << (n * 8)) - 1)).to_bytes(n, 'little')
            patches.append([at, raw.hex()])
            g.mem_write(at, raw)

        object_id = (0xabcd0000 if index else 0xfedc0000) | index
        list_id = 0xecd30000 | index
        related = 0xffffffff if chain == 'invalid_related_owner' else 0x72340019
        links = (0xace10001, 0xfed20002, 0x87330003)
        for at, value in ((0x4ecbd0, OH), (OH + 0x44, OBJECT_ROWS),
                          (OBJECT_ROWS + index * 12 + 8, OBJECT),
                          (0x50eef4, PH), (PH + 0x44, PROPS),
                          (0x50eef8, LH), (LH + 0x44, LISTS),
                          (OBJECT + 0x134, root), (STACK, STOP)):
            put(at, value)
        put(OBJECT + 0xaa, kind, 1)
        put(OBJECT + 0x13a, offset, 2)
        if OBJECT + offset:
            put(OBJECT + offset + 4, related)
        put(LISTS + index * 0x50 + 0x14, 0xffffffff if chain == 'empty' else links[0])
        for i, handle in enumerate(links):
            row = PROPS + (handle & 0xffff) * 0xc4
            candidate = object_id ^ 0x10000  # Same index, different full object identity.
            match = (chain == 'direct_first' and i == 0) or (chain == 'direct_last' and i == 2)
            if match:
                candidate = object_id
            flag = 0 if chain in ('flag_zero', 'wrong_object_salt', 'exhaust') else (1 if i != 1 else 0x80)
            related_candidate = related if chain in ('related_first', 'related_last', 'flag_zero', 'invalid_related_owner') else related ^ 1
            if chain == 'related_last' and i != 2:
                related_candidate ^= 1
            for at, value in ((row + 8, candidate), (row + 0x14, links[i + 1] if i != 2 else 0xffffffff),
                              (row + 0x1c, related_candidate)):
                put(at, value)
            put(row + 0x22, flag, 1)

        initial = [list_id, 0x22334455, 0x33445566, 0x44556677, STACK, 0x55667788, 0x66778899, object_id]
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

        def instruction(_g, pc, size, _u):
            assert pc in body.code and bytes(g.mem_read(pc, size)) == image.read(pc, size), hex(pc)
            trace.append(pc)
            covered.add(pc)

        def read(_g, _kind, at, n, _value, _u):
            assert any(p <= at < at + n <= p + size for p, size in BANKS), ('Undeclared input', hex(at), n)

        def store(_g, _kind, at, n, value, _u):
            assert STACK - 12 <= at < at + n <= STACK
            stores.append([g.reg_read(R.UC_X86_REG_EIP), at, bytes(g.mem_read(at, n)).hex(),
                           (value & ((1 << (n * 8)) - 1)).to_bytes(n, 'little').hex()])

        g.hook_add(UC_HOOK_CODE, instruction)
        g.hook_add(UC_HOOK_MEM_READ, read)
        g.hook_add(UC_HOOK_MEM_WRITE, store)
        g.emu_start(ENTRY, STOP, count=1000)
        assert g.reg_read(R.UC_X86_REG_EIP) == STOP
        returns.add(g.reg_read(R.UC_X86_REG_EAX))
        cases.append(dict(entry=ENTRY, prefix_end=None, kind=kind, root=root, offset=offset, chain=chain,
                          index=index, poison=poison, patches=patches, gp=initial, xmm=xmm, trace=trace, stores=stores,
                          final_gp=[g.reg_read(reg) for reg in GP], final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                          final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0 + i).to_bytes(16, 'little'))) for i in range(8)],
                          final_mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),
                          outputs=[[at, bytes(g.mem_read(at, n)).hex()] for at, n in
                                   ((OBJECT - 0x200, 0x500), (PROPS, 0x320), (LISTS + index * 0x50, 0x50), (STACK - 0x40, 0x44))]))
    assert covered == set(body.code), sorted(set(body.code) - covered)
    assert returns == {0xffffffff, links[0], links[2]}, sorted(returns)
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, instruction_patches=[], supplied_dependency_returns=[],
                   native_component_banks=BANKS, immutable_operands=[], cases=cases)
    raw = (json.dumps(fixture, separators=(',', ':')) + '\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == raw, 'Preserve original property owner lookup reference'
    else:
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_bytes(raw)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, cases=len(cases), whole_cases=len(cases), dependency_prefixes=0,
                  entry=ENTRY, original_instruction_count=len(body.code), observed_instruction_count=len(covered), unexecuted=[],
                  source_return_handles=sorted(returns), signed_offsets=[0x200, -0x200, -OBJECT], declared_low_object_pointer=OBJECT,
                  fixture_bytes=len(raw), fixture_sha256=hashlib.sha256(raw).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), live_actor_or_combat_qualified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
