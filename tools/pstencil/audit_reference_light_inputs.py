#!/usr/bin/env python3
"""Check original sphere light inputs against an actual current material bank.

The original vertex selector executes without instruction or service replacement.
The rasterizer-global binding supplies the original attenuation image. These
declared selector calls and decoded images are not an original GPU capture.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / 'analysis/pstencil/2003-presentation'
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EIP


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Refusing to replace retained evidence')
    resident = module('light_resident', BASE / 'extract_resident_environment.py')
    vertex = module('light_vertex', BASE / 'extract_environment_vertex.py')
    image = vertex.IMAGE
    executable_hash = hashlib.sha256(vertex.PATH.read_bytes()).hexdigest()
    cache_hash = hashlib.sha256(resident.DATA).hexdigest()
    if executable_hash != '5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18' or \
       cache_hash != '1c40d851a19dd9077b1a5cc8d84319e00150678dbfed5d6713e7e786973cc4b1':
        raise RuntimeError('Primary inputs differ')
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    limit = max(s.va + s.vsize for s in image.sections)
    cpu.mem_map(0x10000, (limit - 0x10000 + 4095) & ~4095)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    cpu.mem_map(0x2100000, 0x10000)
    executed = set()

    def instruction(uc, pc, size, _):
        if pc in (0x1db50, 0x1d070):
            raise RuntimeError(f'Original assertion at {pc:08x}')
        if bytes(uc.mem_read(pc, size)) != image.read(pc, size):
            raise RuntimeError('Original instruction differs')
        executed.add((pc, size))

    cpu.hook_add(UC_HOOK_CODE, instruction)
    selections = []
    for context in range(4):
        for layer, tangent in ((16, 0), (16, 1), (17, 0)):
            cpu.mem_write(0x7438a8, struct.pack('<H', context))
            cpu.mem_write(0x2108000, struct.pack('<4I', 0x2100000, layer, 0, 0))
            cpu.reg_write(UC_X86_REG_ESP, 0x2108000)
            cpu.reg_write(UC_X86_REG_EAX, 0)  # Original world vertex class.
            cpu.reg_write(UC_X86_REG_EBX, tangent)
            cpu.emu_start(0x981c0, 0x2100000, count=100000)
            if cpu.reg_read(UC_X86_REG_EIP) != 0x2100000:
                raise RuntimeError('Original selector did not return')
            index = cpu.reg_read(UC_X86_REG_EAX)
            expected = (0x36 if tangent else 0x44) if context == 0 and layer == 16 else \
                       0x28 if context == 0 else (0x2f if tangent else 0x3d) if layer == 16 else 0x25
            if index != expected:
                raise RuntimeError('Original vertex selection differs')
            selections.append(dict(context_7438a8=context, layer=layer,
                                   tangent_selector=tangent, vertex_class=0, program=index))
    programs = []
    for index in sorted({r['program'] for r in selections}):
        descriptor = 0x4beb20 + index * 20
        words = struct.unpack('<5I', image.read(descriptor, 20))
        raw = image.read(words[1], words[3])
        magic, count = struct.unpack_from('<HH', raw)
        if magic != 0x2078 or len(raw) != 4 + count * 16:
            raise RuntimeError('Original vertex program declaration differs')
        instructions = [struct.unpack_from('<4I', raw, 4 + i * 16) for i in range(count)]
        if not all(bool(row[3] & 1) == (i + 1 == count) for i, row in enumerate(instructions)):
            raise RuntimeError('Original vertex termination differs')
        programs.append(dict(index=index, descriptor=f'{descriptor:08x}', code=f'{words[1]:08x}',
            bytes=raw.hex(), sha256=hashlib.sha256(raw).hexdigest(),
            instructions=[dict(words=row, decoded=vertex.decode(row)) for row in instructions]))

    globals_ = next(t for t in resident.TAGS.values() if t['group'] == 'matg')
    root = globals_['payload_file_offset']
    if resident.u(root + 0x134) != 1:
        raise RuntimeError('Original rasterizer row missing')
    rasterizer = resident.u(root + 0x138) - resident.MAGIC
    # Original96200 override9 reads this precise +0C reference.
    bitmap = resident.TAGS[resident.u(rasterizer + 0x0c)]
    if bitmap['group'] != 'bitm' or bitmap['name'] != 'rasterizer\\distance_attenuation':
        raise RuntimeError('Original attenuation reference differs')
    payload = bitmap['payload_file_offset']
    if resident.u(payload + 0x60) != 1:
        raise RuntimeError('Original attenuation image count differs')
    descriptor = resident.u(payload + 0x64) - resident.MAGIC
    u16 = lambda at: int.from_bytes(resident.DATA[at:at + 2], 'little')
    if (resident.u(descriptor), *(u16(descriptor + o) for o in (4, 6, 8, 10, 12, 20))) != \
       (0x6269746d, 32, 32, 32, 1, 2, 5) or not u16(descriptor + 14) & 8:
        raise RuntimeError('Original attenuation volume declaration differs')
    offset, extent = resident.u(descriptor + 0x1c), resident.u(descriptor + 0x2c)
    if offset + extent > len(resident.DATA):
        raise RuntimeError('Original attenuation volume exceeds the cache')
    raw = resident.DATA[offset:offset + extent]
    decoded, mips, consumed = bytearray(), [], 0
    for level in range(6):
        width = 32 >> level
        count = width ** 3
        mips.append([width, width, width, len(decoded), count * 4])
        for z in range(width):
            for y in range(width):
                for x in range(width):
                    morton = sum(((value >> bit) & 1) << (bit * 3 + axis)
                                 for bit in range(width.bit_length() - 1)
                                 for axis, value in enumerate((x, y, z)))
                    decoded.extend([raw[consumed + morton]] * 4)
        consumed += count
    if consumed > len(raw):
        raise RuntimeError('Original mip chain exceeds its padded extent')
    current_path = args.current / 'inputs.json'
    current = json.loads(current_path.read_text())
    current_pixels = (args.current / 'current-attenuation.rgba').read_bytes()
    metadata = current['attenuation']
    same_pixels = current_pixels == decoded
    # A mismatch is retained as evidence, never silently replaced with a formula.
    compatible = (metadata['width'], metadata['height'], metadata['depth'], metadata['layers'],
                  metadata['format'], metadata['mips']) == (32, 32, 32, 1, 11, mips)
    current_programs = []
    for row in current['passes']:
        path = args.current / f"current-vs-{row['pass']}.bin"
        raw = path.read_bytes()
        magic, count = struct.unpack_from('<HH', raw)
        if magic != 0x2078 or len(raw) != 4 + count * 16:
            raise RuntimeError('Current vertex program declaration differs')
        current_programs.append(dict(pass_=row['pass'], vertex_shader=row['vertex_shader'],
            sha256=hashlib.sha256(raw).hexdigest(), instructions=[vertex.decode(struct.unpack_from('<4I', raw, 4 + i * 16))
                for i in range(count)]))
    result = dict(scope=__doc__, original_sha256=executable_hash, cache_sha256=cache_hash,
        current_inputs_sha256=hashlib.sha256(current_path.read_bytes()).hexdigest(),
        selector=dict(entry='000981c0', fixtures=selections, instruction_patches=[], replaced_services=[],
            authenticated_instructions=[dict(address=f'{pc:08x}', size=size,
                sha256=hashlib.sha256(image.read(pc, size)).hexdigest()) for pc, size in sorted(executed)]),
        programs=programs, current_programs=current_programs,
        attenuation=dict(bitmap=bitmap, rasterizer_file_offset=rasterizer, binding_offset=12,
            descriptor_file_offset=descriptor, pixels_file_offset=offset, padded_bytes=extent,
            consumed_bytes=consumed, mips=mips, decoded_bytes=len(decoded),
            original_sha256=hashlib.sha256(decoded).hexdigest(),
            current_sha256=hashlib.sha256(current_pixels).hexdigest(),
            same_pixels=same_pixels, compatible_metadata=compatible), full_presentation_verified=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(selections=len(selections), programs=len(programs),
        authenticated_instructions=len(executed), same_attenuation_pixels=same_pixels,
        compatible_attenuation_metadata=compatible)))


if __name__ == '__main__':
    main()
