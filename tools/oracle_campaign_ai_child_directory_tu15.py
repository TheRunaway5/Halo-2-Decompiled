#!/usr/bin/env python3
"""Compare original authored property merging and whole child-directory startup.

Whole1AF970 owns the20-style directory; whole1AFA90 merges each original
property list. No instructions or dependency returns are changed. Raw caller
registers, stack and prior directory bytes are declared inputs. Every output
pointer, count and ordered write is produced by unchanged original execution.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from export_ai_behavior_descriptor_data_tu15 import descriptor_inputs, child_selection_inputs, property_selection_inputs
from oracle_campaign_ai_placement_settings_tu15 import GP

BASE, STOP, STACK, OUTPUT = 0x2000000, 0x3000000, 0x200ff00, 0x2004000
DIRECTORY = ((0x4fd538, 0x20fc), (0x4ff798, 131*20*4))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    image = Xbe(NEW_DEFAULT); assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    records, _, _ = descriptor_inputs(image)
    parents, _ = child_selection_inputs(image, records)
    styles, ranges = property_selection_inputs(image)
    roots = struct.unpack('<131I', image.read(0x47b478, 524))
    parent_map = {p['address']: p for p in parents}
    specs = []
    for style in styles:
        for prop in style['entries']:
            parent = parent_map[roots[prop['parent_kind']]]
            specs.append(dict(kind='authored_property', entry=0x1afa90,
                style=style['style'], property=prop['address'], table=parent['table'], count=parent['count']))
    specs += [dict(kind='whole_directory', entry=0x1af970, poison=p) for p in (0, 0xa5, 0xff)]
    code = _Code(image, '.text')
    bodies = [_trace(code, p, {p}) for p in (0x1af970, 0x1afa90)]
    allowed = set().union(*(set(body.code) for body in bodies))
    covered, cases = set(), []
    for spec in specs:
        g = Uc(UC_ARCH_X86, UC_MODE_32); g.mem_map(0x10000, 0x600000)
        for section in image.sections: g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x10000); g.mem_map(STOP, 4096)
        patches = []
        def put(at, value):
            raw = struct.pack('<I', value); g.mem_write(at, raw); patches.append([at, raw.hex()])
        put(STACK, STOP)
        if spec['kind'] == 'authored_property':
            for at, value in ((STACK+4, OUTPUT), (STACK+8, spec['property']),
                    (STACK+12, spec['table']), (STACK+16, spec['count'])): put(at, value)
            g.mem_write(OUTPUT, bytes([0xa5])*200)
            outputs = ((OUTPUT, 200),)
        else:
            for at, count in DIRECTORY: g.mem_write(at, bytes([spec['poison']])*count)
            outputs = DIRECTORY
        initial = [0x11223344, 0x22334455, 0x33445566, 0x44556677,
            STACK, 0x55667788, 0x66778899, 0x778899aa]
        xmm = [[0x3f000000+i*4+lane for lane in range(4)] for i in range(8)]
        for reg, value in zip(GP, initial): g.reg_write(reg, value)
        for i, lanes in enumerate(xmm):
            g.reg_write(R.UC_X86_REG_XMM0+i, int.from_bytes(struct.pack('<4I', *lanes), 'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        g.reg_write(R.UC_X86_REG_FPCW, 0x27f); g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        trace, stores = [], []
        def instruction(_g, pc, size, _user):
            assert pc in allowed and bytes(g.mem_read(pc, size)) == image.read(pc, size), hex(pc)
            trace.append(pc); covered.add(pc)
        def store(_g, _kind, address, size, value, _user):
            assert any(at <= address < address+size <= at+count for at, count in ((BASE, 0x10000), *DIRECTORY)), hex(address)
            stores.append([g.reg_read(R.UC_X86_REG_EIP), address,
                bytes(g.mem_read(address, size)).hex(),
                (value & ((1 << (size*8))-1)).to_bytes(size, 'little').hex()])
        g.hook_add(UC_HOOK_CODE, instruction); g.hook_add(UC_HOOK_MEM_WRITE, store)
        g.emu_start(spec['entry'], STOP, count=1000000)
        assert g.reg_read(R.UC_X86_REG_EIP) == STOP
        if spec['kind'] == 'whole_directory':
            assert bytes(g.mem_read(0x4fd8cc, 4)) == struct.pack('<I', 0x47d3cc)
            # The compact header's fourth byte is never initialized by this
            # owner. Preserve its prior contents even on a repeated startup.
            assert all(g.mem_read(at, 1)[0] == spec['poison'] for at in range(0x4ff79b, 0x4ff798+131*20*4, 4))
        cases.append(dict(spec, patches=patches, gp=initial, xmm=xmm,
            final_gp=[g.reg_read(reg) for reg in GP], final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
            final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16, 'little'))) for i in range(8)],
            trace=trace, stores=stores, outputs=[[at, bytes(g.mem_read(at, count)).hex()] for at, count in outputs]))
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, instruction_patches=[], supplied_dependency_returns=[],
        native_component_banks=[[BASE, 0x10000]], directory_regions=DIRECTORY, styles=styles,
        selection_ranges=[[at, image.read(at, count).hex()] for at, count in ranges], cases=cases)
    raw = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == raw, 'Preserve existing independent original evidence'
    else:
        args.fixture.parent.mkdir(parents=True, exist_ok=True); args.fixture.write_bytes(raw)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, authored_properties=14, authored_commands=15,
        cases=len(cases), whole_directory_cases=3, ordinary_constructor_instructions=len(bodies[1].code),
        whole_directory_instructions=len(bodies[0].code), executed_directory_instructions=len(set(bodies[0].code)&covered),
        total_executed_unique_instructions=len(covered), instruction_patches=[], supplied_dependency_returns=[],
        fixture_bytes=len(raw), fixture_sha256=hashlib.sha256(raw).hexdigest(),
        fixture_contains_original_test_outputs_not_runtime_state=True, runtime_reference_required=False,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), full_ai_decision_or_coop_verified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True); args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__': main()
