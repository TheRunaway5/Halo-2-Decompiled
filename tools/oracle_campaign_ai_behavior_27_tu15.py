#!/usr/bin/env python3
"""Execute descriptor27's whole original callback and real lookup/iterator owners.

Declared actor, property, query, script, character-tag and behavior-frame records
are inputs. Original1BD1E0/1ED100/267450/27C710/278DD0 instructions produce all
return values and iterator state. No dependency return or behavior decision is
supplied. Squad fallback in27C710 remains outside these cases.
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
BASE, STACK, STOP = 0x2000000, 0x201f000, 0x3000000
AH, PH, QH, LH, SH, CLOCK = (BASE+x for x in (0x100, 0x200, 0x300, 0x400, 0x500, 0x600))
ACTORS, SCRIPTS, QUERIES, PROPS, LINKS, TAGS = (BASE+x for x in (0x1000, 0x6000, 0x8000, 0xa000, 0xc000, 0xe000))
ACTOR, SCRIPT, PROP, QUERY = ACTORS+2*0x888, SCRIPTS+0xd4, PROPS+0x3c, QUERIES+0x124
CHARACTERS, STYLE, PARAMETERS, CONTEXT = (BASE+x for x in (0x10000, 0x11000, 0x12000, 0x15000))
ACTOR_ID, PROP_ID, QUERY_ID, SCRIPT_ID = 0xe3630002, 0xe4710001, 0xf1280001, 0xaabb0001
ENTRIES = (0x1bd1e0, 0x1ed100, 0x267450, 0x27c710, 0x278dd0)
FIXED = ((0x5020c8, AH), (0x50eef0, PH), (0x50eeec, QH), (0x50eefc, LH),
         (0x50eee0, SH), (0x4f0414, TAGS), (0x51df54, CLOCK))
BANKS = ((BASE, 0x20000), *((at, 4) for at, _ in FIXED))
MODES = ('none', 'match_first', 'match_last', 'wrong_type', 'expired', 'equal_deadline',
         'negative_deadline', 'positive_deadline', 'no_link', 'wrong_prop', 'scan_match')


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
    bodies = {e: _trace(code, e, set(ENTRIES)) for e in ENTRIES}
    sites = set().union(*(set(b.code) for b in bodies.values()))
    body_hashes = {}
    for e in (0x1bd1e0, 0x1ed100, 0x278dd0):
        path = ROOT/('src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp' if e == 0x278dd0 else
                     'src/campaign/ai/source_campaign_ai_actor_behavior_tu15_generated.cpp')
        cpp = path.read_text()
        a = cpp.index(f'static void run_{e:x}(WorldState &s) {{')
        native = cpp[a:cpp.index('\n}', a)+2]
        assert {int(v, 16) for v in re.findall(r'services\.instruction\(0x([0-9a-f]+)u\)', native)} == set(bodies[e].code)
        body_hashes[f'{e:08x}'] = hashlib.sha256(native.encode()).hexdigest()
    specs = []
    specs += [dict(entry=0x1bd1e0, flag225=a, flag223=b, has_prop=p)
              for a, b, p in itertools.product((0, 1), (0, 1), (False, True))]
    specs += [dict(entry=0x1bd1e0, has_query=q, query_status=status, query_kind=kind,
                   character='present' if character else 'none')
              for q, status, kind, character in itertools.product((False, True), (0, 1), (0, 5), (False, True))]
    specs += [dict(entry=0x1bd1e0, query_kind=0, mode=mode) for mode in MODES]
    specs += [dict(entry=entry, variant=variant, override=override, inheritance=inheritance)
              for entry, variant, override, inheritance in itertools.product(
                  (0x1bd1e0, 0x1ed100), (0, 1, 2), (0, 1, 2, 3), (0, 1, 2))]
    specs += [dict(entry=0x1ed100, character=character, inheritance=inheritance)
              for character, inheritance in itertools.product(('none', 'empty'), (0, 1, 2))]
    specs += [dict(entry=0x1ed100, script=script, variant=variant, override=override)
              for script, variant, override in itertools.product(('none', 'inactive', 'active'), (0, 1, 32767), (0, 3))]
    specs += [dict(entry=0x278dd0, counter=counter, mode=mode)
              for counter, mode in itertools.product((-1, 0, 2, 3, 4), MODES)]
    cases, covered = [], set()
    for spec, poison in itertools.product(specs, (0x5a, 0xa5)):
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x20000)
        g.mem_map(STOP, 4096)
        for at, size in BANKS:
            g.mem_write(at, bytes([poison])*size)
        patches = []

        def raw(at, bits):
            g.mem_write(at, bits)
            patches.append([at, bits.hex()])

        def put(at, value, size=4):
            raw(at, (value & ((1 << (size*8))-1)).to_bytes(size, 'little'))

        for at, value in FIXED:
            put(at, value)
        for header, rows in ((AH, ACTORS), (PH, PROPS), (QH, QUERIES), (LH, LINKS), (SH, SCRIPTS)):
            put(header+0x44, rows)
        put(STACK, STOP)
        put(STACK+4, ACTOR_ID)
        put(CLOCK+8, 100)
        put(ACTOR+0x225, spec.get('flag225', 0), 1)
        put(ACTOR+0x223, spec.get('flag223', 0), 1)
        put(ACTOR+0x338, PROP_ID if spec.get('has_prop', True) else 0xffffffff)
        put(PROP+0x14, QUERY_ID if spec.get('has_query', True) else 0xffffffff)
        put(PROP+0x20, 0xe5660005)
        put(PROPS+2*0x3c+0x20, 0xe5660006)
        put(QUERY+0x70, spec.get('query_kind', 5), 2)
        put(QUERY+0x70+0x68, spec.get('query_status', 0), 1)
        character = spec.get('character', 'present')
        put(ACTOR+0x54, 0xffffffff if character == 'none' else 0xabcd0002)
        put(ACTOR+0x30, 0xffffffff)
        script = spec.get('script', 'active')
        put(ACTOR+0x858, 0xffffffff if script == 'none' else SCRIPT_ID)
        put(SCRIPT+0x86, int(script == 'active'), 1)
        put(SCRIPT+0x88, 0xabcd0004)
        put(TAGS+4*16+8, STYLE)
        put(STYLE+0x24, spec.get('variant', 0), 2)
        put(STYLE+0x2c, spec.get('override', 0), 1)
        inheritance = spec.get('inheritance', 0)
        handles = (0xabcd0002, 0xabcd0003, 0xabcd0005)
        for index, handle in enumerate(handles):
            at = CHARACTERS+index*0x200
            put(TAGS+(handle & 65535)*16+8, at)
            put(at+8, handles[index+1] if index < inheritance else 0xffffffff)
            put(at+0xa4, 2 if index == inheritance and character == 'present' else 0)
            put(at+0xa8, PARAMETERS+index*0x100)
        mode = spec.get('mode', 'none')
        for index in range(4):
            kind, deadline, link_prop = 0, 99, 0xffffffff
            admitted = (mode != 'none' and index == (3 if mode == 'match_last' else 0))
            if mode == 'scan_match' and index < 3:
                admitted = True
            if admitted:
                kind = 26 if mode == 'wrong_type' else 27
                deadline = {'expired': 99, 'equal_deadline': 100, 'negative_deadline': -1,
                            'positive_deadline': 0x7fffffff}.get(mode, 101)
                link_prop = (0xffffffff if mode == 'no_link' else
                             0xe4710002 if mode == 'wrong_prop' else PROP_ID)
                if mode == 'scan_match':
                    link_prop = (0xe4710002, 0xffffffff, PROP_ID)[index]
            put(ACTOR+0x194+index*12, kind, 2)
            put(ACTOR+0x194+index*12+4, 0xf3210000 | index)
            put(ACTOR+0x194+index*12+8, deadline)
            put(LINKS+index*0xbc+0x88, link_prop)
        put(CONTEXT, ACTOR_ID)
        put(CONTEXT+4, spec.get('counter', -1), 2)
        put(CONTEXT+6, 27, 2)
        initial = [0x11223344, 0x22334455, 0x33445566, ACTOR_ID, STACK, 0x55667788, 0x66778899, 0x778899aa]
        if spec['entry'] == 0x278dd0:
            initial[1] = CONTEXT
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

        def flush():
            for pc, at, before in pending:
                stores.append([pc, at, before, bytes(g.mem_read(at, len(before)//2)).hex()])
            pending.clear()

        def instruction(_g, pc, size, _u):
            flush()
            assert pc in sites and bytes(g.mem_read(pc, size)) == image.read(pc, size), hex(pc)
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
        g.emu_start(spec['entry'], STOP, count=5000)
        flush()
        assert g.reg_read(R.UC_X86_REG_EIP) == STOP, spec
        cases.append(dict(spec, poison=poison, prefix_end=None, patches=patches, gp=initial, xmm=xmm,
                          trace=trace, stores=stores, final_gp=[g.reg_read(reg) for reg in GP],
                          final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                          final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16, 'little'))) for i in range(8)],
                          outputs=[[CONTEXT, bytes(g.mem_read(CONTEXT, 8)).hex()],
                                   [ACTOR+0x194, bytes(g.mem_read(ACTOR+0x194, 48)).hex()],
                                   [PROP, bytes(g.mem_read(PROP, 0x3c)).hex()],
                                   [STACK-0x60, bytes(g.mem_read(STACK-0x60, 0x68)).hex()]]))
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, instruction_patches=[], supplied_dependency_returns=[],
                   admit_original_behavior_iterator=True, native_component_banks=BANKS, immutable_operands=[], cases=cases)
    encoded = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == encoded, 'Preserve original descriptor27 callback reference'
    else:
        args.fixture.write_bytes(encoded)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, whole_cases=len(cases),
                  cases_by_entry={f'{e:08x}': sum(c['entry'] == e for c in cases) for e in ENTRIES},
                  entry_coverage={f'{e:08x}': dict(observed=len(set(b.code) & covered), static=len(b.code),
                      unobserved=[f'{pc:08x}' for pc in sorted(set(b.code)-covered)]) for e, b in bodies.items()},
                  native_body_sha256=body_hashes, fixture_bytes=len(encoded),
                  fixture_sha256=hashlib.sha256(encoded).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  supplied_dependency_returns=[], instruction_patches=[], squad_fallback_qualified=False,
                  whole_live_callback_and_full_navigation_or_combat_qualified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'scope'}))


if __name__ == '__main__':
    main()
