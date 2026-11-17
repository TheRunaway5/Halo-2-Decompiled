#!/usr/bin/env python3
"""Compare native 3EB10 with its actual TU1.5 and Map Pack retained sources."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import subprocess
import sys

from oracle_smg_firing_1_1 import Xbe
import native_indexed_datum_gate_replacements as binding
from specialize_native_operands import specialize
from validate_native_indexed_datum_gate_frame_tu15 import MAPPACK_SHA, PROFILE, corpus

ROOT = Path(__file__).resolve().parents[1]
FRAME = 'src/gameplay/objects/native_tu15_indexed_datum_gate_source_frame.hpp'
DRIVER = 'tests/native_tu15_indexed_datum_gate_source_tests.cpp'
CONFIG = {
    'Primary': ('src/physics/physics_live_support_tu15_generated.cpp',
                'generate_physics_live_support_tu15.py', 'source_run_physics_live_support_tu15',
                'ed2f524c9d73ccb277241b16f8ad038116dd68c3a8a9493d34cd1716437a68c4'),
    'Particle': ('src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp',
                 'generate_effect_particle_frame_tu15.py', 'source_run_effect_particle_frame_tu15',
                 '4d5fa54cdba4b296fbd4a5c9a5c8f214db1d3aad96b6df06f083bdf4ae2eaa8e'),
    'Death': ('src/gameplay/units/death_startup_runtime_generated.cpp',
              'generate_death_startup_runtime_1_1.py', 'source_run_death_startup_bodies_1_1',
              '23c0d446fa8388321a6b56e749b3f53a7c9dd6ee3cffac5c131e07c51a257f1c'),
}
BODY = re.compile(r'^(?:\[\[gnu::noinline\]\] )?static void run_3eb10\([^)]*\) \{.*?^\}', re.M | re.S)
WORLD = re.compile(r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;', re.M | re.S)
DEATH = re.compile(r'^struct State : physics_contact_runtime_detail::State \{.*?^\};', re.M | re.S)
SELECTED = re.compile(r'^std::uint32_t selected\([^\n]+', re.M)
SIGN_EXTEND = re.compile(r'^std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?^\}', re.M | re.S)
SELECTED_SITE = re.compile(r'selected\((0x[0-9a-f]+u),s\.image\)')
CALLBACK = re.compile(r's\.(?:services\.instruction|instruction_literal)\((0x[0-9a-f]+u)\)')
MUTANTS = (
    ('wrong-selection-word', 'read(Addresses.selection,2)', 'read(Addresses.selection+2u,2)'),
    ('wrong-sign-extension', '(read(g[1]+g[6]*2u,2)^0x8000u)-0x8000u', 'read(g[1]+g[6]*2u,2)'),
    ('lose-ret-callback', 'step(0x3eb5au);s.pop()', 's.pop()'),
)


def sha(value):
    if isinstance(value, Path):
        value = value.read_bytes()
    if isinstance(value, str):
        value = value.encode()
    return hashlib.sha256(value).hexdigest()


def one(pattern, text):
    matches = pattern.findall(text)
    if len(matches) != 1:
        raise ValueError(f'Expected one authenticated source pattern: {pattern.pattern}')
    return matches[0]


def run(command, timeout=600):
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=timeout)
    if result.returncode:
        raise ValueError(f'{Path(command[0]).name} exited {result.returncode}: ' +
                         (result.stdout + result.stderr)[-8000:])
    return result.stdout


def project(body, resolver):
    constants = set(SELECTED_SITE.findall(body))
    if constants:
        mapping = dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);', resolver))
        if any(mapping.get(token) != token for token in constants):
            raise ValueError('3EB10 retained operand is not authenticated as identity')
    body = SELECTED_SITE.sub(r'\1', body)
    body = re.sub(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(([^)]+)\);\}',
                  r's.instruction_literal(\1);', body)
    return body


def export_route(source, symbol, death):
    before, sep, tail = source.partition('void '+symbol+'(')
    if not before or not sep or 'case 0x3eb10u:run_3eb10(s);break;' not in tail:
        raise ValueError('Missing original exported 3EB10 dispatch')
    constructor = ('AnimationSelectionState s{m,observe,retained,services};s.load(abi);' if death else
                   'constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;\n'
                   '    FpScope scope(fp);LiveState s{m,observe,services,image};s.load(abi);')
    if constructor not in tail[:800]:
        raise ValueError('Original 3EB10 source image admission changed')


def prepare_owner(label, spec, scratch, require_binding):
    unit, generator, symbol, raw_pin = spec
    death = label == 'Death'
    fresh = scratch/(label+'.cpp')
    generation = [sys.executable, '-B', str(ROOT/'tools'/generator), '--output', str(fresh)]
    generation += (['--directory', str(scratch/'particle-input')] if label == 'Particle' else
                   ['--manifest', str(scratch/(label+'.json'))])
    run(generation)
    canonical = (('Current', (ROOT/unit).read_text()), ('Fresh', fresh.read_text()))
    if require_binding:
        for _, source in canonical:
            binding.installed_entries(source)
    inputs = tuple((name,binding.remove_indexed_datum_gate(source)) for name,source in canonical)
    current = inputs[0][1]
    baseline = one(BODY, current)
    if sha(baseline) != raw_pin:
        raise ValueError(f'{label} exact current 3EB10 body changed')
    projected = project(baseline, one(SELECTED, current) if not death else '')
    code, evidence = [], []
    for version, raw in inputs:
        namespace = label.lower()+version.lower()
        export_route(raw, symbol, death)
        state = one(DEATH if death else WORLD, raw)
        if death and 'SourcePhysicsContactImage::MapPack11},animation(owners)' not in state:
            raise ValueError('Missing genuine Map Pack 1.1 state construction')
        resolver = one(SELECTED, raw) if not death else ''
        raw_body = one(BODY, raw)
        if project(raw_body, resolver) != projected:
            raise ValueError(f'{label} {version} retained 3EB10 body changed')
        forms = [('raw', raw, raw_body)]
        if not death:
            prepared = specialize(raw)[0]
            export_route(prepared, symbol, False)
            prepared_body = one(BODY, prepared)
            if one(WORLD, prepared) != state or prepared_body != projected:
                raise ValueError(f'{label} {version} prepared 3EB10 transport changed')
            forms.append(('prepared', prepared, prepared_body))
        if len(set(CALLBACK.findall(raw_body))) != 25:
            raise ValueError('3EB10 source callback sites changed')
        extend = one(SIGN_EXTEND, raw)
        code += ['namespace '+namespace+' {', 'using namespace halo2::physics_contact_runtime_detail;' if death
                 else 'using namespace halo2::physics_world_runtime_detail;', state, extend]
        if SELECTED_SITE.search(raw_body):
            code += [resolver]
        for form, whole, body in forms:
            code.append(body.replace('run_3eb10(', form+'(', 1))
            evidence.append(dict(label=label+version, form=form, unit_sha256=sha(whole),
                                 body_sha256=sha(body), state_sha256=sha(state),
                                 sign_extend_sha256=sha(extend),
                                 resolver_sha256=sha(resolver) if resolver else None))
        code += ['}', f'struct {label}{version} {{',
                 f'using State={namespace}::{"State" if death else "WorldState"};',
                 f'static constexpr bool map_pack={"true" if death else "false"};',
                 f'static constexpr auto addresses={"map_pack_11" if death else "title_update_15"};',
                 f'static constexpr auto label="{label}{version}";',
                 'template<bool Literal> static void retained(State& s){',
                 f'static_assert(!Literal);{namespace}::raw(s);' if death else
                 f'if constexpr(Literal){namespace}::prepared(s);else {namespace}::raw(s);',
                 '}};']
    return code, evidence, generation


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--mappack', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--compiler', action='append')
    p.add_argument('--sanitizers', action='store_true')
    p.add_argument('--semantic-mutants', action='store_true')
    p.add_argument('--require-binding', action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Use tools/run_test.py')
    if (platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686')
            or not a.xbe.is_file() or not a.mappack.is_file()):
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    a.output.mkdir(parents=True, exist_ok=False)
    files = {FRAME, DRIVER, PROFILE.kernel, str(Path(__file__).relative_to(ROOT)),
             *(spec[0] for spec in CONFIG.values()), *('tools/'+spec[1] for spec in CONFIG.values())}
    for module in tuple(sys.modules.values()):
        file = getattr(module, '__file__', None)
        if file and (path := Path(file).resolve()).is_relative_to(ROOT/'tools'):
            files.add(str(path.relative_to(ROOT)))
    pins = {name: sha(ROOT/name) for name in sorted(files)}
    headers = {path.resolve(): sha(path) for tree in ('include', 'src', 'tests')
               for path in (ROOT/tree).rglob('*') if path.suffix in ('.hpp', '.h', '.inc')}
    proof = dict(passed=False, owned_scratch=str(scratch), installed_context_authenticated=a.require_binding,
                 retained_large_artifacts=[], compilers=[])
    try:
        if sha(a.xbe) != PROFILE.image_sha256 or sha(a.mappack) != MAPPACK_SHA:
            raise ValueError('Incorrect original 3EB10 images')
        with ThreadPoolExecutor(max_workers=3) as pool:
            jobs = [pool.submit(prepare_owner, label, spec, scratch, a.require_binding) for label, spec in CONFIG.items()]
            prepared = [job.result() for job in jobs]
        (scratch/'indexed_datum_source_owners.inc').write_text('\n'.join(
            row for code, _, _ in prepared for row in code)+'\n')
        proof['contexts'] = [row for _, evidence, _ in prepared for row in evidence]
        proof['generation'] = [command for _, _, command in prepared]
        fixture = scratch/'inputs.json'
        fixture.write_text(json.dumps(dict(tu15=corpus(Xbe(a.xbe)),
            mappack11=corpus(Xbe(a.mappack), True)), separators=(',', ':')))
        proof['fixture_sha256'] = sha(fixture)
        for compiler in a.compiler or ('g++', 'clang++'):
            base = [compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                    *('-I'+str(ROOT/tree) for tree in ('include','src','tests')), '-I'+str(scratch)]
            if a.sanitizers:
                base += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
            binary, dep = scratch/Path(compiler).name, scratch/'driver.d'
            cmd = base+['-MD', '-MF', str(dep), str(ROOT/DRIVER), '-o', str(binary)]
            run(cmd)
            for item in shlex.split(dep.read_text().replace(chr(92)+'\n', ' ').split(':',1)[1]):
                if (path := Path(item).resolve()) in headers:
                    pins[str(path.relative_to(ROOT))] = headers[path]
            command = [str(binary), str(fixture)]
            result = json.loads(run(command))
            row = dict(compiler=compiler, command=cmd, result=result, rejected_defects=[])
            proof['compilers'].append(row)
            if a.semantic_mutants:
                mutation = scratch/'mutation'
                staged = mutation/FRAME.removeprefix('src/')
                staged.parent.mkdir(parents=True, exist_ok=True)
                original = (ROOT/FRAME).read_text()
                for name, old, new in MUTANTS:
                    if original.count(old) != 1:
                        raise ValueError('Missing exact 3EB10 defect anchor: '+name)
                    staged.write_text(original.replace(old,new,1))
                    run([base[0], '-I'+str(mutation), *base[1:], str(ROOT/DRIVER), '-o', str(binary)])
                    tested = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=600)
                    if tested.returncode != 1 or '3EB10 retained source differs:' not in tested.stderr:
                        raise ValueError('3EB10 compiled defect not rejected: '+name+' '+tested.stderr[-4000:])
                    row['rejected_defects'].append(dict(name=name, evidence=tested.stderr.strip()))
        proof['passed'] = True
    except Exception as error:
        proof['error'] = repr(error)
        raise
    finally:
        proof['source_before'] = pins
        proof['source_after'] = {name: sha(ROOT/name) for name in pins}
        if proof['source_after'] != pins:
            proof['passed'] = False
        (a.output/'report.json').write_text(json.dumps(proof, indent=2, sort_keys=True)+'\n')
    return 0 if proof['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
