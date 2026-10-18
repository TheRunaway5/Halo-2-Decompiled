#!/usr/bin/env python3
"""Run unchanged May2003 A3CF0 on actual current shapes and declared projective fixtures.

No instructions or services are replaced. This establishes the adapted
constant calculation, not an original GPU frame or a live May2003 light record.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from oracle_smg_firing_1_1 import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_FPCW


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--current-inputs', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    if args.output.exists():
        raise RuntimeError('Refusing to replace retained evidence')
    path = Path('/mnt/SSD4TB/Leaks/Halo 2/2003/game/halo2debug.xbe')
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != '5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18':
        raise RuntimeError('Original executable differs')
    image = Xbe(path)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    limit = max(s.va + s.vsize for s in image.sections)
    cpu.mem_map(0x10000, (limit - 0x10000 + 4095) & ~4095)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    cpu.mem_map(0x2100000, 0x20000)
    definition, shape, frame, diffuse, specular, output, shift = [0x2110000 + n * 0x400 for n in range(7)]
    instructions, written = set(), set()

    def code(uc, pc, size, _):
        if pc in (0x1db50, 0x1d070):
            raise RuntimeError(f'Original assertion at {pc:08x}')
        if bytes(uc.mem_read(pc, size)) != image.read(pc, size):
            raise RuntimeError('Original instruction differs')
        instructions.add((pc, size))

    def store(uc, access, address, size, value, _):
        if output <= address < output + 128:
            written.update(range(address - output, address - output + size))

    cpu.hook_add(UC_HOOK_CODE, code)
    cpu.hook_add(UC_HOOK_MEM_WRITE, store)
    f32 = lambda v: struct.unpack('<f', struct.pack('<f', v))[0]
    bits = lambda v: struct.unpack('<I', struct.pack('<f', v))[0]
    vector = lambda v: struct.pack('<' + 'f' * len(v), *v)
    random_ = random.Random(0x2003a3cf02)
    rows = json.loads(args.current_inputs.read_text())
    actual_count = len(rows)
    for n in range(512):
        s = [0] * 31
        s[0], s[6] = 2 + (n & 1), 1
        # Deliberately non-axis-aligned declared inputs cover the literal
        # arithmetic, including intermediate float stores. They are not poses.
        for index in (7, 8, 9, 10, 11, 12, 13, 15, 16, 17):
            s[index] = bits(random_.uniform(-10, 10))
        for index in (19, 20):
            s[index] = bits(random_.uniform(.05, 4))
        near = f32(random_.uniform(.01, 5))
        s[24], s[25] = bits(near), bits(near + random_.uniform(.1, 40))
        rows.append(dict(name=f'declared-{n}', shape=s,
                         frame=[f32(random_.uniform(-3, 3)) for _ in range(16)],
                         falloff=1 + (n >> 1 & 1)))
    fixtures = []
    for row in rows:
        if row['shape'][0] not in (2, 3) or row['falloff'] not in (1, 2):
            raise RuntimeError('Unknown current shape/falloff crosswalk')
        previous = [[f32(random_.uniform(-1, 1)) for _ in range(4)] for _ in range(8)]
        c20w = f32(random_.uniform(-1, 1))
        cpu.mem_write(definition, bytes(0x260))
        cpu.mem_write(definition + 0x17a, struct.pack('<H', row['falloff']))
        cpu.mem_write(shape, struct.pack('<31I', *row['shape']))
        cpu.mem_write(frame, vector(row['frame'][:16]))
        cpu.mem_write(diffuse, vector([.2, .3, .4]))
        cpu.mem_write(specular, vector([.5, .6, .7]))
        cpu.mem_write(output, vector([v for r in previous for v in r]))
        cpu.mem_write(0x2108000, struct.pack('<5I2fI', 0x2100000, definition, shape,
                                          diffuse, specular, c20w, 1., shift))
        cpu.reg_write(UC_X86_REG_ESP, 0x2108000)
        cpu.reg_write(UC_X86_REG_EBX, frame)
        cpu.reg_write(UC_X86_REG_ESI, output)
        cpu.reg_write(UC_X86_REG_FPCW, 0x23f)
        written.clear()
        cpu.emu_start(0xa3cf0, 0x2100000, count=100000)
        if cpu.reg_read(UC_X86_REG_EIP) != 0x2100000:
            raise RuntimeError('Original constant builder did not return')
        result_bits = struct.unpack('<24I', bytes(cpu.mem_read(output, 96)))
        fixtures.append(dict(name=row['name'], shape=row['shape'], frame=row['frame'][:16],
            falloff=row['falloff'], constant20_w=c20w, previous=previous[:6],
            constants_bits=result_bits, written_lanes=[n for n in range(24)
                if set(range(n * 4, n * 4 + 4)) <= written]))
    report = dict(scope=__doc__, original_sha256=digest, entry='000a3cf0', fpcw='023f',
        current_inputs_sha256=hashlib.sha256(args.current_inputs.read_bytes()).hexdigest(),
        actual_current_shapes=actual_count, declared_shapes=len(rows) - actual_count,
        instruction_patches=[], replaced_services=[], fixtures=fixtures,
        instructions=[dict(address=f'{pc:08x}', size=size,
            sha256=hashlib.sha256(image.read(pc, size)).hexdigest()) for pc, size in sorted(instructions)],
        literal_scalars={f'{a:08x}': image.read(a, 4).hex() for a in (0x3e22d0, 0x3dc074, 0x3f5b30)},
        full_presentation_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(fixtures=len(fixtures), actual_current_shapes=actual_count,
                         authenticated_instructions=len(instructions))))


if __name__ == '__main__':
    main()
