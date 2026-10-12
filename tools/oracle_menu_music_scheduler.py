#!/usr/bin/env python3
"""Execute original 235B9C transition scheduling with declared backend hooks."""
import hashlib
import json
import random
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "analysis/frontend/profile-regression"
EXPECTED = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
data = (ROOT / "assets/disc/default.xbe").read_bytes()
assert hashlib.sha256(data).hexdigest() == EXPECTED
guest = Uc(UC_ARCH_X86, UC_MODE_32)
guest.mem_map(0x10000, 0x600000)
base = struct.unpack_from("<I", data, 0x104)[0]
count, table = struct.unpack_from("<II", data, 0x11C)
for index in range(count):
    _, address, _, offset, size = struct.unpack_from(
        "<5I", data, table - base + index * 56)
    guest.mem_write(address, data[offset:offset + size])
STATE, STACK, STOP = 0x2000000, 0x200F000, 0x3000000
guest.mem_map(STATE, 0x10000)
guest.mem_map(STOP, 0x1000)
actions = []


def backend_hook(machine, address, size, context):
    if address not in (0x235B5D, 0x235B82):
        return
    actions.append(1 if address == 0x235B5D else 0)
    stack = machine.reg_read(UC_X86_REG_ESP)
    return_address = struct.unpack("<I", machine.mem_read(stack, 4))[0]
    # Start is stdcall ret 4; stop has its music pointer in ESI and ret 0.
    machine.reg_write(UC_X86_REG_ESP, stack + (8 if address == 0x235B5D else 4))
    machine.reg_write(UC_X86_REG_EIP, return_address)


guest.hook_add(UC_HOOK_CODE, backend_hook)
rows = []


def probe(name, playing, previous, wanted, now):
    guest.mem_write(STATE, struct.pack("<6I", playing, wanted, 1, previous, 0, 0))
    guest.mem_write(0x54D5B8, struct.pack("<I", now))
    guest.mem_write(STACK, struct.pack("<I", STOP))
    guest.reg_write(UC_X86_REG_EAX, STATE)
    guest.reg_write(UC_X86_REG_ESP, STACK)
    actions.clear()
    guest.emu_start(0x235B9C, STOP, count=1000)
    assert guest.reg_read(UC_X86_REG_EIP) == STOP
    after = struct.unpack("<4I", guest.mem_read(STATE, 16))
    assert len(actions) <= 1
    rows.append([name, playing, f"{previous:08x}", wanted, f"{now:08x}",
                 actions[0] if actions else -1, after[0], f"{after[3]:08x}"])
    return after[0], after[3]


playing, previous = 0, 0
for name, wanted, now in (
    ("initial-start", 1, 1000),
    ("initial-before-six", 0, 6999),
    ("initial-exact-six", 0, 7000),
    ("retained-profile", 1, 8000),
    ("fade-start", 0, 70500),
    ("early-movie-skip", 1, 76001),
    ("restart-exact-six", 1, 76500),
    ("restart-after-six", 1, 76501),
    ("retained-keyboard", 1, 80000),
    ("disabled-before-six", 0, 82500),
    ("disabled-exact-six", 0, 82501),
    ("disabled-after-six", 0, 82502),
):
    playing, previous = probe(name, playing, previous, wanted, now)
rng = random.Random(0x235B9C)
for index in range(1024):
    previous = 0 if index % 11 == 0 else rng.getrandbits(32)
    elapsed = (-1, 0, 5999, 6000, 6001, 0x7FFFFFFF, 0x80000000)[index % 7]
    now = (previous + elapsed) & 0xFFFFFFFF
    probe(f"probe-{index}", index & 1, previous, (index >> 1) & 1, now)
fixture = OUT / "menu-music-scheduler-original.tsv"
fixture.write_text("\n".join(" ".join(map(str, row)) for row in rows) + "\n")
report = {
    "xbe_sha256": EXPECTED,
    "entry": "00235b9c",
    "cases": len(rows),
    "fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
    "scope": "Unchanged original scheduler, logical playing state, timestamp and exact backend call choice. Only start/stop backend functions are intercepted; decoding, hardware audio and UI-clock mapping are outside this proof.",
}
(OUT / "menu-music-scheduler-original.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, indent=2))
