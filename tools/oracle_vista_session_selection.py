#!/usr/bin/env python3
"""Execute the supplied Vista local map/variant setters unchanged.

The actual UTF-16 bounded copy, scalar CRT memset and security-cookie check
execute too. The native binding deliberately excludes remote transport states.
"""
from pathlib import Path
import hashlib
import itertools
import json
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from oracle_vista_pcr_screen import machine, STOP, STACK, OBJ, SHA, w32

ROOT = Path(__file__).resolve().parents[1]
N = 0x7b88
INPUT = OBJ + 0x9000
OUT = ROOT / "analysis/frontend/vista-session-flow-audit/selection-original"


def digest(value):
    return hashlib.sha256(value).hexdigest()


def initial(q):
    bank = bytearray((i * 37 + q["seed"] * 13 + (i >> 8) * 17) & 255 for i in range(N))
    for offset, value in ((0x73a0, q["state"]), (0x71c0, 7), (0x74, 7 if q["host"] else 8),
                          (0x4c60, q["revision"])):
        struct.pack_into("<I", bank, offset, value)
    return bytes(bank)


def path(q):
    units = [((i * 19 + q["seed"] * 13) % 0xfffe) + 1 for i in range(q["length"])]
    if q["nul"] >= 0:
        units[q["nul"]] = 0
    return b"".join(struct.pack("<H", value) for value in units)


def variant(q):
    value = bytearray((i * 23 + q["seed"] * 17) & 255 for i in range(0x130))
    struct.pack_into("<I", value, 0x44, q["variant_kind"])
    return bytes(value)


def execute(image, q):
    u = machine(image)
    before = initial(q)
    u.mem_write(OBJ, before)
    # Use the original CRT's scalar path; no CRT call is replaced by a hook.
    w32(u, 0xe9e9e4, 0)
    if q["op"] == "map":
        u.mem_write(INPUT, path(q) + b"\0\0")
        entry, args = 0x5c8d6f, [q["type"], q["map_id"], INPUT]
    else:
        u.mem_write(INPUT, variant(q))
        entry, args = 0x5c903b, [0 if q["null_variant"] else INPUT]
    visits = 0
    counts = {"copy": 0, "wide_crt": 0, "memset": 0, "cookie": 0}
    owners = {0x43804a: "copy", 0x687ab5: "wide_crt", 0x687ba9: "memset", 0x6875d3: "cookie"}

    def hook(_u, pc, _size, _):
        nonlocal visits
        visits += 1
        if pc == STOP:
            u.emu_stop()
        if pc in owners:
            counts[owners[pc]] += 1

    u.hook_add(UC_HOOK_CODE, hook)
    returns = []
    for _ in range(q["repeat"]):
        esp = STACK + 0x18000
        u.mem_write(esp, struct.pack("<" + "I" * (1 + len(args)), STOP, *args))
        u.reg_write(UC_X86_REG_ESP, esp)
        u.reg_write(UC_X86_REG_ECX, OBJ)
        u.emu_start(entry, STOP + 1, count=100000)
        assert u.reg_read(UC_X86_REG_EIP) == STOP
        returns.append(u.reg_read(UC_X86_REG_EAX) & 255)
    after = bytes(u.mem_read(OBJ, N))
    return {"input": q, "before": digest(before), "after": digest(after), "returned": returns,
            "selected_hex": after[0x4c60:0x4f6c].hex(), "owners": counts, "visits": visits}


def main():
    image = (ROOT / "Halo 2 Vista installer/halo2.exe").read_bytes()
    assert digest(image) == SHA
    rows = []

    def add(**values):
        q = dict(op="map", state=5, host=1, revision=0xfffffffe, seed=23, length=32, nul=-1,
                 type=2, map_id=17, variant_kind=2, null_variant=0, repeat=1)
        q.update(values)
        rows.append(execute(image, q))

    states = (0, 1, 2, 5, 6, 7, 8, 9, 10, 11, 0xffffffff)
    for state, host, length, nul in itertools.product(states, (0, 1),
                                                     (0, 1, 32, 126, 127, 128, 129, 260), (-1, 0)):
        if nul >= length:
            continue
        add(state=state, host=host, length=length, nul=nul)
    for state, host, kind, null in itertools.product(states, (0, 1), (0, 1, 2, 10, 0xffffffff), (0, 1)):
        add(op="variant", state=state, host=host, variant_kind=kind, null_variant=null)
    for state, revision, repeat in itertools.product((5, 6, 7, 8), (0, 1, 0xfffffffe, 0xffffffff), (1, 2, 3)):
        add(state=state, revision=revision, repeat=repeat, type=0xffffffff, map_id=0xffffffff, length=128)
        add(op="variant", state=state, revision=revision, repeat=repeat, variant_kind=0xffffffff)
    for length, nul, seed in itertools.product((127, 128, 129, 260), (1, 63, 126), (0, 1, 41)):
        add(length=length, nul=nul, seed=seed)
    OUT.mkdir(parents=True, exist_ok=True)
    fixture = ROOT / "tests/fixtures/vista_session_selection_original.json"
    fixture.write_text(json.dumps(rows, separators=(",", ":")) + "\n")
    report = dict(source_sha256=SHA, cases=len(rows), visits=sum(row["visits"] for row in rows),
                  fixture_sha256=digest(fixture.read_bytes()),
                  limits=["Full local setter bodies and their original UTF-16 copy, scalar memset and cookie checks execute unchanged.",
                          "Complete 7B88-byte before/after banks, AL returns, repeated writes and suffix guards are checked.",
                          "Remote states 3/4 deliberately reject before native mutation; transport is outside this binding.",
                          "This proves selected-parameter setters, not map readiness, world loading or a complete live transition."])
    (OUT / "original.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
