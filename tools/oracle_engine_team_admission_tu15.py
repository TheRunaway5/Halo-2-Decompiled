#!/usr/bin/env python3
"""Execute the unchanged TU1.5 base engine team-admission callback."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
    UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_EFLAGS,
)
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    body = Xbe(NEW_DEFAULT).read(0x25bb50, 5)
    assert body == bytes.fromhex("b001c20400")
    regs = (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
            UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)
    rng = random.Random(0x25bb50)
    cases = []
    for team in range(8):
        for sample in range(32):
            u = Uc(UC_ARCH_X86, UC_MODE_32)
            u.mem_map(0x25b000, 4096); u.mem_write(0x25bb50, body)
            u.mem_map(0x60020000, 4096); u.mem_map(0x70000000, 4096)
            gp = [rng.getrandbits(32) for _ in regs]; gp[4] = 0x60020080
            initial = bytearray(rng.randbytes(64))
            struct.pack_into("<II", initial, 32, 0x70000000, team)
            u.mem_write(gp[4]-32, bytes(initial))
            flags = 0x202 | (rng.getrandbits(12) & 0xcd5)
            for reg, value in zip(regs, gp): u.reg_write(reg, value)
            u.reg_write(UC_X86_REG_EFLAGS, flags)
            trace = []
            u.hook_add(UC_HOOK_CODE, lambda uc, address, size, data: trace.append(address))
            u.emu_start(0x25bb50, 0x70000000, count=8)
            assert trace == [0x25bb50, 0x25bb52]
            cases.append(dict(team=team, sample=sample, entry_gp=gp, entry_flags=flags,
                exit_gp=[u.reg_read(reg) for reg in regs], exit_flags=u.reg_read(UC_X86_REG_EFLAGS),
                instructions=trace, bank_address=gp[4]-32, initial_hex=initial.hex(),
                final_hex=bytes(u.mem_read(gp[4]-32, 64)).hex()))
    result = dict(schema=1, entry=0x25bb50, original_sha256=NEW_PIN,
        instruction_patches=[], supplied_dependency_returns=[],
        scope="Complete two-instruction callback only; arbitrary GP/flags and declared stack inputs. No full-world parity claim.",
        cases=cases)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+"\n")
    print(f"PASS {len(cases)} unchanged original callback executions")


if __name__ == "__main__":
    main()
