#!/usr/bin/env python3
"""Unchanged189E10 visual selection over authored weapon and global material rows.

Explicit ABI inputs bypass distance admission. No material/effect lookup or
callee is intercepted. This does not verify contacts, sounds or whole frames.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from oracle_native_footstep_selection import ROOT, Cache, Xbe
from oracle_particle_physics_movement_20261010 import OLD, NEW
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R


def run(path, entry, global_va, table_va, weapon_suffix=None):
    image = Xbe(path)
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x800000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    page = cache.magic & ~4095
    guest.mem_map(page, (len(cache.data) + cache.magic - page + 4095) // 4096 * 4096)
    guest.mem_write(cache.magic, bytes(cache.data))
    table, stack, stop, out = 0x1000000, 0x10f0000, 0x10fe000, 0x10fd000
    guest.mem_map(table, 0x100000)
    for datum, tag in cache.tags.items():
        if tag['offset'] is not None:
            guest.mem_write(table + (datum & 65535) * 16 + 8, struct.pack('<I', tag['offset'] + cache.magic))
    globals = next(tag for tag in cache.tags.values() if tag['group'] == 'matg')
    owner = (next(tag for tag in cache.tags.values() if tag['group']=='weap'
                  and tag['name'].endswith(weapon_suffix)) if weapon_suffix
             else cache.tags[0xe59b0426])
    foot = cache.u(owner['offset'] + 0x58)
    count = cache.u(globals['offset'] + 0x150)
    guest.mem_write(global_va, struct.pack('<I', globals['offset'] + cache.magic))
    guest.mem_write(table_va, struct.pack('<I', table))
    rows = []
    for selected in (foot, 0xffffffff):
        for material in range(count):
            for fallback in (-1, material):
                for action in range(15, 21):
                    for variant in (-1, 0):
                        args = [1, 0, fallback, material, action, variant] + [out + i*4 for i in range(6)]
                        guest.mem_write(out, b'\xa5' * 24)
                        guest.mem_write(stack, struct.pack('<13I', stop, *(v & 0xffffffff for v in args)))
                        guest.reg_write(R.UC_X86_REG_ESP, stack)
                        guest.reg_write(R.UC_X86_REG_EBX, selected)
                        guest.emu_start(entry, stop, count=100000)
                        assert guest.reg_read(R.UC_X86_REG_EIP) == stop
                        values = struct.unpack('<6I', guest.mem_read(out, 24))
                        rows.append((selected, fallback, material, action, variant, values[1], values[3], values[5]))
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--weapon-suffix', help='Use this authored weapon material table instead of the SMG projectile')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    pins = ['aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
            '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd']
    for path, pin in zip((OLD, NEW), pins):
        assert hashlib.sha256(path.read_bytes()).hexdigest() == pin
    rows = run(OLD, 0x189e10, 0x4e791c, 0x4eb114,args.weapon_suffix)
    assert rows == run(NEW, 0x18b1d0, 0x4ecc1c, 0x4f0414,args.weapon_suffix)
    text = ''.join(f'{foot:08x} {fallback} {material} {action} {variant} {a:08x} {b:08x} {c:08x}\n'
                   for foot, fallback, material, action, variant, a, b, c in rows)
    (args.output / 'fixtures.tsv').write_text(text)
    report = dict(cases_per_image=len(rows), xbe_sha256=pins, intercepted_instructions=0,
                  weapon_suffix=args.weapon_suffix,
                  paired_results_byte_exact=True, boundary=__doc__,
                  fixture_sha256=hashlib.sha256(text.encode()).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
