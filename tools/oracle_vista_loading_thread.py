#!/usr/bin/env python3
"""Execute unchanged Vista66B808 with explicit scheduling-owner fixtures.

Sleep and66A512 are recorded services. The latter fixture mutates the actual
mode/fade bytes to exercise original read timing. This does not execute a real
OS thread, native renderer, map load, audio, or loading lifetime initializer.
"""
from pathlib import Path
import hashlib
import itertools
import json
import random
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/"analysis/frontend/vista-session-flow-audit/local-session-inputs-original"))
from audit import PE, EXE, EXE_SHA

STOP, API, STACK, SP = 0x1800000, 0x1801000, 0x1900000, 0x1908000


def run(pe, supplied):
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(pe.base, (len(pe.data)+4095)&~4095)
    u.mem_write(pe.base, pe.data)
    u.mem_map(STOP, 0x2000)
    u.mem_map(STACK, 0x10000)
    def word(address, value):
        u.mem_write(address, struct.pack("<I", value))
    def read(address):
        return struct.unpack("<I", u.mem_read(address, 4))[0]
    def ret(value, pop):
        stack = u.reg_read(UC_X86_REG_ESP)
        u.reg_write(UC_X86_REG_EIP, read(stack))
        u.reg_write(UC_X86_REG_ESP, stack+4+pop)
        u.reg_write(UC_X86_REG_EAX, value)
    word(0x79B260, API)
    word(SP, STOP)
    word(SP+4, 0)
    u.reg_write(UC_X86_REG_ESP, SP)
    u.mem_write(0xE4AF24, bytes([supplied["initial_mode"]]))
    u.mem_write(0xE4AFDC, bytes([supplied["initial_active"]]))
    events = []
    visits = draws = 0
    def hook(unused_u, pc, size, unused):
        nonlocal visits, draws
        visits += 1
        if pc == STOP:
            u.emu_stop()
        elif pc == API:
            duration = read(u.reg_read(UC_X86_REG_ESP)+4)
            assert duration == 33
            events.append(["sleep", duration])
            ret(0, 4)
        elif pc == 0x66A512:
            item = supplied["draws"][draws]
            mode = read(u.reg_read(UC_X86_REG_ESP)+4)
            events.append(["present", mode, item["result_word"]])
            u.mem_write(0xE4AF24, bytes([item["next_mode"]]))
            u.mem_write(0xE4AFDC, bytes([item["next_active"]]))
            draws += 1
            ret(item["result_word"], 0)
    u.hook_add(UC_HOOK_CODE, hook)
    u.emu_start(0x66B808, STOP+1, count=100000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP and draws == len(supplied["draws"])
    return dict(supplied=supplied, events=events, result=u.reg_read(UC_X86_REG_EAX),
                stack_after=u.reg_read(UC_X86_REG_ESP)-SP, instruction_visits=visits,
                final_mode=u.mem_read(0xE4AF24, 1)[0], final_active=u.mem_read(0xE4AFDC, 1)[0])


def main():
    pe = PE(EXE, EXE_SHA)
    rng = random.Random(0x66B808)
    cases = []
    # Any ordinary AL is ignored. In the fade phase only AL, rather than EAX,
    # controls completion; the active byte is deliberately cleared again.
    for ordinary, retries, mode, active in itertools.product((1,2,3,17), (0,1,2,5), (0,1,128,255), (0,1,128)):
        draws = []
        for index in range(ordinary):
            draws.append(dict(result_word=rng.choice((0,1,0x100,0xFFFFFFFF)),
                              next_mode=rng.randrange(256), next_active=255 if index == ordinary-1 else 0))
        for index in range(retries+1):
            draws.append(dict(result_word=rng.choice((1,128,255,0xFFFFFF01)) if index < retries else rng.choice((0,0x100,0xFFFFFF00)),
                              next_mode=rng.randrange(256), next_active=0))
        cases.append(run(pe, dict(initial_mode=mode, initial_active=active, draws=draws)))
    report = dict(schema="H2V_LOADING_THREAD_ORIGINAL_V1", source_sha256=EXE_SHA,
                  generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  source_span=pe.span(0x66B808, 0x66B85E), scope=__doc__, cases=cases)
    fixture = ROOT/"tests/fixtures/vista_loading_thread_original.json"
    fixture.write_text(json.dumps(report, separators=(",", ":"))+"\n")
    print(f"Original loading scheduler: {len(cases)} cases, {sum(case['instruction_visits'] for case in cases)} instructions, "
          f"fixture SHA256={hashlib.sha256(fixture.read_bytes()).hexdigest()}")


if __name__ == "__main__":
    main()
