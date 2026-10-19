#!/usr/bin/env python3
"""Execute complete original1046C0 magazine digits over every signed16 count.

Declared object/definition/magazine banks are inputs. No service, instruction
or callee result is supplied. This is an offline reference, not cache gameplay.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from oracle_weapon_heat_shader_tu15 import Original
from oracle_weapon_barrel_timing_tu15 import WEAPON, DEF, pack
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert args.fixture.resolve().is_relative_to(scratch)
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    original = Original('TU1.5', image)
    machine = original.g
    selectors = [0x1700056b, 0x1700056c]
    cycle = []
    digest = hashlib.sha256()
    cases = 0
    machine.mem_write(DEF + 0x2c0, pack([1]))
    for word in range(65536):
        loaded = struct.unpack('<h', struct.pack('<H', word))[0]
        machine.mem_write(WEAPON + 0x22c, struct.pack('<h', loaded))
        row = [original.call(0, 0, name) for name in selectors]
        if loaded in range(100):
            cycle.append(row)
        else:
            assert row == cycle[loaded % 100 if loaded >= 0 else 0], loaded
        for value, active in row:
            digest.update(struct.pack('<IB', value, active))
        cases += 2
        if word and word % 16384 == 0:
            print(json.dumps(dict(signed_words_completed=word, original_calls=cases)), flush=True)
    controls = []
    for count in [0, 2]:
        machine.mem_write(DEF + 0x2c0, pack([count]))
        for loaded in [-32768, -100, -1, 0, 1, 9, 10, 36, 99, 100, 32767]:
            machine.mem_write(WEAPON + 0x22c, struct.pack('<h', loaded))
            machine.mem_write(WEAPON + 0x23c, struct.pack('<h', 1234))
            row = [original.call(0, 0, name) for name in selectors]
            assert row == ([[0, 0], [0, 0]] if not count else cycle[loaded % 100 if loaded >= 0 else 0])
            controls.append(dict(magazines=count, loaded=loaded, secondary_loaded=1234, results=row))
            cases += 2
    fixture = dict(original_xbe_sha256=NEW_PIN, entry=original.entry,
        selectors=selectors, positive_period=100, cycle=cycle,
        all_negative_counts_equal_zero=True, signed_counts_verified=65536,
        exhaustive_results_sha256=digest.hexdigest(), controls=controls)
    args.fixture.write_text(json.dumps(fixture, separators=(',', ':')) + '\n')
    report = dict(passed=True, scope=__doc__, original_xbe_sha256=NEW_PIN,
        cases=cases, exhaustive_calls=131072, compressed_fixture_verified_for_all_counts=True,
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        exhaustive_results_sha256=digest.hexdigest(),
        complete_original_body=original.complete_body, executed_instructions=original.instructions,
        instruction_patches=[], supplied_services=[], supplied_callee_results=[],
        scratch=str(scratch), cleanup_pending_runner_exit=True, large_artifacts_retained=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, cases=cases, fixture_bytes=args.fixture.stat().st_size)), flush=True)


if __name__ == '__main__':
    main()
