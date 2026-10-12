#!/usr/bin/env python3
"""Run unchanged210DF0 and its callees on the literal mainmenu portal bank.

CPU-only source oracle. No native library, graphics or emulator process runs.
The scratch machine pool is an explicit input, not a title lifetime claim.
Empty authored associations make owner reset irrelevant to these mask outputs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
sys.path.insert(0, str(ROOT/'tools/reference/halo2-decompiled/tools'))
from audit_rendering import Cache
from xbe import Xbe

MAP_SHA = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
XBE_SHA = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE, STOP, STACK = 0x2100000, 0x3000000, 0x22fff00
MASK, ACOUSTIC, POOL, DESCRIPTORS = [BASE+i*0x1000 for i in range(4)]

def pack(values):
    return struct.pack('<'+'I'*len(values), *values)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, default=ROOT/'tests/fixtures/mainmenu_portal_state_original.tsv')
    parser.add_argument('--report', type=Path, default=ROOT/'analysis/frontend/title-fog-mainmenu-portal-original.json')
    args = parser.parse_args()
    cache = Cache(ROOT/'assets/disc/maps/mainmenu.map')
    xbe = Xbe(ROOT/'assets/disc/default.xbe')
    assert hashlib.sha256(cache.data).hexdigest() == MAP_SHA
    assert hashlib.sha256(xbe.data).hexdigest() == XBE_SHA
    _, scenario = cache.tag(cache.scenario, 'scnr')
    assert cache.u16(scenario+0x10) == 2
    record = cache.block(scenario+0x210, 68)[0]
    file, size, address = [cache.u32(record+i) for i in (0, 4, 8)]
    magic = address-file
    portals = cache.block(file+16+0x5c, 36, magic)
    flags = [cache.u32(p+0x18) for p in portals]
    header = cache.pointer(cache.u32(file+16+0x220), magic, 16)
    associations = cache.block(header, 12, magic)
    indices = cache.block(header+8, 2, magic)
    assert len(flags) == 151 and not any(flags) and not associations and not indices
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x10000, 0x600000)
    for section in xbe.sections:
        u.mem_write(section.va, xbe.section_bytes(section))
    aligned = address & ~4095
    u.mem_map(aligned, (address+size-aligned+4095)&~4095)
    u.mem_write(address, bytes(cache.data[file:file+size]))
    u.mem_map(BASE, 0x200000)
    u.mem_map(STOP, 4096)
    entries = {v: 0 for v in (0x210df0, 0x210f00, 0x210e80, 0x210e20, 0xbaeb0)}
    def trace(_u, at, _size, _data):
        if at in entries:
            entries[at] += 1
    u.hook_add(UC_HOOK_CODE, trace)
    u.mem_write(POOL, bytes(0x50))
    # One explicitly empty pool slot exercises the unchanged BAEB0 iterator.
    u.mem_write(POOL+0x38, pack([1]))
    u.mem_write(POOL+0x44, pack([DESCRIPTORS]))
    for at, value in ((0x4e0348, address+16), (0x4f93a4, MASK),
                      (0x4ed288, ACOUSTIC), (0x4e0300, POOL)):
        u.mem_write(at, pack([value]))
    rows = []
    outputs = []
    for case in range(4):
        before_mask = [(0x24681357*(case+1)+i*0x1020304)&0xffffffff for i in range(16)]
        before_acoustic = [(case*0x1357+i*257)&65535 for i in range(128)]
        u.mem_write(MASK, pack(before_mask))
        u.mem_write(ACOUSTIC, bytes(0x244))
        u.mem_write(ACOUSTIC+0x28, struct.pack('<128H', *before_acoustic))
        u.mem_write(STACK, pack([STOP]))
        u.reg_write(UC_X86_REG_ESP, STACK)
        u.emu_start(0x210df0, STOP, count=1000000)
        assert u.reg_read(UC_X86_REG_EIP) == STOP
        after_mask = list(struct.unpack('<16I', u.mem_read(MASK, 64)))
        after_acoustic = list(struct.unpack('<128H', u.mem_read(ACOUSTIC+0x28, 256)))
        assert after_mask == [0xffffffff]*16 and after_acoustic == before_acoustic
        rows.append('R '+ ' '.join(str(v) for v in before_mask+before_acoustic+after_mask+after_acoustic))
        outputs.append({'case': case, 'mask': after_mask, 'acoustic_preserved': after_acoustic == before_acoustic})
    assert all(entries.values()), entries
    # This private authored-input mutation verifies that ignoring flag8 would
    # change the original result. It is not claimed to be an authored title bank.
    u.mem_write(portals[0]+magic+0x18, pack([8]))
    u.mem_write(STACK, pack([STOP]))
    u.reg_write(UC_X86_REG_ESP, STACK)
    u.emu_start(0x210df0, STOP, count=1000000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP
    blocked_mask = list(struct.unpack('<16I', u.mem_read(MASK, 64)))
    assert blocked_mask == [0xfffffffe]+[0xffffffff]*15
    u.mem_write(portals[0]+magic+0x18, pack([0]))
    fixture = '\n'.join(['H2_MAINMENU_PORTAL_STATE_ORIGINAL_V1',
        f'A {len(flags)} {len(associations)} {len(indices)} {portals[0]+0x18} {header} {scenario}', *rows])+'\n'
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(fixture)
    report = {'scope': 'unchanged original mainmenu portal mask constructor only; no visibility/camera/LOD/native rendering',
        'map_sha256': MAP_SHA, 'xbe_sha256': XBE_SHA,
        'portal_count': len(flags), 'portal_flags': flags, 'association_count': len(associations),
        'associated_index_count': len(indices), 'portal_flags_file_offset': portals[0]+0x18,
        'association_header_file_offset': header, 'scenario_file_offset': scenario,
        'bsp_file_offset': file, 'bsp_source_address': address,
        'routine_entries': {f'{at:08X}': count for at, count in entries.items()},
        'cases': outputs, 'private_flag8_control_mask': blocked_mask,
        'fixture_sha256': hashlib.sha256(fixture.encode()).hexdigest()}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({k: report[k] for k in ('portal_count', 'association_count', 'associated_index_count', 'routine_entries', 'fixture_sha256')}, indent=2))
    cache.close()

if __name__ == '__main__':
    main()
