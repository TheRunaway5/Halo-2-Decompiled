#!/usr/bin/env python3
"""Check the call-free physical-result decay and both original RET12 paths."""

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
import native_physical_result_decay_e51d6_replacements as decay
from specialize_native_operands import specialize


ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'src/physics/physics_live_support_tu15_generated.cpp'
FIXTURE = 'tests/native_tu15_e51d6_physical_result_decay_tests.cpp'
REFERENCE = {'HALO2_REFERENCE_PHYSICAL_RESULT_ACTIVITY', 'HALO2_REFERENCE_PHYSICAL_RESULT_DECAY_E51D6'}


def invoke(command, scratch):
    env = {name: value for name, value in os.environ.items() if name not in REFERENCE}
    run = subprocess.run([str(arg) for arg in command], cwd=scratch, env=env, capture_output=True, text=True)
    if run.returncode:
        raise ValueError(f'{command[0]} failed ({run.returncode})\n{run.stdout[-1200:]}{run.stderr[-5000:]}')
    return run.stdout


def exports(original, selected):
    for pc, label in ((decay.ENTRY,'decay'), (decay.BLOCKED,'blocked')):
        if selected.count(decay.hook(pc, label)) != 1:
            raise ValueError('Active physical-result tail route changed')
    stage = re.sub(r'if\(s.services.instruction\)\{s.services.instruction\((0x[0-9a-f]+u)\);\}',
                   r's.instruction_literal(\1);', decay.retained(original)).removeprefix(f'L{decay.ENTRY}: ')
    return (f'void retained(WorldState &s, Entry entry) {{\n    if(entry==Entry::blocked)goto L{decay.BLOCKED};\n' +
            stage + '}\n' + f'bool selected(WorldState &s, Entry entry) {{return {decay.DETAIL}::finish(s,entry);}}\n')


def staged_specialize(canonical):
    active, metadata = specialize(canonical)
    if metadata.get('native_spatial_owners', []).count(decay.ENTRY):
        raise ValueError('The staged validation requires the preceding 336-site checkpoint')
    active, entries = decay.install(canonical, active)
    if entries != [decay.ENTRY]:
        raise ValueError('Isolated physical-result tail refused the pinned checkpoint')
    return active, metadata


def qualify(args, scratch, report):
    canonical = (ROOT/SOURCE).read_text()
    original = decay.canonical_stage(canonical)
    prepare = staged_specialize if args.staged else specialize
    active, metadata = prepare(canonical)
    if not args.staged and metadata.get('native_spatial_owners', []).count(decay.ENTRY) != 1:
        raise ValueError('Physical-result tail production binding is missing')
    selected = activity.BODY.search(active)[0]
    (scratch/'physical_result_decay_reference.inc').write_text(exports(original, selected))
    sites = {int(pc,16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',original)}
    # The accounting binder reconstructs the original specialization when
    # qualifying renamed reference owners. Use the exact staged composition.
    specialization = patch('specialize_native_operands.specialize', staged_specialize) if args.staged else nullcontext()
    with specialization:
        accounted, manifest, _ = instrument_translation(active, canonical, SOURCE, ROOT)
    owner, = (row for row in manifest['functions'] if row['symbol'] == 'e4a40')
    if len(sites) != 60 or owner['literal_sites'] != 565 or owner['dynamic_instruction_forwarders']:
        raise ValueError('Physical-result tail lost its exact 60-site owner accounting')
    report.update(stage_sites=len(sites), canonical_stage_sha256=decay.RAW_PIN, prepared_stage_sha256=decay.PREPARED_PIN,
        previous_owner_sha256=decay.OWNER_PIN, bound_owner_sha256=decay.digest(selected), profiles={},
        sources={path:decay.digest((ROOT/path).read_text()) for path in (SOURCE,'src/'+decay.HEADER,
            'src/'+activity.HEADER,FIXTURE,'tools/native_physical_result_decay_e51d6_replacements.py')})
    paths = (ROOT/'include', ROOT/'src', ROOT/'tests', scratch)
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
        if observed != sites or set(results['exits']) != {0xe52ba,0xe52cb}:
            raise ValueError('Physical-result tail fixture missed a source site or a RET12 path')
    report['passed'] = True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--staged', action='store_true')
    parser.add_argument('--full-syntax', action='store_true')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = dict(passed=False, compiler=args.compiler, scratch=str(scratch), staged=args.staged)
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
