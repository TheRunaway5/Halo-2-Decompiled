#!/usr/bin/env python3
"""Private actual313300 retained source profiles and complete I/O retirement."""
import argparse
import hashlib
import json
import os
import platform
import re
import subprocess
import sys
from pathlib import Path

import native_solver_hull_sample_replacements as binding
from research_mp_sandbox_update_functions import Xbe
from specialize_native_operands import specialize
from validate_native_solver_hull_sample_frame_tu15 import PIN, inputs

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
GENERATOR = 'tools/generate_physics_live_support_tu15.py'
TEST = 'tests/native_tu15_solver_hull_sample_source_tests.cpp'
MODES = ('baseline', 'frontiers', 'feedback', 'prefixes', 'raw-free', 'raw-install', 'own-reference', 'refusals')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(scratch, require_binding=False):
    current = (ROOT / PRIMARY).read_text()
    fresh = scratch / 'fresh.cpp'
    command = [sys.executable, '-B', str(ROOT / GENERATOR), '--output', str(fresh),
               '--manifest', str(scratch / 'fresh-manifest.json')]
    run = subprocess.run(command, capture_output=True, text=True, timeout=600)
    if run.returncode:
        raise ValueError('Complete genuine Primary generation failed: ' + run.stderr[-7000:])
    unprepared = [current, fresh.read_text()]
    raw = [binding.replace_solver_hull_sample(text) for text in unprepared]
    if require_binding and raw != unprepared:
        raise ValueError('Exact installed sample owner is missing')
    for text in raw:
        if binding.replace_solver_hull_sample(binding.remove_solver_hull_sample(text)) != text:
            raise ValueError('Complete sample overlay reversal drift')
    variants = raw + [binding.rebind_prepared(specialize(text)[0]) for text in raw]
    primitive = binding.HELPER.search(raw[1])
    if not primitive or binding.digest(primitive[0]) != binding.HELPER_PIN:
        raise ValueError('Original fresh native spill helper drift')
    blocks = [primitive[0], 'static std::array<unsigned,4> literals{};',
              'static std::array<std::uint32_t,4> suffixes{};',
              'static constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,19> frontiers{{'
              + ','.join(f'{{0x{before:x}u,0x{after:x}u}}' for before, after in binding.FRONTIERS) + '}};']
    contexts, controls = [], []
    for n, text in enumerate(variants):
        if binding.installed_entries(text) != ['313300']:
            raise ValueError('Wrong genuine actual sampler owner')
        match, resolver, dispatcher = binding.context(text)
        state = binding.STATE.search(text)[0]
        body = match[0]
        context = n >= 2, bool(n % 2)
        prefix = binding.PREFIXES[context]
        opening = body.index('{') + 1
        if not body.startswith(prefix, opening):
            raise ValueError('Sample entry instrumentation lost exact prefix')
        bodies = {
            'wrong-tag': body.replace(prefix, binding.PREFIXES[not context[0], context[1]], 1),
            'wrong-spill': body.replace(prefix, binding.PREFIXES[context[0], not context[1]], 1),
            'body-drift': body.replace('0x3e39b193u', '0x3e39b195u', 1),
        }
        first = binding.instruction(0x313300, context[0])
        for pc, label in binding.LABELS.items():
            bodies['relocated-' + hex(pc)] = body.replace(label, '', 1).replace(first, label + first, 1)
        declined = {}
        for label, bad in bodies.items():
            if bad == body:
                raise ValueError('Sample negative test had no authored seam: ' + label)
            mutated = text[:match.start()] + bad + text[match.end():]
            try:
                binding.installed_entries(mutated)
            except ValueError as e:
                declined[label] = str(e)
            else:
                raise ValueError('Sample binder accepted ' + label)
        controls.append(declined)
        contexts.append(dict(format='prepared' if context[0] else 'raw', fresh=context[1],
                             body=binding.digest(body), resolver=binding.digest(resolver),
                             dispatcher=binding.digest(dispatcher), state=binding.digest(state)))
        at = opening + len(prefix)
        measured = body[:at] + f'\n++literals[{n}];' + body[at:]
        for pc, label in binding.LABELS.items():
            branch = f'case 0x{pc:x}u:goto {label[:-2]};'
            if measured.count(branch) != 1:
                raise ValueError('Sample exact continuation measurement seam drift')
            measured = measured.replace(branch, branch.replace('goto', f'suffixes[{n}]=0x{pc:x}u;goto'), 1)
        blocks += [measured.replace('run_313300(', f'bound_{n}(', 1),
                   binding.canonical_body(body).replace('run_313300(', f'retained_{n}(', 1)]
    blocks.append('static void run(WorldState&s,unsigned variant,bool reference){switch(variant){'
                  + ''.join(f'case {n}:if(reference)retained_{n}(s);else bound_{n}(s);return;' for n in range(4))
                  + 'default:throw std::logic_error("Unknown sample source context");}}')
    (scratch / 'solver_hull_sample_source.inc').write_text('\n'.join(blocks) + '\n')
    return dict(genuine_generator=command, contexts=contexts, source_authentication_controls=controls,
                instrumentation='Exact post-prefix entry and pre-goto target counters only after authentication',
                source_profiles='Current legacy spill and exact fresh helper plus the authored load/pop C1 effects')


def source_pins():
    files = {PRIMARY, GENERATOR, TEST, 'tools/native_solver_hull_sample_replacements.py',
             'tools/specialize_native_operands.py', 'tools/validate_native_solver_hull_sample_frame_tu15.py',
             str(Path(__file__).resolve().relative_to(ROOT))}
    pending = list(files)
    while pending:
        path = ROOT / pending.pop()
        body = path.read_text()
        dependencies = set()
        for include in re.findall(r'^#include "([^"\n]+)"', body, re.M):
            resolved = next((p for p in (path.parent / include, ROOT / 'include' / include, ROOT / 'src' / include,
                                         ROOT / 'tests' / include) if p.is_file()), None)
            if resolved:
                dependencies.add(str(resolved.relative_to(ROOT)))
        if path.suffix == '.py':
            modules = set(re.findall(r"['\"]([A-Za-z_]\w+)(?:\.py)?['\"]", body))
            modules.update(re.findall(r'^(?:from|import) ([A-Za-z_]\w+)', body, re.M))
            dependencies.update('tools/' + name + '.py' for name in modules
                                if (ROOT / 'tools' / (name + '.py')).is_file())
        pending.extend(dependencies - files)
        files.update(dependencies)
    return {name: sha(ROOT / name) for name in sorted(files)}


def mutations(scratch, command, inputs):
    cases = [
        ('raw_callable_admitted', 'owner', 'if constexpr(!Literal)if(s.services.instruction)return false;',
         'if constexpr(false)if(s.services.instruction)return false;', 'baseline', 'Actual hull sample retained source differs'),
        ('skipped_policy_retirement', 'owner', 'if(!admitted<Literal>(v,depth_limit_at(next)))throw Continuation{next,&token};',
         'if(false&&!admitted<Literal>(v,depth_limit_at(next)))throw Continuation{next,&token};', 'frontiers', 'Actual hull sample retained source differs'),
        ('stale_seed_replaces_live_slot', 'frame', 'physics_step_detail::si(word(s_.gp[4]+4u))',
         'physics_step_detail::si(s_.gp[0])', 'feedback', 'Actual hull sample retained source differs'),
        ('wrong_fresh_spill_profile', 'frame', 'const auto bits=spill_(s_);',
         'const auto bits=LegacySpill{}(s_);', 'feedback', 'Actual hull sample retained source differs'),
        ('omitted_fresh_final_C1', 'frame', 'void finish(){clear_c1();',
         'void finish(){', 'feedback', 'Actual hull sample retained source differs'),
        ('early_source_FSTP_pop', 'frame',
         'physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[2]+axis*4u,4u,bits);\n        s_.fp.pop();',
         's_.fp.pop();physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[2]+axis*4u,4u,bits);',
         'prefixes', 'Actual hull sample retained source differs'),
        ('foreign_token_swallowed', 'owner', 'if(resume.owner!=&token)throw;',
         'if(false&&resume.owner!=&token)throw;', 'feedback', 'Unsupported native hull-sample continuation'),
    ]
    result = {}
    for name, kind, old, new, mode, expected in cases:
        relative = 'physics/runtime/native_tu15_solver_hull_sample_source_' + kind + '.hpp'
        code = (ROOT / 'src' / relative).read_text()
        if code.count(old) != 1:
            raise ValueError('Complete sample compiled-defect seam drift ' + name)
        overlay = scratch / name
        target = overlay / relative
        target.parent.mkdir(parents=True)
        target.write_text(code.replace(old, new, 1))
        candidate = command.copy()
        candidate.insert(candidate.index('-I' + str(ROOT / 'include')), '-I' + str(overlay))
        candidate[-1] = str(overlay / 'sample-source')
        build = subprocess.run(candidate, capture_output=True, text=True, timeout=600)
        if build.returncode:
            raise ValueError('Actual sample defect failed compilation ' + name + ': ' + build.stderr[-5000:])
        with inputs.open('rb') as data:
            run = subprocess.run([candidate[-1], mode], stdin=data, capture_output=True, text=True, timeout=1200)
        if not run.returncode or expected not in run.stderr:
            raise ValueError('Actual sample defect did not hit its witness ' + name + ': ' + run.stderr[-5000:])
        result[name] = dict(rejected=True, mode=mode, expected=expected, diagnostic=run.stderr[-5000:])
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--sanitizers', action='store_true')
    p.add_argument('--mutations', action='store_true')
    p.add_argument('--modes', nargs='+', choices=MODES)
    p.add_argument('--require-binding', action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686') or not a.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    pins = source_pins()
    report = dict(passed=False, production_bound=False, source_sha256=pins,
                  scratch_owner=str(scratch), retained_large_artifacts=[])
    try:
        if sha(a.xbe) != PIN:
            raise ValueError('Wrong original TU1.5 image')
        report['preparation'] = prepare(scratch, a.require_binding)
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(inputs(Xbe(a.xbe)), separators=(',', ':')))
        report['input_sha256'] = sha(data)
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math', '-ffp-contract=off',
                   '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), '-I' + str(scratch), str(ROOT / TEST), '-o', str(scratch / 'sample-source')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True, timeout=600)
        if built.returncode:
            raise ValueError('Sample source comparison compile failed: ' + built.stderr[-9000:])
        report['routes'] = {}
        for mode in a.modes or MODES:
            env = os.environ.copy()
            env.pop('HALO2_REFERENCE_SOLVER_HULL_SAMPLE', None)
            if mode == 'own-reference':
                env['HALO2_REFERENCE_SOLVER_HULL_SAMPLE'] = '1'
            with data.open('rb') as inp:
                run = subprocess.run([command[-1], mode], stdin=inp, capture_output=True, text=True, env=env)
            if run.returncode:
                raise ValueError('Sample source comparison failed ' + mode + ': ' + run.stderr[-9000:])
            report['routes'][mode] = json.loads(run.stdout.strip().splitlines()[-1])
            print(json.dumps(dict(completed_mode=mode, passed=False, validation_incomplete=True)), flush=True)
        if a.mutations:
            report['compiled_defects'] = mutations(scratch, command, data)
        report.update(passed=True, production_bound=a.require_binding,
                      scope='Exact modeled flags/software FP and source-publication policies; no hardware fault expansion')
    except Exception as error:
        report['error'] = repr(error)
    report['sources_unchanged'] = all(sha(ROOT / name) == value for name, value in pins.items())
    if not report['sources_unchanged']:
        report.update(passed=False, source_drift=True)
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'error') if k in report}))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
