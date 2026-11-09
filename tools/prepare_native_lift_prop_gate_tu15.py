#!/usr/bin/env python3
"""Seal an actual authored inactive prop and nonzero-marker lift gate input."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from mcc_content import MccCache

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--maps', required=True, type=Path)
    parser.add_argument('--metadata', required=True, type=Path)
    parser.add_argument('--cases', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    path = args.maps.resolve() / 'lockout.map'
    pins = {str(p.resolve()): digest(p) for p in (path, args.metadata, args.cases, Path(__file__))}
    cache = MccCache(path)
    scenario = next(t for t in cache.tags.values() if t['group'] == 'scnr')
    q = scenario['offset']
    palette = cache.block(q + 0x330, 0x28)
    placements = []
    for index, at in enumerate(cache.block(q + 0x328, 0x4c)):
        palette_index = struct.unpack_from('<h', cache.data, at)[0]
        definition = cache.u32(palette[palette_index] + 4)
        if definition != 0xe86e0074:
            continue
        placements.append(dict(block='bloc', index=index, content_placement_index=9 + index,
                               definition=definition, definition_name=cache.tags[definition]['name'],
                               position=list(struct.unpack_from('<fff', cache.data, at + 8)),
                               flags=cache.u32(at + 4), unique_id=cache.u32(at + 0x28),
                               raw_hex=cache.data[at:at + 0x4c].hex()))
    metadata = json.loads(args.metadata.read_text())
    assert metadata['passed'] and metadata['map_sha256'] == digest(path)
    observed = metadata['observed']
    islands = {i['identity']: i for i in observed['islands']}
    candidates = []
    for record in observed['records']:
        if record['definition'] != 0xe86e0074 or record['static_flag']:
            continue
        position = list(struct.unpack_from('<fff', bytes.fromhex(record['slot_hex'])))
        matches = [p for p in placements if sum((a-b)**2 for a,b in zip(p['position'], position)) < 1e-8]
        if len(matches) != 1:
            continue
        island = islands[record['island']]
        if island['active'] or not island['pair_count']:
            continue
        factories = [p.get('factory_owner') for p in island['missing_agent_dispatch']]
        assert factories and set(factories) == {0x303c10}
        component = bytes.fromhex(record['component_hex'])
        candidates.append(dict(authored=matches[0], object=record['object'], object_type=record['object_type'],
                               physical_component_group=struct.unpack_from('<b', component, 0x1c)[0],
                               component=record['component'], body=record['body'], slot=record['slot'],
                               initial_position=position, inactive_island=record['island'],
                               motion_vtable=record['motion_vtable'], linear_setter=record['linear_setter'],
                               angular_setter=record['angular_setter'], policy=record['policy'],
                               missing_agent_factories=factories))
    assert candidates
    prop = min(candidates, key=lambda p: p['authored']['index'])
    fields = json.loads(args.cases.read_text())['cases']
    field = next(c for c in fields if c['id'] == 'lockout:bloc:0:e8630069:4:0')
    force = field['force']
    assert force['marker'] and not force['flags'] & (1 << (prop['object_type'] + 5))
    assert force['min_size'] <= prop['physical_component_group'] <= force['max_size']
    assert not force['flags'] & 16
    assert all(digest(Path(p)) == pin for p, pin in pins.items())
    result = dict(schema='native_tu15_authored_prop_lift_gate_input_v1', passed=True,
                  input_pins=pins, map=str(path), map_sha256=digest(path), prop=prop, field=field,
                  canonical_inactive_prop_candidates=len(candidates), source_wake_listener=0x282870,
                  permitted_input='PhysicsLiveWorldTu15::set_object_position (genuine source B95D0)',
                  live_qualified=False,
                  required_live_observations=['full object generation and unique authored position/definition',
                    'inactive before declared input; actual native activation/listener calls and return',
                    'exact field object/member0 plus full entrant datum membership',
                    'native velocity calls and completed stores for the exact physical component',
                    'native BAD60 request for the selected field and nonzero authored marker returns count>0',
                    'finite retained motion, leave and reentry; zero native refusal delta'],
                  forbidden=['activation/velocity afterimage writes', 'fabricated membership or completion'],
                  scope='A concrete authored gate input; source admission flags do not establish actual live behavior.',
                  retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({'prop_object': hex(prop['object']), 'placement': prop['authored']['index'],
                      'group': prop['physical_component_group'], 'field': field['id'],
                      'marker': hex(force['marker']), 'output': str(args.output)}))


if __name__ == '__main__':
    main()
