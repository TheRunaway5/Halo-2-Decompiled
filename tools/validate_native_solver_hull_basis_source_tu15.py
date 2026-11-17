#!/usr/bin/env python3
"""Private actual313790 retained source profiles and complete I/O retirement."""
import argparse
import hashlib
import json
import os
import platform
import re
import subprocess
import sys
from pathlib import Path

import native_solver_hull_basis_replacements as binding
from research_mp_sandbox_update_functions import Xbe
from specialize_native_operands import specialize
from validate_native_solver_hull_basis_frame_tu15 import PIN, inputs
from generated_routine_ownership import function_bodies
import native_solver_hull_edge_replacements as edge_binding
import native_solver_hull_support_pair_replacements as pair_binding
import native_polygon_support_replacements as polygon_binding
import native_solver_hull_append_replacements as append_binding
import native_zero_shape_support_replacements as zero_binding
import native_box_shape_support_replacements as box_binding

ROOT = Path(__file__).resolve().parents[1]
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
GENERATOR = 'tools/generate_physics_live_support_tu15.py'
TEST = 'tests/native_tu15_solver_hull_basis_source_tests.cpp'
MODES = ('baseline', 'paired-nan', 'frontiers', 'feedback', 'child-return', 'journal', 'prefixes',
         'raw-free', 'raw-install', 'own-reference', 'refusals')
JOURNAL_SOURCES = ('src/runtime/source_execution_inputs.cpp', 'src/runtime/source_runtime_memory.cpp',
                   'src/runtime/source_runtime_snapshot_bytes.cpp',
                   'src/assets/cache_file_snapshot.cpp',
                   'src/runtime/source_runtime_write_protection.inc')
ORDERED_SSE = {
    'mul_ps': ((0x313838,3,2),(0x313845,0,2),(0x313851,0,1),(0x3138c1,5,1),(0x3138c7,0,2),(0x313921,0,2)),
    'add_ss': ((0x31385b,3,0),(0x31386c,0,3),(0x3138d1,1,0),(0x3138e4,0,1)),
    'mul_ss': ((0x313884,0,5),(0x313888,0,5),(0x3138b2,0,5),(0x3138b6,0,6),
               (0x3138fc,0,1),(0x313900,0,1),(0x31390d,4,1),(0x313911,4,3)),
}


def ordered_math(body):
    lines = body.splitlines(keepends=True)
    for opcode, sites in ORDERED_SSE.items():
        if body.count('_mm_'+opcode+'(') != len(sites):
            raise ValueError('Complete authored hull-basis SIMD inventory drift '+opcode)
        for pc, dst, src in sites:
            needle = f'_mm_{opcode}(s.xmm[{dst}],s.xmm[{src}])'
            rows = [n for n,line in enumerate(lines) if f'(0x{pc:x}u);' in line and needle in line]
            if len(rows)!=1 or lines[rows[0]].count(needle)!=1:
                raise ValueError('Exact authored hull-basis SIMD operand drift '+hex(pc))
            at = rows[0]
            lines[at] = lines[at].replace(needle, f'ordered_{opcode}(s.xmm[{dst}],s.xmm[{src}])', 1)
    return ''.join(lines)


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
    raw = [binding.replace_solver_hull_basis(text) for text in unprepared]
    if require_binding and raw != unprepared:
        raise ValueError('Exact installed basis owner is missing')
    for text in raw:
        if binding.replace_solver_hull_basis(binding.remove_solver_hull_basis(text)) != text:
            raise ValueError('Complete basis overlay reversal drift')
    variants = raw + [binding.rebind_prepared(specialize(text)[0]) for text in raw]
    primitive = binding.HELPER.search(raw[1])
    if not primitive or binding.digest(primitive[0]) != binding.HELPER_PIN:
        raise ValueError('Original fresh native spill helper drift')
    estimate = binding.ESTIMATE.search(raw[0])
    if not estimate or binding.digest(estimate[0]) != binding.ESTIMATE_PIN:
        raise ValueError('Complete genuine basis estimate helper missing')
    ordered = [f'static __m128 ordered_{op}(__m128 a,__m128 b){{asm volatile("{op.replace("_","")} %1,%0":"+x"(a):"x"(b):"memory");return a;}}'
               for op in ORDERED_SSE]
    blocks = [primitive[0], estimate[0], *ordered, 'static std::array<unsigned,4> literals{};',
              'static std::array<std::uint32_t,4> suffixes{};',
              'static constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,56> frontiers{{'
              + ','.join(f'{{0x{before:x}u,0x{after:x}u}}' for before, after in binding.FRONTIERS) + '}};']
    contexts, controls = [], []
    for n, text in enumerate(variants):
        if binding.installed_entries(text) != ['313790']:
            raise ValueError('Wrong genuine actual facet basis owner')
        match, resolver, dispatcher = binding.context(text)
        state = binding.STATE.search(text)[0]
        body = match[0]
        context = n >= 2, bool(n % 2)
        prefix = binding.PREFIXES[context]
        opening = body.index('{') + 1
        if not body.startswith(prefix, opening):
            raise ValueError('Basis entry instrumentation lost exact prefix')
        bodies = {
            'wrong-tag': body.replace(prefix, binding.PREFIXES[not context[0], context[1]], 1),
            'wrong-spill': body.replace(prefix, binding.PREFIXES[context[0], not context[1]], 1),
            'body-drift': body.replace('0xb8u', '0xb4u', 1),
        }
        first = binding.instruction(0x313790, context[0])
        for pc, label in binding.LABELS.items():
            bodies['relocated-' + hex(pc)] = body.replace(label, '', 1).replace(first, label + first, 1)
        declined = {}
        for label, bad in bodies.items():
            if bad == body:
                raise ValueError('Basis negative test had no authored seam: ' + label)
            mutated = text[:match.start()] + bad + text[match.end():]
            try:
                binding.installed_entries(mutated)
            except ValueError as e:
                declined[label] = str(e)
            else:
                raise ValueError('Basis binder accepted ' + label)
        controls.append(declined)
        contexts.append(dict(format='prepared' if context[0] else 'raw', fresh=context[1],
                             body=binding.digest(body), resolver=binding.digest(resolver),
                             dispatcher=binding.digest(dispatcher), state=binding.digest(state)))
        at = opening + len(prefix)
        measured = body[:at] + f'\n++literals[{n}];' + body[at:]
        for pc, label in binding.LABELS.items():
            branch = f'case 0x{pc:x}u:goto {label[:-2]};'
            if measured.count(branch) != 1:
                raise ValueError('Basis exact continuation measurement seam drift')
            measured = measured.replace(branch, branch.replace('goto', f'suffixes[{n}]=0x{pc:x}u;goto'), 1)
        original = binding.canonical_body(body)
        blocks += [measured.replace('run_313790(', f'bound_{n}(', 1),
                   original.replace('run_313790(', f'retained_{n}(', 1),
                   ordered_math(original).replace('run_313790(', f'ordered_{n}(', 1)]
    blocks.append('static void run(WorldState&s,unsigned variant,bool reference,bool ordered){switch(variant){'
                  + ''.join(f'case {n}:if(reference){{if(ordered)ordered_{n}(s);else retained_{n}(s);}}else bound_{n}(s);return;' for n in range(4))
                  + 'default:throw std::logic_error("Unknown basis source context");}}')
    (scratch / 'solver_hull_basis_source.inc').write_text('\n'.join(blocks) + '\n')
    # Keep the complete installed Edge/Pair/Append/shape owners and their
    # normal admission and retained fallback behavior in the probe.
    children=[];child_pins=[]
    for n,text in enumerate(variants):
        selected={sig.symbol:body for sig,body in function_bodies(text)if sig.symbol in('313470','313390','313290','2e8d20','306790')}
        if len(selected)!=5:raise ValueError('Incomplete actual hull-basis child source set')
        for module,entry in [(edge_binding,'313470'),(pair_binding,'313390'),(append_binding,'313290'),(zero_binding,'2e8d20'),(box_binding,'306790')]:
            if module.installed_entries(text)!=[entry]:raise ValueError('Actual hull basis descendant context drift '+entry)
        child_pins.append({name:binding.digest(body)for name,body in selected.items()})
        for name,label in [('2e8d20','zero'),('306790','box'),('313390','pair'),('313290','append'),('313470','edge')]:children.append(selected[name].replace('run_'+name+'(',label+'_'+str(n)+'(',1))
    world=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text();match,poly=polygon_binding.authenticate(world)
    if match[1]!='ObservedWorldState':raise ValueError('Wrong actual polygon child owner')
    opening=match[0].index('{')+1
    if not match[0].startswith(polygon_binding.PREFIX,opening):raise ValueError('Missing actual polygon default owner')
    installed_tail=match[0][:opening]+match[0][opening+len(polygon_binding.PREFIX):]
    if installed_tail!=polygon_binding.ordered_body(poly):raise ValueError('Actual polygon retained child tail drift')
    # Keep its actual ordered-store helper and16-byte child event. The
    # canonical extractor's8+8 spelling is a different retained protocol.
    poly=installed_tail
    poly=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',lambda m:m[1],poly)
    if poly.count('selected(0x447020u,s.image)')!=1 or 'world_observation_pc('in poly:raise ValueError('Incomplete actual polygon source operand mapping')
    poly=poly.replace('selected(0x447020u,s.image)','0x44aac8u').replace('ObservedWorldState','WorldState').replace('run_1f1d20(','polygon_source(',1)
    children.append(poly)
    for label in('zero','box','pair','append','edge'):
        children.append('static void '+label+'(WorldState&s,unsigned variant){switch(variant){'+''.join('case '+str(n)+':'+label+'_'+str(n)+'(s);return;'for n in range(4))+'default:throw std::runtime_error("Unknown genuine child source context");}}')
    (scratch/'solver_hull_basis_children.inc').write_text('\n'.join(children))
    return dict(genuine_generator=command, contexts=contexts, source_authentication_controls=controls,
                actual_child_body_pins=child_pins, polygon_owner_pin=polygon_binding.BODY_PINS[match[1]],
                ordered_reference_sites=ORDERED_SSE,
                instrumentation='Exact post-prefix entry and pre-goto target counters only after authentication',
                source_profiles='Current legacy spill and exact fresh helper plus the authored load/pop C1 effects')


def source_pins():
    files = {PRIMARY, GENERATOR, TEST, *JOURNAL_SOURCES, 'tools/native_solver_hull_basis_replacements.py',
             'tools/specialize_native_operands.py',
        'src/physics/runtime/physics_world_runtime_generated.cpp',
        'tools/native_solver_hull_edge_replacements.py',
        'tools/native_solver_hull_support_pair_replacements.py',
        'tools/native_solver_hull_append_replacements.py',
        'tools/native_zero_shape_support_replacements.py',
        'tools/native_box_shape_support_replacements.py',
        'tools/native_polygon_support_replacements.py', 'tools/validate_native_solver_hull_basis_frame_tu15.py',
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


def mutations(scratch, command, inputs, compile_timeout):
    cases = [
        ('raw_callable_admitted', 'owner', 'if constexpr(!Literal)if(s.services.instruction)return false;',
         'if constexpr(false)if(s.services.instruction)return false;', 'baseline', 'Actual hull basis retained source differs'),
        ('skipped_policy_retirement', 'owner', 'if(!admitted<Literal>(v,depth_limit_after(next)))throw Continuation{next,&token};',
         'if(false&&!admitted<Literal>(v,depth_limit_after(next)))throw Continuation{next,&token};', 'frontiers', 'Actual hull basis retained source differs'),
        ('wrong_estimate_input', 'frame', 'estimate_(s_,low(s_.xmm[0]))',
         'estimate_(s_,low(s_.xmm[0])^1u)', 'baseline', 'Actual hull basis retained source differs'),
        ('swapped_packed_multiply_operands', 'frame', 'asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;',
         'asm volatile("mulps %1,%0":"+x"(b):"x"(a):"memory");return b;',
         'paired-nan', 'Actual hull basis retained source differs'),
        ('premature_second_ranking_capacity_refusal', 'owner', 'case 0x3137d8u:case 0x3137f2u:return 7u;',
         'case 0x3137d8u:return 7u;case 0x3137f2u:return 6u;',
         'baseline', 'Basis selected the wrong source format or reference route'),
        ('foreign_token_swallowed', 'owner', 'if(resume.owner!=&token)throw;',
         'if(false&&resume.owner!=&token)throw;', 'feedback', 'Unsupported native hull-basis continuation'),
    ]
    result = {}
    for name, kind, old, new, mode, expected in cases:
        relative = 'physics/runtime/native_tu15_solver_hull_basis_source_' + kind + '.hpp'
        code = (ROOT / 'src' / relative).read_text()
        if code.count(old) != 1:
            raise ValueError('Complete basis compiled-defect seam drift ' + name)
        overlay = scratch / name
        target = overlay / relative
        target.parent.mkdir(parents=True)
        target.write_text(code.replace(old, new, 1))
        candidate = command.copy()
        candidate.insert(candidate.index('-I' + str(ROOT / 'include')), '-I' + str(overlay))
        candidate[-1] = str(overlay / 'basis-source')
        build = subprocess.run(candidate, capture_output=True, text=True, timeout=compile_timeout)
        if build.returncode:
            raise ValueError('Actual basis defect failed compilation ' + name + ': ' + build.stderr[-5000:])
        with inputs.open('rb') as data:
            run = subprocess.run([candidate[-1], mode], stdin=data, capture_output=True, text=True, timeout=1200)
        if not run.returncode or expected not in run.stderr:
            raise ValueError('Actual basis defect did not hit its witness ' + name + ': ' + run.stderr[-5000:])
        result[name] = dict(rejected=True, mode=mode, expected=expected, diagnostic=run.stderr[-5000:])
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--compile-timeout', type=int, default=600)
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
        image = Xbe(a.xbe)
        from oracle_native_campaign_screen_lifecycle_tu15 import _Code
        original = _Code(image,'.text')
        for opcode, sites in ORDERED_SSE.items():
            for pc,dst,src in sites:
                instruction = original.at(pc)
                if instruction.mnemonic != opcode.replace('_','') or instruction.op_str != f'xmm{dst}, xmm{src}':
                    raise ValueError('Original hull basis SIMD instruction drift '+hex(pc))
        data = scratch / 'inputs.json'
        data.write_text(json.dumps(inputs(image), separators=(',', ':')))
        report['input_sha256'] = sha(data)
        command = [a.compiler, '-std=c++20', '-Wno-ignored-attributes', '-O2', '-fno-fast-math', '-ffp-contract=off',
                   '-frounding-math', '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'),
                   '-I' + str(ROOT / 'tests'), '-I' + str(scratch), str(ROOT / TEST),
                   *[str(ROOT / name) for name in JOURNAL_SOURCES if name.endswith('.cpp')],
                   '-o', str(scratch / 'basis-source')]
        if a.sanitizers:
            command[1:1] = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
        report['command'] = command
        built = subprocess.run(command, capture_output=True, text=True, timeout=a.compile_timeout)
        if built.returncode:
            raise ValueError('Basis source comparison compile failed: ' + built.stderr[-9000:])
        report['routes'] = {}
        for mode in a.modes or MODES:
            env = os.environ.copy()
            env.pop('HALO2_REFERENCE_SOLVER_HULL_BASIS', None)
            if mode == 'own-reference':
                env['HALO2_REFERENCE_SOLVER_HULL_BASIS'] = '1'
            with data.open('rb') as inp:
                run = subprocess.run([command[-1], mode], stdin=inp, capture_output=True, text=True, env=env)
            if run.returncode:
                raise ValueError('Basis source comparison failed ' + mode + ': ' + run.stderr[-9000:])
            report['routes'][mode] = json.loads(run.stdout.strip().splitlines()[-1])
            print(json.dumps(dict(completed_mode=mode, passed=False, validation_incomplete=True)), flush=True)
        if a.mutations:
            report['compiled_defects'] = mutations(scratch, command, data, a.compile_timeout)
        report.update(passed=True, production_bound=a.require_binding,
                      scope='Exact modeled flags/software FP and source-publication policies, using explicitly ordered authored SIMD expressions only in the phased reference for fully native modes. The preserved unmodified GCC paired-NaN child-entry mismatch remains distinct; no hardware fault expansion.')
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
