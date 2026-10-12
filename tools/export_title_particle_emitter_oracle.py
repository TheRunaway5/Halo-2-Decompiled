#!/usr/bin/env python3
"""Exercise unchanged 247470 with controlled flags, frames and orientations.

Only its 247950 creation callback is intercepted. The stub writes an explicit
particle head/count; the original timing, transform and property code executes.
"""
import argparse, hashlib, json, random, struct, subprocess, sys
from pathlib import Path
from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE, STACK, STOP = 0x2100000, 0x21fff00, 0x3000000

def main():
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--native', type=Path, required=True)
    args = parser.parse_args();x = Xbe(ROOT / 'assets/disc/default.xbe')
    assert hashlib.sha256(x.data).hexdigest() == PIN
    g = Uc(UC_ARCH_X86, UC_MODE_32);g.mem_map(0x10000, 0x600000)
    for section in x.sections:g.mem_write(section.va, x.section_bytes(section))
    g.mem_map(STATE, 0x100000);g.mem_map(STOP, 4096);assets = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    base = assets.magic & ~4095;g.mem_map(base, (assets.magic + len(assets.data) - base + 4095) & ~4095)
    g.mem_write(assets.magic, bytes(assets.data));index = assets.u32(16);header_magic = assets.u32(index) - index - 32
    g.mem_write(0x4e3b44, struct.pack('<I', assets.u32(index + 8) - header_magic + assets.magic))
    owner = 0xe211009c;_, root = assets.tag(owner, 'effe');definitions = assets.block(assets.block(root + 20, 56)[0] + 48, 56)
    emitter_ptr, system_ptr, values_ptr, frame_ptr = STATE, STATE + 0x100, STATE + 0x200, STATE + 0x300
    calls = [];updates = [];case = 0
    def hook(guest, address, size, _):
        if address != 0x247950:return
        sp = guest.reg_read(UC_X86_REG_ESP);w = struct.unpack('<7I', guest.mem_read(sp, 28))
        assert guest.reg_read(UC_X86_REG_ECX) == emitter_ptr and w[1] == system_ptr
        before = bytes(guest.mem_read(emitter_ptr, 76))
        calls.append(struct.pack('<4I', w[4], w[5], w[6], w[3]) + before)
        head = 0x12340000 + case * 64 + len(calls)
        count = (struct.unpack_from('<H', before, 2)[0] + 1) & 65535
        guest.mem_write(emitter_ptr + 2, struct.pack('<HI', count, head));updates.append((head, count))
        guest.reg_write(UC_X86_REG_ESP, sp + 28);guest.reg_write(UC_X86_REG_EIP, w[0])
    g.hook_add(UC_HOOK_CODE, hook)
    rng = random.Random(0x247470);inputs = bytearray(struct.pack('<I', 256));expected = bytearray();total = 0;branches = []
    for case in range(256):
        definition_index = case % 6;definition = definitions[definition_index];emitter = assets.block(definition + 48, 184)[0]
        flags = (4 if case & 1 else 0) | (128 if case & 2 else 0)
        system_flags = (1024 if case & 4 else 0) | (512 if case & 8 else 0)
        offset = struct.pack('<3f', *(rng.uniform(-3, 3) for _ in range(3)))
        threshold = (0., .00009999, .0001, -.0001, .00010001)
        angles = struct.pack('<2f', threshold[case % 5] if case % 3 else rng.uniform(-3, 3), rng.uniform(-3, 3) if case % 4 else 0.)
        g.mem_write(assets.magic + definition + 22, struct.pack('<H', flags))
        g.mem_write(assets.magic + emitter + 156, offset + angles)
        raw_system = bytearray(84);struct.pack_into('<HH3I', raw_system, 12, system_flags, definition_index, owner, 0, 0xffffffff)
        state = bytearray(rng.randbytes(76));head = 0xffffffff if case % 2 else 0x11220000
        remainder = (.9998, .9999, 1., 1.0001, 2.75, -.25, .25)[case % 7]
        struct.pack_into('<HHI', state, 0, 0x8000, case % 65536, head);struct.pack_into('<f', state, 12, remainder)
        struct.pack_into('<15f', state, 16, *(rng.uniform(-3, 3) for _ in range(15)))
        frame = struct.pack('<13f', rng.uniform(.25, 3), *(rng.uniform(-3, 3) for _ in range(12)))
        values = struct.pack('<17f', *(rng.random() for _ in range(17)))
        dt_bits = struct.unpack('<I', struct.pack('<f', (0., 1/30, 1/60, .001, .05)[case % 5]))[0]
        has_frame = case % 9 != 0;color = rng.getrandbits(32)
        for address, data in ((emitter_ptr, state), (system_ptr, raw_system), (values_ptr, values), (frame_ptr, frame)):
            g.mem_write(address, bytes(data))
        g.mem_write(STACK, struct.pack('<7I', STOP, dt_bits, system_ptr, assets.magic + emitter, values_ptr, frame_ptr if has_frame else 0, color))
        for register, value in ((UC_X86_REG_ESP, STACK), (UC_X86_REG_ECX, emitter_ptr), (UC_X86_REG_FPCW, 0x027f), (UC_X86_REG_MXCSR, 0x1f80), (UC_X86_REG_FPSW, 0), (UC_X86_REG_FPTAG, 0xffff)):
            g.reg_write(register, value)
        calls.clear();updates.clear();g.emu_start(0x247470, STOP, count=1000000)
        assert g.reg_read(UC_X86_REG_EIP) == STOP
        inputs += struct.pack('<5I', owner, 0, definition_index, 0, flags) + offset + angles
        inputs += struct.pack('<4I', system_flags, dt_bits, color, has_frame) + frame + state + values
        inputs += struct.pack('<I', len(updates)) + b''.join(struct.pack('<2I', *update) for update in updates)
        expected += struct.pack('<I', len(calls)) + b''.join(calls) + bytes(g.mem_read(emitter_ptr, 76))
        total += len(calls);branches.append({'flags': flags, 'system_flags': system_flags, 'frame': has_frame, 'births': len(calls)})
    ip = ROOT / 'tests/fixtures/title_particle_emitter_oracle.inputs.bin';ep = ROOT / 'tests/fixtures/title_particle_emitter_oracle.original.bin'
    ip.write_bytes(inputs);ep.write_bytes(expected)
    result = subprocess.run([str(args.native.resolve()), '--controlled', str(assets.path.resolve()), str(ip), str(ep)], capture_output=True, text=True)
    if result.returncode:raise RuntimeError(result.stdout + result.stderr)
    report = {'xbe_sha256': PIN, 'map_sha256': hashlib.sha256(assets.data).hexdigest(), 'entry': '247470',
        'intercepted_calls': ['247950 creation callback; explicit particle head/count writes'],
        'controlled_definition_fields': ['system flags', 'emitter offset', 'emitter orientation'],
        'cases': 256, 'birth_requests': total, 'original_output_bytes': len(expected), 'different_bytes': 0, 'all_bytes_exact': True,
        'input_sha256': hashlib.sha256(inputs).hexdigest(), 'original_sha256': hashlib.sha256(expected).hexdigest(),
        'native_binary_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(), 'native_test_output': result.stdout.strip(), 'branches': branches,
        'scope': 'Original placement/matrix/property/rate/threshold/timing code with controlled flags and explicit creation callback outputs.',
        'particle_allocation_creation_history_verified': False, 'complete_frontend_parity_verified': False}
    (ROOT / 'analysis/frontend/title-particle-emitter-oracle.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k:v for k,v in report.items() if k != 'branches'}, indent=2))

if __name__ == '__main__':main()
