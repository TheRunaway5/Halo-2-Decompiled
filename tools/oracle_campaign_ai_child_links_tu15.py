#!/usr/bin/env python3
"""Whole original1AFDE0 child-command link construction and exact head storage.

Caller rows and command contexts are declared inputs. Original instructions
clear the heads and construct/order every resulting link; no linked list,
dependency return, execution patch or completed command result is supplied.
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
from export_ai_behavior_descriptor_data_tu15 import descriptor_inputs, child_selection_inputs, property_selection_inputs

ROOT = Path(__file__).resolve().parents[1]
ENTRY, BASE, STACK, POINTERS, CHILDREN, STOP = 0x1afde0, 0x2000000, 0x2001f00, 0x2000200, 0x2000300, 0x3000000
BANKS = ((BASE, 0x2000), (0x4fafc8, 0x24d8), (0x4fd4a0, 131), (0x4fd524, 20))


def authored_fields(image):
    records, _, _ = descriptor_inputs(image)
    parents, _ = child_selection_inputs(image, records)
    styles, _ = property_selection_inputs(image)
    rows = {child['address'] for parent in parents for child in parent['children']}
    rows.update(command['child_address'] for style in styles for entry in style['entries'] for command in entry['commands'])
    assert len(rows) == 120
    result = []
    for row in sorted(rows):
        address = row+12
        assert image.read(address, 4) == bytes(4)
        assert any(s.name == '.data' and s.va <= address < address+4 <= s.va+s.vsize for s in image.sections)
        result.append(dict(row=row, address=address, bytes=4, initial_hex='00000000',
                           header_hex=image.read(row, 6).hex(), timer_hex=image.read(row+8, 4).hex()))
    return result


def storage_source(fields):
    lines = ['// Original1AFDE0 owns only each authored child row\'s +0xC DWORD head.', '{',
             'constexpr std::array<std::array<std::uint32_t,2>,120> fields{{']
    lines.extend(f'    {{0x{f["address"]:x}u,4u}},' for f in fields)
    lines += ['}};', 'auto owned=[&](std::uint32_t address) {',
              '    try {(void)memory.view(address,1);return true;}',
              '    catch(const std::out_of_range&){return false;}', '};',
              '// Preflight every existing head before admitting any missing head.',
              '// Preserve real mutable values, native addresses and backing lifetimes.',
              'for(const auto& [address,count]:fields)for(std::uint32_t i=0;i<count;++i)',
              '    if(owned(address+i) && memory.is_read_only(address+i,1))',
              '        throw std::invalid_argument("Original child command head is read-only");',
              'for(const auto& [address,count]:fields)for(std::uint32_t i=0;i<count;) {',
              '    if(owned(address+i)){++i;continue;}', '    const auto first=i++;',
              '    while(i<count && !owned(address+i))++i;',
              '    memory.mount(address+first,std::vector<std::byte>(i-first),SourceRuntimeMemory::Access::ReadWrite,',
              '        "Original TU1.5 loader-zero child command link head");', '}', '}']
    return '\n'.join(lines)+'\n'


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--data-output', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.data_output.resolve().is_relative_to(scratch)
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    body = _trace(code, ENTRY, {ENTRY})
    assert not body.calls and not body.tail_jumps
    cpp = (ROOT/'src/campaign/ai/source_ai_frame_tu15_generated.cpp').read_text()
    a = cpp.index('static void run_1afde0(WorldState &s) {')
    source_body = cpp[a:cpp.index('\n}', a)+2]
    assert set(int(v, 16) for v in re.findall(r'services\.instruction\(0x([0-9a-f]+)u\)', source_body)) == set(body.code)
    assert code.at(0x1afe1b).op_str == 'dword ptr [esi + 0xc], eax'
    fields = authored_fields(image)
    # All five original opcodes, all ordered triples, shared-child insertion
    # and distinct/missing target kinds. Three contexts fit the existing bank.
    specs = [(1, 3, True, ops, (1,)*len(ops)) for n in range(4) for ops in itertools.product(range(5), repeat=n)]
    specs += [(1, 3, True, ops, (2, 3, 99)) for ops in itertools.product(range(5), repeat=3)]
    for kind in (0, 31, 32, 63, 64, 127, 128, 130):
        for active in (False, True):
            specs += [(kind, count, active, (), ()) for count in (0, 1, 3)]
            specs += [(kind, 1, active, (4, 2, 1), (1, 1, 1)),
                      (kind, 0, active, (1, 2, 3), (1, 2, 3))]
    cases, covered = [], set()
    for (kind, child_count, active, operations, targets), poison in itertools.product(specs, (0x5a, 0xa5)):
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x2000)
        g.mem_map(STOP, 4096)
        for address, size in BANKS:
            g.mem_write(address, bytes([poison])*size)
        patches = []

        def raw(at, value):
            g.mem_write(at, value)
            patches.append([at, value.hex()])

        raw(STACK, struct.pack('<III', STOP, 0x11220000 | kind, POINTERS))
        raw(POINTERS, struct.pack('<3I', *(CHILDREN+i*20 for i in range(3))))
        for i in range(3):
            raw(CHILDREN+i*20, struct.pack('<H', i+1))
        raw(0x4fd4a0, bytes(131))
        raw(0x4fd4a0+kind, bytes([len(operations)]))
        mask = [0]*5
        if active:
            mask[kind >> 5] |= 1 << (kind & 31)
        raw(0x4fd524, struct.pack('<5I', *mask))
        nodes = []
        for index, (operation, target) in enumerate(zip(operations, targets)):
            at = 0x4fafc8+kind*24+index*0xc48
            raw(at, struct.pack('<hh', operation, target))
            nodes.append(at)
        initial = [0x11223344, 0x22334455, 0x33445566, (poison << 24) | child_count,
                   STACK, 0x55667788, 0x66778899, 0x778899aa]
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
            assert pc in body.code and bytes(g.mem_read(pc, size)) == image.read(pc, size)
            trace.append(pc)
            covered.add(pc)

        def access(at, size):
            assert any(a <= at < at+size <= a+n for a, n in BANKS), (hex(at), size)

        def write(_g, _kind, at, size, _value, _u):
            access(at, size)
            pending.append((g.reg_read(R.UC_X86_REG_EIP), at, bytes(g.mem_read(at, size)).hex()))

        def read(_g, _kind, at, size, _value, _u):
            access(at, size)

        g.hook_add(UC_HOOK_CODE, instruction)
        g.hook_add(UC_HOOK_MEM_WRITE, write)
        g.hook_add(UC_HOOK_MEM_READ, read)
        g.emu_start(ENTRY, STOP, count=5000)
        flush()
        assert g.reg_read(R.UC_X86_REG_EIP) == STOP
        outputs = [(CHILDREN, 60), (POINTERS, 12), (STACK-12, 24), (0x4fd4a0+kind, 1), (0x4fd524, 20)]
        outputs += [(at, 24) for at in nodes]
        cases.append(dict(entry=ENTRY, kind=kind, child_count=child_count, active=active,
                          operations=operations, targets=targets, poison=poison, prefix_end=None,
                          patches=patches, gp=initial, xmm=xmm, trace=trace, stores=stores,
                          final_gp=[g.reg_read(reg) for reg in GP], final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                          final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16, 'little'))) for i in range(8)],
                          outputs=[[at, bytes(g.mem_read(at, size)).hex()] for at, size in outputs]))
    fixture = dict(scope=__doc__, executor='frame', original_xbe_sha256=NEW_PIN,
                   instruction_patches=[], supplied_dependency_returns=[],
                   native_component_banks=BANKS, immutable_operands=[], authored_heads=fields, cases=cases)
    encoded = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == encoded, 'Preserve original child-command link reference'
    else:
        args.fixture.write_bytes(encoded)
    args.data_output.write_text(storage_source(fields))
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, whole_cases=len(cases),
                  observed_instruction_sites=len(covered), static_instruction_sites=len(body.code),
                  uncovered_instruction_sites=[f'{pc:08x}' for pc in sorted(set(body.code)-covered)],
                  authored_rows=len(fields), mutable_head_bytes=len(fields)*4,
                  source_body_sha256=hashlib.sha256(source_body.encode()).hexdigest(), source_owner_changed=False,
                  data_sha256=hashlib.sha256(args.data_output.read_bytes()).hexdigest(),
                  fixture_bytes=len(encoded), fixture_sha256=hashlib.sha256(encoded).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  instruction_patches=[], supplied_dependency_returns=[],
                  full_navigation_or_combat_qualified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'scope'}))


if __name__ == '__main__':
    main()
