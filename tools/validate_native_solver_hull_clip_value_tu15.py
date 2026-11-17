#!/usr/bin/env python3
"""Qualify the private shared 3147D0 public DATA facade against its exact predecessor."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess

from research_mp_sandbox_update_functions import Xbe
from validate_native_solver_hull_facet_clip_tu15 import cases
from validate_native_tu15_prt_helpers import PIN
from validate_native_solver_hull_clear_value_tu15 import REFERENCE as CLEAR_REFERENCE, REFERENCE_SHA as CLEAR_REFERENCE_SHA, installed_reference as remove_clear

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/native_tu15_solver_hull_polytope_state.cpp'
VALUE = 'native_tu15_solver_hull_clip_value.hpp'
KERNEL = 'halo2/native_tu15_solver_hull_clip_kernel.hpp'
DRIVER = 'tests/native_tu15_solver_hull_clip_value_tests.cpp'
PRIOR = '92309b51337f7f37a89ee9d7dc0383698920601ee71e8624d03d8cdf27159887'
ENTRY = 'native_tu15_clip_solver_hull_facet'
SIBLING = 'native_tu15_clear_solver_hull_polytope_flags'
INCLUDE = '#include "' + VALUE + '"\n'
SHARED = ('        native_solver_hull_clip_value_detail::View view(memory,facet,point,lower,upper,fp,depth,observe);\n'
          '        const auto result=native_tu15_solver_hull_clip_kernel(view);\n')
REFERENCE = 'tests/reference/native_tu15_solver_hull_clip_value_predecessor.inc'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_prior(text):
    if hashlib.sha256(text.encode()).hexdigest() != PRIOR:
        if sha(ROOT/CLEAR_REFERENCE)!=CLEAR_REFERENCE_SHA:
            raise ValueError('Complete public table-clear predecessor body drift')
        clear_prior=(ROOT/CLEAR_REFERENCE).read_text().removesuffix('\n')
        if hashlib.sha256(remove_clear(text,clear_prior).encode()).hexdigest()!=PRIOR:
            raise ValueError('Complete public clear/clip predecessor drift')


def prior_payload(text):
    first = text.index('        Environment phase(fp,depth);Memory m(memory,observe);')
    last = text.index('        if(stats)++stats->clip_completed;return result;', first)
    removed = text[first:last]
    if text.count(removed) != 1 or text.index(ENTRY) >= first or text.index(SIBLING) >= text.index(ENTRY):
        raise ValueError('Complete 3147D0 public facade ownership drift')
    return removed


def public_overlay(text):
    verify_prior(text)
    removed=prior_payload(text)
    result = INCLUDE + text.replace(removed, SHARED, 1)
    if installed_reference(result, removed) != text:
        raise ValueError('Exact public hull clip reversal drift')
    return result


def installed_reference(text, removed):
    if not text.startswith(INCLUDE) or text.count(INCLUDE)!=1 or text.count(SHARED)!=1:
        raise ValueError('Exact public hull-clip default not installed')
    before=text.removeprefix(INCLUDE).replace(SHARED,removed,1)
    verify_prior(before)
    if prior_payload(before)!=removed:
        raise ValueError('Authenticated public hull-clip predecessor body drift')
    return before


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--mutations', action='store_true')
    parser.add_argument('--require-binding', action='store_true')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686') or not args.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    paths = (SOURCE, 'src/physics/' + VALUE, 'include/' + KERNEL, DRIVER,
             'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',
             'tests/native_tu15_mopp_ray_frame_test_memory.hpp', 'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
             'tools/validate_native_solver_hull_facet_clip_tu15.py',
             'tools/validate_native_solver_hull_clear_value_tu15.py', CLEAR_REFERENCE, str(Path(__file__).relative_to(ROOT)))
    if args.require_binding:
        paths += (REFERENCE,)
    pins = {path: sha(ROOT / path) for path in paths}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch),
                  retained_large_artifacts=[], source_sha256=pins)
    try:
        if sha(args.xbe) != PIN:
            raise ValueError('Wrong original TU1.5 image')
        text = (ROOT / SOURCE).read_text()
        if args.require_binding:
            before,candidate=installed_reference(text,(ROOT/REFERENCE).read_text()),text
            if public_overlay(before)!=text:
                raise ValueError('Exact public hull-clip installed overlay drift')
        else:
            before,candidate=text,public_overlay(text)
        if before.count(ENTRY) != 1 or before.count(SIBLING) != 1:
            raise ValueError('Complete public clear or clip symbol drift')
        original = scratch / 'legacy.cpp'
        original.write_text(before.replace(ENTRY, 'legacy_clip_solver_hull_facet', 1)
                            .replace(SIBLING, 'legacy_clear_solver_hull_polytope_flags', 1))
        native = scratch / 'candidate.cpp'
        native.write_text(candidate)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(cases(Xbe(args.xbe)), separators=(',', ':')))
        report.update(candidate_sha256=sha(native), input_sha256=sha(data), original_sha256=PIN, exact_reverse=True)
        command = [args.compiler, '-std=c++20', '-O2', '-Wno-ignored-attributes', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'),
                   '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'src/physics'), '-I' + str(ROOT / 'tests'),
                   str(original), str(native), str(ROOT / DRIVER), '-o', str(scratch / 'clip-value')]
        if args.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']

        def run(trial):
            build = subprocess.run(trial, capture_output=True, text=True, timeout=600)
            if build.returncode:
                raise ValueError('Private clip DATA compilation failed: ' + build.stderr[-8000:])
            with data.open('rb') as inp:
                return subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)

        report['command'] = command
        result = run(command)
        if result.returncode:
            raise ValueError(result.stderr[-8000:])
        report.update(json.loads(result.stdout))
        if args.mutations:
            text = (ROOT / 'src/physics' / VALUE).read_text()
            defects = (
                ('wrong_dot', 'sum_three(multiply(normal,difference))', 'scalar_bits(multiply(normal,difference))'),
                ('inverted_upper_test', '&1u)!=0;', '&1u)==0;'),
                ('omitted_lower_epsilon', 'add_scalar(memory_.word(0x41d7a0u));', '(void)memory_.word(0x41d7a0u);'),
            )
            report['compiled_defects'] = {}
            for name, old, new in defects:
                if text.count(old) != 1:
                    raise ValueError('Exact public clip defect seam drift ' + name)
                overlay = scratch / name
                header = overlay / VALUE
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(overlay))
                trial[-1] = str(overlay / 'clip-value')
                mutant = run(trial)
                if not mutant.returncode or 'Complete public hull clip differs' not in mutant.stderr:
                    raise ValueError('Public clip defect missed its witness ' + name + ': ' + mutant.stderr[-5000:])
                report['compiled_defects'][name] = mutant.stderr.strip()[-2000:]
        report.update(passed=True, production_bound=args.require_binding,
                      scope='Exact public DATA predecessor, provider/observer events and stats.')
    except Exception as error:
        report['error'] = repr(error)
    report['sources_unchanged'] = all(sha(ROOT / name) == value for name, value in pins.items())
    report['passed'] &= report['sources_unchanged']
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: value for key, value in report.items() if key in ('passed', 'error')}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
