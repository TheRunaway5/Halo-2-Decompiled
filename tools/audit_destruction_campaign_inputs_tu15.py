#!/usr/bin/env python3
"""Authenticate finite campaign destruction constructor inputs, without execution.

Run through tools/run_test.py. Original/cache assets are read-only reference
inputs; retain only the bounded metadata report. This proves no live outcome.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from inspect_multiplayer import Cache
from mcc_content import MccCache


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def placements(cache, definition, *, mcc, group='vehi'):
    word = cache.u32 if mcc else cache.u
    scenario = cache.tags[word(cache.index + 12)]['offset'] if mcc else cache.scenario
    placed,palette_at,stride = (0x70,0x78,0x54) if group=='vehi' else (0x50,0x58,0x5c)
    palette = [word(at + 4) for at in cache.block(scenario + palette_at, 0x28)]
    result = []
    for ordinal, at in enumerate(cache.block(scenario + placed, stride)):
        choice = struct.unpack_from('<h', cache.data, at)[0]
        if choice == -1:
            continue
        if not 0 <= choice < len(palette):
            raise ValueError('Campaign vehicle row escapes its authored palette')
        tag = cache.tags[palette[choice]]
        if tag['group'] != group or tag['name'] != definition:
            continue
        # Literal fields consumed by Cache::load_content and original C1120.
        result.append(dict(placement=ordinal, definition=tag['name'],
                           flags=word(at + 4),
                           position_rotation_words=list(struct.unpack_from('<6I', cache.data, at + 8)),
                           scale_word=word(at + 0x20),
                           manual_bsps=struct.unpack_from('<H', cache.data, at + 0x26)[0],
                           unique_id=word(at + 0x28),
                           origin_bsp=struct.unpack_from('<h', cache.data, at + 0x2c)[0],
                           source=struct.unpack_from('<b', cache.data, at + 0x2f)[0],
                           bsp_policy=cache.data[at + 0x30]))
        if group=='scen':
            result[-1]['literal_row_words']=list(struct.unpack_from('<'+str(stride//4)+'I',cache.data,at))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', required=True, type=Path)
    parser.add_argument('--map', required=True)
    parser.add_argument('--definition', required=True)
    parser.add_argument('--group',choices=('vehi','scen'),default='vehi')
    parser.add_argument('--weapon', help='Exact resident weapon definition supplied as ordinary equipment input')
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    census = json.loads(args.contracts.read_text())
    if census['schema'] != 'H2_DESTRUCTION_CONTRACTS_V1' or not census['inventory_completed']:
        raise ValueError('Require the completed original/MCC contract census')
    owners = [row for row in census['source_files'] if Path(row['path']).stem == args.map]
    if len(owners) != 1:
        raise ValueError('Campaign map owner is missing or ambiguous')
    owner = owners[0]
    original_path, imported_path = Path(owner['path']), Path(owner['mcc_path'])
    if sha(original_path) != owner['sha256'] or sha(imported_path) != owner['mcc_sha256']:
        raise ValueError('A constructor reference changed since its contract census')
    original, imported = Cache(original_path), MccCache(imported_path)
    try:
        before = placements(original, args.definition, mcc=False,group=args.group)
        after = placements(imported, args.definition, mcc=True,group=args.group)
        weapon = None
        if args.weapon:
            weapon = {}
            for label,cache in [('original',original),('mcc',imported)]:
                weapon[label] = [dict(group=tag['group'],name=tag['name'])
                                 for tag in cache.tags.values()
                                 if tag['group']=='weap' and tag['name']==args.weapon]
            if weapon['original']!=weapon['mcc'] or len(weapon['original'])!=1:
                raise ValueError('Declared campaign weapon is absent or ambiguous in original/MCC owners')
        report = dict(passed=bool(before) and before == after,
                      scope='Read-only authored constructor input identity; no original creation, damage, physics or presentation execution',
                      full_goal_complete=False, map=args.map, definition=args.definition,group=args.group,
                      original=before, mcc=after, source_owner=owner,
                      resident_weapon=weapon,
                      tool_sha256=sha(__file__), contracts_sha256=sha(args.contracts),
                      owned_scratch=os.environ['HALO2_TEST_SCRATCH'],
                      retained_large_artifacts=[])
        if not args.weapon:
            report['resident_weapon_names']={label:sorted(tag['name'] for tag in cache.tags.values() if tag['group']=='weap')
                                            for label,cache in [('original',original),('mcc',imported)]}
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(dict(passed=report['passed'], rows=len(before),
                              origin_bsps=[row['origin_bsp'] for row in before])))
        return 0 if report['passed'] else 1
    finally:
        imported.close()
        if hasattr(original, 'close'):
            original.close()


if __name__ == '__main__':
    raise SystemExit(main())
