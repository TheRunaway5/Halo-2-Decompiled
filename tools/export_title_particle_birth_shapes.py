#!/usr/bin/env python3
"""Exercise unchanged 249170 for all 1026 original vectors and shapes 2/3/4.

The shape enum is a controlled definition input; all authored properties and
function code are unchanged. This is not live creation or emission evidence.
"""
import argparse, collections, hashlib, json, random, struct, subprocess, sys
from pathlib import Path
from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG, UC_X86_REG_EIP

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE, STACK, STOP = 0x2100000, 0x21fff00, 0x3000000

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    args = parser.parse_args()
    x = Xbe(ROOT / 'assets/disc/default.xbe'); assert hashlib.sha256(x.data).hexdigest() == PIN
    assets = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    g = Uc(UC_ARCH_X86, UC_MODE_32); g.mem_map(0x10000, 0x600000)
    for section in x.sections: g.mem_write(section.va, x.section_bytes(section))
    base = assets.magic & ~4095
    g.mem_map(base, (assets.magic + len(assets.data) - base + 4095) & ~4095)
    g.mem_write(assets.magic, bytes(assets.data)); g.mem_map(STATE, 0x100000); g.mem_map(STOP, 4096)
    seed_pointer, particle_pointer, values_pointer, system_pointer = STATE, STATE + 0x100, STATE + 0x200, STATE + 0x300
    g.mem_write(0x4e7408, struct.pack('<I', seed_pointer))
    owner = 0xe211009c; _, root = assets.tag(owner, 'effe')
    definitions = assets.block(assets.block(root + 20, 56)[0] + 48, 56)
    rng = random.Random(0x249170); inputs = bytearray(struct.pack('<I', 3078)); expected = bytearray()
    inverse_lcg = pow(1664525, -1, 1 << 32); counts = collections.Counter(); vector_indices = set()
    for shape in (2, 3, 4):
        for index in range(1026):
            definition_index = index % 6
            emitter = assets.block(definitions[definition_index] + 48, 184)[0]
            g.mem_write(assets.magic + emitter + 120, struct.pack('<I', shape))
            # Force every table index for sphere shapes, and spread ring azimuth
            # over the same upper-word range without altering original RNG code.
            upper = (index * 65536 + 1025) // 1026
            next_seed = upper << 16
            seed = ((next_seed - 1013904223) * inverse_lcg) & 0xffffffff
            assert ((next_seed >> 16) * 1026) >> 16 == index
            if shape != 4: vector_indices.add(index)
            values = struct.pack('<17f', *(rng.uniform(-.25, 1.25) for _ in range(17)))
            particle = rng.randbytes(64)
            system = bytearray(84); struct.pack_into('<I', system, 24, 0xffffffff)
            g.mem_write(seed_pointer + 4, struct.pack('<I', seed)); g.mem_write(particle_pointer, particle)
            g.mem_write(values_pointer, values); g.mem_write(system_pointer, bytes(system))
            g.mem_write(STACK, struct.pack('<3I', STOP, assets.magic + emitter, system_pointer))
            for register, value in ((UC_X86_REG_ESP, STACK), (UC_X86_REG_EAX, values_pointer),
                (UC_X86_REG_EDI, particle_pointer), (UC_X86_REG_FPCW, 0x027f), (UC_X86_REG_MXCSR, 0x1f80),
                (UC_X86_REG_FPSW, 0), (UC_X86_REG_FPTAG, 0xffff)):
                g.reg_write(register, value)
            g.emu_start(0x249170, STOP, count=100000)
            assert g.reg_read(UC_X86_REG_EIP) == STOP
            inputs += struct.pack('<5I', owner, 0, definition_index, 0, shape) + values + struct.pack('<I', seed) + particle
            expected += bytes(g.mem_read(particle_pointer, 64)) + bytes(g.mem_read(seed_pointer + 4, 4))
            counts[shape] += 1
    ip = ROOT / 'tests/fixtures/title_particle_birth_shapes.inputs.bin'
    ep = ROOT / 'tests/fixtures/title_particle_birth_shapes.original.bin'
    ip.write_bytes(inputs); ep.write_bytes(expected)
    result = subprocess.run([str(args.native.resolve()), '--controlled-shapes', str(assets.path.resolve()), str(ip), str(ep)], capture_output=True, text=True)
    if result.returncode: raise RuntimeError(result.stdout + result.stderr)
    report = {'xbe_sha256': PIN, 'map_sha256': hashlib.sha256(assets.data).hexdigest(), 'entry': '249170',
        'intercepted_instructions': [], 'controlled_definition_fields': ['shape enum'],
        'cases': 3078, 'shapes': dict(counts), 'original_vector_indices': len(vector_indices),
        'original_output_bytes': len(expected), 'different_bytes': 0, 'all_bytes_exact': True,
        'input_sha256': hashlib.sha256(inputs).hexdigest(), 'original_sha256': hashlib.sha256(expected).hexdigest(),
        'native_binary_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(), 'native_test_output': result.stdout.strip(),
        'scope': 'Controlled original sphere, inward-sphere and ring initialization across all original direction indices with authored emitter properties and explicit values/RNG.',
        'physical_xbox_rsqrt_verified': False, 'particle_allocation_emission_history_verified': False,
        'complete_frontend_parity_verified': False}
    (ROOT / 'analysis/frontend/title-particle-birth-shapes.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__': main()
