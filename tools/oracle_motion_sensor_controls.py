#!/usr/bin/env python3
"""Execute original two-hand input mapping and grenade-phase radar admission.

The control comparison starts at C708A and stops before1045E0, with explicit
hand/admission inputs. The radar comparison runs the complete visibility owner
and its real E70E0 component-byte lookup. Movement is supplied false at its
separate owner boundary. Device polling, full unit update and pixels are outside
these bounded comparisons. No original instruction bytes are patched.
"""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from weapon_reference_versions import images, unique_entry
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX,
    UC_X86_REG_ESP, UC_X86_REG_EIP)

MEM, STACK, STOP = 0x1000000, 0x100ff00, 0x3000000
POOL, HEADERS, BODY, OPTIONS = MEM, MEM+0x1000, MEM+0x2000, MEM+0x3000


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    originals = images()
    tu = originals[0][1]
    retail = next(x for name, x, pin in originals if name == 'retail')
    control_bytes = tu.read(0xc708a, 0xc71e3-0xc708a)
    phase_bytes = retail.read(0xe70e0, 0x22)
    visibility_anchor = retail.read(0x1a2125, 0x16)
    rows = []
    dis = Cs(CS_ARCH_X86, CS_MODE_32)
    for label, x, pin in originals:
        control = unique_entry(x, control_bytes[:32])
        phase = unique_entry(x, phase_bytes[6:], 6)
        visibility = unique_entry(x, visibility_anchor, 0x3f)
        assert x.read(control, len(control_bytes)) == control_bytes
        assert x.read(phase+6, 0x1c) == phase_bytes[6:]
        body = x.read(visibility, 0xaf)
        ins = list(dis.disasm(body, visibility))
        calls = [int(i.op_str, 16) for i in ins if i.mnemonic == 'call']
        assert calls[0] == phase and len(calls) == 2
        moving = calls[1]
        objects = struct.unpack_from('<I', x.read(phase, 6), 2)[0]
        options = struct.unpack_from('<I', body, 0x5c)[0]
        force = struct.unpack_from('<I', body, 0x9d)[0]
        u = Uc(UC_ARCH_X86, UC_MODE_32)
        u.mem_map(0x10000, 0x800000)
        for section in x.sections:
            u.mem_write(section.va, x.section_bytes(section))
        u.mem_map(MEM, 0x10000)
        u.mem_map(STOP, 0x1000)
        def put(address, value):
            u.mem_write(address, struct.pack('<I', value & 0xffffffff))
        def movement_boundary(uc, pc, size, unused):
            if pc != moving:
                return
            sp = uc.reg_read(UC_X86_REG_ESP)
            uc.reg_write(UC_X86_REG_EIP, struct.unpack('<I', uc.mem_read(sp, 4))[0])
            uc.reg_write(UC_X86_REG_ESP, sp+8)
            uc.reg_write(UC_X86_REG_EAX, 0)
        u.hook_add(UC_HOOK_CODE, movement_boundary)
        put(objects, POOL); put(POOL+0x44, HEADERS); put(HEADERS+8, BODY)
        put(options, OPTIONS); put(OPTIONS+8, 2); u.mem_write(force, b'\0')
        u.mem_write(BODY+0x346, struct.pack('<h', 0x400))
        controls = []
        for hand, bits in itertools.product(range(2), range(256)):
            put(BODY+0x148, bits << 16)
            u.mem_write(BODY+0x240, b'\xff')
            put(STACK+0x24, 0x12340001)
            u.mem_write(STACK+0x13, bytes([hand == 0]))
            u.reg_write(UC_X86_REG_ESP, STACK)
            u.reg_write(UC_X86_REG_EBP, BODY)
            u.reg_write(UC_X86_REG_EBX, int(hand == 0))
            u.reg_write(UC_X86_REG_EAX, 0)
            u.emu_start(control, control+len(control_bytes), count=200)
            assert u.reg_read(UC_X86_REG_EIP) == control+len(control_bytes)
            result = struct.unpack('<H', u.mem_read(STACK+0x20, 2))[0]
            held = bool(result & 2)
            assert held == bool(bits & (1 if hand == 0 else 0x20))
            controls.append([hand, bits << 16, result])
        visible = []
        for bits, grenade_phase in itertools.product((0, 1, 0x20, 0x21), range(4)):
            u.mem_write(BODY+0x14a, bytes([bits]))
            u.mem_write(BODY+0x40c, bytes([grenade_phase]))
            put(STACK, STOP); put(STACK+4, 0x12340000)
            u.reg_write(UC_X86_REG_ESP, STACK)
            u.emu_start(visibility, STOP, count=200)
            assert u.reg_read(UC_X86_REG_EIP) == STOP
            result = u.reg_read(UC_X86_REG_EAX) & 255
            assert result == bool(bits or grenade_phase in (1, 2))
            visible.append([bits, grenade_phase, result])
        rows.append(dict(image=label, image_sha256=pin, control_entry=f'{control:08x}',
            phase_entry=f'{phase:08x}', visibility_entry=f'{visibility:08x}',
            control_cases=controls, visibility_cases=visible))
    assert all(r['control_cases'] == rows[0]['control_cases'] and
               r['visibility_cases'] == rows[0]['visibility_cases'] for r in rows)
    report = dict(schema=1, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        boundary=__doc__, images=rows, original_instruction_patches=False,
        movement_boundary=False, trigger_bits=[16, 21], grenade_phase_offset=0xc)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(f'PASS {len(rows)} original images, 512 hand-input and 16 radar-phase cases per image')


if __name__ == '__main__':
    main()
