#!/usr/bin/env python3
"""Compare native neutral/cached command kernels with original owning traces."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/spawn-source/command-input'
PINS = {
    ROOT / 'tools/oracle_mp_spawn_command_input.py': 'de382cb5f14ccbc64fbc5609419382f2a574fc1f46ba00e371efbfa3a641f4fc',
    ROOT / 'docs/MP_COMMAND_INPUT_RESEARCH.md': 'e29c2cad74a0c10bd6ff4b8612787fd3c89a347c3c9d14c7c133d1aa80a2b967',
    BASE / 'report.json': '1fc1b58c65fd804477ac9bbdcd3bcad1ad10bd954f98b0f5ea17348891ba0a76',
    BASE / 'independent-freeze-verification.json': 'e840504503e5f97869207d0798e3aa92e7d117c2fecc227009414dab878f13fd',
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def verify_authority():
    for path, sha in PINS.items():
        require(digest(path) == sha, f'Changed independently frozen command authority: {path}')
    report = json.loads((BASE / 'report.json').read_text())
    require((BASE / 'report.json').read_bytes() == (BASE / 'reproduction/report.json').read_bytes(),
            'Original independent command report differs')
    for path, sha in report['frozen_pins'].items():
        require(digest(ROOT / path) == sha, f'Changed preceding command dependency: {path}')
    for listing in report['instruction_listings'] + [report['executed_original_listing'],
                                                   report['observed_input_store_listing']]:
        path = Path('instructions') / listing['file']
        require(digest(BASE / path) == listing['sha256'], f'Changed original instruction file: {path}')
        require((BASE / path).read_bytes() == (BASE / 'reproduction' / path).read_bytes(),
                f'Independent original instruction repeat differs: {path}')
    manifest = hashlib.sha256(json.dumps(report['instruction_listings'], sort_keys=True,
                                        separators=(',', ':')).encode()).hexdigest()
    require(manifest == report['instruction_manifest_sha256'], 'Original instruction index differs')
    require(digest(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03') == report['source_sha256'],
            'Original executable differs')
    require(digest(ROOT / 'assets/disc/maps/lockout.map') == report['cache_sha256'], 'Original Lockout differs')
    require(not report['instruction_patches'] and not report['new_supplied_gameplay_or_physics_results'],
            'Changed source composition boundary')
    require(len(report['cases']) == 2 and len(report['frozen_pins']) == 42 and
            len(report['instruction_listings']) == 296, 'Incomplete original suite')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/player_command_tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/player-command-differential.json')
    args = parser.parse_args()
    original = verify_authority()
    rows = []
    for case in original['cases']:
        stages = [s for s in case['stages'] if s['name'] == 'source_neutral_command_initializer']
        require(len(stages) == 1 and stages[0]['completion'] == 'complete_original_return',
                'Missing actual neutral command producer')
        stage = stages[0]
        rows.append(['I', case['case'], stage['command_state_before']['command5c_hex'],
                     stage['command_state_after']['command5c_hex']])
        calls = [c for c in case['input_calls'] if c['entry'] == '00085460']
        require(len(calls) == 1, 'Changed original cached-command call coverage')
        call = calls[0]
        record = bytes.fromhex(call['record88_before_hex'])
        require(len(record) == 0x88 and struct.unpack_from('<I', record, 8)[0] == 0,
                'Unmeasured watcher refresh/kind branch')
        require(call['record88_before_hex'] == call['record88_after_hex'], 'Unexpected original record mutation')
        clock = bytes.fromhex(call['clock12_entry_hex'])
        expiration = bytes.fromhex(call['expiration4_entry_hex'])
        require(len(clock) == 12 and len(expiration) == 4, 'Missing literal original clock/config inputs')
        tick = struct.unpack_from('<I', clock, 8)[0]
        dt = struct.unpack_from('<I', clock, 4)[0]
        expiry = struct.unpack('<I', expiration)[0]
        rows.append(['D', case['case'], call['record88_before_hex'], call['delivery5c_before_hex'],
                     call['delivery5c_after_hex'], f'{tick:08x}', f'{dt:08x}', f'{expiry:08x}',
                     call['defined_return_al'], call['record88_after_hex']])
    fixture = ROOT / 'analysis/multiplayer/integration/player-command-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str, row)) for row in rows) + '\n')
    run = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True)
    print(run.stdout, end='')
    print(run.stderr, end='')
    result = dict(passed=run.returncode == 0, scope=__doc__, source_cases=2,
                  neutral_initializers=2, cached_deliveries=2, accepted_commands=1,
                  complete_neutral_delivery_and_record_buffers_compared=True,
                  original_return_al_only=True, live_device_codec_frame_integration=False,
                  limits='Actual simulation record kind0 and source-owned age-zero clock/configuration inputs. Remote watcher refresh is unavailable. Validator alternative branches have static-only coverage. This does not implement the submission codec, packet assembler, biped control producer, local device/map-ready/state4 scheduling or complete frame/motion integration.',
                  source_pins={str(p.relative_to(ROOT)): sha for p, sha in PINS.items()},
                  fixture_sha256=digest(fixture), native_binary_sha256=digest(args.binary),
                  tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__':
    main()
