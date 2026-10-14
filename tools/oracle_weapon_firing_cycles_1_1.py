#!/usr/bin/env python3
"""Original firing timelines with declared storage/services, through magazine admission.

The original constructor, input/update and both projectile postpasses execute
unchanged. Model/animation notifications and unavailable projectile/effect
allocation retain the inherited explicit boundaries. Stop comparison before a
reload/chamber owner takes over; this file does not prove that separate owner.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN, Trace, Cache, WEAPON


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--fixture', type=Path, required=True)
    args = ap.parse_args()
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    patterns = {
        'held_release': [False] * 5 + [True] * 70 + [False] * 20 + [True] * 10,
        'short_pulses': [False] * 5 + [True, False] * 45,
        'short_burst_release': [False] * 5 + [True] + [False] * 40 + [True] + [False] * 40,
    }
    names = ('magnum', 'battle_rifle', 'covenant_carbine', 'shotgun',
             'sniper_rifle', 'brute_shot', 'needler', 'rocket_launcher')
    args.output.mkdir(parents=True, exist_ok=True)
    fixture = ['# Original TU1.1 firing cycles. Entries end before reload/chamber admission.']
    reports = []
    for name in names:
        tag = next(t for t in cache.tags.values() if t['group'] == 'weap' and
                   t['name'].split('\\')[-1] == name)
        trigger = cache.block(tag['offset'] + 0x2c8, 0x40)[0]
        index = cache.h(trigger + 8)
        for pattern_name, pattern in patterns.items():
            class CycleTrace(Trace):
                def state(self):
                    result = super().state()
                    raw = bytes(self.g.mem_read(WEAPON + 0x1a4 + index * 0x34, 52))
                    result['selected_barrel_hex'] = raw.hex()
                    return result
            t = CycleTrace(1)
            t.w(WEAPON, int(tag['datum'], 16))
            report = t.execute(pattern=pattern)
            report.update(scope=__doc__, actual_weapon_tag=tag, selected_barrel=index,
                          actual_pattern_name=pattern_name)
            path = args.output / f'{name}-{pattern_name}.json'
            path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
            count = 0
            for row in report['rows']:
                after = row['post_dispatch']
                if after['magazine_mode'] != 0:
                    break
                raw = bytes.fromhex(after['selected_barrel_hex'])
                values = [row['tick'], int(row['held']), after['loaded'],
                          raw[0], raw[1], int.from_bytes(raw[2:4], 'little'),
                          int.from_bytes(raw[6:8], 'little'),
                          struct.unpack_from('<I', raw, 0x10)[0],
                          struct.unpack_from('<I', raw, 0x28)[0]]
                first,last=row['event_range']
                calls=[event for event in report['events'][first:last]
                       if event.get('service')=='00107b50' and event['phase']=='update'
                       and event['registers']['eax']!='ffffffff']
                values.append(len(calls))
                for call in calls:
                    values.extend(int(value,16) for value in
                                  (call['registers']['eax'],*call['stack_arguments'][1:3]))
                fixture.append(' '.join([name, pattern_name, *(f'{v:08x}' for v in values)]))
                count += 1
            reports.append(dict(name=name, pattern=pattern_name, compared_ticks=count,
                original_ticks=len(report['rows']), path=str(path),
                sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                next_owner_boundary=(report['rows'][count]['post_dispatch']['magazine_mode']
                                     if count < len(report['rows']) else None)))
            print(name, pattern_name, count, flush=True)
    cache.data.close()
    cache.stream.close()
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text('\n'.join(fixture) + '\n')
    (args.output / 'report.json').write_text(json.dumps(dict(scope=__doc__,
        xbe_sha256=PIN, cache_sha256=CACHE_PIN, cases=reports,
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inherited_tool_sha256=hashlib.sha256(Path('tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest(),
        instruction_patches=[]), indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
