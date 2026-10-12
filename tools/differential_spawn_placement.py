#!/usr/bin/env python3
"""Compare native immediate placements with paired original setup/constructor evidence.

The original complete constructors suspend physics. This does not validate live
Havok admission, later body contact solving, initial-join queue timing or corpses.
Extracted authored data stays in ignored local analysis.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'analysis/multiplayer/integration/spawn-source/placement-report.json'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/spawn_placement_tests')
    p.add_argument('--fixtures', type=Path, default=ROOT / 'analysis/multiplayer/integration/spawn-placement-fixtures.tsv')
    p.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/spawn-placement-differential.json')
    p.add_argument('--export-only', action='store_true')
    args = p.parse_args()
    if ROOT / 'analysis' not in args.fixtures.resolve().parents:
        raise ValueError('Keep original authored fixtures in ignored analysis')
    pins = {
        REPORT: '11ae3316e030ce217f1aa73eaf59b74df483502aa4e6507697dba2bd51b02e34',
        ROOT / 'tools/oracle_mp_spawn_placement.py': '29fe396eb70ae293a2c4a3cfd0469522e294b7d1524def43da6dec0f7bdd826f',
    }
    for path, pin in pins.items():
        if digest(path) != pin: raise ValueError(f'Original placement authority changed: {path}')
    report = json.loads(REPORT.read_text())
    for relative, pin in report['dependencies'].items():
        if digest(ROOT / relative) != pin: raise ValueError(f'Original dependency changed: {relative}')
    if digest(ROOT / 'assets/disc/maps/lockout.map') != report['cache_sha256']:
        raise ValueError('Original Lockout differs')
    for version, relative in [('base', 'assets/disc/default.xbe'),
            ('tu11', 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')]:
        if digest(ROOT / relative) != report['executable_pins'][version]:
            raise ValueError(f'Original executable differs: {version}')
    constructors = {row['authored_index']: row for row in report['real_bsp_constructors']}
    lines = []
    for row in report['normal_setup']:
        if row['version'] != 'tu11': continue
        created = constructors[row['authored_index']]
        if created['collision_queries'] or created['object']['position'] != row['position']:
            raise ValueError('Original valid-position constructor boundary differs')
        raw = bytes.fromhex(row['raw_start_hex'])
        lines.append(' '.join(map(str, [row['authored_index'], row['authored_bsp'],
            *row['position']['bits'], f'{struct.unpack_from("<I", raw, 12)[0]:08x}'])))
    row = next(row for row in report['failures'] if row['version'] == 'tu11' and
        row['case'] == 'explicit_start_bsp_short_7')
    raw = bytes.fromhex(row['raw_start_hex'])
    position = struct.unpack_from('<3I', bytes.fromhex(row['placement_hex']), 0x1c)
    lines.append(' '.join(map(str, [0, 7, *(f'{v:08x}' for v in position),
        f'{struct.unpack_from("<I", raw, 12)[0]:08x}'])))
    args.fixtures.write_text('\n'.join(lines) + '\n')
    if args.export_only:
        print(f'Exported {len(lines)} original placement inputs'); return
    run = subprocess.run([str(args.binary.resolve()), str(ROOT), str(args.fixtures.resolve())],
        text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode == 0, authored_placements=66,
        bsp_metadata_cases=1, failed_retry_phases=100, native_binary_sha256=digest(args.binary),
        original_report_sha256=digest(REPORT), fixtures_sha256=digest(args.fixtures),
        tool_sha256=digest(Path(__file__)), scope=__doc__, stdout=run.stdout,
        stderr=run.stderr), indent=2) + '\n')
    raise SystemExit(run.returncode)

if __name__ == '__main__': main()
