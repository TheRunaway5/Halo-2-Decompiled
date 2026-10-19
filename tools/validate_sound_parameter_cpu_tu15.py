#!/usr/bin/env python3
"""Validate a compiled sound parameter owner against original CPU execution.

Keep the raw independent-emulator failures. Require unchanged original host
execution for every successful case and actual SIGSEGV observations for the
explicit absent-bank controls. No expected game results or missing banks are
supplied. Run under the test storage wrapper.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import tempfile
from original_xbox_pose_case import BASE, ROOT, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    specs = {
        'gain': ('smg-original-sound-range-entry-v1',97,94,3,353,353,'sound_gain',False,None),
        'random-scalar': ('smg-original-sound-secondary-entry-v1',35,33,2,33,32,'sound_random_scalar',False,0x127daa),
        'properties': ('smg-original-sound-properties-entry-v1',61,61,0,189,188,'sound_properties',True,None),
        'random-short': ('smg-original-sound-127e00-entry-v1',34,34,0,34,33,'sound_random_short',False,0x127e4b),
        'range-selection': ('smg-original-sound-21f960-entry-v1',68,68,0,106,106,'sound_range_selection',False,None),
        'permutation': ('smg-original-sound-21fa80-entry-v1',67,61,6,367,366,'sound_permutation',True,0x21fc5a),
        'binding': ('smg-original-sound-12b1c0-entry-v1',22,22,0,54,54,'sound_binding',True,None),
        'chunk-readiness': ('smg-original-sound-128970-entry-v1',114,101,0,187,180,'sound_chunk_readiness',True,None),
    }
    parser.add_argument('--owner', choices=tuple(specs), required=True)
    parser.add_argument('--binary', type=Path)
    parser.add_argument('--generated-source', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if platform.system() != 'Linux' or platform.machine() not in ('x86_64', 'i386', 'i686'):
        return 77
    gain = args.owner == 'gain'
    properties = args.owner == 'properties'
    short = args.owner == 'random-short'
    readiness = args.owner == 'chunk-readiness'
    prefix, fixtures, successes, faults, cfg, reached, stem, emulator_passed, unreachable = specs[args.owner]
    if not (BASE / prefix / 'report.json').exists():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute() if args.output else scratch / (stem + '-acceptance')
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch))) + '\n')
    python = str(Path(os.sys.executable).absolute())
    commands = []
    child_scratch_checks = []

    def run(name, command, expected):
        commands.append(command)
        with tempfile.TemporaryDirectory(prefix=name + '-', dir=scratch) as owned:
            environment = os.environ.copy()
            for key in ('HALO2_TEST_SCRATCH', 'TMPDIR', 'TMP', 'TEMP'):
                environment[key] = owned
            result = subprocess.run(command, capture_output=True, text=True, timeout=90, env=environment)
        assert not Path(owned).exists()
        child_scratch_checks.append(dict(path=owned, removed=True))
        (output / (name + '.log')).write_text((result.stdout + result.stderr)[-65536:])
        assert result.returncode == expected, (name, result.returncode, result.stderr[-3000:])

    command = [python, str(ROOT / ('tools/probe_' + stem + '_tu15.py')),
        '--output', str(output / 'independent-emulator')]
    if args.binary:
        command += ['--binary', str(args.binary.absolute())]
    if args.generated_source:
        command += ['--generated-source', str(args.generated_source.absolute())]
    run('emulator', command, 0 if emulator_passed else 1)
    candidate = json.loads((output / 'independent-emulator/report.json').read_text())
    assert candidate['passed'] == emulator_passed and candidate['fixtures'] == fixtures
    assert candidate['unique_original_instructions_executed'] == reached
    if readiness:
        assert candidate['missing_original_instructions'] == [0x21f2a8,0x21f2ac,0x21f2b0,0x21f305,0x21f308,0x21f30a,0x21f30c]
        assert candidate['allocation_and_scheduling_boundary_cases'] == 13
        assert not candidate['all_original_instructions_covered']
        assert not candidate['external_boundary_results_supplied']
    elif properties:
        assert candidate['missing_original_instructions'] == [0x229110]
    elif unreachable is not None:
        assert candidate['missing_original_instructions'] == [unreachable]
        assert candidate['proven_unreachable_instruction'] == unreachable
    else:
        assert not candidate['missing_original_instructions']
    for row in candidate['comparisons']:
        for key in ('returned', 'gp', 'xmm', 'trace', 'writes'):
            assert row['native'][key] == row['original'][key], (row['name'], key)
        if short:
            assert row['native']['x87_values'] == row['original']['x87_values']
    host_tools = [('original-host', 'compare_' + stem + '_host_tu15.py')]
    if faults:
        host_tools.append(('original-faults', 'observe_' + stem + '_fault_tu15.py'))
    for name, tool in host_tools:
        run(name, [python, str(ROOT / 'tools' / tool),
            '--candidate', str(output / 'independent-emulator'), '--output', str(output / name)], 0)
    host = json.loads((output / 'original-host/report.json').read_text())
    fault = json.loads((output / 'original-faults/report.json').read_text()) if faults else None
    assert host['passed'] and host['actual_host_original_cases'] == (fixtures if readiness else successes)
    if readiness:
        assert host['original_completed_cases'] == successes and host['original_external_boundary_cases'] == 13
        assert not host['external_allocation_or_scheduling_results_supplied']
    if faults:
        assert fault['passed'] and fault['actual_host_original_cases'] == faults
    for report in ([host, fault] if faults else [host]):
        assert report['candidate_native_source_sha256'] == candidate['native_generated_source_sha256']
        assert not report['original_instruction_patches'] and not report['native_expected_data_used']
    result = dict(passed=True, owner=args.owner, fixtures=fixtures,
        original_host_successful_cases=successes, original_host_actual_SIGSEGV_cases=faults,
        complete_original_cfg_instructions=cfg, original_instructions_exercised=reached,
        proven_unreachable_instruction=unreachable,
        unreachable_proof=candidate['unreachable_proof'] if unreachable is not None else None,
        live_x87_result_bits_verified=80 if short else None,
        unexecuted_stable_snapshot_count_guard=0x229110 if properties else None,
        original_host_external_boundary_cases=13 if readiness else 0,
        unexecuted_external_callee_continuations=candidate['missing_original_instructions'] if readiness else [],
        declared_readonly_constant_control_cases=2 if gain else 0,
        diagnostic_controls_are_not_captured_gameplay_or_production_banks=True,
        independent_emulator_passed=candidate['passed'],
        independent_emulator_discrepancies=sum(not row['equal'] for row in candidate['comparisons']),
        all_successful_cases_match_original_host_GP_XMM_defined_flags_raw_FP_state_traces_and_writes=True,
        all_fault_cases_match_original_host_last_retired_defined_flags_and_owned_state=True,
        original_instruction_patches_or_expected_gameplay_returns_supplied=False,
        native_generated_source_sha256=candidate['native_generated_source_sha256'],
        native_binary_sha256=candidate['native_binary_sha256'],
        production_source_regenerated_and_equal=bool(args.generated_source),
        evidence={name: sha(output / name) for name in ['independent-emulator/report.json',
            'original-host/report.json'] + (['original-faults/report.json'] if faults else [])},
        commands=commands, child_scratch_checks=child_scratch_checks,
        runner_scratch=str(scratch), large_artifacts_retained=[],
        physical_xbox_verified=False, full_weapon_parity=False, world_or_full_firing_verified=False)
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    shutil.copyfile(Path(__file__), output / Path(__file__).name)
    print(json.dumps(dict(passed=True, owner=args.owner, original_host_cases=successes,
        original_fault_cases=faults, instructions=reached, runner_scratch=str(scratch))), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
