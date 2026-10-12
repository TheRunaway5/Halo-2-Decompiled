#!/usr/bin/env python3
"""Inventory authored action messages in every supplied Xbox map.

Read-only content evidence, not proof that native gameplay has produced or
executed each action. The weapon/control fields are consumed by original
19188C; seat/flip/boarding fields are retained separately so unsupported
interaction kinds cannot disappear from the coverage inventory.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct

from inspect_multiplayer import Cache


def inspect(path):
    cache = Cache(path)
    try:
        globals_at = next(t['offset'] for t in cache.tags.values()
                          if t['group'] == 'matg' and t['offset'] is not None)
        count, size, indices, strings = struct.unpack_from('<4I', cache.data,
                                                          globals_at + 0x190)
        assert count <= 65536 and strings + size <= len(cache.data)
        locale = {cache.u(indices + i * 8):
                  cache.text(strings + cache.u(indices + i * 8 + 4))
                  for i in range(count)}

        def message(sid):
            return {'id': f'{sid:08x}', 'text': locale.get(sid)}

        result = {'map': path.name, 'path': str(path.resolve()),
                  'scenario_type': cache.h(cache.scenario + 0x10),
                  'sha256': hashlib.sha256(cache.data).hexdigest(),
                  'weapons': [], 'controls': [], 'units': []}
        control_placements = Counter()
        palette = cache.block(cache.scenario + 0xc0, 0x28)
        for placement in cache.block(cache.scenario + 0xb8, 0x44):
            index = cache.h(placement)
            if index >= 0:
                control_placements[cache.u(palette[index] + 4)] += 1
        for datum, tag in cache.tags.items():
            at = tag['offset']
            if at is None:
                continue
            identity = {'datum': tag['datum'], 'name': tag['name']}
            if tag['group'] == 'weap':
                flags = cache.u(at + 0x12c)
                result['weapons'].append({**identity, 'inventory_flags': flags,
                    'dual_wieldable': bool(flags & (1 << 22)),
                    'action_messages': [message(cache.u(at + 0xcc + i * 4))
                                        for i in range(5)],
                    'switch_message': message(cache.u(at + 0xec)),
                    'switch_from_ai_message': message(cache.u(at + 0xf0))})
            elif tag['group'] == 'ctrl':
                model = cache.u(at + 0x38)
                hlmt = cache.tags[model]['offset'] if model != 0xffffffff else None
                result['controls'].append({**identity,
                    'placements': control_placements[datum],
                    'type': cache.h(at + 0x11c),
                    'triggers_when': cache.h(at + 0x11e),
                    'action_message': message(cache.u(at + 0x124)),
                    'model': f'{model:08x}',
                    'animation_graph': f'{cache.u(hlmt + 0x14):08x}' if hlmt is not None else None})
            elif tag['group'] in ('vehi', 'bipd'):
                seats = [{'index': index, 'flags': cache.u(seat),
                          'entry': message(cache.u(seat + 0x84)),
                          'boarding_grenade': message(cache.u(seat + 0x14)),
                          'boarding_melee': message(cache.u(seat + 0x18))}
                         for index, seat in enumerate(cache.block(at + 0x1c8, 0xb0))]
                if seats or tag['group'] == 'vehi':
                    result['units'].append({**identity, 'group': tag['group'],
                        'seats': seats, 'flip': message(cache.u(at + 0x21c))
                        if tag['group'] == 'vehi' else None})
        return result
    finally:
        cache.data.close()
        cache.stream.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('maps', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    rows = [inspect(path) for path in sorted(args.maps.glob('*.map'))
            if path.name not in ('mainmenu.map', 'shared.map', 'single_player_shared.map')]
    weapon_names = {w['name'] for row in rows for w in row['weapons']}
    pickup_names = {w['name'] for row in rows for w in row['weapons']
                    if w['action_messages'][0]['text']}
    controls = {c['name'] for row in rows for c in row['controls']}
    units = {u['name'] for row in rows for u in row['units']}
    summary = {'maps': len(rows), 'scenario_types': dict(Counter(r['scenario_type'] for r in rows)),
               'weapon_definitions': len(weapon_names),
               'weapon_definitions_with_pickup_text': len(pickup_names),
               'control_definitions': len(controls), 'unit_definitions': len(units)}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({'summary': summary, 'maps': rows}, indent=2) + '\n')
    print(json.dumps(summary, sort_keys=True))


if __name__ == '__main__':
    main()
