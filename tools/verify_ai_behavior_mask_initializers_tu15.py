#!/usr/bin/env python3
"""Compare all71 published AI behavior-mask constructors with Xbox code.

Each whole callback receives the authenticated source-world input bank and
declared caller ABI. Original code, independent host hardware and Release
native code must agree on the full retained bank, CPU exit, trace and stores.
This verifies these constructors, not the complete original CRT or AI firing.
"""
import argparse
import concurrent.futures
import json
import subprocess
from pathlib import Path

from audit_rendering import file_sha
from oracle_charged_effect_lifetime_bank_tu15 import stores

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case', type=Path, required=True)
    ap.add_argument('--generation', type=Path, required=True)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--workers', type=int, default=2)
    a = ap.parse_args()
    case, generation, build, out = [p.resolve() for p in
                                  (a.case, a.generation, a.build, a.output)]
    assert not out.exists() and 1 <= a.workers <= 4
    out.mkdir(parents=True)
    generated = json.loads((generation / 'generation.json').read_text())
    compiled = json.loads((build / 'build.json').read_text())
    assert compiled['passed'] and compiled['api'] == 'actor'
    assert compiled['module_sha256'] == generated['generated_cpp_sha256']
    assert file_sha(build / 'replay-native') == compiled['binary_sha256']
    initializers = generated['behavior_mask_initializers']
    assert len(initializers) == 71
    python = str(ROOT / '.tools/python/bin/python')

    def verify(row):
        destination = out / f"{row['entry']:08x}"
        destination.mkdir()
        original, native = destination / 'original', destination / 'native'
        commands = [[python, str(ROOT / 'tools/probe_original_charge_particle_tu15.py'),
                     '--case', str(case), '--output', str(original),
                     '--entry', hex(row['entry']), '--phase', 'before'],
                    [python, str(ROOT / 'tools/verify_original_particle_hardware_tu15.py'),
                     '--case', str(original)],
                    [str(build / 'replay-native'), str(original), hex(row['entry']), str(native)]]
        for i, command in enumerate(commands):
            with (destination / f'command{i}.log').open('w') as log:
                subprocess.run(command, cwd=ROOT, stdout=log,
                               stderr=subprocess.STDOUT, check=True, timeout=120)
        r = json.loads((original / 'report.json').read_text())
        h = json.loads((original / 'hardware/report.json').read_text())
        assert r['completed'] and r['original_instructions'] == 3 and r['retained_stores'] == 1
        assert not r['instruction_patches'] and not r['supplied_game_services']
        assert h['emulator_retained_state_matches_hardware'] and h['retained_bytes_compared'] == 58970198
        writes = list(stores(original / 'original-stores.bin'))
        assert len(writes) == 1 and writes[0][1] == row['destination']
        assert int.from_bytes(writes[0][3], 'little') == row['authored_mask']
        result = dict(**row, original_report_sha256=file_sha(original / 'report.json'),
                      hardware_report_sha256=file_sha(original / 'hardware/report.json'),
                      native_after_sha256=file_sha(native / 'native-after.bin'),
                      native_exit_sha256=file_sha(native / 'native-exit.bin'),
                      prior_mask=int.from_bytes(writes[0][2], 'little'),
                      commands=commands, passed=True)
        (destination / 'comparison.json').write_text(json.dumps(result, indent=2) + '\n')
        return result

    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=a.workers) as pool:
        for future in concurrent.futures.as_completed([pool.submit(verify, row) for row in initializers]):
            results.append(future.result())
            if len(results) % 10 == 0:
                print('Verified behavior initializers', len(results), 'of71', flush=True)
    result = dict(scope=__doc__, tool_sha256=file_sha(Path(__file__)),
                  generation_sha256=file_sha(generation / 'generation.json'),
                  build_sha256=file_sha(build / 'build.json'),
                  origin_build_sha256=file_sha(case / 'build.json'),
                  cases=sorted(results, key=lambda row: row['entry']),
                  instructions=213, writes=71, full_weapon_parity=False,
                  physical_xbox_verified=False, complete_crt_verified=False,
                  passed=len(results) == 71)
    (out / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(passed=result['passed'], constructors=len(results))))


if __name__ == '__main__':
    main()
