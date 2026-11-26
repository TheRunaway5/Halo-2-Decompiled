#!/usr/bin/env python3
"""Compare the active TU1.5 physical-result phase with its pinned reference."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from instrument_native_execution import instrument_translation
import native_physical_result_activity_replacements as binding
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/physics_live_support_tu15_generated.cpp'
FIXTURE = 'tests/native_tu15_physical_result_activity_tests.cpp'
HEADER = 'src/' + binding.HEADER


def invoke(command, scratch):
    env = {name: value for name, value in os.environ.items() if name != 'HALO2_REFERENCE_PHYSICAL_RESULT_ACTIVITY'}
    process = subprocess.run([str(arg) for arg in command], cwd=scratch, env=env, capture_output=True, text=True)
    if process.returncode:
        raise ValueError(f'{command[0]} failed ({process.returncode})\n{process.stdout[-1200:]}{process.stderr[-5000:]}')
    return process.stdout


def literal(stage):
    return re.sub(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',
                  r's.instruction_literal(\1);', stage)


def reference_children(stage, children):
    for site, target, _, dispatch in children:
        code = f'dispatch(s,target,0x{site}u,false);' if dispatch else f'run_{target}(s);'
        if stage.count(code) != 1:
            raise ValueError(f'Original activity fixture child {site} drifted')
        operand = 'target' if dispatch else f'0x{target}u'
        stage = stage.replace(code, f'child(s,0x{site}u,{operand},ctx);')
    return stage


def fixture_hook(hook, children):
    hook = hook.replace('(s,[](auto &v,std::uint32_t site,std::uint32_t target)',
                        '(s,[&](auto &v,std::uint32_t site,std::uint32_t target)')
    for site, target, _, dispatch in children:
        code = f'dispatch(v,target,0x{site}u,false);' if dispatch else f'run_{target}(v);'
        if hook.count(code) != 1:
            raise ValueError(f'Active activity fixture child {site} drifted')
        hook = hook.replace(code, 'child(v,site,target,ctx);')
    return hook


def motion(old, active):
    for api, bounds, children in (('project_result',binding.PROJECT,binding.PROJECT_CHILDREN),
                                  ('update_activity',binding.ACTIVITY,binding.ACTIVITY_CHILDREN)):
        expected = binding.route_hook(api,children)
        if active.count(expected) != 1:
            raise ValueError(f'Active physical-result {api} route changed')
    project = reference_children(literal(binding.retained_motion(old[binding.source_span(old,binding.PROJECT)],binding.PROJECT)),
                                 binding.PROJECT_CHILDREN)
    activity = reference_children(literal(binding.retained_motion(old[binding.source_span(old,binding.ACTIVITY)],binding.ACTIVITY)),
                                  binding.ACTIVITY_CHILDREN).removeprefix('Le4f90: ')
    return (f'''std::uint32_t retained_motion(WorldState &s, ChildContext &ctx, bool projection) {{
    if(projection) {{
{project}Le4d6d: return 0xe4d6du;
    }}
{activity}Le5094: return 0xe5094u;
Le51d6: return 0xe51d6u;
Le51cd: return 0xe51cdu;
}}
std::uint32_t selected_motion(WorldState &s, ChildContext &ctx, bool projection) {{
    if(projection) {{
        if({fixture_hook(binding.route_hook('project_result',binding.PROJECT_CHILDREN),binding.PROJECT_CHILDREN)}) return 0xe4d6du;
        return 0;
    }}
    switch({fixture_hook(binding.route_hook('update_activity',binding.ACTIVITY_CHILDREN),binding.ACTIVITY_CHILDREN)}) {{
    case {binding.DETAIL}::Alternate::apply_impulse: return 0xe5094u;
    case {binding.DETAIL}::Alternate::decay: return 0xe51d6u;
    case {binding.DETAIL}::Alternate::publish: return 0xe51cdu;
    case {binding.DETAIL}::Alternate::reference: return 0;
    }}
    return 0;
}}
''')


def export(old, active):
    sites = binding.ENTER.search(old).start()
    stage = reference_children(literal(binding.retained(old[:sites])),binding.CHILDREN)
    stage = stage.replace('static void run_e4a40(WorldState &s)', 'Continuation retained(WorldState &s, ChildContext &ctx)', 1)

    signature = 'static void run_e4a40(WorldState &s) {'
    end = f'    case {binding.DETAIL}::Continuation::reference: break;\n    }}\n'
    hook = fixture_hook(active[len(signature):active.index(end) + len(end)],binding.CHILDREN)
    continuations = ('    return Continuation::tick;\n'
                     'Le52bd: return Continuation::blocked;\n}\n')
    retained = stage + continuations
    selected = ('Continuation selected(WorldState &s, ChildContext &ctx) {\n' + hook +
                '    return Continuation::reference;\nLe4b81:\n' + continuations)
    return retained + '\n' + selected + '\n' + motion(old, active)


def qualify(args, scratch, report):
    canonical = (ROOT/SOURCE).read_text()
    old = binding.BODY.search(canonical)[0]
    active, metadata = specialize(canonical)
    bound = binding.BODY.search(active)[0]
    if metadata.get('native_spatial_owners', []).count('e4a40') != 1 or active.count(binding.INCLUDE) != 1:
        raise ValueError('Physical-result activity production binder is missing')
    stages = (old[:binding.ENTER.search(old).start()], old[binding.source_span(old,binding.PROJECT)],
              old[binding.source_span(old,binding.ACTIVITY)])
    authored_sites = {int(pc,16) for stage in stages for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',stage)}
    if len(authored_sites) != 213 or binding.sha(old) != binding.PIN:
        raise ValueError('Canonical physical-result activity stage changed')
    if bound.count('goto Le4b81;') != 1 or bound.count('case ' + binding.DETAIL + '::Continuation::blocked: goto Le52bd;') != 1:
        raise ValueError('Physical-result activity continuation changed')
    (scratch/'physical_result_activity_alias.inc').write_text(export(old, bound))
    accounted, manifest, _ = instrument_translation(active, canonical, SOURCE, ROOT)
    owner, = (row for row in manifest['functions'] if row['symbol'] == 'e4a40')
    if owner['literal_sites'] != 565 or owner['dynamic_instruction_forwarders']:
        raise ValueError('Physical-result activity production accounting changed')
    report.update(canonical_sha256=binding.PIN, prepared_sha256=binding.PREPARED, selected_sha256=binding.sha(bound),
        stage_sites=len(authored_sites), sources={name:binding.sha((ROOT/name).read_text()) for name in
           (SOURCE, HEADER, FIXTURE, 'tools/native_physical_result_activity_replacements.py')}, profiles={})
    flags = ['-std=c++20','-O2','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
             *(f'-I{path}' for path in (ROOT/'include',ROOT/'src',ROOT/'tests',scratch))]
    if args.full_syntax:
        for name, source in (('ordinary',active),('accounted',accounted)):
            path = scratch/f'{name}.cpp'
            path.write_text(source)
            invoke([args.compiler,*flags,'-fsyntax-only',path],scratch)
        report['full_production_aliases'] = ['ordinary','accounted']
    profiles = [('release',[]),('asan_ubsan',['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                '-fno-sanitize-recover=all','-fno-pie','-no-pie'])]
    for profile, extra in profiles:
        binary = scratch/profile
        invoke([args.compiler,*flags,*extra,ROOT/FIXTURE,'-o',binary],scratch)
        results = json.loads(invoke([binary],scratch))
        sites = set(results.pop('sites'))
        results['unexercised_sites'] = sorted(hex(site) for site in authored_sites - sites)
        results['observed_sites'] = len(sites)
        report['profiles'][profile] = results
        print(profile,results,flush=True)
        children = binding.CHILDREN + binding.PROJECT_CHILDREN + binding.ACTIVITY_CHILDREN
        if sites != authored_sites or set(results['children']) != {int(row[1],16) for row in children}:
            raise ValueError('Physical-result activity fixture omitted a parent or true child site')
        if set(results['destinations']) != {0xe4b81,0xe52bd,0xe4d6d,0xe5094,0xe51d6,0xe51cd}:
            raise ValueError('Physical-result activity fixture missed a continuation')
    report['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = dict(passed=False, compiler=args.compiler, scratch=str(scratch))
    try:
        qualify(args,scratch,report)
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
