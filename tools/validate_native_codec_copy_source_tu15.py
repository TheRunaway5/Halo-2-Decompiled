#!/usr/bin/env python3
"""Run authenticated first codec triple source routes and genuine I/O continuations."""
import argparse
import copy
import json
import os
import platform
import re
import shlex
import subprocess
import sys
import time
from pathlib import Path

import native_codec_copy_replacements as binding
from audit_native_codec_copy_owners_tu15 import digest
from research_mp_sandbox_update_functions import NEW_DEFAULT
from validate_native_codec_row_copy_tu15 import corpus as pair_corpus
from validate_native_codec_scale_row_tu15 import corpus as scale_corpus

ROOT = Path(__file__).resolve().parents[1]
TEST = 'tests/native_tu15_codec_copy_source_tests.cpp'
RUNTIME = 'src/physics/runtime/'
DEFECTS = (
    ('wrong-quaternion-frame', 'native_tu15_codec_row_copy_source_frame.hpp',
     'g[0]=s_.flags.plus(g[0],2u,4)', 'g[0]=s_.flags.plus(g[0],1u,4)', 'Codec copy source packets differ'),
    ('wrong-translation-first-read', 'native_tu15_codec_row_copy_source_frame.hpp',
     '0x296431u,g[0]+g[1]', '0x296431u,g[0]+g[1]+4u', 'Codec copy source packets differ'),
    ('wrong-scale-output', 'native_tu15_codec_scale_row_source_frame.hpp',
     '0x2cec86u,s_.gp[0]+28u', '0x2cec86u,s_.gp[0]+24u', 'Codec copy source packets differ'),
    ('ignored-live-revocation', 'native_tu15_codec_copy_source_io.hpp',
     'if(!continuation(s,next))throw Continuation{next,&token};', 'if(false&&!continuation(s,next))throw Continuation{next,&token};',
     'Codec copy source packets differ'),
    ('wrong-translation-retirement-pc', 'native_tu15_codec_row_copy_source_frame.hpp',
     '0x296416u,0x510f60u', '0x296419u,0x510f60u', 'Codec copy source'),
    ('foreign-provider-exception-swallowed', 'native_tu15_codec_copy_source_io.hpp',
     'if(resume.owner!=&token)throw;return resume.pc;', 'return resume.pc;', 'Unsupported native codec copy continuation'),
    ('raw-live-callback-skipped', 'native_tu15_codec_copy_source_owner.hpp',
     'if constexpr(!Literal)if(s.services.instruction)return false;', '', 'Codec copy source default/refusal entry differs'),
)
FRONTIERS = {
    binding.QUATERNION: (0x2963D0, 0x2963D6, 0x2963D9, 0x2963E0, 0x2963E7, 0x2963F5, 0x2963F7,
                         0x2963F9, 0x2963FC, 0x2963FF, 0x296402, 0x296405, 0x296408),
    binding.TRANSLATION: (0x296410, 0x296416, 0x296419, 0x296420, 0x29642B, 0x29642E, 0x296433,
                          0x29643C, 0x29643E, 0x296441, 0x296444, 0x296447),
    binding.SCALE: (0x2CEC60, 0x2CEC65, 0x2CEC68, 0x2CEC6F, 0x2CEC78, 0x2CEC7B, 0x2CEC7E, 0x2CEC83),
}


def reject(source):
    try:
        binding.installed_entries(source)
    except ValueError as error:
        return str(error)
    raise ValueError('Codec copy source authentication accepted a defective input')


def reject_rogue(function, source):
    at = binding.BODY[binding.QUATERNION].search(source).start()
    rogue = ('static void rogue_copy_owner(WorldState&s){(void)halo2::native_codec_copy_owner_detail::bridge<'
             'halo2::native_codec_copy_owner_detail::Entry::quaternion,false>(s);}\n')
    try:
        function(source[:at] + rogue + source[at:])
    except ValueError as error:
        if 'Foreign codec copy bridge' not in str(error):
            raise
        return str(error)
    raise ValueError('Codec copy unexpected direct bridge survived complete group authentication')


def prepare(scratch, output, whole_syntax, require_binding, xbe):
    from specialize_native_operands import specialize
    command = [sys.executable, '-B', str(ROOT / 'tools/audit_native_codec_copy_owners_tu15.py'),
               '--output', str(output / 'identity')]
    if xbe:
        command.extend(('--xbe', str(xbe)))
    done = subprocess.run(command, capture_output=True, text=True, timeout=420)
    if done.returncode:
        raise ValueError('Exact codec copy producer audit failed: ' + (done.stdout + done.stderr)[-8000:])
    routes, declarations, variants = [], [], []
    shared_contexts = {}
    for unit, (owner, group) in enumerate(zip(('primary', 'model'), binding.ENTRY_GROUPS, strict=True)):
        for stage in ('current', 'fresh'):
            raw = (scratch / f'{owner}-{stage}-raw.cpp').read_text()
            for form in ('raw', 'prepared'):
                prepared = form == 'prepared'
                source = (scratch / f'{owner}-{stage}-{form}.cpp').read_text()
                if require_binding and binding.installed_entries(source) != [f'{entry:x}' for entry in group]:
                    raise ValueError('Complete codec copy source default group is missing')
                bound = binding.replace_codec_copy(source)
                original = binding.remove_codec_copy(source)
                if binding.remove_codec_copy(bound) != original or binding.replace_codec_copy(bound) != bound:
                    raise ValueError('Complete codec copy binding reversal differs')
                owners, canonical, authenticated_owner, literal, tag = binding.authenticate(bound)
                if authenticated_owner != unit or literal != prepared or tag != prepared:
                    raise ValueError('Actual codec copy owner/format attribution differs')
                controls = {}
                controls['rogue_unbound_bridge'] = reject_rogue(binding.replace_codec_copy, original)
                controls['rogue_installed_bridge'] = reject_rogue(binding.installed_entries, bound)
                if prepared:
                    controls['rogue_before_late_retag'] = reject_rogue(binding.rebind_prepared, bound)
                for entry, match in owners.items():
                    body = match[0]
                    prefix = binding.PREFIXES[entry][literal]
                    wrong = binding.PREFIXES[entry][not literal]
                    cases = {'changed_body': body.replace('s.pop();', 's.gp[4]++;s.pop();', 1),
                             'wrong_tag': body.replace(prefix, wrong, 1),
                             'relocated_prefix': body.replace(prefix, '', 1)[:-1] + prefix + '\n}'}
                    for pc, label in binding.LABELS[entry].items():
                        at = body.index('{') + 1 + len(prefix)
                        moved = body.replace(label, '', 1)
                        cases[f'relocated_{pc:x}'] = moved[:at] + '\n' + label + moved[at:]
                    for name, change in cases.items():
                        if change == body:
                            raise ValueError('Missing codec copy source mutation seam')
                        controls[f'{entry:x}_{name}'] = reject(bound.replace(body, change, 1))
                    controls[f'{entry:x}_duplicate_body'] = reject(bound + '\n' + body)
                for name, change in {
                    'missing_include': bound.replace(binding.INCLUDE, '', 1),
                    'moved_include': binding.INCLUDE + bound.replace(binding.INCLUDE, '', 1),
                    'changed_resolver': bound.replace('selected(std::uint32_t p,SourcePhysicsContactImage image){',
                                                       'selected(std::uint32_t p,SourcePhysicsContactImage image){p^=1u;', 1),
                    'changed_dispatch': bound.replace('std::uint32_t site,bool tail){', 'std::uint32_t site,bool tail){if(false)return;', 1),
                    'changed_State': bound.replace('using physics_world_runtime_detail::WorldState::WorldState;',
                                                   'using physics_world_runtime_detail::WorldState::WorldState;void instruction_literal(std::uint32_t){}', 1),
                }.items():
                    if change == bound:
                        raise ValueError('Missing codec copy complete context mutation seam')
                    controls[name] = reject(change)
                if prepared:
                    transition = bound
                    for entry in group:
                        transition = transition.replace(binding.PREFIXES[entry][True], binding.PREFIXES[entry][False], 1)
                    if binding.rebind_prepared(transition) != bound or binding.rebind_prepared(bound) != bound:
                        raise ValueError('Exact late codec copy format retag differs')
                else:
                    changed, _ = specialize(bound)
                    independent, _ = specialize(binding.remove_codec_copy(raw))
                    if binding.rebind_prepared(changed) != binding.replace_codec_copy(independent):
                        raise ValueError('Actual raw-to-prepared codec copy routes differ')
                syntax = None
                if whole_syntax:
                    path = scratch / f'{owner}-{stage}-{form}-candidate.cpp'
                    path.write_text(bound)
                    syntax = ['g++', '-std=c++20', '-fsyntax-only', '-I' + str(ROOT / 'include'),
                              '-I' + str(ROOT / 'src'), str(path)]
                    done = subprocess.run(syntax, capture_output=True, text=True, timeout=240)
                    if done.returncode:
                        raise ValueError('Whole codec copy source syntax failed: ' + done.stderr[-10000:])
                resolver, _, state = (pattern.search(bound)[0] for pattern in binding.CONTEXT)
                context_key = (resolver, state)
                if context_key not in shared_contexts:
                    context_name = f'codec_copy_context{len(shared_contexts)}'
                    shared_contexts[context_key] = context_name
                    declarations.append(f'namespace {context_name} {{ using namespace halo2; using namespace halo2::physics_world_runtime_detail;\n'
                                        + state + '\n' + resolver + '\n}\n')
                for entry in group:
                    n = len(routes)
                    instrumented = owners[entry][0]
                    calls = re.findall(r'(?:if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}|s.instruction_literal\((0x[0-9a-f]+u)\);)', canonical[entry])
                    if len(calls) != {binding.QUATERNION: 18, binding.TRANSLATION: 17, binding.SCALE: 10}[entry]:
                        raise ValueError('Exact retained codec copy statement set changed')
                    for raw_pc, literal_pc in calls:
                        pc = raw_pc or literal_pc
                        callback = binding._callback(int(pc[:-1], 16), literal)
                        instrumented = instrumented.replace(callback, f'codec_statement({n},{pc});' + callback, 1)
                    name = f'run_{entry:x}('
                    context_name = shared_contexts[context_key]
                    declarations.append(f'namespace codec_copy_source{n} {{ using namespace halo2;\n'
                                        + 'using namespace halo2::physics_world_runtime_detail;\n'
                                        + f'using {context_name}::WorldState; using {context_name}::selected;\n'
                                        + instrumented.replace(name, 'bound(', 1) + '\n'
                                        + canonical[entry].replace(name, 'original(', 1) + '\n}\n')
                    routes.append(f'X({n},0x{entry:x}u,{str(literal).lower()})')
                variants.append(dict(owner=owner, stage=stage, form=form, whole_syntax=syntax,
                                     source_sha256=digest(bound), strict_authentication=controls))
    source = ''.join(declarations) + f'\n#define CODEC_COPY_COUNT {len(routes)}\n#define CODEC_COPY_ROUTES(X) ' + ' '.join(routes) + '\n'
    (scratch / 'codec_copy_sources.inc').write_text(source)
    return dict(variants=variants, route_count=len(routes), source_copy_sha256=digest(source))


def inputs(smoke, mmx, prefixes):
    import random
    normal = pair_corpus(smoke) + scale_corpus(smoke)
    rows = list(normal)
    rng = random.Random(0x2963D02964102CEC60)
    for entry in binding.SITES:
        selected = [row for row in normal if row['entry'] == entry]
        if mmx:
            for mask in range(16 if smoke else 256):
                row = copy.deepcopy(selected[mask % len(selected)])
                row.update(name=row['name'] + f'-opaque-{mask}', live_x87=[])
                row['before_fp'][1] &= ~0x3800
                row['active_mmx'] = dict(known_mask=mask, physical=[
                    (rng.getrandbits(64).to_bytes(8, 'little') + (0xffff, 0x3fff, 0, 0x7fff)[(mask+n)%4].to_bytes(2, 'little')).hex()
                    for n in range(8)])
                rows.append(row)
        if prefixes:
            writes = {binding.QUATERNION: 4, binding.TRANSLATION: 3, binding.SCALE: 1}[entry]
            for original in selected[:2]:
                for phase, count in (('before_read', 8 if entry == binding.SCALE else 10),
                                     ('before_preimage', writes), ('before_write', writes), ('after_write', writes)):
                    for ordinal in range(count):
                        row = copy.deepcopy(original)
                        row.update(name=row['name']+f'-{phase}-{ordinal}', fault=dict(phase=phase, ordinal=ordinal))
                        rows.append(row)
        for frontier, next_pc in zip(FRONTIERS[entry], binding.SITES[entry], strict=True):
            row = copy.deepcopy(selected[1])
            row.update(name=row['name']+f'-retire-{frontier:x}', frontier=frontier, next=next_pc)
            rows.append(row)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', action='append')
    parser.add_argument('--mode', action='append',
                        choices=('default','own-reference','unrelated-control','guards'),
                        help='Select a bounded source validation mode; default runs all four.')
    parser.add_argument('--xbe', type=Path, default=NEW_DEFAULT)
    for option in ('smoke', 'sanitizers', 'mmx', 'prefixes', 'whole-syntax', 'require-binding', 'semantic-mutants'):
        parser.add_argument('--'+option, action='store_true')
    parser.add_argument('--execution-timeout', type=int, default=3600)
    args = parser.parse_args()
    if args.semantic_mutants and args.mode:
        parser.error('Semantic mutants require the complete default source scope')
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    if args.execution_timeout <= 0:
        parser.error('Execution timeout must be positive')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64','amd64'):
        return 77
    if not args.xbe.is_file():
        print('Original TU1.5 XBE unavailable')
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    before = {str(path.relative_to(ROOT)): digest(path.read_bytes()) for folder in ('tools', 'include', 'src', 'tests')
              for path in (ROOT / folder).rglob('*') if path.is_file() and path.suffix in ('.py', '.h', '.hpp', '.cpp', '.inc')}
    inputs_used = {*binding.UNITS, TEST, 'tools/native_codec_copy_replacements.py', 'tools/validate_native_codec_copy_source_tu15.py'}
    selected_modes = args.mode or ['default','own-reference','unrelated-control','guards']
    report = dict(passed=False, status='running', private_unbound=not args.require_binding, owned_scratch=str(scratch),
                  retained_large_artifacts=[], compilers=[], selected_modes=selected_modes,
                  readonly_fp_metadata_host_controls_status=True)

    def save():
        path=args.output/'report.json'
        pending=path.with_suffix('.pending.json')
        pending.write_text(json.dumps(report,indent=2)+'\n')
        pending.replace(path)

    try:
        report['preparation'] = prepare(scratch, args.output, args.whole_syntax, args.require_binding, args.xbe)
        save()
        fixture = scratch / 'codec-copy-inputs.json'
        fixture.write_text(json.dumps(dict(cases=inputs(args.smoke, args.mmx, args.prefixes)), separators=(',', ':')))
        mutant_fixture = scratch / 'codec-copy-mutant-inputs.json'
        if args.semantic_mutants:
            mutant_fixture.write_text(json.dumps(dict(cases=inputs(True, False, False)), separators=(',', ':')))
        identity = json.loads((args.output / 'identity/report.json').read_text())
        inputs_used.update(identity['tool_before'])
        env = {key: value for key, value in os.environ.items() if key not in ('HALO2_REFERENCE_CODEC_COPY_ROWS', 'HALO2_REFERENCE_CODEC_SCALE')}
        for compiler in args.compiler or ('g++', 'clang++'):
            binary = scratch / (Path(compiler).name + '-source')
            dep = binary.with_suffix('.d')
            command = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                       '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'tests'), '-I' + str(scratch),
                       str(ROOT / TEST), '-o', str(binary), '-MD', '-MF', str(dep)]
            if args.sanitizers:
                command.extend(('-fsanitize=address,undefined', '-fno-omit-frame-pointer'))
            started = time.monotonic()
            built = subprocess.run(command, capture_output=True, text=True, timeout=600)
            if built.returncode:
                raise ValueError('Codec copy source test compile failed: ' + built.stderr[-12000:])
            for value in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]):
                path = Path(value).resolve()
                if path.is_relative_to(ROOT):
                    name = str(path.relative_to(ROOT))
                    if name in before:
                        inputs_used.add(name)
            result = dict(compiler=compiler, command=command, modes=[])
            report['compilers'].append(result)
            for mode, flags, run in (('default', {}, [str(fixture)]),
                                     ('own-reference', {'HALO2_REFERENCE_CODEC_COPY_ROWS': '1'}, [str(fixture)]),
                                     ('unrelated-control', {'HALO2_REFERENCE_CODEC_SCALE': '1'}, [str(fixture)]),
                                     ('guards', {}, ['--guards'])):
                if mode not in selected_modes:
                    continue
                done = subprocess.run([str(binary), *run], env=env | flags, capture_output=True, text=True,
                                      timeout=args.execution_timeout)
                if done.returncode:
                    raise ValueError(f'Actual codec copy source {compiler} {mode} failed: ' + (done.stdout + done.stderr)[-16000:])
                result['modes'].append(dict(name=mode, result=json.loads(done.stdout)))
                save()
                print(f'{compiler} {mode} passed', flush=True)
            if args.semantic_mutants:
                result['compiled_defects'] = []
                for name, header, old, new, expected in DEFECTS:
                    relative = RUNTIME + header
                    text = (ROOT / relative).read_text()
                    if text.count(old) != 1:
                        raise ValueError('Codec copy source defect seam changed: ' + name)
                    overlay = scratch / (Path(compiler).name + '-' + name)
                    target = overlay / relative
                    target.parent.mkdir(parents=True)
                    target.write_text(text.replace(old, new, 1))
                    mutant = overlay / 'probe'
                    command_mutant = [command[0], '-I' + str(overlay / 'src'), *command[1:]]
                    command_mutant[command_mutant.index('-o')+1] = str(mutant)
                    command_mutant[command_mutant.index('-MF')+1] = str(overlay / 'probe.d')
                    done = subprocess.run(command_mutant, capture_output=True, text=True, timeout=600)
                    if done.returncode:
                        raise ValueError('Codec copy source mutant failed to compile: ' + name + ' ' + done.stderr[-9000:])
                    done = subprocess.run([str(mutant), str(mutant_fixture)], env=env, capture_output=True, text=True, timeout=300)
                    diagnostic = done.stdout + done.stderr
                    if not done.returncode or expected not in diagnostic:
                        raise ValueError('Codec copy source defect survived: ' + name + ' ' + diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name, rejected=True, diagnostic=diagnostic[-7000:]))
                    save()
            result['elapsed_seconds'] = round(time.monotonic() - started, 3)
        report.update(passed=True,status='passed')
    except BaseException as error:
        report.update(status='failed',error=repr(error))
        raise
    finally:
        inputs_used.update(str(Path(module.__file__).resolve().relative_to(ROOT)) for module in tuple(sys.modules.values())
                           if getattr(module, '__file__', None) and Path(module.__file__).resolve().is_relative_to(ROOT / 'tools'))
        report['source_before'] = {name: before[name] for name in sorted(inputs_used)}
        report['source_after'] = {name: digest((ROOT / name).read_bytes()) for name in report['source_before']}
        if report['source_before'] != report['source_after']:
            report.update(passed=False,status='failed',source_drift=True)
        save()
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
