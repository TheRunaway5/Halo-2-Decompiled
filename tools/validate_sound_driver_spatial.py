#!/usr/bin/env python3
"""Freeze and validate the original driver CPU handoff, including isolated reds.

One real initialized original driver call anchors SDK payloads and retained
state. Declared original CPU prefixes and complete filter helpers extend the
numerical checks. Neither this harness nor its observers implement SDK/DSP
playback. Production files are never changed for deliberate red controls.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    source = args.output / 'source'
    pending = [Path('src/sound_driver_spatial.cpp'),
               Path('src/sound_driver_parameters.cpp'),
               Path('tests/sound_driver_spatial_tests.cpp')]
    pins = {}
    while pending:
        path = pending.pop()
        if str(path) in pins:
            continue
        raw = (ROOT / path).read_bytes()
        pins[str(path)] = hashlib.sha256(raw).hexdigest()
        dest = source / path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(raw)
        for include in re.findall(rb'#include\s+"([^"]+)"', raw):
            pending.append(Path('include') / include.decode())
    fixture = args.output / 'original.tsv'
    shutil.copyfile(args.fixture, fixture)
    commands = []

    def run(command, label, expected_success=True):
        argv = [str(value) for value in command]
        result = subprocess.run(argv, capture_output=True, text=True)
        (args.output / f'{label}.stdout').write_text(result.stdout)
        (args.output / f'{label}.stderr').write_text(result.stderr)
        commands.append(dict(label=label, argv=argv, exit_code=result.returncode))
        assert (result.returncode == 0) == expected_success, (label, result.stdout, result.stderr)
        return result

    owner = source / 'src/sound_driver_spatial.cpp'
    common = ['c++', '-std=c++20', '-fno-fast-math', '-ffp-contract=off',
              '-frounding-math', '-fno-associative-math', '-Wall', '-Wextra',
              '-I' + str(source / 'include'), str(owner),
              str(source / 'src/sound_driver_parameters.cpp'),
              str(source / 'tests/sound_driver_spatial_tests.cpp')]
    for kind, flags in [('normal', ['-O2']),
                        ('sanitizers', ['-O1', '-g', '-fsanitize=address,undefined',
                                        '-fno-omit-frame-pointer'])]:
        binary = args.output / f'driver_spatial_{kind}'
        run(common + flags + ['-o', binary], f'{kind}-build')
        result = run([binary, fixture], f'{kind}-original')
        print(result.stdout.strip(), flush=True)
    original = owner.read_text()
    controls = [
        ('axes', 'position{request[1],request[3],request[2]}',
         'position{request[1],request[2],request[3]}', 'Original spatial SDK payload differs'),
        ('conversion', 'const auto converted=integer_low(result);',
         'const auto converted=std::uint32_t(_mm_cvttss_si32(_mm_set_ss(float(result))));',
         'Original filter helper result differs'),
    ]
    for name, needle, replacement, failure in controls:
        assert original.count(needle) == 1
        mutated = args.output / f'{name}-red.cpp'
        mutated.write_text(original.replace(needle, replacement))
        binary = args.output / f'driver_spatial_red_{name}'
        command = [str(mutated) if value == str(owner) else value for value in common]
        run(command + ['-O2', '-o', binary], f'red-{name}-build')
        result = run([binary, fixture], f'red-{name}-original', expected_success=False)
        assert failure in result.stderr, result.stderr
        print(f'RED {name} caught: {result.stderr.strip().splitlines()[-1]}', flush=True)
    for path, pin in pins.items():
        assert sha(ROOT / path) == pin, f'Concurrent change: {path}'
    report = dict(boundary=__doc__, tool_sha256=sha(__file__), source_sha256=pins,
                  fixture_sha256=sha(fixture), fixture_source=str(args.fixture),
                  commands=commands, normal_and_sanitizers_pass=True,
                  sdk_axis_red_caught=True, integer_conversion_red_caught=True,
                  production_source_mutated=False, audible_or_DSP_parity_claimed=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
