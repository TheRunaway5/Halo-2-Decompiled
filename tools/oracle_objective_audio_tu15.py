#!/usr/bin/env python3
"""Pinned Xbox recipient, sound-selection and recent/queue reference inputs.

The first two cohorts stop at the actual next consumer, without supplying its
result. The queue cohort executes complete248270/248430/248470 over declared
input storage. Executable bytes stay in owned offline-emulator memory only.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as x

BASE, STACK, STOP = 0x4000000, 0x400f000, 0x10000
REGS = [x.UC_X86_REG_EAX, x.UC_X86_REG_ECX, x.UC_X86_REG_EDX,
        x.UC_X86_REG_EBX, x.UC_X86_REG_ESP, x.UC_X86_REG_EBP,
        x.UC_X86_REG_ESI, x.UC_X86_REG_EDI]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x1000000)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    cpu.mem_map(BASE, 0x10000)
    def write(at, value, size=4):
        cpu.mem_write(at, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))
    def word(at, size=4):
        return int.from_bytes(cpu.mem_read(at, size), 'little')
    boundary = [0]
    cpu.hook_add(UC_HOOK_CODE, lambda uc, at, size, user:
                 uc.emu_stop() if boundary[0] and at == boundary[0] else None)
    def run(entry, registers, fp=(0x27f, 0x1f80), stop=0):
        write(STACK, STOP)
        for register in REGS: cpu.reg_write(register, 0)
        cpu.reg_write(x.UC_X86_REG_ESP, STACK)
        for register, value in registers.items(): cpu.reg_write(register, value)
        cpu.reg_write(x.UC_X86_REG_EFLAGS, 2)
        cpu.reg_write(x.UC_X86_REG_FPCW, fp[0])
        cpu.reg_write(x.UC_X86_REG_FPSW, 0)
        cpu.reg_write(x.UC_X86_REG_FPTAG, 0xffff)
        cpu.reg_write(x.UC_X86_REG_MXCSR, fp[1])
        boundary[0] = stop
        cpu.emu_start(entry, STOP, count=50000)
        assert cpu.reg_read(x.UC_X86_REG_EIP) in (STOP, stop)
        return cpu.reg_read(x.UC_X86_REG_EIP)
    def response(subtype, audience, required=0, excluded=0):
        raw = bytearray(0xa8)
        for at, value in ((4, subtype), (6, audience), (0x10, required), (0x12, excluded)):
            struct.pack_into('<h', raw, at, value)
        return raw
    recipients = []
    for kind, audience, required, excluded in itertools.product((0, 3, 10), range(5), range(5), range(5)):
        event = [kind, 5, 0xffffffff, 0x1010000, 0, 0x1010000, 0, 0, 0xffff]
        # Vary the real referenced roles, independently of audience order.
        if required: event[required + 2] = 0xffffffff
        membership = 1 if (required + excluded) & 1 else 0
        rows = [response(6, audience), response(5, audience, required, excluded), response(5, 4)]
        write(0x4ecc1c, BASE)
        write(BASE + 0x16c, 0x10000)
        write(0x4f0414, BASE + 0x1000)
        write(BASE + 0x1008, BASE + 0x2000)
        write(BASE + 0x200c, BASE + 0x3000)
        offset = {0: 0x98, 3: 0xb0, 10: 0xe8}[kind]
        write(BASE + 0x3000 + offset, len(rows))
        write(BASE + 0x3004 + offset, BASE + 0x4000)
        for n, raw in enumerate(rows): cpu.mem_write(BASE + 0x4000 + n * 0xa8, bytes(raw))
        write(0x4f54f4, BASE + 0x6000)
        write(BASE + 0x6044, BASE + 0x7000)
        write(BASE + 0x70c0, membership, 1)
        cpu.mem_write(BASE + 0x8000, struct.pack('<9I', *event))
        stopped = run(0x1a79a0, {x.UC_X86_REG_EAX: BASE + 0x8000,
                                x.UC_X86_REG_ESI: 0x1010000}, stop=0x1a83b0)
        selected = None if stopped == STOP else (cpu.reg_read(x.UC_X86_REG_ECX) - BASE - 0x4000) // 0xa8
        recipients.append(dict(event=event, player=0x1010000, membership=membership,
                               rows=[bytes(raw).hex() for raw in rows], expected=selected))
    selections = []
    for language, flags, delay, seed, alternatives in itertools.product(
            (0, 4, 8), (0, 1), (0., .05, 1.25), (0, 0x12345678), (0, 2)):
        raw = response(5, 4)
        raw[0x40] = flags
        struct.pack_into('<f', raw, 0x3c, delay)
        for n in range(9): struct.pack_into('<I', raw, 0x48 + n * 8, 0x10000 + n)
        struct.pack_into('<II', raw, 0xa0, alternatives, BASE + 0x5000)
        alts = []
        for n, weight in enumerate((.25, .5)[:alternatives]):
            row = bytearray(0x50); row[0] = 1 - flags
            sounds = [0x20000 + n * 0x100 + i for i in range(9)]
            for i, sound in enumerate(sounds): struct.pack_into('<I', row, 8 + i * 8, sound)
            struct.pack_into('<f', row, 0x4c, weight)
            cpu.mem_write(BASE + 0x5000 + n * 0x50, bytes(row))
            alts.append(dict(flags=row[0], sounds=sounds, weight=weight))
        cpu.mem_write(BASE + 0x4000, bytes(raw))
        write(0x48c750, language)
        write(0x4f3cd8, BASE + 0x6000)
        write(BASE + 0x6004, seed)
        write(0x51df54, BASE + 0x7000)
        write(BASE + 0x7002, 30, 2)
        write(STACK + 4, BASE + 0x8000)
        run(0x1a8290, {x.UC_X86_REG_ESI: BASE + 0x4000}, stop=0x1a8387)
        sp = cpu.reg_read(x.UC_X86_REG_ESP)
        selections.append(dict(response=bytes(raw).hex(), alternatives=alts, language=language,
                               seed=seed, rate=30, fp=[0x27f, 0x1f80], expected={
                                   'sound': cpu.reg_read(x.UC_X86_REG_EDI),
                                   'delay': struct.unpack('<i', cpu.reg_read(x.UC_X86_REG_EBP).to_bytes(4, 'little'))[0],
                                   'announcer': bool(word(sp + 0xc, 1)),
                                   'seed': word(BASE + 0x6004)}))
    queues = []
    write(0x51df54, BASE + 0x7000); write(BASE + 0x7002, 30, 2)
    run(0x248390, {})
    for ordinal in range(36):
        subtype = 20 if ordinal in (12, 13) else 5
        kind = 0 if subtype == 20 else 3
        event = [kind, subtype, 0xffffffff, 0x1010000, 0, 0xffffffff, 0xffffffff, 0, 0xffff]
        cue = 0x30000 + (ordinal % 6 if ordinal < 12 else ordinal % 2)
        tick = 100 + ordinal if ordinal < 24 else 0xfffffff0 + ordinal - 24
        tick &= 0xffffffff
        write(BASE + 0x7008, tick)
        cpu.mem_write(BASE + 0x8000, struct.pack('<9I', *event))
        initial = bytes(cpu.mem_read(0x50ee28, 0xa0))
        write(STACK + 4, ordinal - 8); write(STACK + 8, BASE + 0x8000); write(STACK + 12, 1)
        run(0x248270, {x.UC_X86_REG_EDI: cue})
        queues.append(dict(event=event, sound=cue, tick=tick, rate=30, delay=ordinal - 8,
                           initial=initial.hex(), expected=bytes(cpu.mem_read(0x50ee28, 0xa0)).hex()))
    result = dict(schema='H2_OBJECTIVE_AUDIO_TU15_V1', original_sha256=NEW_PIN,
                  supplied_service_results=[], recipients=recipients, selections=selections, queues=queues)
    encoded = json.dumps(result, separators=(',', ':')) + '\n'
    assert len(encoded.encode()) <= 1024 * 1024
    args.output.write_text(encoded)
    print(json.dumps(dict(recipients=len(recipients), selections=len(selections), queues=len(queues),
                          bytes=len(encoded.encode()), sha256=hashlib.sha256(encoded.encode()).hexdigest())))


if __name__ == '__main__': main()
