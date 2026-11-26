#!/usr/bin/env python3
"""Compare the native two-bone solver to its pinned translated body."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

import native_two_bone_solve_replacements as binding

ROOT = Path(__file__).resolve().parents[1]
UNIT = ROOT/'src/physics/physics_live_support_tu15_generated.cpp'
HEADER = ROOT/'src/physics/runtime/native_tu15_two_bone_solve.hpp'
TEST = ROOT/'tests/native_tu15_two_bone_solve_tests.cpp'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    text = UNIT.read_text()
    original, = binding.BODY.findall(text)
    native, installed = binding.install(text, text)
    if installed != ['122be0'] or binding.HOOK not in binding.BODY.search(native)[0]:
        raise ValueError('Current source did not bind the solver')
    drift = original.replace('{\n', '{\n// changed\n', 1)
    modified = text.replace(original, drift, 1)
    if binding.install(modified, modified)[1]:
        raise ValueError('Changed solver was admitted')
    normalization, = re.findall(r'^static void run_30bd0\(WorldState &s\) \{.*?^\}', text, re.M | re.S)
    live_child = re.sub(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}',
                        r's.instruction_literal(\1);', normalization)
    (scratch/'normalize.inc').write_text(live_child.replace('run_30bd0(', 'run_normalization_body(', 1) + '\n')
    (scratch/'solve.inc').write_text(binding.retained(original) + '\n')
    report = {'passed':False,'scratch':str(scratch),'body_sha256':binding.sha(original),
              'normalization_sha256':binding.sha(normalization),'kernel_sha256':binding.sha(HEADER.read_text()),
              'test_sha256':binding.sha(TEST.read_text()),'profiles':{}}
    for name, flags, cases in (('release', [], 4096), ('asan_ubsan',
            ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'], 1024)):
        executable = scratch/name
        command = [args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                   *flags,f'-I{ROOT / "include"}',f'-I{ROOT / "src"}',f'-I{scratch}',str(TEST),'-o',str(executable)]
        subprocess.run(command, check=True, cwd=scratch)
        profiles = {}
        for mode in ('default','census'):
            run = subprocess.run([str(executable),str(cases),mode], capture_output=True, text=True, cwd=scratch)
            if run.returncode:
                raise RuntimeError(f'{name} {mode}\n' + run.stderr[-6000:])
            counts = json.loads(run.stdout)
            if counts['compared'] != cases or counts['children'] < cases or counts['faults'] == 0:
                raise ValueError(f'Incomplete solver coverage in {name} {mode}')
            profiles[mode] = counts
        report['profiles'][name] = profiles
    report['passed'] = True
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS release and ASan/UBSan solver, original normalization and ordered faults')


if __name__ == '__main__':
    main()
