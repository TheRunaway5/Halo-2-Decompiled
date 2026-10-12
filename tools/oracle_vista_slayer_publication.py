#!/usr/bin/env python3
"""Execute unchanged supplied-PC Slayer publication and player activation code.

This CPU oracle supplies literal player-pool/session storage, not a live session.
The actual iterator, active query (except remote lookup), engine query, virtual
dispatch, event constructor and publication owner execute without code patches.
Network/local delivery and the subsequent player notification record call intent.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                              UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_EFLAGS)


SOURCE_SHA = "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
BASE, STOP, DATA, STACK = 0x400000, 0x1800000, 0x1810000, 0x1900000
WORLD, ENGINE, POOL, PLAYERS = DATA, DATA + 0x2000, DATA + 0x4000, DATA + 0x5000
BITS, SESSION, LOOKUPS = DATA + 0x6000, DATA + 0x7000, DATA + 0x8000
LOCAL_ID = bytes.fromhex("123456789abc")
STRIDE, ESP = 0x40, STACK + 0x8000


def u32(value: int) -> int:
    return value & 0xFFFFFFFF


def read32(uc: Uc, address: int) -> int:
    return struct.unpack("<I", bytes(uc.mem_read(address, 4)))[0]


def write32(uc: Uc, address: int, value: int) -> None:
    uc.mem_write(address, struct.pack("<I", u32(value)))


class Original:
    def __init__(self, image: bytes):
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.uc.mem_map(BASE, (len(image) + 0xFFF) & ~0xFFF)
        self.uc.mem_write(BASE, image)
        self.uc.mem_map(STOP, 0x1000)
        self.uc.mem_map(DATA, 0x10000)
        self.uc.mem_map(STACK, 0x10000)
        self.uc.hook_add(UC_HOOK_CODE, self.code)
        self.uc.hook_add(UC_HOOK_MEM_WRITE, self.write)
        self.total_instructions = 0

    def ret(self, value: int = 0, pop: int = 0) -> None:
        esp = self.uc.reg_read(UC_X86_REG_ESP)
        target = read32(self.uc, esp)
        self.uc.reg_write(UC_X86_REG_EAX, u32(value))
        self.uc.reg_write(UC_X86_REG_ESP, esp + 4 + pop)
        self.uc.reg_write(UC_X86_REG_EIP, target)

    def write(self, uc: Uc, access: int, address: int, size: int,
              value: int, _: object) -> None:
        pc = uc.reg_read(UC_X86_REG_EIP)
        if 0x45B05D <= pc < 0x45B08C:
            self.constructor_writes.append({"pc": f"{pc:08x}",
                "offset": address - self.event, "size": size,
                "value": f"{value & ((1 << (8 * size)) - 1):0{size * 2}x}"})

    def payload(self, pointer: int) -> dict:
        raw = bytes(self.uc.mem_read(pointer, 0x24))
        return {"defined_34_bytes": raw[:0x22].hex(),
                "opaque_padding_2_bytes": raw[0x22:].hex(),
                "type": read32(self.uc, pointer),
                "subtype": read32(self.uc, pointer + 4),
                "recipient": read32(self.uc, pointer + 8)}

    def code(self, uc: Uc, address: int, size: int, _: object) -> None:
        self.instructions += 1
        if address == STOP:
            uc.emu_stop()
        elif address == 0x45B15E:
            self.calls.append({"call": "engine_query", "index": read32(uc, ENGINE + 0xC54)})
        elif address == 0x46A29E:
            self.calls.append({"call": "engine_activation_wrapper", "recipient": read32(uc, uc.reg_read(UC_X86_REG_ESP) + 4)})
        elif address == 0x4498B7:
            self.calls.append({"call": "phase_gate", "phase_byte": self.phase})
        elif address == 0x510C2B:
            self.publisher_storage = uc.reg_read(UC_X86_REG_ESP) - 0x24
            self.publisher_before = bytes(uc.mem_read(self.publisher_storage, 0x24)).hex()
            self.calls.append({"call": "slayer_activation", "recipient": read32(uc, uc.reg_read(UC_X86_REG_ESP) + 4)})
        elif address == 0x45B05D:
            self.event = read32(uc, uc.reg_read(UC_X86_REG_ESP) + 12)
            self.calls.append({"call": "event_constructor", "type": read32(uc, uc.reg_read(UC_X86_REG_ESP) + 4),
                               "subtype": read32(uc, uc.reg_read(UC_X86_REG_ESP) + 8)})
        elif address == 0x45DC77:
            self.calls.append({"call": "publication", **self.payload(read32(uc, uc.reg_read(UC_X86_REG_ESP) + 4))})
        elif address in (0x5B67EA, 0x45DBF0):
            name = "network_delivery" if address == 0x5B67EA else "local_delivery"
            self.calls.append({"call": name, **self.payload(read32(uc, uc.reg_read(UC_X86_REG_ESP) + 4))})
            self.ret()  # cdecl; caller owns argument removal; return unconsumed
        elif address == 0x5B6662:
            esp = uc.reg_read(UC_X86_REG_ESP)
            self.calls.append({"call": "player_notification", "recipient": read32(uc, esp + 4),
                               "mask": read32(uc, esp + 8)})
            self.ret()  # cdecl
        elif address == 0x5AE0CB:
            pointer = read32(uc, uc.reg_read(UC_X86_REG_ESP) + 4)
            index = (pointer - PLAYERS - 0x10) // STRIDE
            if pointer != PLAYERS + index * STRIDE + 0x10 or index not in self.players:
                raise RuntimeError("original active query supplied unexpected player identifier")
            self.calls.append({"call": "active_query", "player": index})
        elif address == 0x4533E2:
            self.calls.append({"call": "active_query_result", "byte": uc.reg_read(UC_X86_REG_EAX) & 0xFF})
        elif address == 0x5DCF5F:
            pointer = read32(uc, uc.reg_read(UC_X86_REG_ESP) + 4)
            index = (pointer - PLAYERS - 0x10) // STRIDE
            if index not in self.players or pointer != PLAYERS + index * STRIDE + 0x10:
                raise RuntimeError("unexpected original remote lookup argument")
            if uc.reg_read(UC_X86_REG_ECX) != SESSION:
                raise RuntimeError("unexpected original session owner")
            value = self.players[index].get("remote_byte")
            self.calls.append({"call": "remote_lookup", "player": index,
                               "provided_binding_byte": value})
            self.ret(0 if value is None else LOOKUPS + index * 0x100, 4)

    def run(self, label: str, entry: int, phase: int, engine: bool = True,
            recipient: int = 0x12340000, poison: int = 0xA5,
            mode: str = "offline", players: list[dict] | None = None) -> dict:
        uc = self.uc
        self.phase, self.calls, self.constructor_writes = phase, [], []
        self.instructions, self.event = 0, 0
        self.publisher_storage, self.publisher_before = None, None
        self.players = {p["index"]: dict(p) for p in (players or [])}
        uc.mem_write(DATA, bytes(0x10000))
        uc.mem_write(STACK, bytes([poison]) * 0x10000)
        write32(uc, 0x882D3C, WORLD)
        uc.mem_write(WORLD + 0xC, bytes([phase]))
        write32(uc, 0x8BF8F8, ENGINE)
        write32(uc, ENGINE + 0xC54, 1)  # explicit valid selected table index, not inferred runtime state
        write32(uc, 0x8D8548 + 4, ENGINE if engine else 0)
        write32(uc, ENGINE, 0x7C31D4)  # actual RTTI-qualified Slayer vtable
        write32(uc, 0x8A8260, POOL)
        write32(uc, POOL + 0x20, 8)
        write32(uc, POOL + 0x24, STRIDE)
        write32(uc, POOL + 0x38, 8)
        write32(uc, POOL + 0x44, PLAYERS)
        write32(uc, POOL + 0x48, BITS)
        write32(uc, BITS, sum(1 << index for index in self.players))
        uc.mem_write(0x9178D0, bytes([mode != "offline"]))
        write32(uc, 0x9178DC, SESSION)
        write32(uc, SESSION + 8, 0 if mode == "empty" else 1)
        uc.mem_write(SESSION + 0xD, LOCAL_ID)
        for index, player in self.players.items():
            pointer = PLAYERS + index * STRIDE
            uc.mem_write(pointer, struct.pack("<HH", player.get("salt", 0x1234), player["flags"]))
            identity = LOCAL_ID if mode == "local" else struct.pack("<IH", index + 0x10203040, 0x5060)
            player["salt"] = player.get("salt", 0x1234)
            player["identifier_6_bytes"] = identity.hex()
            uc.mem_write(pointer + 0x10, identity)
            if player.get("remote_byte") is not None:
                uc.mem_write(LOOKUPS + index * 0x100 + 0x78, bytes([player["remote_byte"]]))
        uc.mem_write(ESP, struct.pack("<II", STOP, u32(recipient)))
        for register in (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX):
            uc.reg_write(register, 0)
        uc.reg_write(UC_X86_REG_EFLAGS, 2)
        uc.reg_write(UC_X86_REG_ECX, ENGINE)
        uc.reg_write(UC_X86_REG_ESP, ESP)
        uc.emu_start(entry, STOP + 1, count=100000)
        if uc.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError(f"original {label} did not return")
        final_esp = uc.reg_read(UC_X86_REG_ESP)
        expected_esp = ESP + (8 if entry == 0x510C2B else 4)
        if final_esp != expected_esp:
            raise RuntimeError(f"original {label} stack mismatch {final_esp:x} != {expected_esp:x}")
        after = [{"index": index, "flags": struct.unpack("<H", bytes(uc.mem_read(PLAYERS + index * STRIDE + 2, 2)))[0]}
                 for index in sorted(self.players)]
        self.total_instructions += self.instructions
        return {"label": label, "entry": f"{entry:08x}", "phase_byte": phase,
                "engine_present": engine, "supplied_engine_index": 1,
                "recipient_argument": u32(recipient), "stack_poison_byte": poison,
                "query_mode": mode, "players_before": list(self.players.values()),
                "players_after": after, "calls": self.calls,
                "last_publisher_storage_before": self.publisher_before,
                "last_publisher_storage_after": None if self.publisher_storage is None else bytes(uc.mem_read(self.publisher_storage, 0x24)).hex(),
                "constructor_writes": self.constructor_writes,
                "instructions": self.instructions}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("Halo 2 Vista installer/halo2.exe"))
    parser.add_argument("--report", type=Path, default=Path("analysis/frontend/vista-session-flow-audit/slayer-publication-original.json"))
    parser.add_argument("--fixture", type=Path, default=Path("tests/fixtures/vista_slayer_publication_original.tsv"))
    args = parser.parse_args()
    image = args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest() != SOURCE_SHA:
        raise SystemExit("unexpected supplied Vista executable identity")
    original = Original(image)
    rows = []
    for phase in range(256):
        for recipient in (0, 0xFFFFFFFF, 0x80000001):
            rows.append(original.run(f"publisher_phase_{phase:02x}_recipient_{recipient:08x}", 0x510C2B, phase, recipient=recipient))
        for engine in (False, True):
            rows.append(original.run(f"wrapper_phase_{phase:02x}_engine_{int(engine)}", 0x46A29E, phase, engine))
    for poison in (0, 0x5A, 0xFF):
        rows.append(original.run(f"publisher_padding_{poison:02x}", 0x510C2B, 0, poison=poison))
    modes = [("offline", None), ("empty", None), ("local", None),
             ("remote", None), ("remote", 0), ("remote", 1), ("remote", 0xFF)]
    for phase in (0, 4, 255):
        for engine in (False, True):
            for mode, remote in modes:
                for flags in (0, 1, 2, 3, 0xA5A0, 0xA5A1, 0xFFFC, 0xFFFF):
                    label = f"owner_{len(rows):04d}"
                    rows.append(original.run(label, 0x45339B, phase, engine, mode=mode,
                        players=[{"index": 0, "flags": flags, "salt": 0x8001, "remote_byte": remote}]))
    rows.append(original.run("owner_sparse_pool_order", 0x45339B, 0, mode="remote", players=[
        {"index": 7, "flags": 0, "salt": 0xFFFF, "remote_byte": 1},
        {"index": 1, "flags": 1, "salt": 0x1234, "remote_byte": 0},
        {"index": 3, "flags": 2, "salt": 0x5678, "remote_byte": 1},
        {"index": 4, "flags": 0xFFFC, "salt": 0xABCD, "remote_byte": 1}]))
    rows.append(original.run("owner_empty_pool", 0x45339B, 0, mode="offline"))
    # Retained flag state: source query changes, not a fabricated timer or match start.
    flags = 0xA5A0
    for index, remote in enumerate((1, 1, 0, 0, 1)):
        row = original.run(f"retained_{index}", 0x45339B, 0, mode="remote",
                           players=[{"index": 0, "flags": flags, "remote_byte": remote}])
        rows.append(row)
        flags = row["players_after"][0]["flags"]
    ranges = {"phase_gate": (0x4498B7, 0x4498C8), "engine_query": (0x45B15E, 0x45B172),
              "virtual_wrapper": (0x46A29E, 0x46A2BB), "slayer_publisher": (0x510C2B, 0x510C5F),
              "event_constructor": (0x45B05D, 0x45B08C), "publication_owner": (0x45DC77, 0x45DC96),
              "player_flag_owner": (0x45339B, 0x45343F), "iterator_init": (0x4664E5, 0x4664F9),
              "iterator_find": (0x4664F9, 0x466534), "iterator_next": (0x4669CD, 0x466A15),
              "active_query": (0x5AE0CB, 0x5AE1DE), "local_identifier_copy": (0x5DBF85, 0x5DBF99),
              "cookie_check": (0x6875D3, 0x6875DD)}
    ctor_masks = sorted({tuple(sorted({(w["offset"], w["size"]) for w in row["constructor_writes"]}))
                         for row in rows if row["constructor_writes"]})
    lines = ["# H2V_SLAYER_PUBLICATION_ORIGINAL_V1",
             "# P label phase recipient before_36_bytes after_36_bytes publication_intent",
             "# A label phase flags_before query_present query_byte flags_after engine_activation_intent notification_intent ordered_intents"]
    publisher_rows, activation_rows = 0, 0
    for row in rows:
        if row["entry"] == "00510c2b":
            published = sum(call["call"] == "publication" for call in row["calls"])
            if published not in (0, 1):
                raise RuntimeError("direct publisher emitted multiple original events")
            lines.append("\t".join(("P", row["label"], f"{row['phase_byte']:02x}",
                f"{row['recipient_argument']:08x}", row["last_publisher_storage_before"],
                row["last_publisher_storage_after"], str(published))))
            publisher_rows += 1
        elif row["entry"] == "0045339b" and len(row["players_before"]) == 1:
            results = [call["byte"] for call in row["calls"] if call["call"] == "active_query_result"]
            if len(results) > 1:
                raise RuntimeError("single-player owner queried more than once")
            engine_intent = int(any(call["call"] == "engine_activation_wrapper" for call in row["calls"]))
            notify = int(any(call["call"] == "player_notification" for call in row["calls"]))
            order = ",".join("engine" if call["call"] == "engine_activation_wrapper" else "notify"
                for call in row["calls"] if call["call"] in ("engine_activation_wrapper", "player_notification"))
            lines.append("\t".join(("A", row["label"], f"{row['phase_byte']:02x}",
                f"{row['players_before'][0]['flags']:04x}", str(bool(results) * 1),
                f"{results[0] if results else 0:02x}", f"{row['players_after'][0]['flags']:04x}",
                str(engine_intent), str(notify), order or "-")))
            activation_rows += 1
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text("\n".join(lines) + "\n")
    callsites = {}
    for target in (0x46A29E, 0x45339B, 0x45815E, 0x510C2B):
        callsites[f"{target:08x}"] = [f"{i + BASE:08x}" for i in range(0x1000, 0x39AD56 - 4)
            if image[i] == 0xE8 and u32(i + BASE + 5 + struct.unpack_from("<i", image, i + 1)[0]) == target]
    report = {"version": "H2V_SLAYER_PUBLICATION_ORIGINAL_V1",
              "source": str(args.exe.resolve()), "source_sha256": SOURCE_SHA, "source_size": len(image),
              "tool_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "fixture_sha256": hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
              "fixture_publisher_rows": publisher_rows, "fixture_activation_rows": activation_rows,
              "case_count": len(rows), "instruction_count": original.total_instructions,
              "instruction_ranges": {name: {"start": f"{a:08x}", "end_exclusive": f"{z:08x}",
                  "sha256": hashlib.sha256(image[a-BASE:z-BASE]).hexdigest()} for name, (a, z) in ranges.items()},
              "rtti": {"complete_object_locator": "007fd8f4", "type_descriptor": "0081da08",
                  "type_name": image[0x81DA10-BASE:0x81DA10-BASE+64].split(b"\0")[0].decode("ascii"),
                  "vtable": "007c31d4", "slot_offset": "00000018",
                  "slot_target": f"{struct.unpack_from('<I', image, 0x7C31EC-BASE)[0]:08x}",
                  "locator_bytes": image[0x7FD8F4-BASE:0x7FD908-BASE].hex(),
                  "vtable_bytes": image[0x7C31D0-BASE:0x7C3204-BASE].hex()},
              "constructor_write_masks": ctor_masks,
              "literal_E8_rel32_candidates_in_text": callsites,
              "boundary": {"unchanged_source_instructions": True, "game_gpu_audio_executed": False,
                  "literal_storage": "World phase byte, selected valid engine index/table pointer, actual Slayer vtable, pool capacity/stride/bits/player salt+flags+identifier and active-query session fields are explicit inputs. They are not captured live-session state.",
                  "executed": "Complete publisher/wrapper and whole player-flag method, actual iterator/active query/local identity copy/stack-cookie check/engine query/event constructor/publication owner. Source instructions are never replaced.",
                  "remote_lookup": "5DCF5F receives the actual session and player identifier; supplied NULL or binding pointer supplies byte +78. The original lookup's session graph traversal is not executed.",
                  "delivery": "5B67EA network delivery, 45DBF0 local response delivery, and 5B6662 notification record original arguments and return without side effects. No reentrant service mutation is supplied.",
                  "event_extent": "Constructor defines exactly offsets 0..33 (0x22 bytes); publisher reserves 0x24. Bytes +22/+23 remain opaque stack padding and are retained, not normalized.",
                  "not_proved": ["actual player lifecycle/query service ownership", "engine-table runtime initialization", "full current session readiness", "response audience/hash selection", "actual voice playback/cadence", "reentrant delivery side effects", "DLL-patched runtime equivalence", "complete pregame/gameplay/postgame timing"]},
              "cases": rows}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"cases": len(rows), "instructions": original.total_instructions,
                      "constructor_write_masks": ctor_masks, "report": str(args.report),
                      "fixture_publisher_rows": publisher_rows, "fixture_activation_rows": activation_rows,
                      "report_sha256": hashlib.sha256(args.report.read_bytes()).hexdigest()}))


if __name__ == "__main__":
    main()
