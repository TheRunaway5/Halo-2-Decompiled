#!/usr/bin/env python3
"""Check selected TU1.5 shield recovery against the pinned translated source."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

import native_damage_update_shield_recovery_replacements as binding
from specialize_native_operands import specialize
from instrument_native_execution import instrument_translation
from validate_native_damage_update_entry_tu15 import reports_native_damage_update


ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT / 'src/gameplay/units/source_damage_delivery_runtime_generated.cpp'
HEADER = ROOT / 'src/gameplay/units/native_tu15_damage_update_shield_recovery.hpp'
TEST = ROOT / 'tests/native_tu15_damage_update_shield_recovery_tests.cpp'


def run(command, scratch, **kwargs):
    return subprocess.run(command, cwd=scratch, text=True, check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    canonical = UNIT.read_text()
    if not binding.admitted(canonical):
        raise ValueError('Damage update return reference drift')
    original = binding.owner.body(canonical)[1]
    prepared, receipt = specialize(canonical)
    if (not reports_native_damage_update(receipt)
            or prepared.count('native_damage_update_shield_recovery_detail::bridge') != 2
            or binding.install(canonical, prepared) != (prepared, [])):
        raise ValueError('Damage shield recovery expects both original entry hooks')
    mutations = {
        'owner': canonical.replace(binding.prefix.OWNER_INCLUDE, '#include "halo2/source_vitality_runtime.hpp"\n', 1),
        'selector': canonical.replace(binding.prefix.SELECTOR, 'return disc;', 1),
    }
    body_mutant = original.replace('0xd87ccu,s));}', '0xd87cdu,s));}', 1)
    mutations['recovery_tail'] = canonical.replace(original, body_mutant, 1)
    for key, target in binding.OPERANDS.items():
        old = f'case 0x{key:x}u:return 0x{target:x}u;'
        mutations[f'operand_{key:x}'] = canonical.replace(old, f'case 0x{key:x}u:return 0x{target+1:x}u;', 1)
    for name, mutant in mutations.items():
        if mutant == canonical or binding.admitted(mutant):
            raise ValueError(f'Damage shield recovery did not reject {name}')
    refs = binding.reference(original)
    (scratch / 'damage_shield_recovery_reference.inc').write_text(refs + binding.reference(original, replay=False))
    accounted, manifest, _ = instrument_translation(prepared, canonical, str(UNIT.relative_to(ROOT)), ROOT)
    boundaries = [row for row in manifest['functions']
                  if row['symbol'] == binding.REFERENCE and row['canonical_reference_owner'] == binding.prefix.NAME]
    if len(boundaries) != 1:
        raise ValueError('Damage shield recovery lacks its accounted translated reference')
    compiler = [args.compiler, '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                *(f'-I{path}' for path in (ROOT / 'src', ROOT / 'include', scratch))]
    for name, source in (('ordinary', prepared), ('accounted', accounted)):
        path = scratch / f'{name}.cpp'
        path.write_text(source)
        run([*compiler, '-fsyntax-only', str(path)], scratch)
    results = {}
    for variant, cases in (('release', 8000), ('sanitized', 1500)):
        binary = scratch / variant
        extra = [] if variant == 'release' else [
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
        run([*compiler, *extra, str(TEST), '-o', str(binary)], scratch)
        results[variant] = {
            mode or 'literal': json.loads(run([str(binary), str(cases), mode], scratch, capture_output=True).stdout)
            for mode in ('', 'census', 'verify')}
    sources = (UNIT, HEADER, TEST, Path(binding.__file__), Path(__file__))
    report = dict(reference_pin=binding.owner.PIN,
                  static_sites=len(set(re.findall(r'damage_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)', refs))),
                  rejected_mutations=list(mutations), results=results,
                  source_sha256={str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                                 for path in sources})
    args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
