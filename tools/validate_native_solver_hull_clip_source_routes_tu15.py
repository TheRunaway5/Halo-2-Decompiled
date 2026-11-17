#!/usr/bin/env python3
"""Exercise exact current and genuine fresh 3147d0 source routes in owned scratch."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys

import native_solver_hull_clip_replacements as binding
from specialize_native_operands import specialize
from validate_native_solver_hull_clip_frame_tu15 import inputs
from research_mp_sandbox_update_functions import Xbe
from validate_native_tu15_prt_helpers import PIN

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
GENERATOR = 'tools/generate_physics_live_support_tu15.py'
SPEC = 'tools/specialize_native_operands.py'
OWNER = 'physics/runtime/native_tu15_solver_hull_clip_source_owner.hpp'
TEST = 'tests/native_tu15_solver_hull_clip_source_routes_tests.cpp'
REFERENCE_CONTROL = 'HALO2_REFERENCE_SOLVER_HULL_CLIP'
ANCHOR = "    receipt['native_solver_hull_clear_entries']=solver_hull_clear_entries\n"
BRANCH = '''    solver_hull_clip_entries=[]
    if any(marker in output for marker in ('#include "physics/runtime/native_tu15_solver_hull_clip_source_owner.hpp"','native_solver_hull_clip_owner_detail','native_hull_clip_resume_')):
        clip_spec=importlib.util.spec_from_file_location('native_solver_hull_clip_binding',Path(__file__).with_name('native_solver_hull_clip_replacements.py'))
        clip_helper=importlib.util.module_from_spec(clip_spec);clip_spec.loader.exec_module(clip_helper)
        clip_helper.installed_entries(text)
        output=clip_helper.rebind_prepared(output)
        solver_hull_clip_entries=clip_helper.installed_entries(output)
    receipt['native_solver_hull_clip_entries']=solver_hull_clip_entries
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
    installed = "receipt['native_solver_hull_clip_entries']" in current
    if installed:
        return specialize, digest_specializer(current), True
    if current.count(ANCHOR) != 1:
        raise ValueError('Complete private hull-clip specializer anchor drift')
    candidate = scratch / 'proposed-specializer.py'
    candidate.write_text(current.replace(ANCHOR, ANCHOR + BRANCH, 1))
    namespace = {'__name__': '_private_hull_clip_specializer', '__file__': str(ROOT / SPEC)}
    exec(compile(candidate.read_bytes(), str(candidate), 'exec'), namespace)
    return namespace['specialize'], sha(candidate), False


def digest_specializer(source):
    return hashlib.sha256(source.encode()).hexdigest()


def refusal(action, source):
    try:
        action(source)
    except ValueError as error:
        return str(error)
    raise ValueError('Actual hull-clip source API accepted modified context')


def controls(source, prepared, fresh, prepare):
    match, resolver, dispatcher = binding.context(source)
    state=binding.STATE.search(source)
    first, following = tuple(binding.LABELS)[:2]
    label = binding.LABELS[first]
    needle = binding.instruction(first, prepared)
    next_site = binding.instruction(following, prepared)
    relocated = match[0].replace(label + needle, needle, 1).replace(next_site, label + next_site, 1)
    variants = {
        'wrong-format': source.replace(binding.PREFIXES[prepared,fresh], binding.PREFIXES[not prepared,fresh], 1),
        'wrong-spill-profile': source.replace(binding.PREFIXES[prepared,fresh],binding.PREFIXES[prepared,not fresh],1),
        'mixed-instruction': source.replace(match[0], match[0].replace(needle, binding.instruction(first, not prepared), 1), 1),
        'relocated-label': source.replace(match[0], relocated, 1),
        'changed-LiveState': source.replace(state[0], state[0].replace('\n};\nusing WorldState=LiveState;',
                                          '\n    int changed_clip_owner{};\n};\nusing WorldState=LiveState;', 1), 1),
        'changed-resolver': source.replace(resolver, resolver.replace('{', '{ ', 1), 1),
        'changed-dispatcher': source.replace(dispatcher, dispatcher.replace('{', '{ ', 1), 1),
        'duplicate-include': source.replace(binding.INCLUDE, binding.INCLUDE * 2, 1),
        'foreign-owner': source + '\nstatic void foreign_clip(auto&s){halo2::native_solver_hull_clip_owner_detail::bridge<false>(s);}\n',
        'foreign-label': source + '\nstatic void foreign_clip_label(){' + binding.LABELS[first] + 'return;}\n',
    }
    if fresh:
        helper=binding.HELPER.search(source)[0]
        variants['changed-generated-spill']=source.replace(helper,helper.replace('{','{ ',1),1)
    result = {}
    for name, bad in variants.items():
        if bad == source:
            raise ValueError('Hull-clip source mutation seam missing ' + name)
        result[name] = {'installed': refusal(binding.installed_entries, bad), 'specialize': refusal(prepare, bad)}
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', required=True, type=Path)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--sanitizers', action='store_true')
    p.add_argument('--mutations', action='store_true')
    p.add_argument('--whole-syntax', action='store_true')
    p.add_argument('--require-binding', action='store_true')
    args = p.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        p.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686') or not args.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    pins = source_pins()
    report = dict(passed=False, production_bound=False, source_sha256=pins,
                  scratch_owner=str(scratch), retained_large_artifacts=[])
    try:
        if sha(args.xbe)!=PIN:
            raise ValueError('Wrong TU1.5 original image')
        if (ROOT/'src'/OWNER).read_text().count('std::getenv("'+REFERENCE_CONTROL+'")')!=1:
            raise ValueError('Exact hull-clip reference-control witness drift')
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
            original=binding.remove_solver_hull_clip(prior)
            fresh_profile=name=='fresh'
            raw = binding.replace_solver_hull_clip(original)
            if ((raw==prior)!=args.require_binding or raw==original or
                    binding.remove_solver_hull_clip(raw)!=original or binding.replace_solver_hull_clip(raw)!=raw):
                raise ValueError('Exact staged hull-clip source reversal drift')
            prepared, receipt = prepare(raw)
            normal = specialize(original)[0]
            if receipt.get('native_solver_hull_clip_entries') != ['3147d0'] or prepare(prepared)[0] != prepared:
                raise ValueError('Exact prepared hull-clip source did not repeat')
            for tag, candidate, reference in (('raw', raw, original), ('prepared', prepared, normal)):
                key = name + '_' + tag
                match, resolver, dispatcher = binding.context(candidate)
                state=binding.STATE.search(candidate)
                retained, _, _ = binding.context(reference)
                is_prepared = tag == 'prepared'
                if binding.installed_entries(candidate) != ['3147d0'] or binding.split_body(match[0])[0] != retained[0]:
                    raise ValueError('Complete actual hull-clip source context drift')
                sites = [int(pc, 16) for pc in re.findall(r'(?:services.instruction|instruction_literal)\(0x([0-9a-f]+)u\)', retained[0])]
                if len(sites) != 56 or not set(binding.FRONTIERS) <= set(zip(sites, sites[1:])):
                    raise ValueError('Exact hull-clip authored retired successors drift')
                exact = (candidate[:match.start()] + retained[0] + candidate[match.end():]).replace(binding.INCLUDE, '', 1)
                if exact != reference:
                    raise ValueError('A neighboring owner changed in the staged hull-clip source')
                report['contexts'][key] = dict(source_sha256=binding.digest(candidate), body_sha256=binding.digest(match[0]),
                                               class_sha256=binding.digest(state[0]), resolver_sha256=binding.digest(resolver),
                                               dispatcher_sha256=binding.digest(dispatcher), exact_reversal=True)
                report['exact_API_controls'][key] = controls(candidate, is_prepared, fresh_profile, prepare)
                if args.whole_syntax:
                    whole = scratch / (key + '.cpp')
                    whole.write_text(candidate)
                    syntax = subprocess.run([args.compiler, '-std=c++20', '-Wno-ignored-attributes', '-fsyntax-only',
                                             '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), str(whole)],
                                            capture_output=True, text=True, timeout=600)
                    if syntax.returncode:
                        raise ValueError('Whole source hull-clip syntax ' + key + ': ' + syntax.stderr[-8000:])
                    report['contexts'][key]['whole_cpp_syntax'] = True
                # The oracle executes the original MULPS operand order too; a
                # commuted host multiply returns the other paired NaN payload.
                site = binding.instruction(binding.ORDERED_PC, is_prepared)
                if retained[0].count(site + binding.LEGACY_MULTIPLY) != 1:
                    raise ValueError('Retained hull-clip original MULPS drift')
                oracle = retained[0].replace(site + binding.LEGACY_MULTIPLY, site + binding.ORDERED_MULTIPLY, 1)
                excerpts.append('namespace clip_' + key + ' {\n' + state[0] + '\n' +
                                (binding.HELPER.search(candidate)[0]+'\n' if fresh_profile else '') +
                                oracle.replace('run_3147d0(', 'retained(', 1) + '\n' + match[0] + '\n}\n')
        pairs = ','.join('std::pair{' + hex(pc) + 'u,' + hex(next_pc) + 'u}' for pc, next_pc in binding.FRONTIERS)
        excerpts.append('namespace clip_routes {static constexpr std::array frontiers{' + pairs + '};}\n')
        (scratch / 'solver_hull_clip_routes.inc').write_text(''.join(excerpts))
        data = scratch / 'input.json'
        data.write_text(json.dumps(inputs(Xbe(args.xbe)), separators=(',', ':')))
        command = [args.compiler, '-std=c++20', '-O2', '-Wno-ignored-attributes', '-fno-fast-math', '-ffp-contract=off',
                   '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'tests'),
                   '-I' + str(scratch), str(ROOT / TEST), '-o', str(scratch / 'clip-routes')]
        if args.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command

        def build(trial):
            result = subprocess.run(trial, capture_output=True, text=True, timeout=900)
            if result.returncode:
                raise ValueError('Complete hull-clip source route compile: ' + result.stderr[-8000:])

        def run(executable, reference=False):
            env = os.environ.copy()
            env.pop(REFERENCE_CONTROL, None)
            if reference:
                env[REFERENCE_CONTROL] = '1'
            with data.open('rb') as inp:
                return subprocess.run([executable] + (['reference'] if reference else []), stdin=inp, env=env,
                                      capture_output=True, text=True, timeout=900)

        build(command)
        for route, forced in (('default', False), ('forced_reference', True)):
            result = run(command[-1], forced)
            if result.returncode:
                raise ValueError('Complete hull-clip ' + route + ': ' + result.stderr[-8000:])
            report[route] = json.loads(result.stdout)
        if args.mutations:
            original = (ROOT / 'src' / OWNER).read_text()
            defects = (
                ('raw_callback_admitted', 'if constexpr(!Literal)if(s.services.instruction)return false;',
                 'if constexpr(false)if(s.services.instruction)return false;'),
                ('missing_retirement_checkpoint', 'if(!admitted<Literal>(state,depth_at(next)))throw Continuation{next,&token};',
                 'if(false&&!admitted<Literal>(state,depth_at(next)))throw Continuation{next,&token};'),
                ('premature_owned_x87_capacity_refusal', 'case 0x314856u:return 8u;',
                 'case 0x314856u:return 7u;'),
                ('foreign_token_swallowed', 'if(resume.owner!=&token)throw;', 'if(false&&resume.owner!=&token)throw;'),
            )
            report['compiled_defects'] = {}
            for name, old, new in defects:
                if original.count(old) != 1:
                    raise ValueError('Hull-clip defect source drift ' + name)
                overlay = scratch / name
                mutant = overlay / OWNER
                mutant.parent.mkdir(parents=True)
                mutant.write_text(original.replace(old, new, 1))
                trial = command.copy()
                trial.insert(trial.index('-I' + str(ROOT / 'include')), '-I' + str(overlay))
                trial[-1] = str(overlay / 'clip-routes')
                build(trial)
                result = run(trial[-1])
                if not result.returncode or not any(expected in result.stderr for expected in
                        ('Exact copied hull-clip source route differs', 'Unsupported native hull-clip continuation',
                         'Wrong exact hull-clip admission or retained continuation')):
                    raise ValueError('Compiled hull-clip source defect missed its witness ' + name + ': ' + result.stderr[-5000:])
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
