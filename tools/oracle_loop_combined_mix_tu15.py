#!/usr/bin/env python3
"""Measure the original combined-loop numeric contribution, without audio ports.

The input point is already in the selected listener's local coordinates.
Listener selection, world transforms, voice allocation and speaker routing
are deliberately outside this component. No executed instruction is patched.
Run through tools/run_test.py and retain only the bounded JSON reference.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.output.exists():
        parser.error('Use a new report path; retained references are never overwritten')
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x800000)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    bank = 0x1000000
    cpu.mem_map(bank, 0x10000)
    parent, sound, stack = bank+0x1000, bank+0x2000, bank+0x8000

    def put(address, value):
        cpu.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def word(address):
        return struct.unpack('<I', cpu.mem_read(address, 4))[0]

    def bits(value):
        return struct.unpack('<I', struct.pack('<f', value))[0]

    def words(address, count):
        return list(struct.unpack('<'+'I'*count, cpu.mem_read(address, 4*count)))

    instructions, context = {}, {}

    def observe(_, pc, size, __):
        if pc == context['stop']:
            context['complete'] = True
            cpu.emu_stop()
            return
        instruction = code.at(pc)
        assert instruction and bytes(cpu.mem_read(pc, size)) == image.read(pc, size)
        instructions[f'{pc:08x}'] = instruction.bytes.hex()

    cpu.hook_add(UC_HOOK_CODE, observe)

    def run(entry, stop, registers, rounding):
        context.clear()
        context['stop'] = stop
        cpu.reg_write(R.UC_X86_REG_ESP, stack)
        cpu.reg_write(R.UC_X86_REG_EBP, parent)
        cpu.reg_write(R.UC_X86_REG_EFLAGS, 2)
        cpu.reg_write(R.UC_X86_REG_FPCW, 0x027f | rounding << 10)
        cpu.reg_write(R.UC_X86_REG_MXCSR, 0x1f80 | rounding << 13)
        for register, value in registers.items():
            cpu.reg_write(register, value)
        cpu.emu_start(entry, 0, count=4000)
        assert context.get('complete'), (hex(entry), hex(cpu.reg_read(R.UC_X86_REG_EIP)))

    put(0x4f2c50, sound)
    rng = 0x1a943df7

    def draw():
        nonlocal rng
        rng = (rng*1664525+1013904223) & 0xffffffff
        return rng

    # Include cardinal, diagonal, coincident, subnormal and large points.
    # Random finite points exercise normalization rounding and all four bins.
    points = [(0, 0, 0), (1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0),
              (0, 0, 1), (0, 0, -1), (.70710677, .70710677, 0),
              (10000, -10000, 3), (1e-40, -1e-40, 0), (.001, .002, .003)]
    points += [tuple((int(draw() % 200001)-100000)/997. for _ in range(3)) for _ in range(117)]
    rows = []
    for rounding in range(4):
        prior = [0, 0, 0, 0]
        for index, point in enumerate(points):
            distance = [0, .0001, .001, .25, 1, 3, 8, 16, 64][index % 9]
            minimum = [0, -1, .0001, .001, .5, 3, 8][index % 7]
            maximum = [0, .001, 1, 8, 16, 64][index % 6]
            # Repeated same-frame contributions are interspersed with resets,
            # including the real byte-counter wrap from 255 back to zero.
            frame = [0, 1, 255, 0][index % 4]
            previous = frame if index % 3 else (frame-1) & 255
            if index % 11 == 0:
                prior = list(map(bits, (2, .5, 0, 1)))
            cpu.mem_write(parent+0xc4, struct.pack('<4I', *prior))
            cpu.mem_write(parent+0x53, bytes([previous]))
            cpu.mem_write(sound+0x7f, bytes([frame]))
            run(0x220da3, 0x220dcb, {}, rounding)
            after_reset = words(parent+0xc4, 4)
            cpu.mem_write(stack+0x30, struct.pack('<3f', *point))
            put(stack+0x54, bits(distance))
            # Execute the actual minimum floor and complete198D80->12DB00.
            run(0x220e20, 0x220e4f, {R.UC_X86_REG_XMM0: bits(minimum),
                R.UC_X86_REG_EDI: bits(maximum)}, rounding)
            clamped_minimum = word(stack+0x60)
            gain = cpu.reg_read(R.UC_X86_REG_XMM0) & 0xffffffff
            put(stack+0x50, gain)
            # Complete native normalization30BD0, four-speaker198CF0->198BA0
            # and both accumulator loops. No service/lookup result is supplied.
            run(0x220e83, 0x220f21, {}, rounding)
            result = words(parent+0xc4, 4)
            rows.append(dict(rounding=rounding, point=list(map(bits, point)),
                minimum=bits(minimum), maximum=bits(maximum), distance=bits(distance),
                previous_frame=previous, frame=frame, prior=prior,
                after_reset=after_reset, clamped_minimum=clamped_minimum, gain=gain,
                normalized=words(stack+0x30, 3), speakers=words(stack+0x3c, 4), result=result))
            prior = result

    result = dict(scope=__doc__, original_xbe_sha256=NEW_PIN,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), cases=rows,
        constants={f'{at:08x}': image.read(at, size).hex() for at, size in
            ((0x44eda0, 48), (0x46a0cc, 8), (0x46a0ec, 4), (0x46a180, 4))},
        executed_original_instructions=instructions, original_instruction_patches=[],
        substituted_calls=[], whole_loop_or_device_verified=False, retained_large_artifacts=[])
    raw = json.dumps(result, separators=(',', ':'))+'\n'
    assert len(raw.encode()) < 1024*1024
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(raw)
    print(json.dumps(dict(cases=len(rows), instructions=len(instructions),
        bytes=len(raw.encode()), sha256=hashlib.sha256(raw.encode()).hexdigest())), flush=True)


if __name__ == '__main__':
    main()
