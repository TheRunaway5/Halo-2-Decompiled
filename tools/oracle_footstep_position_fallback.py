#!/usr/bin/env python3
"""Observe unchanged TU1.1 E63C0 through its footstep-position selection.

Actual cache metadata and declared raw caller/unit records isolate the missing
marker ordinal branch. BACC0 executes unchanged. This stops before material,
PVS and sound dispatch; it does not claim an allocated live original unit.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from inspect_multiplayer import Cache
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EDX, UC_X86_REG_EIP


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = Xbe(args.xbe)
    digest = hashlib.sha256(image.data).hexdigest()
    assert digest == 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    cache = Cache(args.map)
    datum, biped = next((d, t) for d, t in cache.tags.items()
                       if t['name'] == 'objects\\characters\\masterchief\\masterchief_mp')
    count = cache.u(biped['offset'] + 0x2f8)
    position = struct.pack('<3f', 1.25, -2.5, 3.75)
    records = []
    for marker in (count, count + 1, 0xffff):
        cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        cpu.mem_map(0x10000, 0x800000)
        for section in image.sections:
            cpu.mem_write(section.va, image.section_bytes(section))
        page = cache.magic & ~4095
        cpu.mem_map(page, (len(cache.data) + cache.magic - page + 4095) // 4096 * 4096)
        cpu.mem_write(cache.magic, bytes(cache.data))
        cpu.mem_map(0x1000000, 0x100000)
        table, unit, pool, entries, stack = 0x1000000, 0x1010000, 0x1020000, 0x1021000, 0x10f0000
        for tag_datum, tag in cache.tags.items():
            if tag['offset'] is not None:
                cpu.mem_write(table + (tag_datum & 65535) * 16 + 8,
                              struct.pack('<I', tag['offset'] + cache.magic))
        cpu.mem_write(0x4eb114, struct.pack('<I', table))
        cpu.mem_write(0x4e78d0, struct.pack('<I', pool))
        cpu.mem_write(pool + 0x44, struct.pack('<I', entries))
        cpu.mem_write(entries + 8, struct.pack('<I', unit))
        cpu.mem_write(unit, struct.pack('<I', datum))
        cpu.mem_write(unit + 0x14, b'\xff' * 4)  # Declared unparented caller record.
        cpu.mem_write(unit + 0x64, position)
        cpu.mem_write(unit + 0x3dc, b'\x01')
        cpu.mem_write(stack, struct.pack('<4I', 0x10fe000, 0xe26f0000, 3, 0x3f800000))
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_EDX, marker)
        cpu.emu_start(0xe63c0, 0xe6477, count=100000)
        assert cpu.reg_read(UC_X86_REG_EIP) == 0xe6477
        result = bytes(cpu.mem_read(cpu.reg_read(UC_X86_REG_ESP) + 0x10, 12))
        assert result == position, (marker, result.hex())
        records.append(dict(marker=marker, authored_marker_count=count,
                            unit_position_hex=position.hex(), output_hex=result.hex()))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(scope=__doc__, xbe_sha256=digest,
        map_sha256=hashlib.sha256(cache.data).hexdigest(), records=records), indent=2) + '\n')
    print(f'PASS {len(records)} unchanged footstep-position prefixes, actual marker count={count}')


if __name__ == '__main__':
    main()
