#!/usr/bin/env python3
"""Freeze and validate original retained voice-data CPU and numerical outputs.

Normal and ASan/UBSan builds compare full retained buffers, ordered stores and
complete numerical helpers. Six isolated incorrect source copies must fail:
curve indexing, unsigned cone width, Doppler rounding, filter scaling, byte
dirty flags and azimuth-store order. Production source is never mutated.
Native test lock/dispatch services are explicit models, not live SDK/DSP.
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
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    frozen = args.output / 'source'
    pending = [Path('src/sound_sdk_spatial.cpp'), Path('tests/sound_sdk_voice_data_tests.cpp')]
    pins = {}
    while pending:
        path = pending.pop()
        if str(path) in pins:
            continue
        raw = (ROOT / path).read_bytes()
        pins[str(path)] = hashlib.sha256(raw).hexdigest()
        destination = frozen / path
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(raw)
        pending.extend(Path('include') / value.decode() for value in re.findall(rb'#include\s+"([^"]+)"', raw))
    fixtures = [Path('tests/fixtures/sound_sdk_voice_data.original.json'),
                Path('tests/fixtures/sound_sdk_spatial_data.original.json')]
    arguments = []
    for index, path in enumerate(fixtures):
        target = args.output / f'original-{index}.json'
        shutil.copyfile(ROOT / path, target)
        arguments.append(target)
    commands = []

    def run(command, label, success=True):
        argv = [str(value) for value in command]
        result = subprocess.run(argv, capture_output=True, text=True)
        (args.output / f'{label}.stdout').write_text(result.stdout)
        (args.output / f'{label}.stderr').write_text(result.stderr)
        commands.append(dict(label=label, argv=argv, exit_code=result.returncode))
        assert (result.returncode == 0) == success, (label, result.stdout, result.stderr)
        return result

    owner = frozen / 'src/sound_sdk_spatial.cpp'
    common = ['c++', '-std=c++20', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
              '-fno-associative-math', '-Wall', '-Wextra', '-I' + str(frozen / 'include'),
              str(owner), str(frozen / 'tests/sound_sdk_voice_data_tests.cpp')]
    for name, flags in [('normal', ['-O2']), ('sanitizers', ['-O1', '-g',
                        '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
        binary = args.output / f'voice_data_{name}'
        run(common + flags + ['-o', binary], f'{name}-build')
        result = run([binary, *arguments], f'{name}-original')
        print(result.stdout.strip(), flush=True)
    original = owner.read_text()
    reds = [
        ('curve', 'auto index=curve_index(index_value);', 'auto index=curve_index(index_value)+1u;'),
        ('cone_width', 'auto difference=outer-inner;', 'auto difference=inner-outer;'),
        ('doppler_rounding', 'flds %1\\n\\tflds %2\\n\\tfyl2x\\n\\tfistpl %0',
                            'flds %1\\n\\tflds %2\\n\\tfyl2x\\n\\tfisttpl %0'),
        ('filter_scale', '"m"(c.filter_scale_bits)', '"m"(c.one_bits)'),
        ('byte_flag', 'flag(0x37b11d,0x80,1);', 'flag(0x37b11d,0x80);'),
        ('angle_order', 'put(0x37b069,voice+0x28u,azimuth);flag(0x37b06c,0x40);',
                        'flag(0x37b06c,0x40);put(0x37b069,voice+0x28u,azimuth);')]
    for name, needle, replacement in reds:
        assert original.count(needle) == 1, (name, original.count(needle))
        mutated = args.output / f'{name}-red.cpp'
        mutated.write_text(original.replace(needle, replacement))
        binary = args.output / f'voice_data_red_{name}'
        compile = [str(mutated) if value == str(owner) else value for value in common]
        run(compile + ['-O2', '-o', binary], f'red-{name}-build')
        result = run([binary, *arguments], f'red-{name}-original', success=False)
        assert ('Original retained SDK voice-data/store output differs' in result.stderr or
                'Original complete voice-data helper output differs' in result.stderr), result.stderr
        print(f'RED {name} caught: {result.stderr.splitlines()[0]}', flush=True)
    for path, pin in pins.items():
        assert sha(ROOT / path) == pin, f'Concurrent source change: {path}'
    report = dict(boundary=__doc__, tool_sha256=sha(__file__), source_sha256=pins,
        fixture_sha256={str(path): sha(ROOT / path) for path in fixtures}, commands=commands,
        normal_and_sanitizers_pass=True, all_six_isolated_red_controls_caught=True,
        production_source_mutated=False, native_or_physical_DSP_claimed=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
