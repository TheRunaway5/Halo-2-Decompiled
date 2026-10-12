#!/usr/bin/env python3
"""Derive a readable index and cross-map assertions from literal inventory rows."""
import argparse
import json
from pathlib import Path


def name(ref):
    return ref['target']['name'] if ref['target'] else None


def effect_summary(ref, effects):
    target = ref['target']
    result = dict(datum=ref['datum'], target=target)
    if not target or target['group'] != 'effe':
        return result
    effect = effects[ref['datum']]
    result['locations'] = [x['marker'] for x in effect['locations']]
    result['events'] = [dict(index=e['index'],
        delay_words=e['delay_words'], duration_words=e['duration_words'],
        parts=[p['reference']['target'] for p in e['parts']],
        systems=[dict(particle=p['particle']['target'], camera_mode=p['camera_mode'],
                      location=p['location'], emitter_count=len(p['emitters']))
                 for p in e['systems']]) for e in effect['events']]
    return result


def run():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('report', type=Path)
    ap.add_argument('live', type=Path)
    ap.add_argument('output', type=Path)
    args = ap.parse_args()
    data = json.loads(args.report.read_text())
    live = [json.loads(line) for line in args.live.read_text().splitlines()]
    assert len(data['maps']) == len(live) == 21
    names = {}
    scalars = []
    for m, native in zip(data['maps'], live):
        assert Path(m['path']).name == Path(native['map']).name
        assert m['scenario_type_i16_at_10'] == 1
        for w in m['weapons']:
            names.setdefault(w['tag']['name'], []).append(dict(map=Path(m['path']).stem,
                datum=w['tag']['datum'], normal_absent=w['normal_firing_reference_absent'],
                world_mode=w['world_mode'], first_person=w['first_person'],
                attachments=[dict(marker=a['marker'], target=a['reference']['target'])
                             for a in w['attachments']],
                charging=[effect_summary(t['charging_effect'], m['effects']) for t in w['triggers']],
                barrels=[dict(index=b['index'], flags=b['flags'], optional_marker=b['optional_marker'],
                    firing=[dict(count_bounds=f['count_bounds'], **{k: effect_summary(f[k], m['effects'])
                        for k in ('normal', 'misfire', 'empty')}) for f in b['firing_effects']])
                    for b in w['barrels']],
                imported_native=next((i for i in native['imported'] if i['name'] == w['tag']['name']), None)))
        smg = next(w for w in m['weapons'] if w['tag']['name'] == 'objects\\weapons\\rifle\\smg\\smg')
        assert smg['prepared_callback_scalar_154_word'] == smg['misfire_time_25c_word'] == '00000000'
        scalars.append(dict(map=Path(m['path']).stem, datum=smg['tag']['datum'],
            weapon_154_word=smg['prepared_callback_scalar_154_word'], weapon_25c_word=smg['misfire_time_25c_word']))
        assert len(native['loadouts']) == 2
        for case in native['loadouts']:
            if case['admitted']:
                assert case['selected'] == 'objects\\weapons\\rifle\\smg\\smg'
                assert (case['ticks'], case['shot_changes'], case['initial_loaded'], case['after_loaded']) == (90, 45, 60, 15)
                assert not case['after_dead'] and case['initial_unit'] == case['after_unit']
            else:
                assert Path(m['path']).stem in ('midship', 'waterworks') and case['option'] == 0
                assert case['selected'] == 'objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle'
                assert 'battery and heat combat are unfinished' in case['error']
    assert len(names) == 36
    result = dict(map_count=21, weapon_rows=sum(len(m['weapons']) for m in data['maps']),
        distinct_weapon_names=len(names), resident_effect_rows=sum(len(m['effects']) for m in data['maps']),
        model_rows=sum(len(m['models']) for m in data['maps']),
        imported_native_rows=sum(len(m['imported']) for m in live),
        admitted_loadout_cases=40, rejected_default_energy_cases=2,
        smg_scalar_admission=scalars, weapons=names)
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / 'summary.json').write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    lines = ['# Authored normal firing-reference index', '',
        'One literal representative per authored weapon name; the full JSON retains every map, datum, barrel, selection, system and marker row. The displayed camera-mode integers are authored fields, not execution results. NONE means no reference in that slot. A normal firing-reference absence does not imply absence of ready/charging/attachment visuals.', '',
        '| Weapon tag | Normal reference(s), preserving barrel/entry order | Normal effect system camera modes | Normal effect locations |',
        '| --- | --- | --- | --- |']
    for weapon, maps in sorted(names.items()):
        w = maps[0]
        refs, cameras, locations = [], [], []
        for b in w['barrels']:
            for f in b['firing']:
                r = f['normal']
                refs.append(f"b{b['index']}:{r['target']['group'] + ' ' + r['target']['name'] if r['target'] else 'NONE'}")
                if r.get('events'):
                    cameras.append('b' + str(b['index']) + ':' + ','.join(str(s['camera_mode']) for e in r['events'] for s in e['systems']))
                    locations.extend(x['text'] or '[empty StringID]' for x in r.get('locations', []))
        if not refs:
            refs = ['No firing rows (' + str(len(w['barrels'])) + ' barrels)']
        lines.append('| `' + weapon + '` | ' + '<br>'.join(refs) + ' | ' + '<br>'.join(cameras or ['No normal effect particle systems']) + ' | ' + ', '.join(dict.fromkeys(locations)) + ' |')
    (args.output / 'authored-weapons.md').write_text('\n'.join(lines) + '\n')


if __name__ == '__main__':
    run()
