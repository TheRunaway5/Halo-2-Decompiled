#!/usr/bin/env python3
"""Execute retail camera-axis generation and c6 upload, stopping before D3D.

This records original instructions as evidence for the native backward-vector
binding. It does not compare native floating-point arithmetic or framebuffers.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EIP,
    UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR,
)

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE, OUTPUT, STACK = 0x2100000, 0x2101000, 0x21fff00


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=ROOT/'analysis/appearance/scope-original-camera.json')
    args = parser.parse_args()
    image = (ROOT/'assets/disc/default.xbe').read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED:
        raise ValueError('unsupported retail executable')
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x10000, 0x600000)
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    for i in range(count):
        _, address, _, offset, size = struct.unpack_from('<5I', image, table-base+i*56)
        u.mem_write(address, image[offset:offset+size])
    u.mem_map(STATE, 0x100000)
    # These independent projection helpers supply no camera-axis or c6 values.
    u.mem_write(0x2fb70, b'\xc3')
    u.mem_write(0x12d50, b'\xc2\x04\x00')
    u.mem_write(0x404fe8, b'\0')
    results = []
    for forward in [(1., 0., 0.), (0., 1., 0.), (0., 0., 1.), (.6, .8, 0.)]:
        up = (0., 0., 1.) if forward[2] == 0 else (0., 1., 0.)
        u.mem_write(STATE, struct.pack('<22f', 10, 20, 30, *forward, *up, *([0.]*13)))
        u.mem_write(OUTPUT, b'\0'*256)
        u.mem_write(STACK, struct.pack('<I', 0x3000000))
        u.reg_write(UC_X86_REG_ESI, STATE)
        u.reg_write(UC_X86_REG_EBX, OUTPUT)
        u.reg_write(UC_X86_REG_ESP, STACK)
        u.reg_write(UC_X86_REG_FPCW, 0x027f)
        u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        u.emu_start(0x2fd90, 0x141590, count=10000)
        if u.reg_read(UC_X86_REG_EIP) != 0x141590:
            raise RuntimeError('retail camera generation did not reach the matrix inversion')
        axes = struct.unpack('<12f', u.mem_read(OUTPUT+0x38, 48))
        if any(abs(a+b) > 2e-7 for a, b in zip(axes[6:9], forward)):
            raise RuntimeError('retail camera backward convention changed')
        u.mem_write(STATE+0x30, struct.pack('<4h', 0, 0, 480, 640))
        u.mem_write(STACK, struct.pack('<3I', 0x3000000, 0, 0))
        u.reg_write(UC_X86_REG_EAX, OUTPUT)
        u.reg_write(UC_X86_REG_ECX, STATE)
        u.reg_write(UC_X86_REG_ESP, STACK)
        u.emu_start(0x12fa0, 0x132b3, count=10000)
        c6 = struct.unpack('<4f', u.mem_read(0x407798, 16))
        if c6 != axes[6:9] + (2.,):
            raise RuntimeError('retail c6 differs from the generated backward vector')
        results.append({'forward': forward, 'original_axes_right_up_backward_eye': axes,
                        'original_uploaded_c6': c6})
    report = {
        'xbe_sha256': EXPECTED, 'executed_functions': ['0002FD90', '00012FA0'],
        'neutralized_projection_functions': ['0002FB70', '00012D50'],
        'stop_before': ['00141590', '000132B3'], 'cases': len(results),
        'scope': 'original camera axes and c6 upload; no native arithmetic or framebuffer comparison',
        'results': results,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(f'Original camera generation and c6 upload: {len(results)}/{len(results)} passed')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
