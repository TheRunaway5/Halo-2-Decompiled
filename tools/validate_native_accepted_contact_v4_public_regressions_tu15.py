#!/usr/bin/env python3
"""Unchanged physical lookup/body-update validators on an owned V4 source overlay."""
import argparse
import hashlib
import json
import os
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
STAGE = ROOT / 'analysis/physics/native-accepted-body-contact-semantic-stage-v4-20261103/staged'
LOOKUP = (
    'src/core/native_tu15_data_array_allocate.cpp',
    *('src/physics/native_tu15_' + name + '.cpp' for name in (
        'body_constraint_update', 'geometry', 'model_constraint_lookup',
        'model_constraint_capacity', 'constraint_body_slots', 'constraint_motion_compare',
        'constraint_scalar_cache', 'constraint_reference_release')),
)
BODY = (
    *('src/physics/native_tu15_' + name + '.cpp' for name in (
        'agent_cleanup_children', 'agent_cleanup', 'array_allocator', 'array_growth',
        'contact_body_update', 'activation_services', 'island_activation')),
    'src/physics/native_tu15_contact_unwind_internal.hpp',
)
COMMON_TOOLS = (
    'tools/run_test.py', 'tools/run_tests.py', 'tools/validate_native_accepted_contact_v4_public_regressions_tu15.py',
    'tools/research_mp_sandbox_update_functions.py', 'tools/oracle_smg_firing_1_1.py',
)
LOOKUP_TOOLS = (
    'tools/validate_native_body_constraint_update_tu15.py', 'tools/observe_native_body_constraint_leaves_tu15.cpp',
    'tools/observe_native_activation_agents_tu15.cpp', 'tools/build_native_tu15_before_reference.py',
)
BODY_TOOLS = (
    'tools/validate_native_contact_body_update_tu15.py', 'tools/validate_native_agent_cleanup_children_tu15.py',
    'tools/validate_native_array_growth_tu15.py', 'tools/observe_native_array_growth_tu15.cpp',
    'tests/native_tu15_array_growth_tests.cpp',
)
DEFAULT_LOOKUP_CASES = r'(actual-warthog_[0-4]|gemini-column[01]_(0|256)|elongation-crates_[0-2])_[0-3]'


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('mode', choices=('lookup', 'body'))
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--lookup-case', default=DEFAULT_LOOKUP_CASES)
    args = p.parse_args()
    output = args.output.resolve()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    overlay = scratch / 'cohort'
    overlay.mkdir()
    report = dict(passed=False, mode=args.mode, source_overlay_only=True, unchanged_harness_logic=True,
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    try:
        for name in ('tools', 'tests', 'analysis'):
            (overlay / name).symlink_to(ROOT / name, target_is_directory=True)
        shutil.copytree(ROOT / 'include', overlay / 'include')
        unit_paths = LOOKUP if args.mode == 'lookup' else BODY
        copied = {rel: STAGE / rel if (STAGE / rel).is_file() else ROOT / rel for rel in unit_paths}
        for source in (STAGE / 'include').rglob('*.hpp'):
            copied[str(source.relative_to(STAGE))] = source
        pins = {}
        for rel, source in copied.items():
            target = overlay / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, target)
            pins[rel] = dict(path=str(source.relative_to(ROOT)), sha256=sha(target))
        report['source_overlay'] = pins
        tools = (*COMMON_TOOLS, *(LOOKUP_TOOLS if args.mode == 'lookup' else BODY_TOOLS))
        report['tools_sha256'] = {rel: sha(ROOT / rel) for rel in tools}
        report['compiled_header_sha256'] = {str(path.relative_to(overlay)): sha(path) for path in (overlay / 'include').rglob('*') if path.is_file()}
        sys.argv = [sys.argv[0], '--output', str(output)]
        if args.mode == 'lookup':
            import validate_native_body_constraint_update_tu15 as harness
            harness.ROOT = overlay
            harness.__file__ = str(overlay / 'tools/validate_native_body_constraint_update_tu15.py')
            sys.argv.extend(('--leaf', 'lookup', '--case', args.lookup_case))
        else:
            import validate_native_contact_body_update_tu15 as harness
            harness.child.ROOT = overlay
            harness.child.base.ROOT = overlay
        report['harness_args'] = list(sys.argv)
        code = harness.main()
        report['harness_exit_code'] = code
        assert code == 0, 'Unchanged public physical harness rejected its V4 overlay'
        assert all(sha(ROOT / row['path']) == row['sha256'] and sha(overlay / rel) == row['sha256']
                   for rel, row in pins.items())
        assert all(sha(ROOT / rel) == h for rel, h in report['tools_sha256'].items())
        assert all(sha(overlay / rel) == h for rel, h in report['compiled_header_sha256'].items())
        report['physical_report_sha256'] = sha(output / 'report.json')
        report['passed'] = True
        return 0
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        output.mkdir(parents=True, exist_ok=True)
        report['temporary_allocated_bytes_before_cleanup'] = sum(
            path.stat().st_blocks * 512 for path in scratch.rglob('*') if path.is_file() and not path.is_symlink())
        (output / 'overlay-receipt.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    raise SystemExit(main())
