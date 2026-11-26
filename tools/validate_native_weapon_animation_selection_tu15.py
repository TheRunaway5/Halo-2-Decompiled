#!/usr/bin/env python3
"""Compare the native weapon animation selector with its pinned TU1.5 body."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_weapon_animation_selection_replacements as binding
from specialize_native_operands import specialize

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
HEADER = ROOT/'src'/binding.HEADER
SITES = HEADER.with_name('native_tu15_weapon_animation_selection_sites.inc')
TEST = ROOT/'tests/native_tu15_weapon_animation_selection_tests.cpp'
SYMBOLS = ROOT/'src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp'
LITERAL = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    source = UNIT.read_text()
    prepared, receipt = specialize(source)
    original, = binding.BODY.findall(source)
    bound, = binding.BODY.findall(prepared)
    if receipt['native_spatial_owners'].count('1e3420') != 1 or binding.HOOK not in bound:
        raise ValueError('Current 1E3420 owner did not bind')
    reference = LITERAL.sub(r's.instruction_literal(\1);', binding.retained(original))
    for child, fixture in (('2a0100','identify'), ('1e5810','query'),
                           ('1e5c80','load_graph'), ('126200','register_animation')):
        match = f';run_{child}(s);'
        if reference.count(match) != binding.CHILDREN[child]:
            raise ValueError(f'Fixture child count changed: {child}')
        reference = reference.replace(match,f';{fixture}(s);')
    (scratch/'weapon_animation_selection_reference.inc').write_text(reference+'\n')
    report = {'passed': False, 'scratch': str(scratch), 'canonical_body_sha256': binding.sha(original),
              'bound_body_sha256': binding.sha(bound), 'kernel_sha256': binding.sha(HEADER.read_text()),
              'sites_sha256': binding.sha(SITES.read_text()), 'fixture_sha256': binding.sha(TEST.read_text()),
              'profiles': {}, 'retained_large_artifacts': []}
    profiles = (
        ('release', [], 660),
        ('asan_ubsan', ['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                        '-fno-sanitize-recover=all','-fno-pie','-no-pie'], 330),
    )
    try:
        for name, flags, count in profiles:
            executable = scratch/name
            command = [args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off',
                       '-frounding-math',*flags,f'-I{ROOT/"include"}',f'-I{ROOT/"src"}',
                       f'-I{scratch}',str(TEST),str(SYMBOLS),'-o',str(executable)]
            subprocess.run(command,check=True,cwd=scratch)
            run = subprocess.run([str(executable),str(count)],capture_output=True,text=True,cwd=scratch)
            if run.returncode:
                raise RuntimeError(f'{name}\n'+run.stderr[-10000:])
            report['profiles'][name] = json.loads(run.stdout)
        report['passed'] = True
    except BaseException as error:
        report['error'] = str(error)
        raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
