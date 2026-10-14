#!/usr/bin/env python3
"""Execute unchanged TU1.5 point impulses against read-only live captures.

The native world has already computed every captured result. This independent
test executes the original79-instruction324F80 body with the same motion,
impulse and point inputs, and compares all256 motion bytes. It supplies no
results to the game and does not establish full contact/solver parity.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code
from generate_physics_live_support_tu15 import complete_body
from research_mp_sandbox_update_functions import NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EFLAGS,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR,
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--require-object', type=lambda value: int(value, 0))
    args = parser.parse_args()
    assert hashlib.sha256(args.xbe.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(args.xbe)
    code = _Code(image, '.text')
    body = complete_body(code, 0x324f80)
    assert len(body.code) == 79
    assert code.at(0x1d94ce).bytes.hex() == 'ff5060'
    captures = ([json.loads(line) for line in args.capture.read_text().splitlines() if line]
                if args.capture.suffix == '.jsonl' else json.loads(args.capture.read_text()))
    assert captures, 'No live point impulse was captured'
    if args.require_object is not None:
        assert any(row['object'] == args.require_object for row in captures), \
            'The required actual object received no observed point impulse'
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    motion, impulse, point, stack, returned = (
        0x1000000, 0x1000200, 0x1000220, 0x100f000, 0x1100000)
    machine.mem_map(motion, 0x10000)
    trace = []
    machine.hook_add(UC_HOOK_CODE, lambda _machine, address, _size, _data: trace.append(address))
    results = []
    for ordinal, row in enumerate(captures):
        before, after = (bytes.fromhex(row[key]) for key in ('motion_before', 'motion_after'))
        assert len(before) == len(after) == 0x100
        impulse_bytes, point_bytes = (bytes.fromhex(row[key]) for key in ('impulse', 'point'))
        assert len(impulse_bytes) == len(point_bytes) == 16
        machine.mem_write(motion, before)
        machine.mem_write(impulse, impulse_bytes)
        machine.mem_write(point, point_bytes)
        machine.mem_write(stack, struct.pack('<III', returned, impulse, point))
        machine.reg_write(UC_X86_REG_ECX, motion)
        machine.reg_write(UC_X86_REG_ESP, stack)
        machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
        machine.reg_write(UC_X86_REG_FPCW, 0x27f)
        machine.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        trace.clear()
        machine.emu_start(0x324f80, returned, count=1000)
        assert machine.reg_read(UC_X86_REG_EIP) == returned
        assert machine.reg_read(UC_X86_REG_ESP) == stack + 12
        assert trace == sorted(body.code), 'Original point-impulse path differs'
        actual = bytes(machine.mem_read(motion, 0x100))
        assert actual == after, f'Original motion output differs for live impulse {ordinal}'
        results.append(dict(ordinal=ordinal, object=row['object'], tick=row['tick'],
                            body_index=row['body_index'],
                            linear_before=struct.unpack_from('<4f', before, 0x40),
                            linear_after=struct.unpack_from('<4f', after, 0x40),
                            angular_before=struct.unpack_from('<4f', before, 0x50),
                            angular_after=struct.unpack_from('<4f', after, 0x50)))
    report = dict(reference='English retail TU1.5 only', xbe_sha256=NEW_PIN,
                  capture_sha256=hashlib.sha256(args.capture.read_bytes()).hexdigest(),
                  passed=True, cases=len(results), original_instructions=len(results)*79,
                  compared_motion_bytes=len(results)*256, results=results, limit=__doc__)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS actual live TU1.5 point impulses: {len(results)} cases, {len(results)*256} exact motion bytes')


if __name__ == '__main__':
    main()
