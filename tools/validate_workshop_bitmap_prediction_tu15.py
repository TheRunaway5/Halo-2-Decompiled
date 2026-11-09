"""Compare Workshop bitmap prediction with the unchanged TU1.5 script path.

Run inside tools/run_test.py. Borrow the cache routines and their link
dependencies from one coherent native build. The original and native fixtures
exercise nonpaged lookup and retry plus resident page tracking.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

from generate_workshop_script_builtins_tu15 import generated_bodies


MODULE = 'src/campaign/scripting/source_workshop_script_builtins_tu15_generated.cpp'
NEW_ENTRIES = {0x2AA200, 0x2AC680}


def sha(path):
    with path.open('rb') as data:
        return hashlib.file_digest(data, 'sha256').hexdigest()


def main():
    if not __debug__:
        raise RuntimeError('Bitmap prediction validation requires assertions')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--frozen-source', type=Path, required=True)
    parser.add_argument('--contract', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    root = Path(__file__).resolve().parents[1]
    source = args.frozen_source.resolve(strict=True)
    assert source.name == 'source' and source.is_relative_to(root / '.local/test-scratch')
    assert not args.report.exists() and not args.report.resolve().is_relative_to(owner)

    contract = json.loads(args.contract.read_text())
    row, = [case for case in contract['cases'] if case['declaration']['name'] == 'bitmap_predict']
    matches = [{key: value for key, value in match.items() if key.startswith('xbox_')}
               for match in row['xbox_matches']]
    audit = owner / 'declaration.json'
    audit.write_text(json.dumps(dict(xbox_tu15_sha256=contract['xbox_tu15_sha256'],
        declaration_candidates=[dict(row['declaration'], matches=matches)])))

    commands = []
    generated = {}
    for name, path in [('current', root / MODULE), ('frozen', source / MODULE)]:
        output, directory = owner / (name + '.cpp'), owner / name
        command = [sys.executable, '-B', str(root / 'tools/generate_workshop_script_builtins_tu15.py'),
            '--audit-report', str(audit), '--output', str(output), '--directory', str(directory),
            '--preserve-module', str(path), '--names', 'bitmap_predict', '--owned-callees', '0x2aa200']
        commands.append(command)
        subprocess.run(command, check=True)
        generated[name] = generated_bodies(output.read_text())
    assert all(generated['current'][entry] == generated['frozen'][entry] for entry in NEW_ENTRIES)

    oracle = root / 'tools/oracle_workshop_bitmap_prediction_tu15.py'
    fixtures = {}
    for name, options in [('effect', []), ('script', ['--script'])]:
        fixture = owner / (name + '.json')
        command = [sys.executable, '-B', str(oracle), '--fixture', str(fixture), *options]
        commands.append(command)
        subprocess.run(command, check=True)
        fixtures[name] = fixture

    build = source.parent / 'build'
    link = build / 'CMakeFiles/source_sound_duration_tu15_tests.dir/link.txt'
    words = shlex.split(link.read_text())
    output_at = words.index('-o')
    assert words[output_at + 1] == 'source_sound_duration_tu15_tests'
    dependencies = words[output_at + 2:]
    assert 'libhalo2_physics_world_runtime.a' in dependencies
    paths = {build / word for word in dependencies if not word.startswith('-')}
    test = root / 'tests/workshop_bitmap_prediction_tests.cpp'
    paths.update([link, test, oracle, root / MODULE, source / MODULE, args.contract.resolve(),
                  source / 'include/halo2/physics_contact_runtime.hpp'])
    pins = {str(path): sha(path) for path in sorted(paths)}

    native = owner / 'bitmap-prediction'
    compiler = [words[0], '-std=c++20', '-O2', '-fno-fast-math', '-ffp-contract=off',
        '-I' + str(source / 'include'), '-I' + str(source / 'src')]
    command = compiler + [str(owner / 'frozen.cpp'), str(test), '-o', str(native), *dependencies]
    commands.append(command)
    subprocess.run(command, cwd=build, check=True)
    observations = {}
    for name, fixture in fixtures.items():
        command = [str(native), str(fixture)]
        commands.append(command)
        subprocess.run(command, check=True)
        cases = json.loads(fixture.read_text())['cases']
        observations[name] = dict(sha256=sha(fixture),
            cases=[{key: case[key] for key in ('name', 'lookups', 'requests', 'parser_results')
                    if key in case} for case in cases])
    assert all(sha(Path(path)) == pin for path, pin in pins.items())

    manifest = json.loads((owner / 'current/generation.json').read_text())
    report = dict(scope=__doc__, passed=True, commands=commands, link_working_directory=str(build),
        unchanged_inputs=pins, current_generation_sha256=manifest['generated_cpp_sha256'],
        current_preserved_body_sha256=manifest['preserved_body_sha256'],
        new_entries=[hex(entry) for entry in sorted(NEW_ENTRIES)],
        identical_new_bodies_across_source_cohorts=True, original_fixtures=observations,
        cold_allocation_qualified=False, MCC_script_admitted=False, owned_scratch=str(owner),
        retained_large_artifacts=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
