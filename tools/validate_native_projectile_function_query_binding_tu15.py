#!/usr/bin/env python3
"""Qualify exact current/fresh CombatAction projectile binding and prepared retagging."""
import argparse
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys
from pathlib import Path

import native_projectile_function_query_replacements as bind
from specialize_native_operands import specialize

ROOT = Path(__file__).resolve().parents[1]
UNIT = 'src/gameplay/units/source_combat_action_tu15_generated.cpp'
GENERATOR = 'tools/generate_combat_action_tu15.py'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def reject(function, *args):
    try:
        function(*args)
    except ValueError:
        return
    raise ValueError('Projectile query accepted changed owner context')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--compiler', default='c++')
    p.add_argument('--require-binding', action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    a.output.mkdir(parents=True, exist_ok=False)
    names = (UNIT, GENERATOR, 'tools/generate_physics_live_support_tu15.py',
             'tools/specialize_native_operands.py', 'tools/native_projectile_function_query_replacements.py',
             'tools/validate_native_projectile_function_query_binding_tu15.py',
             'src/gameplay/objects/native_tu15_projectile_function_query_source_frame.hpp',
             'src/gameplay/objects/native_tu15_projectile_function_query_source_owner.hpp',
             'include/halo2/native_tu15_projectile_function_query_kernel.hpp')
    before = {name:digest(ROOT/name) for name in names}
    headers = {p.resolve():digest(p) for tree in ('include','src') for p in (ROOT/tree).rglob('*')
               if p.suffix in ('.hpp','.h','.inc')}
    report = dict(passed=False, owned_scratch=str(scratch), retained_large_artifacts=[], source_before=before)
    try:
        fresh = scratch/'fresh.cpp'
        command = [sys.executable,'-B',str(ROOT/GENERATOR),'--output',str(fresh),'--directory',str(scratch/'generation')]
        generated = subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=600)
        report['generation'] = dict(command=command, exit_code=generated.returncode, stdout=generated.stdout[-4000:])
        if generated.returncode:
            raise ValueError('Actual CombatAction generation failed: '+generated.stderr[-6000:])
        current = (ROOT/UNIT).read_text()
        fresh_text = fresh.read_text()
        report['production_bound'] = bool(bind.authenticate(current)[3])
        if a.require_binding:
            bind.installed_entries(current)
            bind.installed_entries(fresh_text)
        from native_object_definition_mode_query_replacements import remove_object_definition_mode_query
        from native_object_table_ratio_replacements import remove_object_table_ratio
        raw = {name:bind.remove_projectile_function_query(remove_object_definition_mode_query(remove_object_table_ratio(text)))
               for name,text in (('current',current),('fresh',fresh_text))}
        if re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',bind.body(raw['current'])) != bind.body(raw['fresh']):
            raise ValueError('Fresh projectile body differs beyond authenticated identity substitution')
        report['exact_raw_identity_projection'] = True
        report['contexts'] = []
        for origin, source in raw.items():
            prepared = specialize(source)[0]
            installed = bind.replace_projectile_function_query(source)
            specialized, receipt = specialize(installed)
            official_retag = receipt.get('native_projectile_function_query_entries') == ['100e70']
            for literal, base in ((False,source),(True,prepared)):
                candidate = bind.replace_projectile_function_query(base)
                if bind.replace_projectile_function_query(candidate) != candidate or bind.remove_projectile_function_query(candidate) != base:
                    raise ValueError('Projectile query overlay is not exactly reversible/idempotent')
                retagged = specialized if official_retag else bind.retag_specialized(installed,specialized)
                if literal and retagged != candidate:
                    raise ValueError('Projectile query prepared retag differs from direct prepared insertion')
                profile = origin+('-prepared' if literal else '-raw')
                original_body = bind.body(candidate)
                entry = bind.prefix(literal)
                controls = (
                    candidate.replace(bind.INCLUDE,'',1),
                    bind.INCLUDE+candidate.replace(bind.INCLUDE,'',1),
                    bind.INCLUDE+candidate,
                    candidate.replace(entry,bind.prefix(not literal),1),
                    candidate.replace(bind.ORDERED_MULTIPLY,bind.LEGACY_MULTIPLY,1),
                    candidate.replace(original_body,original_body.replace(entry,'',1)[:-2]+entry+'\n}',1),
                    candidate+'\n// '+bind.BRIDGE+'\n',
                    candidate.replace(original_body,original_body.replace('0x100e70u','0x100e71u',1),1),
                    candidate.replace('static void dispatch(WorldState &s,','static void dispatch(WorldState &s, ',1),
                    candidate.replace('using physics_world_runtime_detail::WorldState::WorldState;',
                        'using physics_world_runtime_detail::WorldState::WorldState; /* changed owner */',1),
                    candidate.replace('case 0x100e70u:run_100e70(s);break;','case 0x100e70u:break;',1),
                    candidate+'\nnamespace unexpected = halo2::'+bind.OWNER_NAMESPACE+';\n',
                    candidate+'\nusing namespace halo2::'+bind.CORE_NAMESPACE+';\n',
                    candidate+'\n#include <gameplay/objects/'+bind.INCLUDE_BASENAME+'>\n',
                )
                for changed in controls:
                    if changed == candidate:
                        raise ValueError('Missing projectile strict mutation seam in '+profile)
                    reject(bind.installed_entries,changed)
                if not literal:
                    reject(bind.retag_specialized,controls[3],specialized)
                    if official_retag:reject(specialize,controls[3])
                output = scratch/(profile+'.cpp');output.write_text(candidate)
                dep = output.with_suffix('.d')
                command = [a.compiler,'-std=c++20','-fsyntax-only','-MD','-MF',str(dep),
                           '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(output)]
                checked = subprocess.run(command,capture_output=True,text=True,timeout=600)
                if checked.returncode:
                    raise ValueError('Whole projectile candidate syntax failed '+profile+': '+checked.stderr[-6000:])
                for item in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(item).resolve()
                    if path in headers:before[str(path.relative_to(ROOT))]=headers[path]
                report['contexts'].append(dict(profile=profile,sha256=digest(output),strict_refusals=len(controls)+(not literal),
                                               cpp_syntax=True,command=command,official_retag=official_retag))
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        report['source_after']={name:digest(ROOT/name) for name in before}
        if report['source_after'] != before:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
