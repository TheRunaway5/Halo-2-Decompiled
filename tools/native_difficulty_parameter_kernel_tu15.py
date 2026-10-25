#!/usr/bin/env python3
"""Install qualified semantic difficulty kernels without changing literal bodies."""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re

from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = '// BEGIN native difficulty parameter include\n#include "gameplay/objects/source_difficulty_parameter_native_tu15_internal.hpp"\n// END native difficulty parameter include\n'
PINS = {
    'actual': {0x1f1950: '64a8a1b4e72bcf082886024f5df3b6ce28afe96cd3d3afa82523e7250ba03929',
               0x1f19b0: '7b187f73c9e0c1d54d1bb78150cf63d17f5c10f6bc808e7c958f01155e2830f5'},
    'canonical': {0x1ed030: 'ab52b8a44c20e1857c43fd7a6931e6acf73bf01b6d53cd3ec355ce2ce106e72f',
                  0x1ed090: '904041ee840a911d00045aa5c9171543d628be234637fcc0e26f46f79333a5ba'},
    'delivery': {0x1ed030: '66215c67167e49cb6f8349ccc14c9bb0baf153a93195e9771a6b5e478db49a99',
                 0x1ed090: 'd4a7ecac1bdcaaa14dde77ca022cb89fee8a96b0fe3323f54991da35e4b6bef7'},
}


def route(entry, kind):
    parameter = entry in (0x1ed030, 0x1f1950)
    kernel = 'difficulty_parameter_native_tu15' if parameter else 'current_difficulty_parameter_native_tu15'
    observe = {'actual': 'actual', 'canonical': 'canonical',
               'delivery': 'damage_observation_pc(canonical,actual,s)'}[kind]
    return ('// BEGIN native difficulty parameter route\n'
        'if(s.image==SourcePhysicsContactImage::TitleUpdate15) {\n'
        f'    {kernel}(s,[&](std::uint32_t canonical,std::uint32_t actual){{\n'
        f'        if(s.services.instruction)s.services.instruction({observe});\n'
        '    });\n    return;\n}\n// END native difficulty parameter route\n')


def strip_native_difficulty_routes(source):
    for kind, entries in PINS.items():
        for entry in entries:
            source = source.replace(route(entry, kind), '')
    source = source.replace(INCLUDE, '')
    if 'BEGIN native difficulty parameter' in source or 'END native difficulty parameter' in source:
        raise ValueError('Native difficulty route changed; preserve and re-audit')
    return source


def install_native_difficulty_routes(source, kind):
    source = strip_native_difficulty_routes(source)
    state = 'ObservedDamageState' if kind == 'delivery' else 'WorldState'
    found = 0
    for entry, pin in PINS[kind].items():
        anchor = f'static void run_{entry:x}({state} &s) {{\n'
        if anchor not in source: continue
        if source.count(anchor) != 1: raise ValueError('Duplicate difficulty owner')
        start = source.index(anchor); end = source.index('\n}', start) + 2
        if hashlib.sha256(source[start:end].encode()).hexdigest() != pin:
            raise ValueError('Complete qualified difficulty literal body changed: ' + hex(entry))
        source = source.replace(anchor, anchor + route(entry, kind), 1); found += 1
    if found not in (1, 2): raise ValueError('No qualified difficulty owner in composition')
    return INCLUDE + source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--negative-control', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    qualification = json.loads(args.qualification.read_text())
    negative = json.loads(args.negative_control.read_text())
    kernel = ROOT / 'src/gameplay/objects/source_difficulty_parameter_native_tu15_internal.hpp'
    if (not qualification['passed'] or qualification['cases'] != 528 or qualification['fault_cases'] != 40 or
            qualification['native_kernel_sha256'] != sha(kernel) or not qualification['source_unchanged'] or
            not qualification['instruction_coverage_complete'] or not negative['passed'] or
            not negative['comparison_rejected_mutation'] or negative['negative_control']['before'] != sha(kernel)):
        raise ValueError('Current native kernel needs original, fault and deliberate-mutation qualification')
    changes = {}; rows = []
    for name, record in qualification['literal_source_bodies'].items():
        path = ROOT / name
        before = path.read_text()
        if sha(path) != record['sha256']: raise ValueError('Source changed since native qualification: ' + name)
        kind = 'actual' if '/ai/' in name else 'delivery' if 'damage_delivery' in name else 'canonical'
        after = install_native_difficulty_routes(before, kind)
        if strip_native_difficulty_routes(after) != before:
            raise ValueError('An unrelated literal body would change')
        changes[name] = after
        rows.append(dict(path=name, before=sha(path), after=hashlib.sha256(after.encode()).hexdigest(),
            route=kind, literal_bodies_preserved=True, body_pins=record['bodies']))
    args.output.mkdir(parents=True, exist_ok=False)
    for row in rows:
        path = ROOT / row['path']; before = path.read_text()
        if sha(path) != row['before']: raise ValueError('Source changed during scoped native admission')
        after = changes[row['path']]; path.write_text(after)
        if sha(path) != row['after']: raise ValueError('Native route write differs')
        (args.output / (path.name + '.patch')).write_text(''.join(difflib.unified_diff(
            before.splitlines(True), after.splitlines(True), fromfile='before/' + row['path'],
            tofile='after/' + row['path'], n=2)))
    report = dict(passed=True, scope='Qualified native routes installed; live rebuilt gameplay remains required',
        routes=rows, native_kernel_sha256=sha(kernel), qualification_sha256=sha(args.qualification),
        negative_control_sha256=sha(args.negative_control), installer_sha256=sha(Path(__file__)),
        original_instruction_changes=[], gameplay_verified=False, launcher_installed=False, full_vehicle_parity=False)
    (args.output / 'admission.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, routes=len(rows), report=str(args.output / 'admission.json'))), flush=True)


if __name__ == '__main__':
    main()
