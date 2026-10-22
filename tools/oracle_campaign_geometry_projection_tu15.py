#!/usr/bin/env python3
"""Original whole projection/unprojection calls and their immutable tables.

The reached219030 owner and its218FE0 companion copy component bits through
two original six-row signed-short permutation tables. Original123210 returns
axes0..2;14CEA0 forms a Boolean orientation and indexes the projection table
by2*axis+orientation. Authored reference-frame metadata reaches these same
selectors through218780/218880 and218960/218A40. Malformed metadata is not
qualified or clamped. No coordinates, paths or successful query are supplied.
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
BASE, STACK, INPUT, OUTPUT, STOP = 0x2000000, 0x2001f00, 0x2000200, 0x2000300, 0x3000000
ENTRIES = (0x218fe0, 0x219030)
TABLES = (0x44ac8c, 0x44acb0)
VECTORS = ((0x3f800000, 0xc0200000, 0x40a00000), (0, 0x80000000, 0x3f800000),
           (0x7fc12345, 0x7fa55555, 0xffcabcde), (0x7f800000, 0xff800000, 0x7f7fffff),
           (1, 0x80000001, 0x007fffff), (0x3f800000,)*3)


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
    bodies = {entry: _trace(code, entry, set(ENTRIES)) for entry in ENTRIES}
    owner = ROOT/'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'
    cpp = owner.read_text()
    body_hashes = {}
    for entry, body in bodies.items():
        marker = f'static void run_{entry:x}(WorldState &s) {{'
        a = cpp.index(marker)
        text = cpp[a:cpp.index('\n}', a)+2]
        sites = {int(v, 16) for v in re.findall(r'services\.instruction\(0x([0-9a-f]+)u\)', text)}
        assert sites == set(body.code) and len(sites) == 17
        assert not body.calls and not body.tail_jumps
        body_hashes[f'{entry:08x}'] = hashlib.sha256(text.encode()).hexdigest()
    for pc, value in ((0x123249, 2), (0x12325c, 1)):
        ins = code.at(pc)
        assert ins.mnemonic == 'mov' and ins.op_str == f'ax, {value}'
    assert code.at(0x123264).mnemonic == 'xor' and code.at(0x123264).op_str == 'ax, ax'
    # Preserve the already authenticated forward projection family identity.
    assert hashlib.sha256(image.read(TABLES[0], 36)).hexdigest() == 'afaf7ee3fe87b6989541d81f79a6a737b1e7cadede1ab3ed6737c6a29b43c5db'
    operands = [[address, image.read(address, 36).hex()] for address in TABLES]
    rows = [list(struct.iter_unpack('<3h', bytes.fromhex(bits))) for _, bits in operands]
    for table in rows:
        assert len(table) == 6 and all(sorted(row) == [0, 1, 2] for row in table)
    for forward, inverse in zip(*rows):
        assert all(inverse[forward[n]] == n for n in range(3))
    for address in TABLES:
        assert any(s.name == '.rdata' and s.va <= address < address+36 <= s.va+s.vsize for s in image.sections)
    world_source = (ROOT/'src/platform/native_tu15_world_memory.cpp').read_text()
    begin = world_source.index('memory.mount(0x44ac8cu,encode(std::array{')
    forward_publication = world_source[begin:world_source.index(');', begin)+2]
    words = [int(v, 16) for v in re.findall(r'0x([0-9a-f]+)u', forward_publication)]
    assert words[0] == TABLES[0] and len(words) == 10
    assert struct.pack('<9I', *words[1:]) == image.read(TABLES[0], 36)
    assert 'SourceRuntimeMemory::Access::ReadOnly' in forward_publication
    cases, covered = [], set()
    for entry, axis, orientation, vector, upper, alias, poison in itertools.product(
            ENTRIES, range(3), (0, 1), VECTORS, (0, 0xa55a0000), ('separate', -4, 0, 4), (0x5a, 0xa5)):
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x2000)
        g.mem_write(BASE, bytes([poison])*0x2000)
        g.mem_map(STOP, 4096)
        destination = OUTPUT if alias == 'separate' else INPUT+alias
        patches = []

        def raw(at, value):
            g.mem_write(at, value)
            patches.append([at, value.hex()])

        raw(STACK, struct.pack('<II', STOP, 0x11223300 | orientation))
        raw(INPUT, struct.pack('<3I', *vector))
        initial = [destination, upper | axis, INPUT, 0x44556677, STACK, 0x55667788, 0x66778899, 0x778899aa]
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
            assert pc in bodies[entry].code and bytes(g.mem_read(pc, size)) == image.read(pc, size)
            trace.append(pc)
            covered.add(pc)

        def write(_g, _kind, at, size, _value, _u):
            assert BASE <= at < at+size <= BASE+0x2000
            pending.append((g.reg_read(R.UC_X86_REG_EIP), at, bytes(g.mem_read(at, size)).hex()))

        def read(_g, _kind, at, size, _value, _u):
            section = next((s for s in image.sections if s.va <= at < at+size <= s.va+s.vsize), None)
            if section:
                assert section.name == '.rdata' and any(a <= at < at+size <= a+36 for a in TABLES)

        g.hook_add(UC_HOOK_CODE, instruction)
        g.hook_add(UC_HOOK_MEM_WRITE, write)
        g.hook_add(UC_HOOK_MEM_READ, read)
        g.emu_start(entry, STOP, count=100)
        flush()
        assert g.reg_read(R.UC_X86_REG_EIP) == STOP
        cases.append(dict(entry=entry, axis=axis, orientation=orientation, vector=vector,
                          upper=upper, alias=alias, poison=poison, prefix_end=None,
                          patches=patches, gp=initial, xmm=xmm, trace=trace, stores=stores,
                          final_gp=[g.reg_read(reg) for reg in GP], final_flags=g.reg_read(R.UC_X86_REG_EFLAGS),
                          final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16, 'little'))) for i in range(8)],
                          outputs=[[INPUT-4, bytes(g.mem_read(INPUT-4, 20)).hex()],
                                   [OUTPUT, bytes(g.mem_read(OUTPUT, 12)).hex()],
                                   [STACK-4, bytes(g.mem_read(STACK-4, 12)).hex()]]))
    fixture = dict(scope=__doc__, executor='geometry_permutation', original_xbe_sha256=NEW_PIN,
                   instruction_patches=[], supplied_dependency_returns=[],
                   native_component_banks=[[BASE, 0x2000]], immutable_operands=operands, cases=cases)
    encoded = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if args.verify or args.fixture.exists():
        assert args.fixture.read_bytes() == encoded, 'Preserve original geometry permutation reference'
    else:
        args.fixture.write_bytes(encoded)
    lines = ['// Original219030 inverse table.218FE0 reuses its existing native-world forward provider.']
    for address, bits in operands[1:]:
        raw_bytes = bytes.fromhex(bits)
        lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,36>{{'+
                     ','.join(f'0x{b:02x}' for b in raw_bytes)+'});')
    args.data_output.write_text('\n'.join(lines)+'\n')
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, cases=len(cases), whole_cases=len(cases),
                  entry_coverage={f'{e:08x}': dict(observed=len(set(b.code)&covered), static=len(b.code)) for e, b in bodies.items()},
                  immutable_bytes=72, tables=operands, rows=rows, source_body_sha256=body_hashes,
                  newly_published_bytes=36, reused_forward_bytes=36,
                  existing_forward_publication_sha256=hashlib.sha256(forward_publication.encode()).hexdigest(),
                  source_owner_changed=False, data_sha256=hashlib.sha256(args.data_output.read_bytes()).hexdigest(),
                  fixture_bytes=len(encoded), fixture_sha256=hashlib.sha256(encoded).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  malformed_selectors_qualified=False, full_navigation_or_combat_qualified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: v for k, v in report.items() if k not in ('tables', 'rows', 'scope')}))


if __name__ == '__main__':
    main()
