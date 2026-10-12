#!/usr/bin/env python3
"""Exercise unchanged 175610/248450 with explicit completed coordinate states.

Only final 23ED30 GPU submission is intercepted. The original tag dispatch,
owner/cache handling, properties, matrix construction and inline transforms
execute. The coordinate enum is an explicit controlled definition input;
these cases are not observations of live emission or first-person title use.
"""
import argparse, hashlib, json, random, struct, subprocess, sys
from pathlib import Path
from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EDI, UC_X86_REG_EBX, UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE, STACK, STOP = 0x2100000, 0x21fff00, 0x3000000

def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--cases', type=int, default=96)
    args = parser.parse_args()
    assert 6 <= args.cases <= 4096
    xbe = Xbe(ROOT / 'assets/disc/default.xbe')
    assert hashlib.sha256(xbe.data).hexdigest() == PIN
    g = Uc(UC_ARCH_X86, UC_MODE_32); g.mem_map(0x10000, 0x600000)
    for section in xbe.sections:
        g.mem_write(section.va, xbe.section_bytes(section))
    g.mem_map(STATE, 0x100000); g.mem_map(STOP, 4096)
    assets = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    base = assets.magic & ~4095
    g.mem_map(base, (assets.magic + len(assets.data) - base + 4095) & ~4095)
    g.mem_write(assets.magic, bytes(assets.data))
    index = assets.u32(16)
    header_magic = assets.u32(index) - index - 32
    table = assets.u32(index + 8) - header_magic + assets.magic
    g.mem_write(0x4e3b44, struct.pack('<I', table))
    system_ptr, location_ptr, cache_ptr, context_ptr = STATE, STATE + 0x100, STATE + 0x200, STATE + 0x300
    emitter_ptr, particle_base = STATE + 0x1000, STATE + 0x2000
    g.mem_write(0x51ec88, struct.pack('<I', STATE + 0x400))
    g.mem_write(STATE + 0x444, struct.pack('<I', emitter_ptr))
    g.mem_write(0x51ec84, struct.pack('<I', STATE + 0x500))
    g.mem_write(STATE + 0x544, struct.pack('<I', particle_base))
    g.mem_write(0x510c54, struct.pack('<I', STATE + 0x600))
    captured = []

    def hook(guest, address, size, _):
        sp = guest.reg_read(UC_X86_REG_ESP)
        if address == 0x224550:
            words = struct.unpack('<8I', guest.mem_read(sp, 32))
            assert words[2] == cache_ptr
            raw = struct.pack('<I', words[1]) + bytes(guest.mem_read(cache_ptr, 72))
            raw += bytes(guest.mem_read(words[3], 12)) + bytes(guest.mem_read(words[4], 12)) + struct.pack('<I', words[5])
            color = bytes(guest.mem_read(words[6], 16))
            captured.append(raw + color[4:16] + color[:4])
        elif address == 0x23ed30:
            words = struct.unpack('<9I', guest.mem_read(sp, 36))
            raw = bytes(guest.mem_read(words[1], 12)) + bytes(guest.mem_read(words[2], 12))
            raw += struct.pack('<4I', *words[3:7])
            for pointer in (guest.reg_read(UC_X86_REG_EBX), *words[7:9]):
                raw += bytes(guest.mem_read(pointer, 16))
            assert len(captured[-1]) == 120 and len(raw) == 88
            captured[-1] += raw
            guest.reg_write(UC_X86_REG_ESP, sp + 36); guest.reg_write(UC_X86_REG_EIP, words[0])

    g.hook_add(UC_HOOK_CODE, hook)
    rng = random.Random(0x175610)
    inputs = bytearray(struct.pack('<I', args.cases)); expected = bytearray()
    owner = 0xe211009c
    _, effect_root = assets.tag(owner, 'effe')
    definitions = assets.block(assets.block(effect_root + 20, 56)[0] + 48, 56)
    branches = []
    total = 0
    for n in range(args.cases):
        definition_index = (2, 4, 5)[(n // 6) % 3]
        at = definitions[definition_index]
        coordinates, first_person = n % 3, bool((n // 3) % 2)
        g.mem_write(assets.magic + at + 12, struct.pack('<H', coordinates))
        count = 0 if n % 13 == 0 else 1 + n % 5
        system = bytearray(84)
        struct.pack_into('<H', system, 14, definition_index)
        struct.pack_into('<3I', system, 16, owner, 0, 0xffffffff)
        struct.pack_into('<f', system, 4, f32(rng.random()))
        struct.pack_into('<2f', system, 40, f32(rng.random()), f32(rng.random()))
        location = bytearray(52); location[2] = first_person; location[3] = n % 256
        struct.pack_into('<I', location, 4, 0x12340000)
        struct.pack_into('<f', location, 28, f32(rng.random()))
        initial = [f32(rng.random()) for _ in range(17)]
        mask = rng.randrange(0x20000)
        owners = (system_ptr if n % 2 else 0, location_ptr if n % 3 else 0, particle_base if n % 4 else 0)
        cache = struct.pack('<17f4I', *initial, mask, *owners)
        clock = struct.pack('<IfI', 0, f32(1/30), n * 31)
        camera = struct.pack('<9f', *(f32(rng.uniform(-10, 10)) for _ in range(3)),
            *(f32(rng.uniform(-1, 1)) for _ in range(6)))
        emitter = bytearray(76)
        struct.pack_into('<2I', emitter, 4, 0x23450000 if count else 0xffffffff, 0xffffffff)
        struct.pack_into('<12f', emitter, 16, *(f32(rng.uniform(-3, 3)) for _ in range(12)))
        particles = []
        for i in range(count):
            particle = bytearray(64)
            struct.pack_into('<I', particle, 4, 0x23450000 + i + 1 if i + 1 < count else 0xffffffff)
            struct.pack_into('<f', particle, 8, f32(rng.random()))
            struct.pack_into('<f', particle, 16, f32(rng.random()))
            struct.pack_into('<4H', particle, 20, *(rng.randrange(65536) for _ in range(4)))
            struct.pack_into('<6f', particle, 28, *(f32(rng.uniform(-100, 100)) for _ in range(6)))
            struct.pack_into('<fI', particle, 56, f32(rng.uniform(-4, 4)), rng.getrandbits(32))
            particles.append(bytes(particle)); g.mem_write(particle_base + i * 64, bytes(particle))
        particle_datum = assets.u32(at + 4)
        tag, particle_root = assets.tag(particle_datum, 'prt3')
        bitmap_datum = assets.u32(assets.block(particle_root + 20, 40)[0] + 12)
        bitmap, bitmap_root = assets.tag(bitmap_datum, 'bitm')
        sequences = assets.block(bitmap_root + 60, 60)
        sequence = 0 if sequences and n % 7 else -1
        width = f32(rng.uniform(.25, 2))
        context = struct.pack('<3If', tag['address'], bitmap['address'], sequences[0] + assets.magic if sequence == 0 else 0, width)
        for address, data in ((system_ptr, system), (location_ptr, location), (cache_ptr, cache),
            (context_ptr, context), (emitter_ptr, emitter), (STATE + 0x600, clock), (0x485618, camera)):
            g.mem_write(address, bytes(data))
        g.mem_write(STACK, struct.pack('<5I', STOP, system_ptr, location_ptr, 0x224550, context_ptr))
        g.reg_write(UC_X86_REG_ESP, STACK); g.reg_write(UC_X86_REG_EDI, cache_ptr)
        for register, value in ((UC_X86_REG_FPCW, 0x027f), (UC_X86_REG_MXCSR, 0x1f80), (UC_X86_REG_FPSW, 0), (UC_X86_REG_FPTAG, 0xffff)):
            g.reg_write(register, value)
        captured.clear()
        try:
            g.emu_start(0x175610, STOP, count=1000000)
        except Exception as error:
            raise RuntimeError(f'Original render producer fault at {g.reg_read(UC_X86_REG_EIP):08x}, case {n}') from error
        assert g.reg_read(UC_X86_REG_EIP) == STOP and len(captured) == count
        inputs += struct.pack('<6I', owner, 0, definition_index, system_ptr, location_ptr, coordinates)
        inputs += cache + system + location + struct.pack('<I', 0) + bytes(400) + clock + camera
        inputs += struct.pack('<ifI', sequence, width, 1) + emitter + struct.pack('<I', count)
        for i, particle in enumerate(particles):
            inputs += struct.pack('<I', particle_base + i * 64) + particle
        expected += struct.pack('<I', count) + b''.join(captured) + bytes(g.mem_read(cache_ptr, 84))
        branches.append({'coordinates': coordinates, 'first_person': first_person, 'particles': count})
        total += count
    fixtures = ROOT / 'tests/fixtures'
    ip = fixtures / 'title_particle_render_coordinates.inputs.bin'
    ep = fixtures / 'title_particle_render_coordinates.original.bin'
    ip.write_bytes(inputs); ep.write_bytes(expected)
    result = subprocess.run([str(args.native.resolve()), '--coordinates', str(assets.path.resolve()), str(ip), str(ep)], text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    report = {'xbe_sha256': PIN, 'original_entry': '175610 -> 1751D0, 173BA0/248DF0, 246CD0/246D80, 248450/142EB0, 224550',
        'intercepted_calls': ['23ED30 final GPU submission'], 'controlled_definition_fields': ['coordinate enum'],
        'cases': args.cases, 'sprite_calls': total, 'original_bytes': len(expected), 'different_bytes': 0,
        'all_handoff_and_submission_bytes_exact': True, 'local_or_first_person_transform_verified': True,
        'input_sha256': hashlib.sha256(inputs).hexdigest(), 'original_sha256': hashlib.sha256(expected).hexdigest(),
        'native_binary_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(),
        'branches': branches, 'native_test_output': result.stdout.strip(),
        'scope': 'Original rendering-input and sprite construction for controlled world/local/parent coordinate inputs and explicit first-person frames; supplied completed simulation state.',
        'camera_scale_branch_verified': False, 'emission_simulation_history_verified': False, 'complete_frontend_parity_verified': False}
    (ROOT / 'analysis/frontend/title-particle-render-coordinates.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k:v for k,v in report.items() if k != 'branches'}, indent=2))

if __name__ == '__main__':
    main()
