#!/usr/bin/env python3
"""Analyze live detached weapon identities and original retirement requests.

Run through run_test.py. An unretired observation window is reported as such;
no cleanup timer, source return, GPU result or framebuffer equality is assumed.
"""
import argparse
import json
import os
from pathlib import Path
from original_xbox_pose_case import sha
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def case(path, image):
    trace = json.loads(path.read_text())
    lifetime = trace.get('drop_lifetime')
    rows = trace.get('drop_samples', [])
    retirements = trace.get('source_retirement_requests', [])
    calls = []
    for request in retirements:
        caller = request['caller_return'] - 5
        raw = image.read(caller, 5)
        destination = ((caller + 5 + int.from_bytes(raw[1:], 'little', signed=True)) & 0xffffffff
                       if raw[0] == 0xe8 else None)
        calls.append(dict(**request, possible_direct_call_pc=f'{caller:08x}',
                          preceding_original_bytes=raw.hex(), direct_call_destination=destination))
    checks = bool(lifetime) and lifetime['publication_checks'] > 0 and lifetime['publication_mismatches'] == 0
    observed_retirement = bool(lifetime and lifetime['source_retirement_observed'])
    owner_returned = bool(calls) and all(row['completed'] for row in calls)
    if observed_retirement:
        checks = checks and owner_returned
    return dict(path=str(path), sha256=sha(path), representation=trace['representation'],
        source_unit_definition=trace['expected_unit_definition'], render_loop_completed=trace['render_loop_completed'],
        lifetime=lifetime, original_retirement_requests=calls,
        original_retirement_return_observed=owner_returned,
        retirement_not_observed_within_window=bool(lifetime and not observed_retirement),
        live_source_samples=len(rows), render_loop_samples=sum(row['render_loop_sample'] for row in rows),
        headless_samples=sum(not row['render_loop_sample'] for row in rows),
        all_sampled_publications_match=bool(rows) and all(row['publication_matches'] for row in rows),
        completed_source_preview_node_differences=sum(not row['completed_source_and_preview_nodes_equal'] for row in rows),
        source_node_counts=sorted({row['source_nodes'] for row in rows}),
        preview_node_counts=sorted({row['preview_nodes'] for row in rows}),
        retained_source_node_banks=sum('source_world_node_words' in row for row in rows),
        observations=trace.get('observations', {}), bounded_lifetime_checks_passed=checks,
        completed_pose_comparison_is_not_gpu_or_framebuffer_comparison=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    cases = [case(path, image) for path in args.input]
    report = dict(passed=all(row['bounded_lifetime_checks_passed'] for row in cases),
        component='Detached source weapon identity, ownership, world publication and bounded lifetime',
        original_xbe_sha256=NEW_PIN, retirement_entry='BA490', original_retirement_entry_bytes=image.read(0xba490, 16).hex(),
        cases=cases, original_instruction_patches=[], supplied_callee_results=[],
        full_weapon_parity=False, fully_certified_weapons=0, framebuffer_parity_verified=False,
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'], tool_sha256=sha(Path(__file__)), large_artifacts_retained=[])
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], cases=len(cases),
        publication_checks=sum((row['lifetime'] or {}).get('publication_checks', 0) for row in cases),
        original_retirements=sum(len(row['original_retirement_requests']) for row in cases),
        sampled_source_preview_differences=sum(row['completed_source_preview_node_differences'] for row in cases))), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
