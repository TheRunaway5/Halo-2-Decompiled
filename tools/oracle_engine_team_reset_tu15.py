#!/usr/bin/env python3
"""Execute complete TU1.5 pre-spawn team reset with declared options/banks."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EFLAGS, UC_X86_REG_EIP
from oracle_engine_team_admission_tu15 import (
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
    UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI,
)
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    regs = (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
            UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)
    rng = random.Random(0x15a470)
    cases = []
    for limit in range(1, 9):
        for occupied in (0, 2, 8, 16):
            u = Uc(UC_ARCH_X86, UC_MODE_32)
            end = max(s.va+s.vsize for s in image.sections)
            u.mem_map(image.base, (end-image.base+4095)&~4095)
            for section in image.sections:
                if section.rsize:
                    u.mem_write(section.va, image.data[section.raw:section.raw+section.rsize])
            u.mem_map(0x60000000, 0x60000);u.mem_map(0x70000000, 4096)
            game, engine, pool = 0x60030000, 0x60040000, 0x60050000
            banks = {game:bytearray(rng.randbytes(0x1200)), engine:bytearray(rng.randbytes(0xc20)),
                     pool:bytearray(0x60), 0x60020000:bytearray(rng.randbytes(1024)),
                     0x4f3218:bytearray(struct.pack('<I',game)),
                     0x4f63b8:bytearray(struct.pack('<I',engine)),
                     0x4f54f4:bytearray(struct.pack('<I',pool)),
                     0x56dfa8:bytearray(struct.pack('<I',0x48c498)),
                     0x48c498:bytearray(struct.pack('<I',0x468be8)),
                     0x468c50:bytearray(image.read(0x468c50,4))}
            banks[game][0xc] = 2;banks[game][0x184] = 1
            struct.pack_into('<I',banks[game],0x1f0,limit)
            for slot in range(16):
                banks[game][0x2dc+slot*0xe4] = int(slot < occupied)
                struct.pack_into('<b',banks[game],0x374+slot*0xe4,rng.choice((-1,0,1,2,3,4,5,6,7)))
            struct.pack_into('<HHHHHH',banks[engine],4,0xff,0,0,0,0,0)
            struct.pack_into('<I',banks[engine],0x24,0xffffffff)
            struct.pack_into('<I',banks[engine],0xc14,2)
            struct.pack_into('<I',banks[pool],0x38,0) # Actual pre-spawn pool is empty.
            gp = [rng.getrandbits(32) for _ in regs];gp[4] = 0x60020300
            struct.pack_into('<I',banks[0x60020000],0x300,0x70000000)
            for address, data in banks.items():u.mem_write(address,bytes(data))
            for reg, value in zip(regs,gp):u.reg_write(reg,value)
            flags = 0x202 | (rng.getrandbits(12)&0xcd5);u.reg_write(UC_X86_REG_EFLAGS,flags)
            trace=[];u.hook_add(UC_HOOK_CODE,lambda uc,address,size,data:trace.append(address))
            u.emu_start(0x15a470,0x70000000,count=10000)
            assert u.reg_read(UC_X86_REG_EIP)==0x70000000
            assert {0x15a2c0,0x15a1a0,0x154ef0,0x1cb030} <= set(trace)
            cases.append(dict(limit=limit,occupied=occupied,entry_gp=gp,entry_flags=flags,
                exit_gp=[u.reg_read(reg) for reg in regs],exit_flags=u.reg_read(UC_X86_REG_EFLAGS),instructions=trace,
                initial_banks=[dict(address=a,hex=bytes(b).hex()) for a,b in banks.items()],
                final_banks=[dict(address=a,hex=bytes(u.mem_read(a,len(b))).hex()) for a,b in banks.items()]))
    result=dict(schema=1,entry=0x15a470,case_count=len(cases),original_sha256=NEW_PIN,
        instruction_patches=[],supplied_dependency_returns=[],
        scope="Complete original team reset before player allocation. Declared 16-seat game options and empty source player pool; no full-world or live profile-update parity claim.",cases=cases)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(f"PASS {len(cases)} complete unchanged pre-spawn team resets")


if __name__ == "__main__":
    main()
