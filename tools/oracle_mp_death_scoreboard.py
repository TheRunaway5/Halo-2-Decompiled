#!/usr/bin/env python3
"""Execute the unchanged TU1.5 automatic death scoreboard tail.

Player, clock, processed-input and lifecycle records are declared API inputs.
No death, spectator, respawn, world or local-session producer is inferred here.
The whole original tail and its exhaustion/exclusion callees execute.
"""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_mp_full_scoreboard_1_5 import (
    Trace, HudCache, ROOT, NEW, NEW_PIN, CACHE_PIN, C, R, M, G,
    UC_X86_REG_ESI, UC_X86_REG_EBX)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--fixtures', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW) == NEW_PIN
    cache_path = ROOT / 'assets/disc/maps/lockout.map'
    assert sha(cache_path) == CACHE_PIN
    cache = HudCache(cache_path)
    trace = Trace(cache)
    trace.fixture()
    trace.run('original_engine_table_constructor', 0x3825d0)
    trace.w(R + 0x24, 0)
    trace.w(0x4f2a9c, 1)
    trace.g.mem_write(0x4f2aac, bytes(0x48))
    trace.watch = [(0x4f63c0, 24), (R, 0x21c)]
    cases = [
        ('alive', 101, 0, 1, 0, -1, 0xe2700000, False),
        ('countdown_two', 101, 2, 1, 0, -1, -1, False),
        ('countdown_one', 101, 1, 1, 0, -1, -1, False),
        ('countdown_zero', 101, 0, 1, 0, -1, -1, False),
        ('negative_countdown', 101, -1, 1, 0, -1, -1, False),
        ('first_three_seconds', 90, 0, 1, 0, -1, -1, False),
        ('after_three_seconds', 91, 0, 1, 0, -1, -1, False),
        ('flag4000', 101, 0, 0x4001, 0, -1, -1, False),
        ('flag1_clear', 101, 0, 0, 0, -1, -1, False),
        ('exhausted', 101, 0, 1, 2, 0, -1, False),
        ('one_life', 101, 0, 1, 2, 1, -1, False),
        ('manual_during_countdown', 101, 150, 1, 0, -1, -1, True),
    ]
    rows, fixtures = [], []
    for name, tick, countdown, flags, limit, lives, unit, held in cases:
        trace.g.mem_write(0x4f63c0, bytes(24))
        trace.w(C + 8, tick)
        trace.w(R + 0x2c, unit)
        trace.w(R + 0x170, countdown)
        trace.h(R + 2, flags)
        trace.h(R + 0x1ac, lives)
        trace.w(R + 0x1b0, -1)  # no watched-player owner
        trace.w(G + 0x1b8, limit)
        trace.g.mem_write(0x4f2aac + 0x1d, bytes([held]))
        steps = []
        # Includes cap saturation and actual fade-out when ownership resumes.
        for index in range(20):
            if index == 10:
                trace.w(R + 0x2c, 0xe2700001)
                trace.g.mem_write(0x4f2aac + 0x1d, b'\x00')
            before = trace.u(0x4f63c0, 1)
            call = trace.run(name + '_' + str(index), 0x15e6bc,
                (0x80000000,), ((UC_X86_REG_ESI, R),
                (UC_X86_REG_EBX, 0xffffffff)), tail=True)
            assert call['completion'] == 'complete_original_return', call
            after = trace.u(0x4f63c0, 1)
            current = trace.u(R + 0x2c)
            requested = int(held and index < 10)
            fixtures.append(' '.join(map(str, (tick, countdown, flags, limit,
                lives, current, requested, before, after))))
            steps.append(dict(index=index, before=before, after=after, call=call))
        rows.append(dict(name=name, steps=steps))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(passed=True, boundary=__doc__,
        tool_sha256=sha(__file__), source_sha256=NEW_PIN, cache_sha256=CACHE_PIN,
        rows=rows, instruction_patches=[], supplied_gameplay_returns=[]), indent=2) + '\n')
    args.fixtures.parent.mkdir(parents=True, exist_ok=True)
    args.fixtures.write_text('\n'.join(fixtures) + '\n')
    args.fixtures.with_suffix('.provenance.json').write_text(json.dumps(dict(
        schema='original-death-scoreboard-v1', rows=len(fixtures),
        columns=['tick', 'countdown', 'flags', 'configured_lives', 'remaining_lives',
            'current_unit', 'held', 'counter_before', 'counter_after'],
        source_xbe_sha256=NEW_PIN, cache_sha256=CACHE_PIN,
        tool='tools/oracle_mp_death_scoreboard.py', tool_sha256=sha(__file__),
        fixture_sha256=sha(args.fixtures), boundary=__doc__), indent=2) + '\n')
    print(f'{len(fixtures)} original automatic death/respawn scoreboard calls returned')


if __name__ == '__main__':
    main()
