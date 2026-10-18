#!/usr/bin/env python3
"""Check the compiled listener owner against unchanged original instructions.

Retain the independent emulator's raw precision-flag discrepancies. Require
every successful case to match actual original host execution, including raw
x87 state, and keep the real absent-bank fault as a separate negative case.
This does not certify physical Xbox execution, device playback or a weapon.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
from original_xbox_pose_case import BASE, ROOT, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--generated-source', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if platform.system() != 'Linux' or platform.machine() not in ('x86_64', 'i386', 'i686'):
        return 77
    if not (BASE / 'smg-original-listener-entry-v1/report.json').exists():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute() if args.output else scratch / 'listener-acceptance'
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch))) + '\n')
    python = str(Path(os.sys.executable).absolute())
    command = [python, str(ROOT / 'tools/probe_sound_listener_selection_tu15.py'),
        '--output', str(output / 'independent-emulator')]
    if args.binary:
        command += ['--binary', str(args.binary.absolute())]
    if args.generated_source:
        command += ['--generated-source', str(args.generated_source.absolute())]
    probe = subprocess.run(command, capture_output=True, text=True, timeout=90)
    (output / 'emulator.log').write_text((probe.stdout + probe.stderr)[-65536:])
    candidate = json.loads((output / 'independent-emulator/report.json').read_text())
    assert probe.returncode == 1 and not candidate['passed'], 'Raw emulator disagreement must remain visible'
    assert candidate['all_312_original_instructions_covered'] and candidate['fixtures'] == 55
    discrepancies = []
    negative = []
    for row in candidate['comparisons']:
        native, original = row['native'], row['original']
        for field in ('returned', 'gp', 'xmm', 'trace', 'writes', 'mxcsr'):
            assert native[field] == original[field], (row['name'], field)
        mask = original['defined_flags_mask']
        assert (native['eflags'] ^ original['eflags']) & mask == 0
        assert (native['x87_control'] ^ original['x87_control']) & ~0x40 == 0
        assert native['x87_depth'] == 0 and original['x87_tag'] == 65535
        status_xor = native['x87_status'] ^ original['x87_status']
        assert status_xor in (0, 0x20), (row['name'], status_xor)
        if status_xor:
            assert native['x87_status'] & 0x20 and not original['x87_status'] & 0x20
            discrepancies.append(dict(name=row['name'], native=native['x87_status'],
                independent_emulator=original['x87_status']))
        if not original['returned']:
            assert row['equal'] and original['trace'][-1] == 0x253213
            negative.append(row['name'])
    assert len(discrepancies) == 9 and len(negative) == 1
    host_command = [python, str(ROOT / 'tools/compare_sound_listener_host_tu15.py'),
        '--candidate', str(output / 'independent-emulator'), '--output', str(output / 'original-host')]
    host_run = subprocess.run(host_command, capture_output=True, text=True, timeout=90)
    (output / 'host.log').write_text((host_run.stdout + host_run.stderr)[-65536:])
    assert host_run.returncode == 0, host_run.stderr[-3000:]
    host = json.loads((output / 'original-host/report.json').read_text())
    assert host['passed'] and host['actual_host_original_cases'] == 54 and host['all_312_instructions_executed']
    assert host['candidate_native_source_sha256'] == candidate['native_generated_source_sha256']
    result = dict(passed=True, original_host_cases=54, original_negative_fault_cases=len(negative),
        complete_original_bodies=5, complete_original_instructions=312,
        all_GP_XMM_defined_flags_raw_x87_controls_status_traces_and_ordered_writes_equal_to_original_host=True,
        independent_emulator_passed=False, independent_emulator_precision_discrepancies=discrepancies,
        independent_emulator_report_sha256=sha(output / 'independent-emulator/report.json'),
        original_host_report_sha256=sha(output / 'original-host/report.json'),
        native_generated_source_sha256=candidate['native_generated_source_sha256'],
        native_binary_sha256=candidate['native_binary_sha256'],
        commands=[command, host_command], runner_scratch=str(scratch), large_artifacts_retained=[],
        production_source_regenerated_and_equal=bool(args.generated_source),
        world_dispatch_or_full_firing_validated=False, full_weapon_parity=False, physical_xbox_verified=False)
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    shutil.copyfile(Path(__file__), output / Path(__file__).name)
    print(json.dumps(dict(passed=True, original_host_cases=54, instructions=312,
        independent_emulator_precision_discrepancies=len(discrepancies), runner_scratch=str(scratch))), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
