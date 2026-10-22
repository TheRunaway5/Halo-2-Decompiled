#!/usr/bin/env python3
"""Execute unchanged disc/TU1.5 pause constructors and window-clear resume policy.

UI constructors/window occupancy and sound hardware are explicit caller controls.
The real game-clock predicate, pause stores and sound transition body execute.
This is CPU policy evidence, not a running Xbox UI or audible device comparison.
Run under tools/run_test.py; only the requested small JSON report is retained.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
DISC_PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
CLOCK, SOUND, OBJECT, STACK, STOP = 0x2100000, 0x2100100, 0x2101000, 0x211f000, 0x3000000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    image = Xbe(args.xbe)
    digest = hashlib.sha256(image.data).hexdigest()
    if digest == PIN:
        version = 'TU1.5'
        entries = [('solo', 0x238ac2), ('multiplayer', 0x14a0d9), ('resume', 0x14ae01)]
        clock_slot, sound_slot = 0x51df54, 0x4f2c50
        pause_store, resume_store = 0x238b00, 0x14ae13
        service_addresses = [0x236eee, 0x238942, 0x239104, 0x14bd64,
                             0x225f80, 0x225fb0, 0x2207a0, 0x33b590]
    elif digest == DISC_PIN:
        version = 'disc'
        entries = [('solo', 0x23203e), ('multiplayer', 0x147605), ('resume', 0x1482c4)]
        clock_slot, sound_slot = 0x510c54, 0x4e6380
        pause_store, resume_store = 0x23207c, 0x1482d6
        service_addresses = [0x230451, 0x231e88, 0x232708, 0x149227,
                             0x21f570, 0x21f5a0, 0x219d90, 0x3314b0]
    else:
        raise ValueError('This oracle requires the owned English disc or TU1.5 executable')
    expected = {pause_store: b'\xa1' + struct.pack('<I', clock_slot) + bytes.fromhex('c6400101'),
                resume_store: b'\xa1' + struct.pack('<I', clock_slot) + bytes.fromhex('c6400100')}
    for at, data in expected.items():
        if image.read(at, len(data)) != data:
            raise ValueError(f'Unsupported executable at {at:08x}')
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    guest.mem_map(CLOCK, 0x20000)
    guest.mem_map(STOP, 0x1000)

    def put(at, value):
        guest.mem_write(at, struct.pack('<I', value))

    def word(at):
        return struct.unpack('<I', guest.mem_read(at, 4))[0]

    state = {}
    # Parameter byte counts reflect the original callee-cleanup ABI.
    services = dict(zip(service_addresses, [('screen_base', 20), ('solo_list', 4),
                    ('multiplayer_list', 4), ('windows_in_use', 0),
                    ('pause_impulse_buffers', 0), ('resume_impulse_buffers', 0),
                    ('resume_looping_sounds', 4), ('GetTickCount', 0)]))

    def hook(uc, at, size, data):
        state['pcs'].add(at)
        if at not in services:
            return
        name, popped = services[at]
        state['services'].append(name)
        guest.reg_write(UC_X86_REG_EAX, state['windows'] if name == 'windows_in_use' else 1234)
        stack = guest.reg_read(UC_X86_REG_ESP)
        guest.reg_write(UC_X86_REG_EIP, word(stack))
        guest.reg_write(UC_X86_REG_ESP, stack + 4 + popped)

    guest.hook_add(UC_HOOK_CODE, hook)
    cases = []
    for kind, entry in entries:
        for paused in [0, 1]:
            for windows in ([0, 1] if kind == 'resume' else [0]):
                state.update(pcs=set(), services=[], windows=windows)
                guest.mem_write(CLOCK, bytes(0x20000))
                put(clock_slot, CLOCK)
                put(sound_slot, SOUND)
                guest.mem_write(CLOCK, bytes([1, paused, 30, 0]))
                guest.mem_write(SOUND + 0x7b, bytes([paused]))
                put(STACK, STOP)
                for n, value in enumerate([1, 4, 1]):
                    put(STACK + 4 + 4*n, value)
                guest.reg_write(UC_X86_REG_ESP, STACK)
                guest.reg_write(UC_X86_REG_EDI, OBJECT)
                guest.emu_start(entry, STOP, count=2000)
                if guest.reg_read(UC_X86_REG_EIP) != STOP:
                    raise RuntimeError(f'Original {entry:08x} did not return')
                after = guest.mem_read(CLOCK + 1, 1)[0]
                expected_pause = 1 if kind == 'solo' else paused if kind == 'multiplayer' or windows else 0
                if after != expected_pause or guest.mem_read(SOUND + 0x7b, 1)[0] != expected_pause:
                    raise AssertionError('Original pause clock/audio policy differs')
                cases.append(dict(kind=kind, paused_before=paused, windows_in_use=windows,
                                  paused_after=after, services=state['services'],
                                  executed_pcs=[f'{pc:08x}' for pc in sorted(state['pcs'])]))
    report = dict(passed=True, version=version, xbe_sha256=digest, instruction_patches=[], cases=cases,
                  scope=__doc__)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(cases)} unchanged {version} pause/resume clock and audio policy cases')


if __name__ == '__main__':
    main()
