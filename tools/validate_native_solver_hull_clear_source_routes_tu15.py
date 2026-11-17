#!/usr/bin/env python3
"""Exercise exact current and genuine fresh 314870 source routes in owned scratch."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys

import native_solver_hull_clear_replacements as binding
from specialize_native_operands import specialize
from validate_native_solver_hull_clear_frame_tu15 import inputs

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
GENERATOR = 'tools/generate_physics_live_support_tu15.py'
SPEC = 'tools/specialize_native_operands.py'
OWNER = 'physics/runtime/native_tu15_solver_hull_clear_source_owner.hpp'
TEST = 'tests/native_tu15_solver_hull_clear_source_routes_tests.cpp'
ANCHOR = "    receipt['native_solver_hull_minimum_entries']=solver_hull_minimum_entries\n"
BRANCH = '''    solver_hull_clear_entries=[]
    if any(marker in output for marker in ('#include "physics/runtime/native_tu15_solver_hull_clear_source_owner.hpp"','native_solver_hull_clear_owner_detail','native_hull_clear_resume_')):
        clear_spec=importlib.util.spec_from_file_location('native_solver_hull_clear_binding',Path(__file__).with_name('native_solver_hull_clear_replacements.py'))
        clear_helper=importlib.util.module_from_spec(clear_spec);clear_spec.loader.exec_module(clear_helper)
        clear_helper.installed_entries(text)
        output=clear_helper.rebind_prepared(output)
        solver_hull_clear_entries=clear_helper.installed_entries(output)
    receipt['native_solver_hull_clear_entries']=solver_hull_clear_entries
'''


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_pins():
    files = {PRIMARY, GENERATOR, SPEC, 'src/' + OWNER, TEST, str(Path(__file__).relative_to(ROOT))}
    pending = list(files)
    while pending:
        path = ROOT / pending.pop()
        body = path.read_text()
        found = set()
        for include in re.findall(r'^#include "([^"\n]+)"', body, re.M):
            match = next((p for p in (path.parent / include, ROOT / 'include' / include,
                                    ROOT / 'src' / include, ROOT / 'tests' / include) if p.is_file()), None)
            if match:
                found.add(str(match.relative_to(ROOT)))
        if path.suffix == '.py':
            modules = set(re.findall(r"['\"]([A-Za-z_]\w+)(?:\.py)?['\"]", body))
            modules.update(re.findall(r'^(?:from|import) ([A-Za-z_]\w+)', body, re.M))
            found.update('tools/' + name + '.py' for name in modules if (ROOT / 'tools' / (name + '.py')).is_file())
        pending.extend(found - files)
        files.update(found)
    return {name: sha(ROOT / name) for name in sorted(files)}


def proposed_specializer(scratch):
    current = (ROOT / SPEC).read_text()
    installed = "receipt['native_solver_hull_clear_entries']" in current
    if installed:
        return specialize, digest_specializer(current), True
    if current.count(ANCHOR) != 1:
        raise ValueError('Complete private hull-clear specializer anchor drift')
    candidate = scratch / 'proposed-specializer.py'
    candidate.write_text(current.replace(ANCHOR, ANCHOR + BRANCH, 1))
    namespace = {'__name__': '_private_hull_clear_specializer', '__file__': str(ROOT / SPEC)}
    exec(compile(candidate.read_bytes(), str(candidate), 'exec'), namespace)
    return namespace['specialize'], sha(candidate), False


def digest_specializer(source):
    return hashlib.sha256(source.encode()).hexdigest()


def refusal(action, source):
    try:
        action(source)
    except ValueError as error:
        return str(error)
    raise ValueError('Actual hull-clear source API accepted modified context')


def controls(source, prepared, prepare):
    match, resolver, dispatcher, state = binding.context(source)
    first, following = tuple(binding.LABELS)[:2]
    label = binding.LABELS[first] + ': '
    needle = binding.instruction(first, prepared)
    next_site = binding.instruction(following, prepared)
    relocated = match[0].replace(label + needle, needle, 1).replace(next_site, label + next_site, 1)
    variants = {
        'wrong-format': source.replace(binding.PREFIXES[prepared], binding.PREFIXES[not prepared], 1),
        'mixed-instruction': source.replace(match[0], match[0].replace(needle, binding.instruction(first, not prepared), 1), 1),
        'relocated-label': source.replace(match[0], relocated, 1),
        'changed-LiveState': source.replace(state[0], state[0].replace('\n};\nusing WorldState=LiveState;',
                                          '\n    int changed_clear_owner{};\n};\nusing WorldState=LiveState;', 1), 1),
        'changed-resolver': source.replace(resolver[0], resolver[0].replace('{', '{ ', 1), 1),
        'changed-dispatcher': source.replace(dispatcher[0], dispatcher[0].replace('{', '{ ', 1), 1),
        'duplicate-include': source.replace(binding.INCLUDE, binding.INCLUDE * 2, 1),
        'foreign-owner': source + '\nstatic void foreign_clear(auto&s){halo2::native_solver_hull_clear_owner_detail::bridge<false>(s);}\n',
        'foreign-label': source + '\nstatic void foreign_clear_label(){' + binding.LABELS[first] + ':return;}\n',
    }
    result = {}
    for name, bad in variants.items():
        if bad == source:
            raise ValueError('Hull-clear source mutation seam missing ' + name)
        result[name] = {'installed': refusal(binding.installed_entries, bad), 'specialize': refusal(prepare, bad)}
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', required=True, type=Path)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--sanitizers', action='store_true')
    p.add_argument('--mutations', action='store_true')
    p.add_argument('--whole-syntax', action='store_true')
    p.add_argument('--require-binding', action='store_true')
    args = p.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        p.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686'):
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    pins = source_pins()
    report = dict(passed=False, production_bound=False, source_sha256=pins,
                  scratch_owner=str(scratch), retained_large_artifacts=[])
    try:
        fresh = scratch / 'fresh.cpp'
        generator = [sys.executable, '-B', str(ROOT / GENERATOR), '--output', str(fresh),
                     '--manifest', str(scratch / 'fresh-manifest.json')]
        report['generator'] = generator
        generated = subprocess.run(generator, cwd=ROOT, capture_output=True, text=True, timeout=600)
        if generated.returncode:
            raise ValueError('Genuine Primary generation failed: ' + generated.stderr[-8000:])
        prepare, report['proposed_specializer_sha256'], installed = proposed_specializer(scratch)
        if installed!=args.require_binding:
            raise ValueError('Source route registration did not select its declared private/installed owner mode')
        report['contexts'] = {}
        report['exact_API_controls'] = {}
        excerpts = []
        for name, prior in (('current', (ROOT / PRIMARY).read_text()), ('fresh', fresh.read_text())):
            original=binding.remove_solver_hull_clear(prior)
            raw = binding.replace_solver_hull_clear(original)
            if ((raw==prior)!=args.require_binding or raw==original or
                    binding.remove_solver_hull_clear(raw)!=original or binding.replace_solver_hull_clear(raw)!=raw):
                raise ValueError('Exact staged hull-clear source reversal drift')
            prepared, receipt = prepare(raw)
            normal = specialize(original)[0]
            if receipt.get('native_solver_hull_clear_entries') != ['314870'] or prepare(prepared)[0] != prepared:
                raise ValueError('Exact prepared hull-clear source did not repeat')
            for tag, candidate, reference in (('raw', raw, original), ('prepared', prepared, normal)):
                key = name + '_' + tag
                match, resolver, dispatcher, state = binding.context(candidate)
                retained, _, _, _ = binding.context(reference)
                is_prepared = tag == 'prepared'
                if binding.installed_entries(candidate) != ['314870'] or binding.split_body(match[0])[0] != retained[0]:
                    raise ValueError('Complete actual hull-clear source context drift')
                sites = [int(pc, 16) for pc in re.findall(r'(?:services.instruction|instruction_literal)\(0x([0-9a-f]+)u\)', retained[0])]
                if len(sites) != 32 or not set(binding.FRONTIERS) <= set(zip(sites, sites[1:])):
                    raise ValueError('Exact hull-clear authored retired successors drift')
                exact = (candidate[:match.start()] + retained[0] + candidate[match.end():]).replace(binding.INCLUDE, '', 1)
                if exact != reference:
                    raise ValueError('A neighboring owner changed in the staged hull-clear source')
                report['contexts'][key] = dict(source_sha256=binding.digest(candidate), body_sha256=binding.digest(match[0]),
                                               class_sha256=binding.digest(state[0]), resolver_sha256=binding.digest(resolver[0]),
                                               dispatcher_sha256=binding.digest(dispatcher[0]), exact_reversal=True)
                report['exact_API_controls'][key] = controls(candidate, is_prepared, prepare)
                if args.whole_syntax:
                    whole = scratch / (key + '.cpp')
                    whole.write_text(candidate)
                    syntax = subprocess.run([args.compiler, '-std=c++20', '-Wno-ignored-attributes', '-fsyntax-only',
                                             '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), str(whole)],
                                            capture_output=True, text=True, timeout=600)
                    if syntax.returncode:
                        raise ValueError('Whole source hull-clear syntax ' + key + ': ' + syntax.stderr[-8000:])
                    report['contexts'][key]['whole_cpp_syntax'] = True
                excerpts.append('namespace clear_' + key + ' {\n' + state[0] + '\n' +
                                retained[0].replace('run_314870(', 'retained(', 1) + '\n' + match[0] + '\n}\n')
        pairs = ','.join('std::pair{' + hex(pc) + 'u,' + hex(next_pc) + 'u}' for pc, next_pc in binding.FRONTIERS)
        excerpts.append('namespace clear_routes {static constexpr std::array frontiers{' + pairs + '};}\n')
        (scratch / 'solver_hull_clear_routes.inc').write_text(''.join(excerpts))
        data = scratch / 'input.json'
        data.write_text(json.dumps(inputs(), separators=(',', ':')))
        command = [args.compiler, '-std=c++20', '-O2', '-Wno-ignored-attributes', '-fno-fast-math', '-ffp-contract=off',
                   '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'tests'),
                   '-I' + str(scratch), str(ROOT / TEST), '-o', str(scratch / 'clear-routes')]
        if args.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command

        def build(trial):
            result = subprocess.run(trial, capture_output=True, text=True, timeout=900)
            if result.returncode:
                raise ValueError('Complete hull-clear source route compile: ' + result.stderr[-8000:])

        def run(executable, reference=False):
            env = os.environ.copy()
            env.pop('HALO2_REFERENCE_SOLVER_HULL_CLEAR', None)
            if reference:
                env['HALO2_REFERENCE_SOLVER_HULL_CLEAR'] = '1'
            with data.open('rb') as inp:
                return subprocess.run([executable] + (['reference'] if reference else []), stdin=inp, env=env,
                                      capture_output=True, text=True, timeout=900)

        build(command)
        for route, forced in (('default', False), ('forced_reference', True)):
            result = run(command[-1], forced)
            if result.returncode:
                raise ValueError('Complete hull-clear ' + route + ': ' + result.stderr[-8000:])
            report[route] = json.loads(result.stdout)
        if args.mutations:
            original = (ROOT / 'src' / OWNER).read_text()
            defects = (
                ('raw_callback_admitted', 'if constexpr(!Literal)if(s.services.instruction)return false;',
                 'if constexpr(false)if(s.services.instruction)return false;'),
                ('missing_retirement_checkpoint', 'if(admitted<Literal>(state))return;',
                 'if(true)return;'),
                ('foreign_token_swallowed', 'if(resume.owner!=&token)throw;', 'if(false&&resume.owner!=&token)throw;'),
            )
            report['compiled_defects'] = {}
            for name, old, new in defects:
                if original.count(old) != 1:
                    raise ValueError('Hull-clear defect source drift ' + name)
                overlay = scratch / name
                mutant = overlay / OWNER
                mutant.parent.mkdir(parents=True)
                mutant.write_text(original.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(overlay))
                trial[-1] = str(overlay / 'clear-routes')
                build(trial)
                result = run(trial[-1])
                if not result.returncode or not any(expected in result.stderr for expected in
                        ('Exact copied hull-clear source route differs', 'Unsupported native hull-clear continuation')):
                    raise ValueError('Compiled hull-clear source defect missed its witness ' + name + ': ' + result.stderr[-5000:])
                report['compiled_defects'][name] = result.stderr.strip()[-2000:]
        report.update(passed=True, production_bound=args.require_binding,
                      scope='Exact source transport with live retained callback fallback; no hardware fault claim.')
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
