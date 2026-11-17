#!/usr/bin/env python3
"""Qualify the private shared 314870 public DATA facade against its exact predecessor."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess

from validate_native_solver_hull_clear_frame_tu15 import inputs

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/native_tu15_solver_hull_polytope_state.cpp'
VALUE = 'halo2/native_tu15_solver_hull_clear_value.hpp'
KERNEL = 'halo2/native_tu15_solver_hull_clear_kernel.hpp'
DRIVER = 'tests/native_tu15_solver_hull_clear_value_tests.cpp'
PRIOR = '92309b51337f7f37a89ee9d7dc0383698920601ee71e8624d03d8cdf27159887'
ENTRY = 'native_tu15_clear_solver_hull_polytope_flags'
SIBLING = 'native_tu15_clip_solver_hull_facet'
REFERENCE = 'tests/reference/native_tu15_solver_hull_clear_value_predecessor.inc'
REFERENCE_SHA = '85607f6c0663e621403ab3e4cb3a70379bf9da530c0d20ab23eb644f8ca34254'
BODY = re.compile(r'void native_tu15_clear_solver_hull_polytope_flags\(.*?\n\}', re.S)
INCLUDE = '#include "' + VALUE + '"\n'
SHARED = ('        native_solver_hull_clear_value_detail::View view(memory,polytope,observe,stats);\n'
          '        native_tu15_solver_hull_clear_kernel(view);\n')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def shared_body(text):
    first = text.index('        const auto word=')
    last = text.index('        if(stats)++stats->clear_completed;', first)
    removed = text[first:last]
    if text.count(removed) != 1 or text.index(ENTRY) >= first:
        raise ValueError('Complete 314870 public facade ownership drift')
    result = text[:first] + SHARED + text[last:]
    if result.replace(SHARED, removed, 1) != text:
        raise ValueError('Exact public hull clear reversal drift')
    return result


def public_overlay(text):
    if hashlib.sha256(text.encode()).hexdigest() != PRIOR or len(list(BODY.finditer(text)))!=1:
        raise ValueError('Complete public clear/clip predecessor drift')
    match=BODY.search(text)
    return INCLUDE+text[:match.start()]+shared_body(match[0])+text[match.end():]


def installed_reference(text, before):
    matches=list(BODY.finditer(text))
    if len(matches)!=1 or text.count(INCLUDE)!=1 or matches[0][0]!=shared_body(before):
        raise ValueError('Exact public hull-clear default not installed')
    match=matches[0]
    return (text[:match.start()]+before+text[match.end():]).replace(INCLUDE,'',1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--mutations', action='store_true')
    parser.add_argument('--require-binding', action='store_true')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686'):
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    paths = (SOURCE, 'include/' + VALUE, 'include/' + KERNEL, DRIVER, REFERENCE,
             'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',
             'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
             'tools/validate_native_solver_hull_clear_frame_tu15.py', str(Path(__file__).relative_to(ROOT)))
    pins = {path: sha(ROOT / path) for path in paths}
    report = dict(passed=False, production_bound=False, scratch_owner=str(scratch),
                  retained_large_artifacts=[], source_sha256=pins)
    try:
        text = (ROOT / SOURCE).read_text()
        if sha(ROOT/REFERENCE)!=REFERENCE_SHA:
            raise ValueError('Authenticated public hull-clear predecessor body drift')
        prior=(ROOT/REFERENCE).read_text().removesuffix('\n')
        if args.require_binding:
            before,candidate=installed_reference(text,prior),text
        else:
            if len(list(BODY.finditer(text)))!=1 or BODY.search(text)[0]!=prior:
                raise ValueError('Private public clear overlay did not find exact predecessor')
            before,candidate=text,public_overlay(text)
        if before.count(ENTRY) != 1 or before.count(SIBLING) != 1:
            raise ValueError('Complete public clear or clip symbol drift')
        original = scratch / 'legacy.cpp'
        original.write_text(before.replace(ENTRY, 'legacy_clear_solver_hull_polytope_flags', 1)
                            .replace(SIBLING, 'legacy_clip_solver_hull_facet', 1))
        native = scratch / 'candidate.cpp'
        native.write_text(candidate)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(inputs(), separators=(',', ':')))
        report.update(candidate_sha256=sha(native), input_sha256=sha(data), exact_reverse=True)
        command = [args.compiler, '-std=c++20', '-O2', '-Wno-ignored-attributes', '-fno-fast-math',
                   '-ffp-contract=off', '-frounding-math', '-I' + str(ROOT / 'include'),
                   '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'src/physics'), '-I' + str(ROOT / 'tests'),
                   str(original), str(native), str(ROOT / DRIVER), '-o', str(scratch / 'clear-value')]
        if args.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']

        def run(trial):
            build = subprocess.run(trial, capture_output=True, text=True, timeout=600)
            if build.returncode:
                raise ValueError('Private clear DATA compilation failed: ' + build.stderr[-8000:])
            with data.open('rb') as inp:
                return subprocess.run([trial[-1]], stdin=inp, capture_output=True, text=True, timeout=600)

        report['command'] = command
        result = run(command)
        if result.returncode:
            raise ValueError(result.stderr[-8000:])
        report.update(json.loads(result.stdout))
        if args.mutations:
            text = (ROOT / 'include' / VALUE).read_text()
            defects = (
                ('cached_count', 'const auto count=word(owner_+layout.count);', 'const auto count=3u;'),
                ('omitted_live_observer', 'if(observe_){event.written.assign', 'if(!event.before.empty()&&observe_){event.written.assign'),
                ('wrong_edge_stats', 'else ++stats_->edges_cleared;', 'else ++stats_->facets_cleared;'),
            )
            report['compiled_defects'] = {}
            for name, old, new in defects:
                if text.count(old) != 1:
                    raise ValueError('Exact public clear defect seam drift ' + name)
                overlay = scratch / name
                header = overlay / VALUE
                header.parent.mkdir(parents=True)
                header.write_text(text.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(overlay))
                trial[-1] = str(overlay / 'clear-value')
                mutant = run(trial)
                if not mutant.returncode or 'Complete public hull clear differs' not in mutant.stderr:
                    raise ValueError('Public clear defect missed its witness ' + name + ': ' + mutant.stderr[-5000:])
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
