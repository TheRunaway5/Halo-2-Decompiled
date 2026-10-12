#!/usr/bin/env python3
"""Execute unchanged Vista lobby active/stage/start/beep owners dynamically."""
from pathlib import Path
import hashlib
import itertools
import json
import struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from oracle_vista_pcr_screen import machine, STOP, STACK, OBJ, SHA, u32, w32

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "analysis/frontend/vista-session-flow-audit/lobby-presentation-original"


def signed(value):
    return struct.unpack("<i", struct.pack("<I", value & 0xffffffff))[0]


def execute(image, q):
    u = machine(image)
    w32(u, OBJ + 0x347c, q["before_stage"])
    w32(u, OBJ + 0x3480, q["before_clock"])
    w32(u, 0xd71900, q["clock"])
    events, ui_queries, visits = [], 0, 0

    def ret(value):
        sp = u.reg_read(UC_X86_REG_ESP)
        u.reg_write(UC_X86_REG_EAX, value & 0xffffffff)
        u.reg_write(UC_X86_REG_EIP, u32(u, sp))
        u.reg_write(UC_X86_REG_ESP, sp + 4)

    def hook(_u, pc, _size, _):
        nonlocal visits, ui_queries
        visits += 1
        sp = u.reg_read(UC_X86_REG_ESP)
        if pc == STOP:
            u.emu_stop()
        elif pc == 0x616490:
            assert u32(u, sp + 4) == 0
            value = q["ui"][ui_queries]
            events.append(["ui", value]); ui_queries += 1; ret(value)
        elif pc == 0x5b5038:
            events.append(["active", q["active"]]); ret(q["active"])
        elif pc == 0x5ad660:
            events.append(["slot", q["slot"]]); ret(q["slot"])
        elif pc == 0x5b13c7:
            events.append(["stage", q["stage"]]); ret(q["stage"])
        elif pc == 0x61dd04:
            events.append(["sound", u32(u, sp + 4), signed(u32(u, OBJ + 0x347c)), u32(u, OBJ + 0x3480)])
            if q["sound_mutation"] is not None:
                w32(u, OBJ + 0x347c, q["sound_mutation"])
            ret(0)
        elif pc == 0x6096ae:
            events.append(["clock", q["clock"]])  # Execute the actual getter.
        elif pc == 0x5b1bde:
            args = [u32(u, sp + 4 + 4 * i) for i in range(9)]
            assert all(value == 0 for i, value in enumerate(args) if i != 4)
            assert u32(u, args[4]) == 0
            w32(u, args[4], 0x87654321)
            events.append(["duration"]); ret(0x80)
        elif pc == 0x60687f:
            events.append(["user", signed(u32(u, sp + 4))]); ret(q["user"])
        elif pc == 0x5ad940:
            events.append(["host", q["host"]]); ret(q["host"])
        elif pc == 0x5b11e2:
            args = [u32(u, sp + 4 + 4 * i) for i in range(5)]
            assert args[4] == 0
            events.append(["write", args[0], signed(args[1]), args[2], signed(args[3])]); ret(q["write"])

    u.hook_add(UC_HOOK_CODE, hook)
    entry = {"active": 0x616ba2, "stage": 0x616b7a, "update": 0x644878, "start": 0x643a40}[q["op"]]
    args = [0] if q["op"] == "update" else [q["controller"] & 0xffffffff] if q["op"] == "start" else []
    sp = STACK + 0x18000
    u.mem_write(sp, struct.pack("<" + "I" * (1 + len(args)), STOP, *args))
    u.reg_write(UC_X86_REG_ESP, sp); u.reg_write(UC_X86_REG_ECX, OBJ)
    u.emu_start(entry, STOP + 1, count=100000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP
    returned = None if q["op"] == "update" else signed(u.reg_read(UC_X86_REG_EAX)) if q["op"] == "stage" else u.reg_read(UC_X86_REG_EAX) & 255
    return dict(input=q, events=events, returned=returned, stage=signed(u32(u, OBJ + 0x347c)),
                clock=u32(u, OBJ + 0x3480), visits=visits)


def main():
    image = (ROOT / "Halo 2 Vista installer/halo2.exe").read_bytes()
    assert hashlib.sha256(image).hexdigest() == SHA
    rows = []

    def add(**values):
        q = dict(op="update", ui=[11, 11, 11], active=0x80, slot=1, stage=3, before_stage=4,
                 before_clock=0x87654321, clock=0x12345678, sound_mutation=None,
                 controller=2, user=1, host=0x80, write=0x80)
        q.update(values); rows.append(execute(image, q))

    for ui, active, slot in itertools.product((-1, 0, 9, 10, 11, 12), (0, 1, 0x80), (0, 1, 2, 3, 4, 0xffffffff)):
        add(op="active", ui=[ui, 11, 11], active=active, slot=slot)
    for ui, stage in itertools.product((-1, 0, 10, 11, 12), (-0x80000000, -1, 0, 1, 2, 3, 4, 10, 0x7fffffff)):
        add(op="stage", ui=[ui, 11, 11], stage=stage)
    for first_ui, second_ui, slot, stage, previous in itertools.product((10, 11), (10, 11), (1, 2),
            (-1, 0, 1, 2, 3, 4, 10), (-1, 0, 1, 2, 3, 4, 10)):
        add(ui=[first_ui, second_ui, 11], slot=slot, stage=stage, before_stage=previous)
    for stage, mutation, clock in itertools.product((0, 1, 2, 3, 4, 10), (None, -1, 0, 2, 3, 4, 10), (0, 0xffffffff)):
        add(stage=stage, before_stage=-1, sound_mutation=mutation, clock=clock)
    for first_ui, request_ui, user, host, write in itertools.product((10, 11), (9, 10, 11, 12), (-1, 0, 3), (0, 0x80), (0, 0x80)):
        add(op="start", ui=[first_ui, request_ui, 11], user=user, host=host, write=write)
    for first_ui, query_ui, stage, slot, controller in itertools.product((10, 11), (10, 11),
            (-0x80000000, -1, 0, 3, 4, 10, 0x7fffffff), (1, 2), (-1, 0, 0x7fffffff)):
        add(op="start", ui=[first_ui, 11, query_ui], stage=stage, slot=slot, controller=controller)
    OUT.mkdir(parents=True, exist_ok=True)
    fixture = ROOT / "tests/fixtures/vista_lobby_presentation_original.json"
    fixture.write_text(json.dumps(rows, separators=(",", ":")) + "\n")
    report = dict(source_sha256=SHA, cases=len(rows), visits=sum(row["visits"] for row in rows),
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        limits=["Full source active/stage/start/consumer routines execute unchanged; readiness, session write, audio device and account services are controlled boundaries.",
                "The actual UI clock getter executes, and sound callbacks may mutate the displayed stage before its source reread.",
                "The consumer's unrelated sign-in tail is disabled by the original false argument.",
                "No native audio-device, loaded world, online service or complete transition parity is claimed."])
    (OUT / "original.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
