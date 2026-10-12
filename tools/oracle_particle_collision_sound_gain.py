#!/usr/bin/env python3
"""Execute unchanged248F80 collision-child gain in both update images.

Explicit particle velocity is the ABI input. No call or instruction is
intercepted. FCW023F/MXCSR1F80 are declared; no spatial mixer is substituted.
"""
import argparse, hashlib, json, random, struct
from pathlib import Path
from oracle_native_footstep_selection import Xbe
from oracle_particle_physics_movement_20261010 import OLD, NEW
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R


def execute(path, velocities):
    image = Xbe(path)
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x800000)
    prefix = bytes.fromhex('51d94030d9402cd94028d9c0d8c9d9c2d8cbd ec1'.replace(' ', ''))
    entries = []
    for section in image.sections:
        raw = image.section_bytes(section)
        guest.mem_write(section.va, raw)
        at = raw.find(prefix)
        if at >= 0:
            entries.append(section.va + at)
    assert len(entries) == 1, entries
    entry = entries[0]
    guest.mem_map(0x1000000, 0x10000)
    stack, particle, stop = 0x100f000, 0x1001000, 0x100ff00
    results = []
    for velocity in velocities:
        guest.mem_write(particle, bytes(64))
        guest.mem_write(particle + 0x28, struct.pack('<3f', *velocity))
        guest.mem_write(stack, struct.pack('<I', stop))
        guest.reg_write(R.UC_X86_REG_ESP, stack)
        guest.reg_write(R.UC_X86_REG_EAX, particle)
        guest.reg_write(R.UC_X86_REG_FPCW, 0x023f)
        guest.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        guest.emu_start(entry, stop, count=1000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == stop
        results.append(guest.reg_read(R.UC_X86_REG_XMM0) & 0xffffffff)
    return entry, results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    rng = random.Random(0x248f80)
    velocities = [(0., 0., 0.), (.5, 0., 0.), (1.5, 0., 0.)]
    for word in (0x3effffff, 0x3f000001, 0x3fbfffff, 0x3fc00001):
        value = struct.unpack('<f', struct.pack('<I', word))[0]
        velocities.extend([(value, 0., 0.), (0., -value, 0.), (0., 0., value)])
    while len(velocities) < 2048:
        scale = 1. if len(velocities) % 2 else 5.
        velocities.append(tuple(struct.unpack('<f', struct.pack('<f', rng.uniform(-scale, scale)))[0] for _ in range(3)))
    pins = ['aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
            '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd']
    for path, pin in zip((OLD, NEW), pins):
        assert hashlib.sha256(path.read_bytes()).hexdigest() == pin
    first, values = execute(OLD, velocities)
    second, other = execute(NEW, velocities)
    assert values == other
    data = b'PCG1' + struct.pack('<I', len(values))
    for velocity, value in zip(velocities, values):
        data += struct.pack('<3fI', *velocity, value)
    (args.output/'fixtures.bin').write_bytes(data)
    report = dict(cases_per_image=len(values), entries=[hex(first), hex(second)],
                  xbe_sha256=pins, intercepted_instructions=0,
                  paired_results_byte_exact=True, fpcw='023F', mxcsr='1F80',
                  boundary=__doc__, fixture_sha256=hashlib.sha256(data).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
