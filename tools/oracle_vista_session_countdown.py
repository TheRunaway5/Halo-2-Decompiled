#!/usr/bin/env python3
"""Bounded original-PC start admission and countdown task oracle.

The session state graph, UI readiness, controller ownership, network delivery,
and Cartographer hooks are not executed. No original code bytes are changed.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from oracle_vista_lobby_countdown import SOURCE_SHA, BASE, STOP, OBJECT, STACK, make_machine, read32, ret_input, word


def token(name: str, *arguments: int) -> str:
    return name + "".join(f":{word(value):08x}" for value in arguments)


def start(image: bytes, controller: int, requested: int, minimum: int, ui: int,
          user: int, host: int, stage: int, write_result: int) -> dict:
    uc = make_machine(image)
    esp = STACK + 0x8000
    uc.mem_write(esp, struct.pack("<4I", STOP, word(controller), word(requested), word(minimum)))
    uc.reg_write(UC_X86_REG_ESP, esp)
    calls = []
    values = {0x616490: ("ui", ui), 0x60687F: ("user", user),
              0x5AD940: ("host", host), 0x616B7A: ("active", stage)}

    def hook(machine, address, size, _):
        if address == STOP:
            machine.emu_stop()
        elif address in values:
            name, value = values[address]
            if name in ("ui", "user"):
                calls.append(token(name, read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4)))
            else:
                calls.append(name)
            ret_input(machine, value)
        elif address == 0x5B11E2:
            s = machine.reg_read(UC_X86_REG_ESP)
            arguments = [read32(machine, s + 4 + index * 4) for index in range(5)]
            calls.append(token("write", *arguments))
            ret_input(machine, write_result)  # Cdecl; original caller removes five args.

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(0x617328, STOP + 1, count=10000)
    if uc.reg_read(UC_X86_REG_EIP) != STOP:
        raise RuntimeError("start leaf did not return")
    return {"success": uc.reg_read(UC_X86_REG_EAX) & 0xFF, "calls": calls}


def timer(image: bytes, before_stage: int, before_clock: int, stage: int,
          flag: int, change_clock: int, elapsed_now: int, decrement_clock: int) -> dict:
    uc = make_machine(image)
    session, owner = 0x1A00000, 0x1A10000
    uc.mem_map(session, 0x8000)
    uc.mem_map(owner, 0x1000)
    esp = STACK + 0x8000
    uc.mem_write(esp, struct.pack("<I", STOP))
    uc.mem_write(OBJECT + 8, struct.pack("<I", owner))
    uc.mem_write(owner + 0x24, struct.pack("<I", session))
    uc.mem_write(OBJECT + 0x10, struct.pack("<II", word(before_clock), word(before_stage)))
    # Explicit valid session metadata; original getters, not replacements, read it.
    uc.mem_write(session + 0x73A0, struct.pack("<I", 5))
    uc.mem_write(session + 0x4C94, b"\1")
    uc.mem_write(session + 0x4C98, struct.pack("<I", word(stage)))
    uc.mem_write(session + 0x4C9C, bytes([flag]))
    # Actual original clock's override branch; each sample is supplied explicitly.
    uc.mem_write(0x91ABC8, b"\1")
    uc.reg_write(UC_X86_REG_ESP, esp)
    uc.reg_write(UC_X86_REG_ECX, OBJECT)
    calls, raw_writes = [], []

    def clock_word(machine, value):
        machine.mem_write(0x91ABCC, struct.pack("<I", word(value)))

    def hook(machine, address, size, _):
        if address == STOP:
            machine.emu_stop()
        elif address == 0x5C25DF:
            calls.append("session")
        elif address == 0x5C261F:
            calls.append("flag")
        elif address == 0x5B3CA0:
            pc = read32(machine, machine.reg_read(UC_X86_REG_ESP))
            if pc == 0x5D9BD1:
                clock_word(machine, change_clock)
            elif pc == 0x5D9BF7:
                clock_word(machine, decrement_clock)
            else:
                raise RuntimeError(f"unknown original clock callsite {pc:x}")
            calls.append("clock")
        elif address == 0x5B3CB4:
            clock_word(machine, elapsed_now)
            calls.append(token("elapsed", read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4)))
        elif address == 0x5C96FB:
            s = machine.reg_read(UC_X86_REG_ESP)
            arguments = [read32(machine, s + 4 + index * 4) for index in range(5)]
            raw_writes.append(arguments)
            # Arguments1 and3 consume only their low byte. In particular, the
            # original flag scratch retains unowned upper bytes of pushed ECX.
            semantic = list(arguments)
            semantic[0] &= 0xFF
            semantic[2] &= 0xFF
            calls.append(token("write", *semantic))
            ret_input(machine, 0)
            machine.reg_write(UC_X86_REG_ESP, machine.reg_read(UC_X86_REG_ESP) + 20)

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(0x5D9BA7, STOP + 1, count=10000)
    if uc.reg_read(UC_X86_REG_EIP) != STOP:
        raise RuntimeError("timer leaf did not return")
    return {"after_stage": read32(uc, OBJECT + 0x14), "after_clock": read32(uc, OBJECT + 0x10),
            "return_al": uc.reg_read(UC_X86_REG_EAX) & 0xFF, "calls": calls, "raw_writes": raw_writes}


def duration(image: bytes, query_result: int) -> dict:
    uc = make_machine(image)
    esp = STACK + 0x8000
    uc.mem_write(esp, struct.pack("<I", STOP))
    uc.reg_write(UC_X86_REG_ESP, esp)
    calls = []

    def hook(machine, address, size, _):
        if address == STOP:
            machine.emu_stop()
        elif address == 0x5B1BDE:
            calls.append("5B1BDE")
            ret_input(machine, query_result)

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(0x615A4B, STOP + 1, count=1000)
    if uc.reg_read(UC_X86_REG_EIP) != STOP:
        raise RuntimeError("duration leaf did not return")
    return {"query_result": word(query_result), "eax": uc.reg_read(UC_X86_REG_EAX), "calls": calls}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("Halo 2 Vista installer/halo2.exe"))
    parser.add_argument("--fixture", type=Path, default=Path("tests/fixtures/vista_session_countdown_original.tsv"))
    parser.add_argument("--report", type=Path, default=Path("analysis/frontend/vista-session-flow-audit/countdown-producer-original.json"))
    args = parser.parse_args()
    image = args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest() != SOURCE_SHA:
        raise SystemExit("unexpected Vista executable identity")
    starts, timers = [], []
    for controller in (-1, 0):
        for ui in (-1, 0, 9, 10, 11, 12):
            for user in (-1, 0, 15):
                for host in (0, 1):
                    for stage in (-0x80000000, -1, 0, 3, 4, 10, 0x7FFFFFFF):
                        for accepted in (0, 1):
                            row = dict(label=f"start_{len(starts):04d}", controller=word(controller),
                                       requested=10, minimum=3, ui=word(ui), user=word(user),
                                       host=host, active_stage=word(stage), write_result=accepted)
                            row.update(start(image, controller, 10, 3, ui, user, host, stage, accepted))
                            starts.append(row)
    for minimum in (-0x80000000, -1, 0x7FFFFFFF):
        for stage in (-0x80000000, -1, 0, 3, 4, 10, 0x7FFFFFFF):
            row = dict(label=f"minimum_{len(starts)}", controller=2, requested=17,
                       minimum=word(minimum), ui=11, user=1, host=0, active_stage=word(stage), write_result=1)
            row.update(start(image, 2, 17, minimum, 11, 1, 0, stage, 1))
            starts.append(row)
    deltas = (0, 999, 1000, 1001, 5000, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF)
    for stage in (-0x80000000, -1, 0, 1, 3, 10, 0x7FFFFFFF):
        for before in (stage, -1, 3):
            for flag in (0, 1, 0x80, 0xFF):
                for base_clock in (0x12345678, 0xFFFFFFF0):
                    for delta in deltas:
                        changed = stage != before
                        change = word(base_clock + 19)
                        last = change if changed else base_clock
                        elapsed_now = word(last + delta)
                        row = dict(label=f"timer_{len(timers):04d}", before_stage=word(before), before_clock=base_clock,
                                   session_stage=word(stage), flag=flag, change_clock=change,
                                   elapsed_now=elapsed_now, decrement_clock=word(elapsed_now + 7))
                        row.update(timer(image, before, base_clock, stage, flag, change, elapsed_now, row["decrement_clock"]))
                        timers.append(row)
    last_stage, last_clock = -1, 0
    for index, (stage, now) in enumerate(((10, 0), (10, 999), (10, 1000), (9, 2000), (8, 5000), (7, 5001))):
        row = dict(label=f"retained_{index}", before_stage=word(last_stage), before_clock=last_clock,
                   session_stage=stage, flag=1, change_clock=now, elapsed_now=now, decrement_clock=now)
        row.update(timer(image, last_stage, last_clock, stage, 1, now, now, now))
        timers.append(row)
        last_stage, last_clock = row["after_stage"], row["after_clock"]
    durations = [duration(image, value) for value in (0, 1, 0xFFFFFFFF, 0x80000000)]
    if any(item["eax"] != 10 for item in durations):
        raise RuntimeError("literal PC duration oracle failed")
    lines = ["# H2V_SESSION_COUNTDOWN_ORIGINAL_V1",
             "# S label controller requested minimum ui user host active_stage write_result success call_count calls",
             "# T label before_stage before_clock session_stage flag change_clock elapsed_now decrement_clock after_stage after_clock return_al call_count calls"]
    for row in starts:
        fields = ["S", row["label"]]
        fields.extend(f"{row[k]:08x}" for k in ("controller", "requested", "minimum", "ui", "user", "host", "active_stage", "write_result", "success"))
        fields.extend((str(len(row["calls"])), ",".join(row["calls"])))
        lines.append("\t".join(fields))
    for row in timers:
        fields = ["T", row["label"]]
        fields.extend(f"{row[k]:08x}" for k in ("before_stage", "before_clock", "session_stage", "flag", "change_clock", "elapsed_now", "decrement_clock", "after_stage", "after_clock", "return_al"))
        fields.extend((str(len(row["calls"])), ",".join(row["calls"])))
        lines.append("\t".join(fields))
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text("\n".join(lines) + "\n")
    ranges = {"duration": (0x615A4B,0x615A77), "lobby_request": (0x643A40,0x643A83),
              "start_admission": (0x617328,0x6173A5), "timer": (0x5D9BA7,0x5D9C1A),
              "network_clock": (0x5B3CA0,0x5B3CD1), "timer_initialization": (0x5D9B62,0x5D9BA2)}
    report = dict(version="H2V_SESSION_COUNTDOWN_ORIGINAL_V1", source=str(args.exe.resolve()), source_sha256=SOURCE_SHA,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                  start_count=len(starts), timer_count=len(timers), duration_count=len(durations),
                  instruction_ranges={name: dict(start=f"{a:08x}", end_exclusive=f"{z:08x}",
                      sha256=hashlib.sha256(image[a-BASE:z-BASE]).hexdigest()) for name,(a,z) in ranges.items()},
                  boundary={"unchanged_code": True, "start": "UI/controller/host/active-stage/write query services are explicit input/result boundaries. Actual617328 instructions execute.",
                      "timer": "Actual5D9BA7 plus actual5C25DF/5C261F getters execute on supplied session metadata state5/running1. Actual5B3CA0/5B3CB4 override paths execute with supplied per-call override words. Setter5C96FB records intent only; no network delivery.",
                      "timer_flag": "Raw setter argument3 retains scratch upper24 bits; fixture includes only consumed low byte. JSON preserves raw words separately.",
                      "retained": "Six original task-state updates retain original prior results; the explicitly supplied next session stage is not a claim of executed network delivery.",
                      "not_proved": ["live UI readiness", "task/session admission lifetime", "delay and cancel delivery", "Cartographer runtime patch configuration", "loading/world-start completion"]},
                  starts=starts, timers=timers, durations=durations)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key:report[key] for key in ("start_count","timer_count","duration_count","fixture_sha256")}))


if __name__ == "__main__":
    main()
