#!/usr/bin/env python3
"""Execute the unchanged May2003 sphere light constant builder on declared inputs.

The actual A4E75 call supplies EBX (completed record), ESI (C15..22 output)
and seven stack arguments. Sphere geometry avoids the separate directional
normalization helper. No instructions or services are replaced. Fixtures are
not captured light records or original GPU frames.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from oracle_smg_firing_1_1 import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ESI, UC_X86_REG_FPCW


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
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
    definition, shape, record, diffuse, specular, output, shift = [0x2110000 + n * 0x400 for n in range(7)]
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
    vector = lambda values: struct.pack('<' + 'f' * len(values), *values)
    random_ = random.Random(0x2003a3cf0)
    fixtures = []
    for n in range(256):
        position = [f32(random_.uniform(-80, 80)) for _ in range(3)]
        radius = f32(random_.uniform(.125, 30))
        colors = [[f32(random_.uniform(0, 1)) for _ in range(3)] for _ in range(2)]
        brightness = (.25, 1., 2., 4.)[n % 4]
        previous = [[f32(random_.uniform(-.7, .7)) for _ in range(4)] for _ in range(8)]
        left, up, forward = [0., 1., 0.], [0., 0., 1.], [1., 0., 0.]
        if n & 4:
            left, up, forward = [1., 0., 0.], [0., 1., 0.], [0., 0., 1.]
        cpu.mem_write(definition, bytes(0x260))
        cpu.mem_write(definition + 0x17a, struct.pack('<H', 2))  # Authored spherical falloff.
        cpu.mem_write(shape, bytes(0x80))
        cpu.mem_write(shape + 0x18, b'\x01')  # Original completed geometry validity flag.
        cpu.mem_write(shape + 0x1c, vector(left))
        cpu.mem_write(shape + 0x2c, vector(position))
        cpu.mem_write(shape + 0x44, vector([radius]))
        cpu.mem_write(record, bytes(0x80))
        cpu.mem_write(record + 0x28, vector(forward))
        cpu.mem_write(record + 0x34, vector(up))
        cpu.mem_write(diffuse, vector(colors[0]))
        cpu.mem_write(specular, vector(colors[1]))
        cpu.mem_write(output, vector([v for row in previous for v in row]))
        cpu.mem_write(shift, b'\xff' * 4)
        stack = 0x2108000
        cpu.mem_write(stack, struct.pack('<5I2fI', 0x2100000, definition, shape, diffuse,
            specular, 0., brightness, shift))
        cpu.reg_write(UC_X86_REG_ESP, stack)
        cpu.reg_write(UC_X86_REG_EBX, record)
        cpu.reg_write(UC_X86_REG_ESI, output)
        cpu.reg_write(UC_X86_REG_FPCW, 0x23f)
        written.clear()
        cpu.emu_start(0xa3cf0, 0x2100000, count=100000)
        if cpu.reg_read(UC_X86_REG_EIP) != 0x2100000:
            raise RuntimeError('Original constant builder did not return')
        values = struct.unpack('<32f', bytes(cpu.mem_read(output, 128)))
        changed_lanes = [i for i in range(32) if set(range(i * 4, i * 4 + 4)) <= written]
        if any(set(range(i * 4, i * 4 + 4)) & written and i not in changed_lanes for i in range(32)):
            raise RuntimeError('Original partial scalar output write')
        fixtures.append(dict(position=position, radius=radius, left=left, up=up, forward=forward,
            diffuse=colors[0], specular=colors[1], brightness=brightness, previous=previous,
            constants=[list(values[i:i + 4]) for i in range(0, 32, 4)],
            written_lanes=changed_lanes, color_shift=struct.unpack('<I', bytes(cpu.mem_read(shift, 4)))[0]))
    color_fixtures = []
    colors = [[0., 0., 0.], [1., 1., 1.]]
    for i in range(255):
        threshold = f32((i + .5) / 255.)
        bits = struct.unpack('<I', struct.pack('<f', threshold))[0]
        for adjacent in (-1, 0, 1):
            value = struct.unpack('<f', struct.pack('<I', bits + adjacent))[0]
            colors.append([value, f32((254 - i) / 255.), f32(i / 255.)])
    colors += [[f32(random_.uniform(0, 1)) for _ in range(3)] for _ in range(256)]
    for color in colors:
        cpu.mem_write(diffuse, vector(color))
        cpu.mem_write(0x2108000, struct.pack('<2I', 0x2100000, diffuse))
        cpu.reg_write(UC_X86_REG_ESP, 0x2108000)
        cpu.reg_write(UC_X86_REG_FPCW, 0x23f)
        cpu.emu_start(0x2f380, 0x2100000, count=100000)
        if cpu.reg_read(UC_X86_REG_EIP) != 0x2100000:
            raise RuntimeError('Original RGB packer did not return')
        color_fixtures.append(dict(rgb=color, packed=cpu.reg_read(UC_X86_REG_EAX)))
    globals_ = {f'{a:08x}': image.read(a, 4).hex() for a in
                (0x48f37c, 0x48f460, 0x48f478, 0x48f488, 0x3e2284, 0x3e22d0, 0x3dc074)}
    result = dict(scope=__doc__, original_sha256=digest, entry='000a3cf0', caller='000a4e75',
        fpcw='023f', globals_from_image=globals_, instruction_patches=[], replaced_services=[],
        instructions=[dict(address=f'{pc:08x}', size=size,
            sha256=hashlib.sha256(image.read(pc, size)).hexdigest()) for pc, size in sorted(instructions)],
        fixtures=fixtures, color_packer_entry='0002f380', color_fixtures=color_fixtures,
        full_presentation_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(fixtures=len(fixtures), color_fixtures=len(color_fixtures),
                         authenticated_instructions=len(instructions),
                         written_lanes=fixtures[0]['written_lanes'])))


if __name__ == '__main__':
    main()
