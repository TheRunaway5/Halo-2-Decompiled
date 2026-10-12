#!/usr/bin/env python3
"""Audit original particle-render rejection and partial context constructor.

Replays original ordered stores against complete retained renderer/player/pool
records. Does not fabricate missing particle-to-sprite callback outputs.
"""
import argparse
import json
from pathlib import Path
from oracle_smg_effect_owner_1_1 import ROOT, sha, save, encoded


def regions(state):
    result = []
    def add(name, address, raw):
        result.append((name, int(address, 16), bytes.fromhex(raw)))
    for b in state['renderer_banks']:
        add('renderer_bank_' + b['address'], b['address'], b['raw2ae0_hex'])
    for g in state['globals']:
        add('global_' + g['address'] + '_' + str(g['size']), g['address'], g['raw_hex'])
    s = state['source_owner']
    add('player', s['player']['pointer'], s['player']['raw21c_hex'])
    for p in s['pools']:
        if not int(p['pointer'], 16):
            continue
        add('pool_header_' + p['global_address'], p['pointer'], p['header4c_hex'])
        add('pool_bitmap_' + p['global_address'], p['bitmap_pointer'], p['bitmap_hex'])
        for slot in p['occupied']:
            add('pool_' + p['global_address'] + '_' + slot['datum'], slot['pointer'], slot['raw_hex'])
    return {name: (address, raw) for name, address, raw in result}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--report', required=True, type=Path)
    ap.add_argument('--static', required=True, type=Path)
    ap.add_argument('--output', required=True, type=Path)
    a = ap.parse_args()
    r = json.loads(a.report.read_text())
    for path, pin in r['frozen_pins'].items():
        assert sha(ROOT / path) == pin, path
    replayed = []
    for stage in r['stages']:
        before, after = regions(stage['before']), regions(stage['after'])
        for name, (start, initial) in before.items():
            if name not in after or after[name][0] != start:
                continue
            current = bytearray(initial)
            count = 0
            for write in r['stores']:
                if write['phase'] != stage['name']:
                    continue
                w = int(write['address'], 16)
                value = bytes.fromhex(write['written_hex'])
                lo, hi = max(start, w), min(start+len(current), w+len(value))
                if lo < hi:
                    observed = bytes.fromhex(write['before_hex'])
                    assert current[lo-start:hi-start] == observed[lo-w:hi-w], (stage['name'], name, write)
                    current[lo-start:hi-start] = value[lo-w:hi-w]
                    count += 1
            assert bytes(current) == after[name][1], (stage['name'], name)
            replayed.append(dict(stage=stage['name'], record=name, size=len(current), writes=count,
                before_sha256=__import__('hashlib').sha256(initial).hexdigest(),
                after_sha256=__import__('hashlib').sha256(current).hexdigest()))
    assert len(r['stages']) >= 3
    assert r['unreturned_calls'] == [q['id'] for q in r['calls'] if 'return_cpu' not in q]
    constructor = r['stages'][2]
    assert constructor['completion'] == 'blocked_original_instruction'
    assert constructor['stopped_eip'] == '800000b8'
    assert constructor['invalid']['requested_base'] == '08001000'
    assert constructor['invalid']['requested_size'] == 4096
    assert any(q['entry'] == '00133760' and 'return_cpu' not in q for q in r['calls'])
    assert any(q['entry'] == '001360f0' and 'return_cpu' not in q for q in r['calls'])
    rejected = []
    for stage in r['stages'][:2]:
        points = [q for q in r['points'] if q['phase'] == stage['name']]
        assert any(q['instruction'] == '00175d30' for q in points)
        assert any(q['instruction'] == '00175d39' and int(q['cpu']['registers']['eax'], 16)&255 == 0 for q in points)
        assert not any(q['entry'] in ('00175ea0', '00176a00', '002263b0', '00226430',
                                      '00177090', '002265c0', '00240e30')
                       for q in r['calls'] if q['phase'] == stage['name'])
        assert stage['before']['source_owner'] == stage['after']['source_owner']
        rejected.append(dict(stage=stage['name'], original_visibility_false=True,
            no_location_sprite_or_vertex_handoff=True, complete_source_owner_unchanged=True))
    static = json.loads(a.static.read_text())
    for owner in static['owners']:
        assert len(owner['full_body_matches']) == 1
        assert owner['complete_instruction_and_internal_branch_equal']
        for listing in owner['listings']:
            assert sha(a.static.parent / listing['file']) == listing['sha256']
    for owner in static['tu_only_owning_context']:
        assert sha(a.static.parent / owner['file']) == owner['sha256']
    for listing in r['instructions']:
        assert sha(a.report.parent / listing['file']) == listing['sha256']
    save(a.output, encoded(dict(tool_sha256=sha(__file__), report_sha256=sha(a.report),
        static_sha256=sha(a.static), prior_pins_verified=len(r['frozen_pins']),
        rejected_original_passes=rejected, literal_buffer_store_replays=replayed,
        interval_count=len(replayed), complete_owner_correspondences=len(static['owners']),
        no_supplied_draw_results=True)))


if __name__ == '__main__':
    main()
