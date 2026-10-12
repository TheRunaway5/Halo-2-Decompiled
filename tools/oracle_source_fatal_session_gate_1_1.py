#!/usr/bin/env python3
"""Execute the complete original A8620 session-byte gate in both owned images.

Only declared session/stack/register inputs are supplied. Value5 stops before
the first lower instruction; no object, inventory, entity or readiness state
is installed, and no lower result is returned.
"""
import argparse
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_mp_title_update_display import OLD, NEW, PINS, Xbe, sha

GAME, STACK, STOP = 0x1001000, 0x200d000, 0x3000000
REGISTERS = tuple(getattr(R, 'UC_X86_REG_' + n) for n in
                  ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI'))
INITIAL = (0x11223344, 0x22334455, 0x33445566, 0x44556677,
           STACK, 0x55667788, 0x66778899, 0x778899aa)


def capture(path, entry, lower, slot):
    assert sha(path) == PINS[str(path)]
    image = Xbe(path)
    # The original owner's first load names this literal pointer slot.
    assert image.read(entry, 5) == b'\xa1' + struct.pack('<I', slot)
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    for address in (GAME & ~0xffff, STACK & ~0xffff, STOP):
        machine.mem_map(address, 0x10000)
    machine.mem_write(slot, struct.pack('<I', GAME))
    trace, stopped, cases = [], [], []

    def instruction(g, pc, size, user):
        if pc in (STOP, lower):
            stopped.append(pc)
            g.emu_stop()
        else:
            trace.append(pc)

    machine.hook_add(UC_HOOK_CODE, instruction)
    for value in range(256):
        trace.clear()
        stopped.clear()
        machine.mem_write(GAME, bytes(0x200))
        machine.mem_write(GAME + 0xc, bytes([value]))
        machine.mem_write(STACK - 0x100, bytes([0xcd]) * 0x120)
        machine.mem_write(STACK, struct.pack('<II', STOP, 0xe2820013))
        for register, initial in zip(REGISTERS, INITIAL):
            machine.reg_write(register, initial)
        machine.reg_write(R.UC_X86_REG_EFLAGS, 2)
        machine.emu_start(entry, STOP + 1, count=1000)
        assert len(stopped) == 1
        pending = stopped[0] == lower
        assert pending == (value == 5)
        case = dict(input_byte=value, signed_input=value if value < 128 else value - 256,
                    stopped_at='first_lower_A8720' if pending else 'original_RET4',
                    instruction_count=len(trace),
                    exit_gp=[machine.reg_read(r) for r in REGISTERS],
                    exit_eflags=machine.reg_read(R.UC_X86_REG_EFLAGS))
        sp = machine.reg_read(R.UC_X86_REG_ESP)
        if pending:
            arguments = list(struct.unpack('<6I', machine.mem_read(sp + 4, 24)))
            count = struct.unpack('<I', machine.mem_read(arguments[3], 4))[0]
            assert arguments[:3] == [0xe2820013, 1, 4] and count == 0
            case.update(pending_arguments=arguments, actual_count_store=count)
        else:
            assert sp == STACK + 8
        cases.append(case)
    assert sha(path) == PINS[str(path)]
    return dict(image_sha256=sha(path), entry=f'{entry:08x}',
                session_slot=f'{slot:08x}', cases=cases)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'Preserve existing original evidence'
    images = [capture(OLD, 0xa8620, 0xa8720, 0x4edf18),
              capture(NEW, 0xb1a70, 0xb1b70, 0x4f3218)]
    assert images[0]['cases'] == images[1]['cases']
    result = dict(schema=1, source_pins=PINS,
                  declared_input=dict(game=hex(GAME), stack=hex(STACK), stop=hex(STOP),
                                      object='e2820013', initial_gp=list(INITIAL),
                                      session_bytes=list(range(256)),
                                      scope='Supplied session-byte sweep only. Active value5 stops before first lower. No native live-state capture or active entity service completion claimed.'),
                  images=images, instruction_patches=[], supplied_lower_results=[])
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(json.dumps(dict(output=str(args.output), sha256=sha(args.output),
                          cases_per_image=256, images_equal=True)))


if __name__ == '__main__':
    main()
