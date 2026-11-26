#!/usr/bin/env python3
"""Qualify the native E4DC0 phase and its pinned TU1.5 reference route."""

import argparse
from contextlib import nullcontext
import json
import os
from pathlib import Path
import re
import subprocess
from unittest.mock import patch

from instrument_native_execution import instrument_translation
import native_physical_result_activity_replacements as activity
import native_physical_result_contact_e4dc0_replacements as contact
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/physics_live_support_tu15_generated.cpp'
FIXTURE = 'tests/native_tu15_e4dc0_contact_transition_tests.cpp'
REFERENCE = {'HALO2_REFERENCE_PHYSICAL_RESULT_ACTIVITY', 'HALO2_REFERENCE_PHYSICAL_RESULT_CONTACT_E4DC0'}


def invoke(command, scratch):
    env = {name: value for name, value in os.environ.items() if name not in REFERENCE}
    run = subprocess.run([str(arg) for arg in command], cwd=scratch, env=env, capture_output=True, text=True)
    if run.returncode:
        raise ValueError(f'{command[0]} failed ({run.returncode})\n{run.stdout[-1200:]}{run.stderr[-5000:]}')
    return run.stdout


def exports(original, selected):
    stage = re.sub(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',
                   r's.instruction_literal(\1);', contact.retained(original)).removeprefix(contact.ENTRY_LABEL)
    hook = activity.route_hook('update_contact_transition', contact.CHILDREN)
    if selected.count(hook) != 1 or selected.count(')goto Le4f90;') != 1:
        raise ValueError('Active physical contact continuation or child hook changed')
    hook = hook.replace('(s,[](auto &v,', '(s,[&](auto &v,')
    for site, target, _, _ in contact.CHILDREN:
        old = f'run_{target}(s);'
        # The two 1CC310 sites share a genuine owner but different return addresses.
        prefix = f's.push(0x{site}u,'
        body_line = next(line for line in stage.splitlines() if prefix in line)
        if body_line.count(old) != 1:
            raise ValueError(f'Physical contact fixture child {site} changed')
        stage = stage.replace(body_line, body_line.replace(old, f'child(s,0x{site}u,0x{target}u,ctx);'))
        route = f'case 0x{site}u: run_{target}(v); return;'
        if hook.count(route) != 1:
            raise ValueError(f'Active physical contact fixture child {site} changed')
        hook = hook.replace(route, f'case 0x{site}u: child(v,site,target,ctx); return;')
    return ('void retained(WorldState &s, ChildContext &ctx) {\n' + stage +
            'Le4f90: return;\n}\n' + f'bool selected(WorldState &s, ChildContext &ctx) {{return {hook};}}\n')


def staged_specialize(canonical):
    active, metadata = specialize(canonical)
    if metadata.get('native_spatial_owners', []).count('e4dc0'):
        raise ValueError('The staged validation requires the active 213-site checkpoint')
    active, entries = contact.install(canonical, active)
    if entries != ['e4dc0']:
        raise ValueError('Isolated contact binding refused the pinned checkpoint')
    return active, metadata


def qualify(args, scratch, report):
    canonical = (ROOT/SOURCE).read_text()
    original = contact.canonical_stage(canonical)
    prepare = staged_specialize if args.staged_include else specialize
    active, metadata = prepare(canonical)
    if not args.staged_include and metadata.get('native_spatial_owners', []).count('e4dc0') != 1:
        raise ValueError('Contact production binding is missing')
    selected = activity.BODY.search(active)[0]
    header = (args.staged_include or ROOT/'src') / activity.HEADER
    if not header.is_file():
        raise ValueError('Contact adapter header is missing')
    (scratch/'contact_transition_reference.inc').write_text(exports(original, selected))
    sites = {int(pc,16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',original)}
    # Accounting reconstructs the complete specialized body to qualify its
    # retained alias. Supply the same staged specialization on that check.
    specialization = patch('specialize_native_operands.specialize', staged_specialize) if args.staged_include else nullcontext()
    with specialization:
        accounted, manifest, _ = instrument_translation(active, canonical, SOURCE, ROOT)
    owner, = (row for row in manifest['functions'] if row['symbol'] == 'e4a40')
    if len(sites) != 123 or owner['literal_sites'] != 565 or owner['dynamic_instruction_forwarders']:
        raise ValueError('Physical contact phase lost its exact 123-site owner accounting')
    report.update(stage_sites=len(sites), canonical_stage_sha256=contact.RAW_PIN, prepared_stage_sha256=contact.PREPARED_PIN,
        previous_owner_sha256=contact.OWNER_PIN, bound_owner_sha256=contact.digest(selected),
        adapter_sha256=contact.digest(header.read_text()), fixture_sha256=contact.digest((ROOT/FIXTURE).read_text()),
        binder_sha256=contact.digest(Path(contact.__file__).read_text()), profiles={})
    prefix = [args.staged_include] if args.staged_include else []
    paths = [*prefix, ROOT/'include',ROOT/'src',ROOT/'tests',scratch]
    flags = ['-std=c++20','-O2','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',*(f'-I{p}' for p in paths)]
    if args.full_syntax:
        for name, source in (('ordinary',active),('accounted',accounted)):
            path = scratch/f'{name}.cpp'
            path.write_text(source)
            invoke([args.compiler,*flags,'-fsyntax-only',path],scratch)
        report['full_production_aliases'] = ['ordinary','accounted']
    profiles = [('release',[]),('asan_ubsan',['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                '-fno-sanitize-recover=all','-fno-pie','-no-pie'])]
    for name, extra in profiles:
        binary = scratch/name
        invoke([args.compiler,*flags,*extra,ROOT/FIXTURE,'-o',binary],scratch)
        results = json.loads(invoke([binary],scratch))
        observed = set(results.pop('sites'))
        results.update(observed_sites=len(observed), missing_sites=sorted(hex(pc) for pc in sites-observed))
        report['profiles'][name] = results
        print(name,results,flush=True)
        if observed != sites or set(results['call_sites']) != {int(row[0],16) for row in contact.CHILDREN}:
            raise ValueError('Physical contact fixture omitted a source site or child call')
    report['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--staged-include', type=Path)
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = dict(passed=False, compiler=args.compiler, scratch=str(scratch), staged=bool(args.staged_include))
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
