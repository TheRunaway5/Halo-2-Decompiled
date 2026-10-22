#!/usr/bin/env python3
"""Original decision-workspace/query component cases, without service results.

Execute unchanged TU1.5 instructions from independent actor/property/pool and
caller inputs. The1B7D20 caller stops before its planning-query handoff; the
26AF90 component stops before its candidate-selector service. Other paths run
until return or the first required external call. No collision, navigation,
allocation or target-selection result is supplied. Unexecuted body branches
remain explicitly unqualified.
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
from oracle_campaign_ai_placement_settings_tu15 import GP
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from export_final_startup_storage_data_tu15 import regions

BASE, STACK, STOP = 0x2000000, 0x20ef000, 0x3000000
AH, PH, VH, QH, OH, ACTORS, PROPS, VIEWS, QUERIES, OBJECTS, OBJECT, OUTPUT, INPUT, TAGS, TAG, PARAM = (
    BASE + p for p in (0x100, 0x200, 0x300, 0x400, 0x500, 0x1000, 0x4000, 0x5000,
                       0x6000, 0xa000, 0xb000, 0xc000, 0xd000, 0xe000, 0xf000, 0x10000))
ACTOR, PROP = ACTORS + 2 * 0x888, PROPS + 0x3c
ACTOR_ID, PROP_ID, QUERY_ID, OBJECT_ID = 0xe3610002, 0xf2810001, 0xe4510001, 0xf2700005
CELLS = ((0x5020c8, 4), (0x50eef0, 4), (0x50eef4, 4), (0x50eeec, 4),
         (0x52d87c, 4), (0x4ecbd0, 4), (0x4f0414, 4), (0x4f3cd8, 4),
         (0x51df54, 4), (0x47d6c0, 4), (0x47d636, 2), (0x52be58, 1),
         (0x52be54, 4), (0x51df44, 4), (0x51df48, 1), (0x50208c, 24))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--generation', type=Path, required=True)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    assert 'HALO2_TEST_SCRATCH' in os.environ
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    generation = json.loads(args.generation.read_text())
    assert generation['original_xbe_sha256'] == NEW_PIN
    entries = {int(p, 16) for p in generation['entry_instructions']} | set(generation['reused_frame_entries'])
    code = _Code(image, '.text')
    bodies = {entry: _trace(code, entry, entries) for entry in entries}
    sites = set().union(*(set(body.code) for body in bodies.values()))
    covered, points, cases = set(), set(), []
    specs = []
    for poison in (0x5a, 0xa5):
        specs += [dict(entry=0x1b7d20, alert=alert, optional=optional, poison=poison)
                  for alert, optional in itertools.product((0, 1), (0, 1))]
        specs += [dict(entry=0x26af90, last=last, active=active, poison=poison)
                  for last, active in itertools.product((0xffffffff, QUERY_ID), (0, 1))]
        specs += [dict(entry=0x1b5220, object=valid, old=old, poison=poison)
                  for valid, old in itertools.product((False, True), (False, True))]
        specs += [dict(entry=0x277860, mask=mask, match=match, selector=selector, poison=poison)
                  for mask, match, selector in itertools.product((0, 1, 2, 5, 7), (-1, 0, 1, 2), (-1, 0, 1))]
        specs += [dict(entry=0x2778e0, previous=previous, count=count, poison=poison)
                  for previous, count in itertools.product((0xffffffff, QUERY_ID), (0, 1, 65535))]
        specs += [dict(entry=0x277210, count=count, flags=flags, poison=poison)
                  for count, flags in itertools.product((-1, 0, 1), (0, 4))]
        specs += [dict(entry=0x2776d0, selector=selector, cached=cached, poison=poison)
                  for selector, cached in itertools.product((-32768, -1, 0, 1, 31, 32, 32767), (False, True))]
        specs += [dict(entry=0x277c50, object=valid, poison=poison) for valid in (False, True)]
        specs += [dict(entry=0x277e90, kind=kind, poison=poison) for kind in (0, 1, 2)]
        specs += [dict(entry=0x2a7750, selector=selector, actor=actor, alternate=alternate, poison=poison)
                  for selector, actor, alternate in itertools.product((-32768, -1, 0, 1, 2, 3, 4, 32767), (False, True), (0, 1))]
        specs += [dict(entry=entry, poison=poison) for entry in (0x1fcce0, 0x277100, 0x277da0, 0x276fa0)]
    for ordinal, spec in enumerate(specs):
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x100000)
        g.mem_write(BASE, bytes([spec['poison']]) * 0x100000)
        g.mem_map(STOP, 4096)
        patches = []

        def raw(at, value):
            g.mem_write(at, value)
            patches.append([at, value.hex()])

        def put(at, value, size=4):
            raw(at, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))

        initial = [0x11223344, 0x22334455, 0x33445566, 0x44556677, STACK, 0x55667788, 0x66778899, 0x778899aa]
        for at, value in ((STACK, STOP), (STACK+4, ACTOR_ID), (STACK+8, INPUT),
                          (STACK+12, OUTPUT), (STACK+16, OUTPUT+0x40), (STACK+20, OUTPUT+0x80),
                          (STACK+24, OUTPUT+0xc0), (0x5020c8, AH), (AH+0x44, ACTORS),
                          (0x50eef0, PH), (PH+0x44, PROPS), (0x50eef4, VH), (VH+0x44, VIEWS),
                          (0x50eeec, QH), (QH+0x44, QUERIES), (0x52d87c, QH),
                          (QH+0x38, 3), (QH+0x24, 0x484), (QH+0x48, BASE+0x11000),
                          (0x4ecbd0, OH), (OH+0x44, OBJECTS), (OBJECTS+5*12+8, OBJECT),
                          (0x4f0414, TAGS), (TAGS+3*16+8, TAG), (OBJECT, 0xabcd0003),
                          (TAG+0x5c, spec.get('count', 0)), (TAG+0x60, PARAM),
                          (ACTOR+0x18, OBJECT_ID if spec.get('object', False) else 0xffffffff),
                          (ACTOR+0x54, 0xffffffff), (ACTOR+0x338, PROP_ID), (PROP+8, 0xabcd0000),
                          (PROP+0x14, 0xffffffff), (0x47d6c0, 0xffffffff),
                          (ACTOR+0x418, spec.get('last', 0xffffffff)),
                          (ACTOR+0x3f4, spec.get('previous', 0xffffffff)),
                          (ACTOR+0x3f8, QUERY_ID if spec.get('old', False) else 0xffffffff),
                          (0x4f3cd8, BASE+0x12000), (BASE+0x12000, 0x12345678),
                          (0x51df54, BASE+0x13000), (0x51df44, BASE+0x20000),
                          (0x52be54, 0), (BASE+0x11000, spec.get('mask', 0))):
            put(at, value)
        put(PROP+0x18, 0, 2)
        put(0x47d636, 1, 2)
        put(ACTOR+0x3c, spec.get('alert', 0), 1)
        put(ACTOR+0x227, spec.get('optional', 0), 1)
        put(ACTOR+0x3f0, spec.get('active', 0), 1)
        put(0x52be58, 1, 1)
        put(0x51df48, 1, 1)
        for i in range(2):
            put(0x50208c+i*12, 0, 1)
            put(0x502090+i*12, BASE+0x20000+i*0x22974)
            put(0x502094+i*12, 0x22974)
        for i in range(3):
            put(QUERIES+i*0x484, 0x6000+i, 2)
            put(QUERIES+i*0x484+2, i, 2)
            put(QUERIES+i*0x484+4, ACTOR_ID if i == spec.get('match', -1) else 0xffffffff)
            put(QUERIES+i*0x484+0xa, spec.get('count', 0), 2)
        entry = spec['entry']
        if entry == 0x1b7d20:
            initial[1] = OUTPUT
        elif entry == 0x26af90:
            initial[0] = INPUT
            put(STACK+8, OUTPUT)
        elif entry == 0x277860:
            put(STACK+8, spec['selector'], 2)
        elif entry == 0x2778e0:
            initial[1], initial[2] = ACTOR_ID, QUERY_ID
        elif entry == 0x277210:
            initial[2] = OBJECT_ID
            put(PARAM, spec['flags'], 1)
        elif entry == 0x2776d0:
            initial[0], initial[1], initial[2] = QUERY_ID, spec['selector'] & 0xffffffff, INPUT
            put(QUERIES+0x484+0x40, (1 << (spec['selector'] & 31)) if spec['cached'] else 0)
        elif entry == 0x277c50:
            initial[0], initial[1] = ACTOR_ID, OBJECT_ID if spec['object'] else 0xffffffff
        elif entry == 0x277e90:
            initial[0], initial[1] = QUERY_ID, INPUT
            put(QUERIES+0x484+2, spec['kind'], 2)
            raw(INPUT+0x48, struct.pack('<3f', 0, 0, 0))
        elif entry == 0x2a7750:
            initial[0], initial[1], initial[2] = spec['alternate'], ACTOR_ID if spec['actor'] else 0xffffffff, spec['selector'] & 0xffffffff
            put(STACK+4, OUTPUT)
            put(STACK+8, 7)
            put(STACK+12, INPUT)
            raw(INPUT, struct.pack('<3f', 0, 0, 0))
            raw(OUTPUT, struct.pack('<3f', 0, 0, 0))
        elif entry == 0x276fa0:
            initial[1], initial[2], initial[3], initial[7] = INPUT, INPUT+0x10, OUTPUT, 7
            raw(INPUT, struct.pack('<3f', 0, 0, 0))
            raw(INPUT+0x10, struct.pack('<3f', 0, 0, 0))
        elif entry == 0x277100:
            put(STACK+4, OBJECT_ID)
        elif entry == 0x1fcce0:
            initial[0] = INPUT
        xmm = [[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg, value in zip(GP, initial):
            g.reg_write(reg, value)
        for i, lanes in enumerate(xmm):
            g.reg_write(R.UC_X86_REG_XMM0+i, int.from_bytes(struct.pack('<4I', *lanes), 'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        g.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        g.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        trace, stores, pending = [], [], []
        boundary = dependency = None

        def flush():
            for pc, at, before in pending:
                stores.append([pc, at, before, bytes(g.mem_read(at, len(before)//2)).hex()])
            pending.clear()

        def instruction(_g, pc, size, _u):
            nonlocal boundary, dependency
            flush()
            ins = code.at(pc)
            assert pc in sites and bytes(g.mem_read(pc, size)) == image.read(pc, size), hex(pc)
            if ins.mnemonic == 'call' and (pc in (0x1b7e59, 0x26afe9, 0x1b5287,
                    0x27711c, 0x1fcd10, 0x277dbd, 0x2a77db, 0x27772f, 0x277c76) or
                    ins.operands[0].type != X86_OP_IMM or ins.operands[0].imm not in entries):
                boundary = pc
                dependency = ins.operands[0].imm if ins.operands[0].type == X86_OP_IMM else 'indirect'
                g.emu_stop()
                return
            if not trace or trace[-1] != pc:
                trace.append(pc)
            covered.add(pc)

        def store(_g, _kind, at, size, _value, _u):
            assert BASE <= at < at+size <= BASE+0x100000 or any(p <= at < at+size <= p+n for p, n in CELLS), hex(at)
            pending.append((g.reg_read(R.UC_X86_REG_EIP), at, bytes(g.mem_read(at, size)).hex()))

        def read(_g, _kind, at, size, _value, _u):
            section = next((s for s in image.sections if s.va <= at < at+size <= s.va+s.vsize), None)
            if section and section.name == '.rdata':
                points.update(range(at, at+size))
            elif section and section.name == '.data':
                assert any(p <= at < at+size <= p+n for p, n in CELLS), ('Undeclared original field', hex(at), size, spec)

        g.hook_add(UC_HOOK_CODE, instruction)
        g.hook_add(UC_HOOK_MEM_READ, read)
        g.hook_add(UC_HOOK_MEM_WRITE, store)
        try:
            g.emu_start(entry, STOP, count=50000)
            flush()
        except Exception as error:
            raise RuntimeError(f'Original case {ordinal} {spec} PC={g.reg_read(R.UC_X86_REG_EIP):x}') from error
        assert g.reg_read(R.UC_X86_REG_EIP) == (boundary or STOP), spec
        cases.append(dict(spec, prefix_end=boundary, dependency=dependency, patches=patches,
                          gp=initial, xmm=xmm, trace=trace, stores=stores,
                          final_gp=[g.reg_read(reg) for reg in GP], final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                          final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16, 'little'))) for i in range(8)],
                          x87_depth=sum(((g.reg_read(R.UC_X86_REG_FPTAG) >> (i*2)) & 3) != 3 for i in range(8)),
                          outputs=[[ACTOR+0x3f4, bytes(g.mem_read(ACTOR+0x3f4, 12)).hex()],
                                   [OUTPUT, bytes(g.mem_read(OUTPUT, 0x40)).hex()],
                                   [QUERIES+0x484, bytes(g.mem_read(QUERIES+0x484, 0x44)).hex()]]))
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, instruction_patches=[], supplied_dependency_returns=[],
                   native_component_banks=[[BASE, 0x100000], *CELLS],
                   immutable_operands=[[p, image.read(p, n).hex()] for p, n in regions(points)], cases=cases)
    raw_bytes = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == raw_bytes, 'Preserve original decision-workspace reference'
    else:
        args.fixture.write_bytes(raw_bytes)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, cases=len(cases),
                  whole_cases=sum(c['prefix_end'] is None for c in cases),
                  prefix_boundaries=sorted({(c['entry'], c['prefix_end'], c['dependency']) for c in cases if c['prefix_end']}, key=str),
                  entry_coverage={f'{p:08x}': dict(observed=len(set(body.code) & covered), static=len(body.code))
                                  for p, body in bodies.items() if set(body.code) & covered},
                  fixture_bytes=len(raw_bytes), fixture_sha256=hashlib.sha256(raw_bytes).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), full_navigation_or_combat_qualified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'entry_coverage'}))


if __name__ == '__main__':
    main()
