#!/usr/bin/env python3
"""Unchanged physical i386 qualification for scripted goal publisher 1FC720.

EBX is the actor; four stack words carry a framed point, navigation reference,
object and flag. All actual callees execute original instructions. No owner
return, route, completion or script result is supplied. Raw observations and
compiler products require the official HALO2_TEST_SCRATCH lifecycle.
"""
import argparse
import gzip
import json
import os
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_publication_tu15 as publication

reference = publication.reference
search = publication.search
ENTRY = 0x1fc720
ENTRIES = (ENTRY,) + publication.ENTRIES
POINT = publication.BASE + 0x5500
NONE = 0xffffffff


def inputs(image):
    rows = []
    variants = []
    for flying in (0, 1, 128):
        for navigation in (NONE, 0, 1, 0x00010002):
            for gate in ('disabled', 'resume'):
                variants.append((f'restart_fly{flying}_nav{navigation:x}_{gate}',
                                 dict(flying=flying, navigation=navigation, gate=gate)))
    for active in (0, 1, 128):
        for completed in (0, 1, 128):
            variants.append((f'same_active{active}_completed{completed}',
                             dict(same=True, active=active, completed=completed)))
    for name, value in (('zero', 0.0), ('near', 0.0001), ('far', 4.0),
                        ('negzero', 0x80000000), ('subnormal', 1),
                        ('qnan', 0x7fc12345), ('snan', 0x7f812345), ('infinity', 0x7f800000)):
        variants.append(('distance_' + name, dict(same=True, point=(value, 0.0, 0.0), previous=(0.0, 0.0, 0.0))))
    for mode in (0, 1, 2, 3, 7, 0xffff):
        variants.append((f'mode{mode}', dict(action=mode)))
    for flag in (0, 1, 128, 255):
        for object_datum in (NONE, publication.UNIT):
            variants.append((f'flag{flag}_object{object_datum:x}', dict(flag=flag, object=object_datum)))
    for name, offset in (('point_self', 0x4b8), ('clear_prefix', 0x4b0),
                          ('point_forward', 0x4bc), ('reference', 0x4c8), ('unaligned', 0x4b9)):
        variants.append(('alias_' + name, dict(alias=offset)))
    variants.extend((
        ('same_different_navigation', dict(same=True, previous_navigation=1)),
        ('existing_assignment', dict(assigned=True)),
        ('same_existing_assignment', dict(same=True, assigned=True)),
        ('full_ground_query', dict(gate='full', navigation=1, point=(6.0, 2.0, 0.0))),
        ('full_flying_query', dict(gate='full', navigation=NONE, flying=1, point=(6.0, 2.0, 0.0))),
        ('update_path_refusal', dict(same=True, gate='path')),
    ))
    for label, changes in variants:
        for profile in publication.FP_PROFILES:
            scene = publication.Scene()
            publication.standard_zone(scene)
            actor = publication.ACTOR
            row = scene.row(actor)
            gate = changes.get('gate', 'disabled')
            if gate != 'full':
                # These declared early-exit inputs never lease navigation
                # workspaces; retain zero residue instead of redundant FF
                # parent-chain padding in both unused descriptor banks.
                scene.ws[:] = bytes(len(scene.ws))
            point = changes.get('point', (1.0, 2.0, 3.0))
            previous = changes.get('previous', point)
            navigation = changes.get('navigation', 0)
            actor_fields = dict(active=changes.get('active', 1), flying=changes.get('flying', 0),
                                disabled=int(gate == 'disabled'), resume=int(gate == 'resume'),
                                path=2 if gate == 'path' else 0,
                                action=changes.get('action', 2 if changes.get('same') else 4),
                                point=(*previous, 0xffff), position=(2.0, 2.0, 0.0),
                                raw={0x4c8: changes.get('previous_navigation', navigation),
                                     0x506: (changes.get('completed', 0), 1), 0x5b4: (0, 2)},
                                keep=(0x4c8, 0x506, 0x5b4))
            if changes.get('assigned'):
                actor_fields['goal'] = publication.GOAL5
            scene.actor(actor, **actor_fields)
            if changes.get('assigned'):
                scene.assign(publication.GOAL5, actor)
            pointer = row + changes['alias'] if 'alias' in changes else POINT
            for axis, value in enumerate(point):
                scene.put(pointer + axis * 4, publication.fv(value))
            scene.put(pointer + 12, 0xaabbffff)
            result = publication.make(image, rows, f'{label}_fp{profile[0]:x}_{profile[1]:x}_{profile[2]:x}',
                                      ENTRY, scene, (pointer, navigation, changes.get('object', NONE),
                                                     changes.get('flag', 0x80)), fp=profile)
            result['gp'][3] = actor
            # Query children already have independent DATA/store qualification;
            # their volatile registers and private stack are not this contract.
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--verify', type=Path)
    parser.add_argument('--select')
    args = parser.parse_args()
    assert not args.output.exists()
    assert reference.sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    meta = reference.metadata(image, ENTRIES)
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):
            site['operands'][0].update(read=False, write=True)
    owned = reference.prepare(image, meta, scratch)
    selected = inputs(image)
    if args.select:
        import re
        selected = [r for r in selected if re.search(args.select, r['name'])]
        for n, row in enumerate(selected):
            row['id'] = n
    rows = search.physical(owned, selected, 'whole')
    for row in rows:
        assert row['completed'], row['name']
        if 0x2cabe0 in row['executed_sites']:
            row['compare_reads']=False
            if row['name'].startswith('update_path_refusal_'):
                # The resumed caller now reaches the existing BD080/14D120
                # semantic child stores; retain every other exact store PC.
                row['semantic_child_store_pcs']=[0xbd0fa,0xbd10d,0x14d12d,0xbd11d,0xbd148,0xbd14e,0xbd433]
            else:
                row['semantic_child_stores']=True
    body = set(complete_body(_Code(image, '.text'), ENTRY).code)
    originals = {r['name']: r for r in rows}
    prefixes = []
    for prefix in search.select_prefixes(rows):
        phase = prefix['fault']['phase']
        original = originals[prefix['name'].rsplit('_' + phase + '_', 1)[0]]
        events = original['reads' if phase == 'before_read' else 'writes']
        if events[prefix['fault']['ordinal']]['instruction'] in body:
            prefixes.append(prefix)
    for n, prefix in enumerate(prefixes):
        prefix['id'] = len(rows) + n
    rows += search.physical(owned, prefixes, 'prefix')
    reached = set().union(*(set(row['executed_sites']) for row in rows))
    fixture = dict(fixture_kind='command-point-1fc720-v1', image_sha256=NEW_PIN,
                   backend='physical-unchanged-i386-offline', owners=meta['owners'], instruction_patches=[],
                   supplied_original_outcomes=False, stop_entries=list(publication.STOP_ENTRIES),
                   movs_sites=sorted(s['instruction'] for s in meta['sites'] if s['bytes'] in ('a4', 'a5', 'f3a4', 'f3a5')),
                   bank_encoding='sparse-runs-v1', cases=reference.compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Independent original 1FC720 repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    report = dict(passed=not args.select, cases=len(rows), whole=len(selected), prefixes=len(prefixes),
                  coverage=dict(reached=len(body & reached), total=len(body), unreached=[hex(s) for s in sorted(body - reached)]),
                  reads=sum(len(r['reads']) for r in rows), stores=sum(len(r['writes']) for r in rows),
                  original_instructions=sum(r['original_instructions'] for r in rows),
                  named_refusals={hex(e): sum(r.get('stopped_at', reference.STOP) == e for r in rows if 'fault' not in r)
                                  for e in publication.STOP_ENTRIES if any(r.get('stopped_at', reference.STOP) == e for r in rows)},
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=reference.sha(args.output),
                  independent_repeat_identical=True if args.verify else None,
                  source_sha256={str(p.relative_to(reference.ROOT)): reference.sha(p) for p in
                                 (Path(__file__), Path(publication.__file__), Path(reference.__file__), Path(search.__file__))},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
