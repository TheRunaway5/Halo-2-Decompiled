#!/usr/bin/env python3
"""Execute MCC's unchanged first-person camera save/override instructions.

This executes 7E0DE7..7E0F69, ending before the crosshair preference service.
It proves the lens override for declared incoming cameras, not live pixels.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_64
from unicorn import x86_const as R

PIN = 'de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946'
BASE, STACK = 0x180000000, 0x40000000
CAMERA = BASE + 0x1996A28

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dll', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    raw = args.dll.read_bytes()
    assert hashlib.sha256(raw).hexdigest() == PIN, 'Audit the changed MCC binary first'
    pe = pefile.PE(data=raw)
    assert pe.OPTIONAL_HEADER.ImageBase == BASE
    image = pe.get_memory_mapped_image()
    uc = Uc(UC_ARCH_X86, UC_MODE_64)
    uc.mem_map(BASE, (len(image) + 4095) & ~4095)
    uc.mem_write(BASE, image)
    uc.mem_map(STACK, 0x10000)
    cases = []
    for angle in (.1, .4, .895, 1.2, 2.094395160675049):
        for width, height in ((640, 480), (1280, 720), (3440, 1440), (2560, 720)):
            camera = bytearray(0x70)
            struct.pack_into('<3f', camera, 0, 10., 20., 30.)
            struct.pack_into('<3f', camera, 0x10, 1., 0., 0.)
            struct.pack_into('<3f', camera, 0x1c, 0., 0., 1.)
            struct.pack_into('<f', camera, 0x28, angle)
            struct.pack_into('<8h', camera, 0x30, 0, 0, height, width, 8, 8, height-8, width-8)
            struct.pack_into('<2f', camera, 0x40, .015625, 1024.)
            uc.mem_write(CAMERA, bytes(camera))
            uc.reg_write(R.UC_X86_REG_RSP, STACK + 0x8000)
            uc.emu_start(BASE + 0x7e0de7, BASE + 0x7e0f69, count=200)
            assert uc.reg_read(R.UC_X86_REG_RIP) == BASE + 0x7e0f69
            result = bytes(uc.mem_read(CAMERA, 0x70))
            lens = struct.unpack_from('<f', result, 0x28)[0]
            near, far = struct.unpack_from('<2f', result, 0x40)
            assert result[:0x28] == camera[:0x28]
            assert result[0x2c:0x40] == camera[0x2c:0x40]
            assert lens == .8655884265899658 and near == .015625 and far == 1.
            cases.append((angle, width, height, lens, near, far))
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text('# incoming-vertical-radians width height first-person-vertical-radians near far\n' +
        ''.join(f'{a:.9g}\t{w}\t{h}\t{v:.9g}\t{n:.9g}\t{f:.9g}\n' for a,w,h,v,n,f in cases))
    report = dict(reference_sha256=PIN, instructions=['7e0de7', '7e0f69'],
        hooked_services={}, cases=len(cases), preserves=['position', 'basis', 'outer/safe rectangles'],
        first_person_vertical_angle=cases[0][3], near=cases[0][4], far=cases[0][5],
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(), scope=__doc__)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))

if __name__ == '__main__':
    main()
