#!/usr/bin/env python3
"""Original shape1 births from the actual Lockout plasma explosion ring emitter.

Runs unchanged 249170 and all property/RNG helpers. Only completed property
values and initial particle bytes are controlled; no instruction is replaced.
"""
import argparse
import hashlib
import json
import random
import struct
import sys
from pathlib import Path

from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    pin = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    assert hashlib.sha256(image.data).hexdigest() == pin
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    owner = 0xe7c0063b
    _, root = cache.tag(owner, 'effe')
    systems = cache.block(cache.block(root + 0x14, 0x38)[0] + 0x30, 0x38)
    emitter = cache.block(systems[4] + 0x30, 0xb8)[0]
    assert cache.u32(emitter + 0x78) == 1
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    base = cache.magic & ~4095
    guest.mem_map(base, (cache.magic + len(cache.data) - base + 4095) & ~4095)
    guest.mem_write(cache.magic, bytes(cache.data))
    state, stack, stop = 0x2100000, 0x21fff00, 0x3000000
    guest.mem_map(state, 0x100000)
    guest.mem_map(stop, 4096)
    guest.mem_write(0x4e7408, struct.pack('<I', state))
    system = bytearray(84)
    struct.pack_into('<I', system, 24, 0xffffffff)
    guest.mem_write(state + 0x300, bytes(system))
    rng = random.Random(0x24917001)
    count = 2048
    inputs, expected = bytearray(struct.pack('<I', count)), bytearray()
    for case in range(count):
        values = struct.pack('<17f', *(rng.random() for _ in range(17)))
        seed = rng.getrandbits(32)
        particle = rng.randbytes(64)
        guest.mem_write(state + 4, struct.pack('<I', seed))
        guest.mem_write(state + 0x100, particle)
        guest.mem_write(state + 0x200, values)
        guest.mem_write(stack, struct.pack('<3I', stop, cache.magic + emitter, state + 0x300))
        for reg, value in ((R.UC_X86_REG_ESP, stack), (R.UC_X86_REG_EAX, state + 0x200),
                           (R.UC_X86_REG_EDI, state + 0x100), (R.UC_X86_REG_FPCW, 0x27f),
                           (R.UC_X86_REG_MXCSR, 0x1f80), (R.UC_X86_REG_FPSW, 0),
                           (R.UC_X86_REG_FPTAG, 0xffff)):
            guest.reg_write(reg, value)
        guest.emu_start(0x249170, stop, count=100000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == stop
        inputs += struct.pack('<4I', owner, 0, 4, 0) + values + struct.pack('<I', seed) + particle
        expected += bytes(guest.mem_read(state + 0x100, 64)) + bytes(guest.mem_read(state + 4, 4))
    (args.output / 'inputs.bin').write_bytes(inputs)
    (args.output / 'original.bin').write_bytes(expected)
    report = dict(xbe_sha256=pin, map_sha256=hashlib.sha256(cache.data).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  input_sha256=hashlib.sha256(inputs).hexdigest(),
                  original_sha256=hashlib.sha256(expected).hexdigest(), cases=count,
                  owner=f'{owner:08x}', event=0, system=4, emitter=0, shape=1,
                  entry='00249170', intercepted_instructions=0, boundary=__doc__)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
