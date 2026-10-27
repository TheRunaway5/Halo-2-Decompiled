#!/usr/bin/env python3
"""Compare unchanged TU1.5 roster decision blocks with the native row kernel.

Run through tools/run_test.py. This executes bounded original decision blocks
with explicit synthetic session-getter outputs. It neither replaces callees nor
claims to execute the complete original networking, allocator or widget path.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_EBX, UC_X86_REG_EDI,
    UC_X86_REG_ESI, UC_X86_REG_ESP,
)

from oracle_game_variants import ROOT, Source, STACK, STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    if path is not None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(value, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tu15", type=Path, default=NEW_DEFAULT)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--fixture", type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    source = Source(dict(path=str(args.tu15.resolve()), pin=NEW_PIN, globals=0x4ECC1C))
    machine = source.g
    machine.mem_map(0x1100000, 0x10000)
    frame, owner, pool, datum = 0x1101000, 0x1102000, 0x1103000, 0x1104000

    def write(at, value):
        machine.mem_write(at, struct.pack("<I", value & 0xFFFFFFFF))

    def read(at):
        return struct.unpack("<i", machine.mem_read(at, 4))[0]

    def block(entry, stops, registers):
        write(STACK, STOP)
        machine.reg_write(UC_X86_REG_ESP, STACK)
        for register, value in registers:
            machine.reg_write(register, value & 0xFFFFFFFF)
        reached = []

        def stop(_machine, address, _size, _data):
            if address in stops:
                reached.append(address)
                machine.emu_stop()

        hook = machine.hook_add(UC_HOOK_CODE, stop)
        try:
            machine.emu_start(entry, STOP, count=2000)
        finally:
            machine.hook_del(hook)
        assert len(reached) == 1, (hex(entry), reached)
        source.executed.discard(reached[0])
        return reached[0]

    cases = []
    for pane in range(4):
        for ranked in (False, True):
            for party_players in (1, 2, 4):
                for game_players in (0, 1, 2, 3, 4, 7, 8):
                    for maximum in (None, 4, 8):
                        machine.mem_write(frame - 0x100, bytes(0x100))
                        machine.mem_write(frame - 1, bytes([ranked]))
                        selected = block(0x257835, {0x1946AA, 0x1946E6}, [
                            (UC_X86_REG_EBP, frame), (UC_X86_REG_EBX, pane), (UC_X86_REG_EAX, 0)])
                        party = selected == 0x1946AA
                        known = party_players if party else game_players
                        write(frame - 0xC, maximum if maximum is not None else 777)
                        write(frame - 0x10, game_players)
                        if party:
                            block(0x257862, {0x257873}, [(UC_X86_REG_EBP, frame),
                                (UC_X86_REG_ESI, party_players), (UC_X86_REG_EAX, maximum is not None)])
                        elif pane == 1:
                            block(0x2578F6, {0x257950}, [(UC_X86_REG_EBP, frame),
                                (UC_X86_REG_EAX, maximum is not None)])
                        else:
                            block(0x25790E, {0x257917}, [(UC_X86_REG_EBP, frame), (UC_X86_REG_EBX, pane)])
                        count = read(frame - 0xC)
                        write(frame - 8, known)
                        write(frame - 0x10, game_players)
                        write(owner + 0x70, pool)
                        write(pool + 0x44, datum)
                        for index in range(16):
                            write(frame - 0x54 + index * 4, 100 + index)
                        rows = []
                        for index in range(count):
                            machine.mem_write(datum, bytes(4))
                            block(0x257977, {0x2579A4}, [(UC_X86_REG_EBP, frame),
                                (UC_X86_REG_EBX, owner), (UC_X86_REG_ESI, 0xFFFF),
                                (UC_X86_REG_EDI, index), (UC_X86_REG_EAX, 0)])
                            value = struct.unpack("<h", machine.mem_read(datum + 2, 2))[0]
                            assert value >= 100 or value in (16, -1), value
                            rows.append("player" if value >= 100 else "matched" if value == 16 else "empty")
                        cases.append(dict(pane=pane, ranked=ranked, party_players=party_players,
                            game_players=game_players, maximum=maximum, use_party=party, rows=rows))

    probe, binary = scratch / "matchmaking-roster.cpp", scratch / "matchmaking-roster"
    probe.write_text('''#include "halo2/frontend_matchmaking_progress.hpp"
#include <iostream>
int main() {
  unsigned pane, ranked, party, game; int maximum;
  while (std::cin >> pane >> ranked >> party >> game >> maximum) {
    std::optional<unsigned> limit; if (maximum >= 0) limit = maximum;
    const auto rows = halo2::matchmaking_progress_rows(pane, ranked, party, game, limit);
    std::cout << halo2::matchmaking_progress_uses_party(pane, ranked);
    for (auto row : rows) std::cout << " " << int(row);
    std::cout << "\\n";
  }
}
''')
    subprocess.run(["c++", "-std=c++20", "-I" + str(ROOT / "include"), str(probe), "-o", str(binary)], check=True)
    inputs = "".join(f"{c['pane']} {int(c['ranked'])} {c['party_players']} {c['game_players']} "
        f"{c['maximum'] if c['maximum'] is not None else -1}\n" for c in cases)
    actual = subprocess.check_output([str(binary)], input=inputs.encode()).decode().splitlines()
    assert len(actual) == len(cases)
    for case, line in zip(cases, actual):
        expected = [int(case["use_party"]), *({"player": 0, "matched": 1, "empty": 2}[r] for r in case["rows"])]
        assert list(map(int, line.split())) == expected, case
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    for address in source.executed:
        instruction = next(decoder.disasm(source.image.read(address, 15), address))
        assert bytes(machine.mem_read(address, instruction.size)) == instruction.bytes
    fixture = dict(schema="H2_MATCHMAKING_ROSTER_TU15_ORIGINAL_V1",
        original_tu15_sha256=NEW_PIN, routine="0x25780b", cases=cases)
    save(args.fixture, fixture)
    report = dict(schema="H2_MATCHMAKING_ROSTER_TU15_BLOCK_ORACLE_V1", passed=True,
        cases=len(cases), original_tu15_sha256=NEW_PIN, entry="0x25780b",
        original_executed_instructions=len(source.executed),
        executed_original_addresses=[hex(p) for p in sorted(source.executed)],
        native_header_sha256=sha(ROOT / "include/halo2/frontend_matchmaking_progress.hpp"),
        tool_sha256=sha(__file__), fixture_sha256=sha(args.fixture) if args.fixture else None,
        scope="Actual unchanged TU1.5 selection257835 before session getters; count-result consumers "
            "257862,2578F6,25790E with explicit synthetic getter outputs; datum assignment257977. "
            "No instruction patches, callee replacement or complete network/widget/session execution claim.",
        scratch=str(scratch), scratch_retained=False)
    save(args.report, report)
    print(json.dumps(report))


if __name__ == "__main__":
    main()
