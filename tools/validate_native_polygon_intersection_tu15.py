#!/usr/bin/env python3
"""Check the production polygon owner against its pinned TU1.5 body."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_polygon_intersection_replacements as binding
from specialize_native_operands import specialize

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
TEST = ROOT/'tests/native_tu15_polygon_intersection_tests.cpp'
HEADER = ROOT/'src'/binding.HEADER
LITERAL = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    canonical = UNIT.read_text()
    bound, info = specialize(canonical)
    original, = binding.BODY.findall(canonical)
    if (binding.sha(original) != binding.PIN or
            info['native_spatial_owners'].count('1f6220') != 1 or
            binding.HOOK not in binding.BODY.search(bound)[0]):
        raise ValueError('Current TU1.5 polygon owner did not bind')
    children = {}
    for child in ('123210', '243510'):
        children[child], = re.findall(r'^static void run_'+child+r'\(WorldState &s\) \{.*?^\}',
                                     canonical, re.M | re.S)
    reference = '\n'.join(children.values())+'\n'+binding.retained(original)
    reference = LITERAL.sub(r's.instruction_literal(\1);', reference)
    reference = reference.replace('run_1f6220(', 'reference_polygon_intersection(', 1)
    for child, adapter in (('123210','axis'), ('243510','contains')):
        reference = f'void {adapter}(WorldState &);\n'+reference.replace(f');run_{child}(s);',f');{adapter}(s);')
    (scratch/'polygon_intersection_reference.inc').write_text(reference+'\n')
    report = {'passed':False, 'scratch':str(scratch), 'body_sha256':binding.PIN,
              'bound_body_sha256':binding.sha(binding.BODY.search(bound)[0]),
              'kernel_sha256':binding.sha(HEADER.read_text()),
              'plaintext_child_sha256':{child:binding.sha(text) for child,text in children.items()},
              'profiles':{}}
    profiles = (
        ('release',[],2592),
        ('asan_ubsan',['-O1','-fsanitize=address,undefined','-fno-omit-frame-pointer',
                       '-fno-sanitize-recover=all','-fno-pie','-no-pie'],864),
    )
    for name, flags, cases in profiles:
        executable = scratch/name
        command = [args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off',
                   '-frounding-math',*flags,f'-I{ROOT/"include"}',f'-I{ROOT/"src"}',
                   f'-I{scratch}',str(TEST),'-o',str(executable)]
        subprocess.run(command,check=True,cwd=scratch)
        run = subprocess.run([str(executable),str(cases)],capture_output=True,text=True,cwd=scratch)
        if run.returncode:
            raise RuntimeError(f'{name}\n'+run.stderr[-6000:])
        counts = json.loads(run.stdout)
        if counts['compared'] != cases or counts['coverage'] != 253:
            raise ValueError(f'Incomplete polygon coverage in {name}')
        report['profiles'][name] = counts
    report['passed'] = True
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
