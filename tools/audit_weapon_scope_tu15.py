#!/usr/bin/env python3
"""Inventory every resident weapon in the owned campaign/MP/TU1.5 caches.

Raw authored contracts are evidence of scope, never completed live behavior.
No game asset is changed. Old duplicate DLC directories are excluded explicitly.
"""
import argparse
import hashlib
import json
from pathlib import Path

from inspect_multiplayer import Cache, ROOT
from weapon_reference_versions import images


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def weapon(cache, tag):
    c, p = cache, tag['offset']
    if p is None:
        return dict(tag=tag, resident=False)
    result = dict(tag=tag, resident=True, root_hex=bytes(c.data[p:p + 0x31c]).hex(),
                  inventory_flags=f'{c.u(p + 0x12c):08x}', weapon_type=c.h(p + 0x292),
                  magazines=[], triggers=[], barrels=[], first_person=[])
    for at in c.block(p + 0x2a8, 0x10):
        result['first_person'].append(dict(model=c.reference(c.u(at + 4)),
                                          animation=c.reference(c.u(at + 12))))
    for at in c.block(p + 0x2c0, 0x5c):
        result['magazines'].append(dict(file_offset=at,
            raw_hex=bytes(c.data[at:at + 0x5c]).hex(),
            initial=c.h(at + 6), capacity=c.h(at + 0xa), reserve=c.h(at + 0xc)))
    for at in c.block(p + 0x2c8, 0x40):
        result['triggers'].append(dict(file_offset=at,
            raw_hex=bytes(c.data[at:at + 0x40]).hex(), behavior=c.h(at + 6),
            primary_barrel=c.h(at + 8), secondary_barrel=c.h(at + 0xa)))
    for at in c.block(p + 0x2d0, 0xec):
        result['barrels'].append(dict(file_offset=at,
            raw_hex=bytes(c.data[at:at + 0xec]).hex(), flags=f'{c.u(at):08x}',
            rate=[c.f(at + 4), c.f(at + 8)], burst=[c.h(at + 0x1c), c.h(at + 0x1e)],
            recovery=c.f(at + 0x20), recovery_fraction=c.f(at + 0x24),
            magazine=c.h(at + 0x28), projectile=c.reference(c.u(at + 0x90)),
            firing_effects=[dict(file_offset=e, raw_hex=bytes(c.data[e:e + 0x34]).hex(),
                normal=c.reference(c.u(e + 8)), misfire=c.reference(c.u(e + 0x10)),
                empty=c.reference(c.u(e + 0x18))) for e in c.block(at + 0xe4, 0x34)]))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--dlc-root', type=Path, default=Path.home() /
        '.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c')
    args = parser.parse_args()
    paths = sorted((ROOT / 'assets/disc/maps').glob('*.map'))
    paths += sorted(p for p in args.dlc_root.rglob('*.map') if not p.parent.name.endswith('old'))
    excluded = {'mainmenu', 'shared', 'single_player_shared'}
    report = dict(schema='weapon-tu15-goal-scope-v1', scope=__doc__,
                  tool_sha256=sha(Path(__file__)),
                  images=[dict(label=label, path=str(image.path), sha256=pin)
                          for label, image, pin in images()], maps=[])
    by_name = {}
    for path in paths:
        if path.stem in excluded:
            continue
        c = Cache(path)
        try:
            rows = [weapon(c, t) for t in c.tags.values() if t['group'] == 'weap']
            report['maps'].append(dict(path=str(path), sha256=sha(path),
                scenario_type=c.h(c.scenario + 0x10), weapons=rows))
            for row in rows:
                by_name.setdefault(row['tag']['name'], []).append(dict(map=str(path), datum=row['tag']['datum']))
            print(f'{path.stem}: {len(rows)} authored weapon definitions', flush=True)
        finally:
            c.data.close()
            c.stream.close()
    report['weapons'] = [dict(name=name, occurrences=occurrences,
        feature_parity='unverified', visual_parity='unverified')
        for name, occurrences in sorted(by_name.items())]
    report['summary'] = dict(maps=len(report['maps']), distinct_weapons=len(by_name),
        weapon_rows=sum(len(m['weapons']) for m in report['maps']),
        completed_weapon_feature_and_visual_parity=0)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps(report['summary']), flush=True)


if __name__ == '__main__':
    main()
