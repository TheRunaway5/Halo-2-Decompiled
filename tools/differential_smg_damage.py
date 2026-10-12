#!/usr/bin/env python3
"""Compare native stock SMG amount and live-used vitality composition with TU1.1.

Original complete-entry outputs supply the expected values. Hit finding,
reactions, full event/result construction and death attribution are separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'analysis/multiplayer/integration/spawn-source/smg-damage-report.json'
PRODUCERS = REPORT.parent / 'smg-producers-report.json'
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def word(value): return f'{value & 0xffffffff:08x}'
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/smg_damage_tests')
    p.add_argument('--fixtures', type=Path, default=ROOT / 'analysis/multiplayer/integration/smg-damage-fixtures.tsv')
    p.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/smg-damage-differential.json')
    p.add_argument('--export-only', action='store_true')
    args = p.parse_args()
    if ROOT / 'analysis' not in args.fixtures.resolve().parents: raise ValueError('Keep original exports in ignored analysis')
    for path, pin in {
        REPORT: '2b7d072e92dde4ad6af7a92f301cf5b734410df44745c988bbcf6b5b372e9aca',
        ROOT / 'tools/oracle_mp_spawn_smg_damage.py': '4d20d8c2e7229dd2bf78a601482d1862535c0208ab7a57b86f47192337c1a378',
        PRODUCERS: 'c8f66fbb8f5041acad27e2b113d0f8952f3654ec8bfd31fd626e48acfef4fcfa',
        ROOT / 'tools/oracle_mp_spawn_smg_producers.py': 'a86cff27319ce75a838ea42bca7fd9f1907b01fbb8b5329a1ede22f16627dd82',
    }.items():
        if digest(path) != pin: raise ValueError(f'Original damage authority changed: {path}')
    report = json.loads(REPORT.read_text())
    for relative, pin in report['dependencies'].items():
        if digest(ROOT / relative) != pin: raise ValueError(f'Original dependency changed: {relative}')
    for path, pin in {
        ROOT / report['instruction_listing']: report['instruction_listing_sha256'],
        ROOT / 'assets/disc/maps/lockout.map': report['cache_sha256'],
        ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03': report['executable_sha256'],
    }.items():
        if digest(path) != pin: raise ValueError(f'Original source changed: {path}')
    lines = []
    for i, row in enumerate(report['source_amount']):
        lines.append(' '.join(['A', str(i), word(row['strength_byte']), row['event_scale54_bits'],
            row['event_scale5c_bits'], row['amount_bits']]))
    producers = json.loads(PRODUCERS.read_text())
    if digest(ROOT / producers['instruction_listing']) != producers['instruction_listing_sha256']:
        raise ValueError('Original producer listing changed')
    for relative, pin in producers['dependencies'].items():
        if digest(ROOT / relative) != pin: raise ValueError(f'Original producer dependency changed: {relative}')
    for i, row in enumerate(producers['source_strength_cases'], len(lines)):
        event = bytes.fromhex(row['composed_event_hex'])
        fraction, scale = struct.unpack_from('<I', event, 0x54)[0], struct.unpack_from('<I', event, 0x5c)[0]
        lines.append(' '.join(['A', str(i), word(row['player_after_copy']['c1']), word(fraction),
            word(scale), row['raw_damage_bits']]))
    amounts = len(lines)
    hits = list(report['complete_damage_entry_hits']) + [hit
        for sequence in report['damage_entry_sequences'] for hit in sequence['hits']]
    for i, row in enumerate(hits):
        before, after = row['before']['health'], row['after']['health']
        amount = next(e['raw_amount_bits'] for e in row['original_and_supplied_entries'] if e['entry'] == '000da710')
        values = [before['body_bits'], before['shield_bits'], word(before['body_stun']),
            word(before['shield_stun']), before['flags'], amount, after['body_bits'], after['shield_bits'],
            word(after['body_stun']), word(after['shield_stun']), after['flags'],
            row['result_body_bits'], row['result_shield_bits']]
        lines.append(' '.join(['H', str(i), *values]))
    args.fixtures.write_text('\n'.join(lines) + '\n')
    if args.export_only:
        print(f'Exported {amounts} amounts and {len(hits)} hits'); return
    run = subprocess.run([str(args.binary.resolve()), str(ROOT / 'assets/disc/maps/lockout.map'),
        str(args.fixtures.resolve())], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode == 0,
        amounts=amounts, hits=len(hits), scope=__doc__,
        native_binary_sha256=digest(args.binary), original_report_sha256=digest(REPORT),
        original_producer_report_sha256=digest(PRODUCERS),
        fixtures_sha256=digest(args.fixtures), tool_sha256=digest(Path(__file__)),
        stdout=run.stdout, stderr=run.stderr), indent=2) + '\n')
    raise SystemExit(run.returncode)
if __name__ == '__main__': main()
