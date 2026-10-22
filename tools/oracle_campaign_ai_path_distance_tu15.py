#!/usr/bin/env python3
"""Execute original200C00 path-distance guards and world-space accumulation.

Actor position/path records are declared caller inputs. Original x87 arithmetic
produces all distances and stores. Reference-frame requests stop before the real
218780 call; no transform, path, dependency return or distance is supplied.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from oracle_campaign_ai_placement_settings_tu15 import GP
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).resolve().parents[1]
ENTRY, BASE, STACK, STOP = 0x200c00, 0x2000000, 0x2003f00, 0x3000000
HEADER, ROWS = BASE+0x100, BASE+0x1000
ACTOR = ROWS+2*0x888
BANKS = ((BASE, 0x4000), (0x5020c8, 4))
PATHS = (
    ((0, 0, 0), ((3, 4, 0), (3, 4, 12), (0, 0, 0))),
    ((1, -2, 3), ((1, -2, 3), (1, -2, 3), (1, -2, 3))),
    ((-2, 3, -4), ((5, -7, 11), (-13, 17, -19), (23, -29, 31))),
    ((.125, -.25, .5), ((.75, 1.125, -.375), (-.5, .25, -.125), (0, 0, 0))),
    ((1e-15, -1e-15, 0), ((-1e-15, 0, 1e-15), (0, 2e-15, -3e-15), (4e-15, 0, 0))),
    ((1e15, -1e15, 0), ((-1e15, 0, 1e15), (0, 2e15, -3e15), (4e15, 0, 0))),
)


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
    assert len(body.code) == 84 and set(body.calls) == {0x218780} and not body.tail_jumps
    source = (ROOT/'src/campaign/ai/source_campaign_ai_actor_behavior_tu15_generated.cpp').read_text()
    a = source.index('static void run_200c00(WorldState &s) {')
    native = source[a:source.index('\n}', a)+2]
    assert {int(v, 16) for v in re.findall(r'services\.instruction\(0x([0-9a-f]+)u\)', native)} == set(body.code)
    specs = [dict(path=path, start=start, end=end, active=1, state=1, frame=-1)
             for path, (start, end) in itertools.product(range(len(PATHS)),
                 ((0, 0), (0, 1), (0, 3), (1, 3), (3, 1), (2, 2), (2, 3)))]
    specs += [dict(path=0, start=0, end=3, active=active, state=state, frame=-1)
              for active, state in itertools.product((0, 1), (0, 2, 65535))]
    specs += [dict(path=0, start=0, end=1, active=1, state=1, frame=frame) for frame in (0, 2)]
    cases, covered = [], set()
    for spec, poison in itertools.product(specs, (0x5a, 0xa5)):
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x4000)
        g.mem_map(STOP, 4096)
        for at, size in BANKS:
            g.mem_write(at, bytes([poison])*size)
        patches = []

        def raw(at, value):
            g.mem_write(at, value)
            patches.append([at, value.hex()])

        raw(STACK, struct.pack('<II', STOP, 0x11223344))
        raw(0x5020c8, struct.pack('<I', HEADER))
        raw(HEADER+0x44, struct.pack('<I', ROWS))
        raw(ACTOR+0x50c, bytes([spec['active']]))
        raw(ACTOR+0x504, struct.pack('<H', spec['state']))
        raw(ACTOR+0x539, struct.pack('<bb', spec['end'], spec['start']))
        position, path = PATHS[spec['path']]
        raw(ACTOR+0x238, struct.pack('<3f', *position))
        for index, point in enumerate(path):
            raw(ACTOR+0x548+index*28, struct.pack('<3fh', *point, spec['frame']))
        initial = [0xe3630002, 0x22334455, 0x33445566, 0x44556677, STACK, 0x55667788, 0x66778899, 0x778899aa]
        xmm = [[0x3f000000+i*4+j for j in range(4)] for i in range(8)]
        for reg, value in zip(GP, initial):
            g.reg_write(reg, value)
        for i, lanes in enumerate(xmm):
            g.reg_write(R.UC_X86_REG_XMM0+i, int.from_bytes(struct.pack('<4I', *lanes), 'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        g.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        g.reg_write(R.UC_X86_REG_FPTAG, 0xffff)
        trace, stores, pending = [], [], []
        boundary = None

        def flush():
            for pc, at, before in pending:
                stores.append([pc, at, before, bytes(g.mem_read(at, len(before)//2)).hex()])
            pending.clear()

        def instruction(_g, pc, size, _u):
            nonlocal boundary
            flush()
            assert pc in body.code and bytes(g.mem_read(pc, size)) == image.read(pc, size)
            if pc == 0x200c9e:
                boundary = pc
                g.emu_stop()
                return
            trace.append(pc)
            covered.add(pc)

        def access(at, size):
            assert any(a <= at < at+size <= a+n for a, n in BANKS), (hex(at), size)

        def read(_g, _kind, at, size, _value, _u):
            access(at, size)

        def write(_g, _kind, at, size, _value, _u):
            access(at, size)
            pending.append((g.reg_read(R.UC_X86_REG_EIP), at, bytes(g.mem_read(at, size)).hex()))

        g.hook_add(UC_HOOK_CODE, instruction)
        g.hook_add(UC_HOOK_MEM_READ, read)
        g.hook_add(UC_HOOK_MEM_WRITE, write)
        g.emu_start(ENTRY, STOP, count=1000)
        flush()
        assert g.reg_read(R.UC_X86_REG_EIP) == (boundary or STOP)
        cases.append(dict(spec, entry=ENTRY, poison=poison, prefix_end=boundary,
                          patches=patches, gp=initial, xmm=xmm, trace=trace, stores=stores,
                          final_gp=[g.reg_read(reg) for reg in GP], final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                          final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16, 'little'))) for i in range(8)],
                          x87_depth=sum(((g.reg_read(R.UC_X86_REG_FPTAG) >> (i*2)) & 3) != 3 for i in range(8)),
                          outputs=[[ACTOR+0x238, bytes(g.mem_read(ACTOR+0x238, 12)).hex()],
                                   [ACTOR+0x504, bytes(g.mem_read(ACTOR+0x504, 0xa0)).hex()],
                                   [STACK-0x40, bytes(g.mem_read(STACK-0x40, 0x48)).hex()]]))
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, instruction_patches=[], supplied_dependency_returns=[],
                   native_component_banks=BANKS, immutable_operands=[], cases=cases)
    encoded = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == encoded, 'Preserve original path-distance reference'
    else:
        args.fixture.write_bytes(encoded)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, cases=len(cases),
                  whole_cases=sum(c['prefix_end'] is None for c in cases),
                  transform_prefix_cases=sum(c['prefix_end'] is not None for c in cases),
                  observed_instruction_sites=len(covered), static_instruction_sites=len(body.code),
                  unobserved_instruction_sites=[f'{pc:08x}' for pc in sorted(set(body.code)-covered)],
                  native_body_sha256=hashlib.sha256(native.encode()).hexdigest(),
                  fixture_bytes=len(encoded), fixture_sha256=hashlib.sha256(encoded).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  transformed_path_completion_qualified=False, full_navigation_or_combat_qualified=False,
                  x87_exception_status_and_nonfinite_inputs_qualified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'scope'}))


if __name__ == '__main__':
    main()
