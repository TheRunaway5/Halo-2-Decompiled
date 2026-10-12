#!/usr/bin/env python3
"""Execute unchanged Vista lobby countdown code with declared API boundaries.

This is a consumer oracle, not a complete session, audio, or timer execution.
The original active/stage query calls receive explicit test inputs; the sound
service records its actual argument without playing audio. The actual UI clock
getter is executed. All instruction bytes come from the pinned PE image.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP


SOURCE_SHA = "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
BASE = 0x400000
STOP = 0x1800000
OBJECT = 0x1810000
STACK = 0x1900000
SERVICES = {0x616BA2: "active", 0x616B7A: "stage", 0x61DD04: "sound"}


def word(value: int) -> int:
    return value & 0xFFFFFFFF


def signed(value: int) -> int:
    return struct.unpack("<i", struct.pack("<I", word(value)))[0]


def make_machine(image: bytes) -> Uc:
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(BASE, (len(image) + 0xFFF) & ~0xFFF)
    uc.mem_write(BASE, image)
    uc.mem_map(STOP, 0x1000)
    uc.mem_map(OBJECT, 0x4000)
    uc.mem_map(STACK, 0x10000)
    return uc


def read32(uc: Uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def ret_input(uc: Uc, value: int) -> None:
    esp = uc.reg_read(UC_X86_REG_ESP)
    pc = read32(uc, esp)
    uc.reg_write(UC_X86_REG_EAX, word(value))
    uc.reg_write(UC_X86_REG_ESP, esp + 4)
    uc.reg_write(UC_X86_REG_EIP, pc)


def consumer(image: bytes, active: int, stage: int, clock: int,
             before_stage: int, before_clock: int) -> dict:
    uc = make_machine(image)
    esp = STACK + 0x8000
    uc.mem_write(esp, struct.pack("<II", STOP, 0))  # sign-in tail explicitly disabled
    uc.mem_write(OBJECT + 0x347C, struct.pack("<II", word(before_stage), word(before_clock)))
    uc.mem_write(0xD71900, struct.pack("<I", word(clock)))
    uc.reg_write(UC_X86_REG_ESP, esp)
    uc.reg_write(UC_X86_REG_ECX, OBJECT)
    calls = []

    def hook(machine: Uc, address: int, size: int, _: object) -> None:
        if address == STOP:
            machine.emu_stop()
        elif address in SERVICES:
            service = SERVICES[address]
            if service == "sound":
                slot = read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4)
                calls.append(f"sound:{slot}")
                ret_input(machine, 0)  # return value is unconsumed by this caller
            else:
                calls.append(service)
                ret_input(machine, active if service == "active" else stage)
        elif address == 0x6096AE:
            calls.append("clock")  # execute the unchanged getter, do not replace it

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(0x644878, STOP + 1, count=10000)
    if uc.reg_read(UC_X86_REG_EIP) != STOP:
        raise RuntimeError("original consumer did not return")
    return {"stage": read32(uc, OBJECT + 0x347C),
            "clock": read32(uc, OBJECT + 0x3480), "calls": calls}


def stage_producer(image: bytes, session_state: int, session_stage: int) -> dict:
    uc = make_machine(image)
    esp = STACK + 0x8000
    uc.mem_write(esp, struct.pack("<I", STOP))
    uc.reg_write(UC_X86_REG_ESP, esp)
    calls = []

    def hook(machine: Uc, address: int, size: int, _: object) -> None:
        if address == STOP:
            machine.emu_stop()
        elif address == 0x616490:
            arg = read32(machine, machine.reg_read(UC_X86_REG_ESP) + 4)
            calls.append({"service": "session_state", "argument": arg})
            if arg != 0:
                raise RuntimeError("unexpected original state-query argument")
            ret_input(machine, session_state)
        elif address == 0x5B13C7:
            calls.append({"service": "session_stage"})
            ret_input(machine, session_stage)

    uc.hook_add(UC_HOOK_CODE, hook)
    uc.emu_start(0x616B7A, STOP + 1, count=10000)
    if uc.reg_read(UC_X86_REG_EIP) != STOP:
        raise RuntimeError("original stage query did not return")
    return {"session_state": word(session_state), "session_stage": word(session_stage),
            "result": uc.reg_read(UC_X86_REG_EAX), "calls": calls}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("Halo 2 Vista installer/halo2.exe"))
    parser.add_argument("--fixture", type=Path, default=Path("tests/fixtures/vista_lobby_countdown_original.tsv"))
    parser.add_argument("--report", type=Path, default=Path("analysis/frontend/vista-session-flow-audit/countdown-original.json"))
    args = parser.parse_args()
    image = args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest() != SOURCE_SHA:
        raise SystemExit("unexpected Vista executable identity")
    values = [-0x80000000, -1, 0, 1, 2, 3, 4, 0x7FFFFFFF]
    clocks = [(0, 0), (0x12345678, 0x87654321), (0xFFFFFFFE, 1), (1, 0xFFFFFFFF)]
    rows = []
    for active in (0, 1):
        for stage in values:
            for before_stage in values:
                for before_clock, clock in clocks:
                    label = f"case_{len(rows):04d}"
                    result = consumer(image, active, stage, clock, before_stage, before_clock)
                    rows.append(dict(label=label, active=active, stage_input=word(stage),
                                     clock_input=clock, before_stage=word(before_stage),
                                     before_clock=before_clock, **result))
    stage, clock = -1, 0xF0000000
    for index, supplied in enumerate((3, 3, 0, 3, -1, 3)):
        result = consumer(image, 1, supplied, 100 + index, stage, clock)
        rows.append(dict(label=f"sequential_{index}", active=1, stage_input=word(supplied),
                         clock_input=100 + index, before_stage=word(stage), before_clock=clock,
                         **result))
        stage, clock = result["stage"], result["clock"]
    producer = [stage_producer(image, state, stage)
                for state in (-1, 0, 5, 9, 10, 11, 12, 0x7FFFFFFF) for stage in values]
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    lines = ["# H2V_LOBBY_COUNTDOWN_ORIGINAL_V1", "# C label active stage_query_word clock_word before_stage_word before_clock_word after_stage_word after_clock_word call_count calls"]
    for row in rows:
        fields = ["C", row["label"], str(row["active"])]
        fields.extend(f"{row[key]:08x}" for key in ("stage_input", "clock_input", "before_stage", "before_clock", "stage", "clock"))
        fields.extend((str(len(row["calls"])), ",".join(row["calls"])))
        lines.append("\t".join(fields))
    args.fixture.write_text("\n".join(lines) + "\n")
    ranges = {"consumer": (0x644878, 0x644935), "bounded_consumer": (0x644878, 0x6448CD),
              "stage_producer": (0x616B7A, 0x616BA2), "ui_clock_getter": (0x6096AE, 0x6096B4),
              "timer_update": (0x5D9BA7, 0x5D9C1A)}
    report = {"version": "H2V_LOBBY_COUNTDOWN_ORIGINAL_V1", "source": str(args.exe.resolve()),
              "source_sha256": SOURCE_SHA, "source_size": len(image),
              "tool_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "fixture_sha256": hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
              "consumer_case_count": len(rows), "stage_producer_case_count": len(producer),
              "instruction_ranges": {name: {"start": f"{a:08x}", "end_exclusive": f"{z:08x}",
                      "sha256": hashlib.sha256(image[a-BASE:z-BASE]).hexdigest()} for name, (a, z) in ranges.items()},
              "boundary": {"unchanged_instruction_bytes": True, "imports_executed": False,
                  "consumer": "Complete original method with original sign-in argument zero; active/stage query results are explicit API inputs. Sound service records intent only. Actual original UI clock getter executes against supplied D71900 word.",
                  "stage_producer": "Original 616B7A executes with declared 616490(NULL) and 5B13C7 result boundaries; full session/task graph is not executed.",
                  "not_proved": ["actual session start duration", "actual network cadence", "audio playback", "loading/fades/announcer sequencing", "Project Cartographer DLL patch effects"]},
              "consumer_cases": rows, "stage_producer_cases": producer}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"consumer_cases": len(rows), "stage_producer_cases": len(producer),
                      "fixture": str(args.fixture), "fixture_sha256": report["fixture_sha256"]}))


if __name__ == "__main__":
    main()
