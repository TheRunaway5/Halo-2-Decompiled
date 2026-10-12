#!/usr/bin/env python3
"""Execute the unchanged original collider response arithmetic in both updates.

The inputs are explicit contact results, not a claim about collision queries.
Runs the real frame helper and normalization; no instructions are intercepted.
Emission, collision admission, child effects and complete frames are separate.
"""
import argparse
import hashlib
import json
import math
import random
import struct
from pathlib import Path

from oracle_particle_physics_movement_20261010 import OLD, NEW, ROOT, Xbe, raw, find
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn import x86_const as R

PINS = {
    OLD: 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
    NEW: '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
}
PARTICLE, STACK = 0x1000000, 0x1010000
ENTRY, END = 0x2bf6e7, 0x2bf838


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fixtures():
    source = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-local-effect-owner/audio-unavailable/report.json'
    assert sha(source) == 'e30bf2ee9f87012a8778e370196679d2ee9739bbbb3409b85e238f611656dcc5'
    report = json.loads(source.read_text())['final_state']
    pool = next(p for p in report['pools'] if p['global_address'] == '00526514')
    original = bytes.fromhex(pool['occupied'][0]['raw_hex'])
    assert len(original) == 64
    rng = random.Random(ENTRY)
    cases = []
    for index in range(2048):
        particle = bytearray(original)
        velocity = [rng.uniform(-100, 100) for _ in range(3)]
        normal = [rng.uniform(-1, 1) for _ in range(3)]
        if index < 6:
            normal = [0., 0., 0.]
            normal[index // 2] = -1. if index & 1 else 1.
        n = math.sqrt(sum(v*v for v in normal))
        normal = [v/n for v in normal]
        position = [rng.uniform(-250, 250) for _ in range(3)]
        struct.pack_into('<3f', particle, 0x28, *velocity)
        struct.pack_into('<f', particle, 0x34, rng.uniform(-20, 20))
        friction = (0., 1., .5, -.25, 1.25)[index % 5]
        restitution = (0., 1., .5, 1.25)[index % 4]
        parameters = struct.pack('<8f', *position, *normal, friction, restitution)
        cases.append((bytes(particle), parameters))
    return cases, source


def run(path, entry, cases):
    assert sha(path) == PINS[path]
    image = Xbe(path)
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x900000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    guest.mem_map(PARTICLE, 0x20000)
    outputs = []
    for particle, parameters in cases:
        position = parameters[:12]
        normal = parameters[12:24]
        guest.mem_write(PARTICLE, particle)
        guest.mem_write(STACK - 0x1000, bytes(0x2000))
        guest.mem_write(STACK + 0x2c, parameters[24:])
        guest.mem_write(STACK + 0xb8, position)
        guest.mem_write(STACK + 0xd8, normal)
        guest.mem_write(STACK + 0xd4, struct.pack('<I', 0xffffffff))
        for reg, value in ((R.UC_X86_REG_ESP, STACK), (R.UC_X86_REG_EBP, PARTICLE),
                           (R.UC_X86_REG_FPCW, 0x27f), (R.UC_X86_REG_MXCSR, 0x1f80),
                           (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff)):
            guest.reg_write(reg, value)
        guest.emu_start(entry, entry + END - ENTRY, count=10000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == entry + END - ENTRY
        outputs.append(bytes(guest.mem_read(PARTICLE, 64)))
    return outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    cases, source = fixtures()
    old = Xbe(OLD)
    new_entry = find(Xbe(NEW), raw(old, ENTRY, 16))
    original = run(OLD, ENTRY, cases)
    updated = run(NEW, new_entry, cases)
    assert original == updated
    data = b'PCR1' + struct.pack('<I', len(cases))
    data += b''.join(p + parameters + output for (p, parameters), output in zip(cases, original))
    fixture = args.output / 'fixtures.bin'
    fixture.write_bytes(data)
    report = dict(tool_sha256=sha(Path(__file__)), fixture_sha256=sha(fixture),
                  cases_per_image=len(cases), native_boundary=__doc__,
                  source_particle_sha256=sha(source), intercepted_instructions=0,
                  images=[dict(path=str(path), sha256=pin, entry=f'{entry:08x}')
                          for (path, pin), entry in zip(PINS.items(), (ENTRY, new_entry))],
                  paired_results_byte_exact=True, whole_frame_parity_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
