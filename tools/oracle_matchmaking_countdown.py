#!/usr/bin/env python3
"""Replay complete unchanged retail/TU1.5 countdown parameter writers.

Synthetic session fields and caller operands only. No original Live, network,
controller, rendered frame or full original caller provenance is claimed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

OLD_PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE, REQUESTER, STACK, STOP = 0x1000000, 0x1009000, 0x100e000, 0x1010000


def signed(value):
    value &= 0xffffffff
    return value - 0x100000000 if value & 0x80000000 else value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    edges = [-0x80000000, -1, 0, 1, 3, 9, 10, 11, 0x7fffffff]
    state = 0x1515
    def draw():
        nonlocal state
        state = (1664525 * state + 1013904223) & 0xffffffff
        return state
    controls = []
    # Include the exact unchanged tuple with a different requester explicitly.
    controls.append(dict(active=1, seconds=10, mode=1, watermark=10, updates=7,
        requester='6162636465666768696a6b6c', leader=False, start=1,
        next_seconds=10, next_mode=1, next_requester='313233343536373839303132'))
    for index in range(767):
        controls.append(dict(active=draw() >> 31, seconds=edges[draw() % len(edges)],
            mode=[0, 1, 2, -1][draw() % 4], watermark=edges[draw() % len(edges)],
            updates=draw() if index % 3 else 0xffffffff, requester='6162636465666768696a6b6c',
            leader=bool(draw() >> 31), start=draw() >> 31, next_seconds=edges[draw() % len(edges)],
            next_mode=[0, 1, 2, -1][(draw() >> 16) % 4], next_requester='313233343536373839303132'))
    results, epochs = [], {}
    for epoch, path, pin, entry in (('retail', Path('assets/disc/default.xbe'), OLD_PIN, 0x5df30),
                                    ('tu15', NEW_DEFAULT, NEW_PIN, 0x5ec20)):
        image = Xbe(path)
        assert hashlib.sha256(image.data).hexdigest() == pin
        code = image.read(entry, 0xf3)
        md = Cs(CS_ARCH_X86, CS_MODE_32)
        ins = list(md.disasm(code, entry))
        assert ins[-1].mnemonic == 'ret' and ins[-1].op_str == '8'
        machine = Uc(UC_ARCH_X86, UC_MODE_32)
        machine.mem_map(0x10000, 0x800000)
        for section in image.sections:
            machine.mem_write(section.va, image.section_bytes(section))
        machine.mem_map(BASE, 0x20000)
        visits, covered = [0], set()
        def observe(uc, pc, length, _):
            if pc == STOP:
                uc.emu_stop()
                return
            assert entry <= pc < entry + len(code)
            assert bytes(uc.mem_read(pc, length)) == image.read(pc, length)
            visits[0] += 1
            covered.add(pc)
        machine.hook_add(UC_HOOK_CODE, observe)
        def put(at, value, size=4):
            machine.mem_write(at, int(value & ((1 << (8*size))-1)).to_bytes(size, 'little'))
        def word(at):
            return int.from_bytes(machine.mem_read(at, 4), 'little')
        for index, row in enumerate(controls):
            machine.mem_write(BASE, bytes(0x8000))
            put(BASE+0x50, 1)
            put(BASE+0x4978, row['updates'])
            put(BASE+0x49a8, row['active'], 1)
            for offset, name in ((0x49ac, 'seconds'), (0x49b0, 'mode'), (0x49b4, 'watermark')):
                put(BASE+offset, row[name])
            machine.mem_write(BASE+0x49b8, bytes.fromhex(row['requester']))
            machine.mem_write(REQUESTER, bytes.fromhex(row['next_requester']))
            machine.mem_write(STACK, struct.pack('<III', STOP, 1 if row['leader'] else 2, REQUESTER))
            for register, value in ((UC_X86_REG_ESP, STACK), (UC_X86_REG_ECX, BASE),
                    (UC_X86_REG_EAX, row['next_seconds'] & 0xffffffff), (UC_X86_REG_EDX, row['start']),
                    (UC_X86_REG_EDI, row['next_mode'] & 0xffffffff)):
                machine.reg_write(register, value)
            machine.emu_start(entry, STOP, count=1000)
            assert machine.reg_read(UC_X86_REG_EIP) == STOP
            expected = dict(active=int.from_bytes(machine.mem_read(BASE+0x49a8,1),'little'),
                seconds=signed(word(BASE+0x49ac)), mode=signed(word(BASE+0x49b0)),
                watermark=signed(word(BASE+0x49b4)), updates=word(BASE+0x4978),
                requester=bytes(machine.mem_read(BASE+0x49b8,12)).hex(),
                changed=bool(machine.reg_read(UC_X86_REG_EAX) & 0xff))
            if epoch == 'retail':
                results.append(dict(input=row, expected=expected))
            else:
                assert results[index]['expected'] == expected
        assert bytes(machine.mem_read(entry, len(code))) == code
        epochs[epoch] = dict(xbe_sha256=pin, entry=entry, bytes=len(code), code_sha256=hashlib.sha256(code).hexdigest(),
            calls=len(controls), instruction_visits=visits[0], covered_instructions=len(covered), instructions=len(ins),
            all_executed_instruction_bytes_verified=True)
    fixture = dict(schema='H2_COUNTDOWN_PARAMETER_ORIGINAL_V1', scope=__doc__, epochs=epochs, cases=results)
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(fixture, separators=(',', ':'))+'\n')
    report = dict(schema='H2_COUNTDOWN_WRITER_ORIGINAL_REPORT_V1', scope=__doc__, epochs=epochs,
        original_calls=sum(v['calls'] for v in epochs.values()), paired_controls=len(results),
        fixture=dict(path=str(args.fixture), bytes=args.fixture.stat().st_size,
            sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest()), scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print('PASS unchanged original countdown writers:', report['original_calls'], 'calls;', len(results), 'paired controls')


if __name__ == '__main__':
    main()
