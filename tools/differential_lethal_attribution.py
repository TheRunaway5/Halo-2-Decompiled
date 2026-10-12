#!/usr/bin/env python3
"""Validate TU1.1 lethal credit/assist selection; full world/writer effects remain separate."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/spawn-source'
REPORT = BASE / 'lethal-attribution-report.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(value):
    return f'{value & 0xffffffff:08x}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/lethal_attribution_tests')
    parser.add_argument('--fixtures', type=Path, default=BASE.parent / 'lethal-attribution-fixtures.tsv')
    parser.add_argument('--output', type=Path, default=BASE.parent / 'lethal-attribution-differential.json')
    parser.add_argument('--export-only', action='store_true')
    parser.add_argument('--match-binary', type=Path, default=ROOT / 'build-multiplayer/lethal_match_tests')
    args = parser.parse_args()
    if ROOT / 'analysis' not in args.fixtures.resolve().parents:
        raise ValueError('Keep original-data fixtures in ignored analysis')
    pins = {
        REPORT: '46d65d088255fc1fc357a84aa6e8d17b6065387327be9c03ba9f392ce0b1c84d',
        ROOT / 'tools/oracle_mp_spawn_lethal_attribution.py':
            '093e30f102597a2440415cadabc6bd3cfd56225dc8f025761742cc6e9d38e7cc',
        BASE / 'tu11-lethal-attribution-instructions.txt':
            '73fb9a16ee378d218a8238a5cad833af2821dad8eb216b1ae523d821f21855f7',
        ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03':
            'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
        ROOT / 'assets/disc/maps/lockout.map':
            '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9',
    }
    report = json.loads(REPORT.read_text())
    pins.update({ROOT / relative: pin for relative, pin in report['dependencies'].items()})
    for path, pin in pins.items():
        if digest(path) != pin:
            raise ValueError(f'Frozen lethal authority changed: {path}')
    hits = [(f'{sequence["case"]}-{index}', hit) for sequence in report['sequence_cases']
            for index, hit in enumerate(sequence['hits'])]
    hits.extend((case['case'], case['lethal_hit']) for case in report['previous_history_cases'])
    for case in report['engine_gate_cases']:
        if 'lethal_hit' in case:
            hits.append((case['case'], case['lethal_hit']))
        else:
            hits.append((case['case'], dict(before=case['before'], after_damage=case['after'],
                result_hex=case['explicitly_changed_result_hex'],
                original_trace=case['original_consumer_trace'], death_writer=None,
                tick=case['before']['tick'])))
    lines = []
    admissions = 0
    for name, hit in hits:
        before = hit['before']
        raw = bytes.fromhex(hit['result_hex'])
        flags, = struct.unpack_from('<I', raw, 4)
        incoming, = struct.unpack_from('<I', raw, 0xc)
        designator, = struct.unpack_from('<h', raw, 0x14)
        victim = before['units']['victim']['owner']
        victim_membership = before['stats'][str(int(victim, 16) & 0xffff)]['membership']
        players = [value for _, value in sorted(before['stats'].items(), key=lambda item: int(item[0]))
                   if value['pool_salt'] != '0000']
        trace = hit['original_trace']
        writer = hit.get('death_writer')
        # Original helper entries are authority for expected selection, counters
        # and order. No native selector is used when exporting expectations.
        admitted = writer is not None
        fields = [name, word(hit['tick']), word(incoming), victim, word(flags), word(30),
                  word(designator), word(victim_membership), word(raw[0]), word(before['mode']),
                  word(0), word(int(before['engine_pointer'], 16) != 0),
                  word(before['stat_valid'] != 0), word(len(players))]
        for player in players:
            fields.extend([player['full_player'], word(player['membership'])])
        for entry in before['history']:
            fields.extend([entry['timestamp'], entry['damage_bits'], entry['object'],
                           entry['player'], word(entry['cause'])])
        fields.append(word(admitted))
        if admitted:
            admissions += 1
            semantic = writer['semantic_arguments']
            credited = semantic['credited_player']
            stat_deltas = [row for row in trace if row['stage'] == 'stat_delta']
            kills = [row for row in stat_deltas if row['field_index'] == 2]
            assists = [row['recipient_full'] for row in stat_deltas if row['field_index'] == 6]
            nonenemy = [row['recipient_full'] for row in trace if row['stage'] == 'nonenemy_history_counter']
            nonenemy_credit = credited != 'ffffffff' and not kills
            fields.extend([credited, word(semantic['cause_byte']), word(semantic['relationship_byte']),
                           word(bool(kills)), word(nonenemy_credit),
                           word(any(row['field_index'] == 3 for row in stat_deltas)),
                           word(any(row['field_index'] == 4 for row in stat_deltas)), word(len(assists)),
                           *assists, word(len(nonenemy)), *nonenemy])
            assert writer['state']['history'] == before['history'], name
        lines.append(' '.join(fields))
    args.fixtures.write_text('\n'.join(lines) + '\n')
    live_fixtures = args.fixtures.parent / 'lethal-match-fixtures.tsv'
    live_lines = []
    live_cases = [case for case in report['previous_history_cases'] if case['case'] in
                  ('direct_enemy_smaller_history', 'direct_enemy_no_matching_history')]
    live_cases += [case for case in report['sequence_cases'] if case['case'] == 'stale_after_leave_rejoin']
    for case in live_cases:
        stale = 'hits' in case
        hit = case['hits'][-1] if stale else case['lethal_hit']
        before = hit['before']
        body = '3a83126f' if stale else before['units']['victim']['body_bits']
        shield = '00000000' if stale else before['units']['victim']['shield_bits']
        roles_player = {value['full_player']: int(slot) for slot, value in before['stats'].items()}
        roles_object = {value['datum']: value['designator'] for value in before['units'].values()}
        # Departed full identities remain source role0; the new occupant is
        # intentionally a different generation in the live reproduction.
        if stale:
            roles_player[hit['attacker']] = 0
        fields = [case['case'], word(0 if stale else roles_player[hit['attacker']]), word(stale), body, shield]
        for entry in before['history']:
            fields.extend([entry['timestamp'], entry['damage_bits'],
                           word(roles_object.get(entry['object'], 0xffffffff)),
                           word(roles_player.get(entry['player'], 0xffffffff)), word(entry['cause'])])
        for slot in ('0', '3', '7', '15'):
            values = hit['after_damage']['stats'][slot]['fields']
            fields.extend(word(values[index]) for index in (0, 2, 3, 4, 6))
        live_lines.append(' '.join(fields))
    live_fixtures.write_text('\n'.join(live_lines) + '\n')
    if args.export_only:
        print(f'Exported {len(lines)} original result gates, {admissions} lethal selections')
        return
    run = subprocess.run([str(args.binary.resolve()), str(args.fixtures.resolve())],
                         text=True, capture_output=True)
    print(run.stdout, end='')
    print(run.stderr, end='')
    live = subprocess.run([str(args.match_binary.resolve()), str(ROOT / 'assets/disc/maps/lockout.map'),
                           str(live_fixtures.resolve())], text=True, capture_output=True)
    print(live.stdout, end=''); print(live.stderr, end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode == 0 and live.returncode == 0, scope=__doc__,
        rows=len(lines), admitted=admissions, native_binary_sha256=digest(args.binary),
        source_report_sha256=digest(REPORT), fixture_sha256=digest(args.fixtures),
        tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr,
        live_match_binary_sha256=digest(args.match_binary), live_fixture_sha256=digest(live_fixtures),
        live_cases=6, live_stdout=live.stdout, live_stderr=live.stderr), indent=2) + '\n')
    raise SystemExit(run.returncode or live.returncode)


if __name__ == '__main__':
    main()
