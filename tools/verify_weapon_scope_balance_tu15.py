#!/usr/bin/env python3
"""Compare TU1.5 combat postprocessing across every map in the weapon scope.

The complete original routine runs unchanged over explicit raw cache/global
inputs. This verifies that component and its native Cache consumers, not full
map activation, weapon gameplay, audio, or framebuffer parity. Large temporary
maps are discarded after each case; only reports, stores, and changed tag bytes
remain.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from oracle_mp_combat_postprocess_updates import Cache, IMAGES, ROOT, run_one, sha

SCOPE = ROOT / 'analysis/weapons-tu15-goal-20261013/authored-scope.json'
SCOPE_PIN = '6855b4f33585b332ca82e4b688b9e29c6a7a04eca5757c1bc8fe9b20640c6a1b'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--content', action='store_true',
                        help='Exercise the existing native Cache weapon/damage and UDP identity checks too.')
    args = parser.parse_args()
    assert sha(SCOPE) == SCOPE_PIN
    scope = json.loads(SCOPE.read_text())
    assert len(scope['maps']) == 38
    args.output.mkdir(parents=True, exist_ok=False)
    native = args.native.resolve()
    native_pin = sha(native)
    config = IMAGES[1]
    assert config[0] == 'owned_1_5'
    cases = []
    for index, authored in enumerate(scope['maps']):
        path = Path(authored['path'])
        assert sha(path) == authored['sha256'], path
        case = args.output / f'{index:02d}-{path.stem}'
        cache = Cache(path)
        try:
            reference = run_one(cache, config, case / 'original', require_changes=False)
            stores = [row for row in reference['ordered_stores']
                      if row['region'] == 'original_cache_metadata']
            ordered = [[int(row['instruction'], 16), int(row['file_offset'], 16),
                        int(row['before_bits'], 16), int(row['after_bits'], 16)]
                       for row in stores]
            fixture = case / 'stores.tsv'
            fixture.write_text('# instruction file_offset before_bits after_bits\n' +
                               ''.join(' '.join(f'{word:x}' for word in row) + '\n'
                                       for row in ordered))
            original_report = case / 'original.json'
            original_report.write_text(json.dumps(reference, indent=2, allow_nan=False) + '\n')
            with tempfile.TemporaryDirectory(prefix='balance-', dir=case) as scratch:
                scratch = Path(scratch)
                expected = scratch / 'original.map'
                with expected.open('wb') as output:
                    output.write(cache.data)
                with expected.open('r+b') as output:
                    for changed in reference['changed_tags']:
                        output.seek(changed['tag']['offset'])
                        output.write(bytes.fromhex(changed['processed_hex']))
                assert sha(expected) == reference['complete_cache_output_sha256']
                actual = scratch / 'native.map'
                command = [str(native), str(path), str(config[1]),
                           str(fixture.resolve()), str(actual.resolve())]
                content_checked = args.content and reference['source_scenario_gate_i16_at_10'] == 1
                if content_checked:
                    command += ['--content', str(expected.resolve())]
                environment = dict(os.environ, HALO2_TEST_BALANCE_SHARED_MAPS=str(ROOT / 'assets/disc/maps'))
                result = subprocess.run(command, text=True, capture_output=True, timeout=180, env=environment)
                (case / 'native.stdout').write_text(result.stdout)
                (case / 'native.stderr').write_text(result.stderr)
                assert result.returncode == 0, (path, result.returncode, result.stderr[-1500:])
                measured = json.loads(result.stdout)
                assert measured == {
                    'admitted': reference['source_scenario_gate_i16_at_10'] == 1,
                    'stores': ordered,
                }, path
                assert sha(actual) == reference['complete_cache_output_sha256'], path
            assert sha(native) == native_pin, 'Native probe changed during the matrix'
            row = {
                'map': str(path), 'cache_sha256': authored['sha256'],
                'scenario_type': reference['source_scenario_gate_i16_at_10'],
                'weapon_count': len(authored['weapons']),
                'ordered_stores': len(ordered),
                'complete_output_sha256': reference['complete_cache_output_sha256'],
                'original_report': str(original_report.relative_to(args.output)),
                'original_report_sha256': sha(original_report),
                'native_cache_weapon_damage_and_udp_identity_checked': content_checked,
                'consumer_scope': 'Multiplayer Cache imports and UDP identity' if content_checked else 'Complete native combat postprocessor only; campaign import uses its separate owner',
                'passed': True,
            }
            cases.append(row)
            (args.output / 'progress.json').write_text(json.dumps(cases, indent=2) + '\n')
            print(json.dumps({k: row[k] for k in ('map', 'weapon_count', 'ordered_stores', 'passed')}), flush=True)
        finally:
            cache.data.close()
            cache.stream.close()
    report = {
        'scope': __doc__, 'tool_sha256': sha(Path(__file__)),
        'original_observer_sha256': sha(ROOT / 'tools/oracle_mp_combat_postprocess_updates.py'),
        'authored_scope_sha256': SCOPE_PIN, 'native_binary_sha256': native_pin,
        'original_xbe_sha256': config[2], 'original_entry': f'{config[3]:08x}',
        'maps': len(cases), 'weapon_paths': len(scope['weapons']),
        'weapon_occurrences': sum(row['weapon_count'] for row in cases),
        'passed': True, 'cases': cases,
        'original_function_or_platform_results_supplied': [],
        'full_map_activation_verified': False, 'full_weapon_parity': False,
        'physical_xbox_verified': False, 'temporary_full_cache_files_retained': False,
        'declared_shared_cache_root': str(ROOT / 'assets/disc/maps'),
    }
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('maps', 'weapon_paths', 'weapon_occurrences', 'passed')}), flush=True)


if __name__ == '__main__':
    main()
