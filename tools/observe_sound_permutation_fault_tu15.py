#!/usr/bin/env python3
"""Observe real unmapped-read faults in unchanged sound permutation bytes.

This separates actual host x87 behavior from independent-emulator behavior.
It supplies only retained entry/input banks and declared fixture bytes. Linux
segment/stop routing remains harness policy; this is not physical Xbox proof.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
from original_xbox_pose_case import BASE, ROOT, reconstructed_replay_ram, sha
from oracle_sandbox_first_person_hardware_tu15 import execute
from probe_sound_random_scalar_tu15 import defined_flag_mask


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute()
    output.mkdir(exist_ok=False)
    (output / 'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch), argv=os.sys.argv)) + '\n')
    report = json.loads((args.candidate / 'report.json').read_text())
    declared = json.loads((args.candidate / 'declared-inputs.json').read_text())
    assert report['unique_original_instructions_executed'] == 366 and report['fixtures'] == 67
    assert report['missing_original_instructions']==[0x21fc5a]
    assert len(declared['fixtures']) == len(report['comparisons'])
    case = BASE / 'smg-original-fire-v1'
    prefix = BASE / 'smg-original-sound-21fa80-entry-v1'
    assert report['original_prefix_sha256'] == sha(prefix / 'report.json')
    paired = json.loads((args.candidate / 'permutation-bodies.json').read_text())
    # Attempt the declared unmapped read on the real CPU and capture SIGSEGV.
    # The harness records a fault boundary, never a successful guest return.
    positive = [(i, fixture, row) for i, (fixture, row) in enumerate(zip(declared['fixtures'], report['comparisons']))
                if not row['original']['returned']]
    assert len(positive) == 6
    host_cases = []
    with reconstructed_replay_ram(case, prefix) as (bank, memory, original):
        entry = bank[:]
        for index, fixture, row in positive:
            bank[:] = entry
            for patch in fixture['declared_input_bytes']:
                raw = bytes.fromhex(patch['hex'])
                at = memory.translate(patch['address'])[0]
                assert (patch['address'] & 4095) + len(raw) <= 4096
                bank[at:at+len(raw)] = raw
            pages = row['original']['mapped_pages']
            aliases = {}
            for page in pages:
                aliases.setdefault(memory.translate(page)[0], []).append(page)
            written = {memory.translate(store['address'])[0] & ~4095 for store in row['original']['writes']}
            assert not any(len(values) > 1 and at in written for at, values in aliases.items()), 'Mutable aliases need shared host mappings'
            ranges = []
            for page in sorted(pages):
                if ranges and page == ranges[-1][1]:
                    ranges[-1][1] += 4096
                else:
                    ranges.append([page, page+4096])
            banks = [dict(address=left, hex=memory.read(left, right-left).hex()) for left, right in ranges]
            cpu = fixture['cpu']
            assert cpu['x87_tag'] == 65535
            abi = dict(gp=cpu['gp'], eflags=cpu['eflags'], mxcsr=cpu['mxcsr'], fs_base=cpu['fs_base'],
                x87_control_word=cpu['x87_control'], x87_status=cpu['x87_status'], x87_tag=cpu['x87_tag'],
                x87_values=cpu['x87_register_bytes'],
                xmm=[b''.join(word.to_bytes(4, 'little') for word in words)[::-1].hex() for words in cpu['xmm']])
            host_cases.append(dict(id=index+1, entry=fixture['entry'], entry_abi=abi,
                return_address=0x128ddc, exit_entry=row['original']['trace'][-1], exit={},
                unmapped_read_fault=dict(instruction=row['original']['trace'][-1],address=4,width=4),
                original_instructions=row['original']['trace'], initial_banks=banks))
    actual = execute(dict(image='1.5', cases=host_cases), paired, scratch / 'host')
    assert not actual['original_instruction_patches'] and not actual['native_expected_data_used']
    observations = []
    comparisons = []
    covered = set()
    for result, (index, fixture, row) in zip(actual['cases'], positive):
        assert result['id'] == index+1
        host = result['exit']
        native = row['native']
        xmm = [[int.from_bytes(bytes.fromhex(value)[::-1][i*4:i*4+4], 'little') for i in range(4)] for value in host['xmm']]
        errors = []
        for key, value in (('gp', host['gp']), ('xmm', xmm), ('x87_status', host['x87_status']),
                           ('x87_control', host['x87_control_word']), ('mxcsr', host['mxcsr'])):
            if native[key] != value:
                errors.append(dict(kind=key, native=native[key], original_host=value))
        mask = defined_flag_mask(row['original']['trace'][:-1])
        if native['eflags'] & mask != host['eflags'] & mask:
            errors.append(dict(kind='defined_flags', mask=mask, native=native['eflags'], original_host=host['eflags']))
        if native['trace'] != result['instructions']:
            errors.append(dict(kind='instruction_trace'))
        if native['writes'] != result['writes']:
            errors.append(dict(kind='ordered_source_writes'))
        if host['x87_tag'] != 65535 or native['x87_depth'] != 0:
            errors.append(dict(kind='x87_stack_not_empty'))
        comparisons.append(dict(name=fixture['name'], equal=not errors, errors=errors,
            native_x87_status=native['x87_status'], original_host_x87_status=host['x87_status'],
            raw_native_flags=native['eflags'], raw_original_host_flags=host['eflags'],
            raw_independent_emulator_flags=row['original']['eflags'],
            observed_read_fault=result['observed_read_fault'],
            independent_emulator_x87_status=row['original']['x87_status'], defined_flags_mask=mask))
        covered.update(result['instructions'])
        observations.append(dict(id=result['id'], name=fixture['name'], exit=host,
            instructions=result['instructions'], writes=result['writes'], estimates=result['estimates']))
    expected_coverage = {pc for _, _, row in positive for pc in row['original']['trace']}
    assert expected_coverage
    result = dict(passed=all(row['equal'] for row in comparisons) and covered == expected_coverage,
        actual_host_original_cases=len(comparisons), unique_original_instructions_executed=len(covered),
        all_reached_instructions_executed=covered == expected_coverage, comparisons=comparisons,
        one_proven_unreachable_original_instruction=0x21fc5a,
        fault_cases_use_no_readonly_constant_controls=True,
        original_negative_cases_verified_by_actual_SIGSEGV=6, original_fault_has_no_supplied_return=True,
        original_instruction_patches=[], expected_gameplay_returns_or_native_output_bytes_supplied=False,
        host_segment_and_stop_routing_is_harness_policy=True,
        source_case_sha256=sha(case / 'call.json'), source_prefix_sha256=sha(prefix / 'report.json'),
        candidate_report_sha256=sha(args.candidate / 'report.json'),
        candidate_native_source_sha256=report['native_generated_source_sha256'],
        actual_original_hardware_binary_sha256=actual['original_hardware_binary_sha256'],
        native_expected_data_used=False, physical_xbox_verified=False, full_weapon_parity=False,
        production_integration=False, runner_scratch=str(scratch), large_artifacts_retained=[])
    sources = [Path(__file__), ROOT / 'tools/oracle_sandbox_first_person_hardware_tu15.py',
        ROOT / 'tools/sandbox_first_person_hardware_trace_tu15.cpp',
        ROOT / 'tools/original_i386_game_frame_bridge_20261011.py']
    result['source_files_sha256'] = {str(path): sha(path) for path in sources}
    for path in sources:
        shutil.copyfile(path, output / path.name)
    (output / 'original-host-observations.json').write_text(json.dumps(observations, separators=(',', ':')) + '\n')
    (output / 'report.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(passed=result['passed'], original_host_cases=len(comparisons), instructions=len(covered),
        errors=[row for row in comparisons if not row['equal']], runner_scratch=str(scratch))), flush=True)
    assert result['passed']


if __name__ == '__main__':
    main()
