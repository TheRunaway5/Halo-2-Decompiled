#!/usr/bin/env python3
"""Original deterministic/shared seed defaults and match-option application."""
import argparse
import hashlib
import json
from pathlib import Path

from oracle_smg_firing_1_1 import ROOT, PIN, Trace, SEED, GAME, UC_X86_REG_ESI

OPTIONS = 0x10E0000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/multiplayer/map-pack/multiplayer-seed-1.1-oracle.json")
    args = parser.parse_args()
    trace = Trace(5)
    trace.services.update({
        0x1656A0: ("supplied fixed arena registration", 0, 4),
        0x32803E: ("supplied system time", 0x11223344, 0),
        0x328505: ("supplied C-library rand output", 0x1234, 0),
        0x337A40: ("supplied system uptime milliseconds", 0x56789ABC, 0),
    })
    trace.w(0x4ED650, SEED)
    trace.w(0x4ED654, 0)
    trace.g.mem_write(0x4D6D58, b"\x00")
    trace.active = "random_global_initialize"
    trace.run(0x1479B0)
    initialized = {"deterministic_word": f"{trace.u(SEED):08x}",
                   "nondeterministic_word": f"{trace.u(SEED + 4):08x}"}
    assert trace.u(SEED) == 0x78A8
    assert trace.u(SEED + 4) == (0x11223344 ^ 0x1234 ^ 0x56789ABC)
    trace.active = "default_options"
    trace.run(0x139880, regs=((UC_X86_REG_ESI, OPTIONS),))
    defaults = {"options_hex": bytes(trace.g.mem_read(OPTIONS, 0x1118)).hex(),
                "deterministic_seed": f"{trace.u(OPTIONS + 0x10):08x}",
                "supplied_system_random_id_hex": bytes(trace.g.mem_read(OPTIONS + 8, 8)).hex(),
                "tick_rate": trace.u(OPTIONS + 6, 2)}
    assert trace.u(OPTIONS + 0x10) == 0x78A8
    cases = []
    for state in (1, 2):
        for seed in (0x78A8, 0, 1, 0x12345678, 0xFFFFFFFF):
            trace.w(OPTIONS, state)
            trace.w(OPTIONS + 0x10, seed)
            trace.w(SEED, 0xDEADBEEF)
            trace.active = f"apply_options_state{state}_{seed:08x}"
            event_start = len(trace.events)
            trace.run(0x139540, (OPTIONS,))
            cases.append({"options_state": state, "options_seed": f"{seed:08x}",
                          "game_globals_seed_field": f"{trace.u(GAME + 0x18):08x}",
                          "shared_seed": f"{trace.u(SEED):08x}",
                          "events": trace.events[event_start:]})
            assert trace.u(SEED) == seed and trace.u(GAME + 0x18) == seed
    report = {
        "xbe_sha256": PIN,
        "oracle_tool_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "initialization": initialized, "defaults": defaults, "cases": cases,
        "events": trace.events,
        "scope": "Unchanged original random-global initialization1479B0, options defaults139880 and options application139540. Original multiplayer variant normalization1A0EC0 executes. Host arena registration/system time/rand/uptime are explicitly supplied. Does not execute map/player/world startup or establish the first-shot seed after those consumers.",
    }
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"options_application_cases": len(cases), "source_default_seed": "000078a8"}))


if __name__ == "__main__":
    main()
