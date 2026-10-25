#!/usr/bin/env python3
"""Regenerate a bounded independent TU1.5 component subset in owned scratch.

This does not replace the historical full corpora. The original executable
produces every expected instruction, store and output; native results are
never inputs to this generator. Actual MCC match acceptance remains separate.
"""
import argparse
import hashlib
import importlib
import json
import lzma
import os
from pathlib import Path

from inspect_multiplayer import Cache
from oracle_smg_firing_1_1 import ROOT, CACHE_PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'), 'sha256').hexdigest()


def subset(rows):
    groups = {}
    for index, row in enumerate(rows):
        if not row.get('continuation'):
            groups.setdefault((row['entry'], row.get('definition', -1)), []).append(index)
    selected = sorted({indices[n] for indices in groups.values()
                       for n in (0, len(indices)//2, len(indices)-1)})
    return selected, [rows[index] for index in selected]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--retain-references', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output = args.output.resolve()
    if not args.output.is_relative_to(scratch):
        parser.error('Raw original fixtures must stay in the owned scratch directory')
    assert sha(NEW_DEFAULT) == NEW_PIN
    cache = Cache(ROOT/'assets/disc/maps/lockout.map')
    assert sha(cache.path) == CACHE_PIN
    tags = [tag for tag in cache.tags.values() if tag['group'] == 'jpt!' and tag['offset'] is not None]
    args.output.mkdir(parents=True, exist_ok=False)
    args.retain_references.mkdir(parents=True, exist_ok=False)
    report = dict(reference='English OG Xbox TU1.5', original_sha256=NEW_PIN,
                  cache_sha256=CACHE_PIN, instruction_patches=[], native_expected_data_used=False,
                  scope=__doc__, fixtures={}, retained_bytes=0)
    for name, module_name in (
            ('vehicle', 'oracle_vehicle_runtime_20261011'),
            ('vitality', 'measure_source_vitality_runtime'),
            ('direct', 'measure_source_direct_damage_runtime'),
            ('delivery', 'measure_source_damage_delivery_runtime'),
            ('feedback', 'measure_source_damage_feedback_vitality')):
        module = importlib.import_module(module_name)
        if name == 'vehicle':
            correspondence = ROOT/'analysis/multiplayer/sandbox-source/vehicle-runtime-20261011/correspondence-v3/report.json'
            paired = json.loads(correspondence.read_text())
            assert paired['extension_tool_sha256'] == sha(ROOT/'tools/research_vehicle_runtime_20261011.py')
            definitions = module.definitions()
            indices, rows = subset(module.controls(definitions))
            cases = module.execute(NEW_DEFAULT, paired, True, definitions, rows)
        else:
            correspondence = module.BASE/'generated/normalized-correspondence.json'
            paired = json.loads(correspondence.read_text())
            assert paired['later_owned_xbe_sha256'] == NEW_PIN
            indices, rows = subset(module.rows(tags) if name in ('direct', 'delivery') else module.rows())
            cases = module.execute(NEW_DEFAULT, True, paired, cache, rows)
            if name == 'delivery':
                # Preserve the oracle's original bounded owner scope when a
                # newer linked source closure also owns its exit helper.
                # Names come from the authenticated correspondence, never
                # from native execution or expected gameplay outcomes.
                inverse = {value: int(key, 16) for key, value in paired['address_map'].items()}
                for case in cases:
                    if not case['complete'] and case['exit_entry'] in inverse:
                        case['original_prefix_exit_site'] = inverse[case['exit_entry']]
        fixture = dict(schema=1, image='1.5', cases=cases, original_sha256=NEW_PIN,
                       oracle_tool_sha256=sha(module.__file__), correspondence_sha256=sha(correspondence),
                       selected_oracle_rows=indices, instruction_patches=[], native_expected_data_used=False,
                       bounded_subset=True)
        path = args.output/name/'u5.json'
        path.parent.mkdir()
        raw = (json.dumps(fixture, separators=(',', ':'), allow_nan=False)+'\n').encode()
        path.write_bytes(raw)
        packed = lzma.compress(raw)
        assert len(packed) <= 1024*1024 and report['retained_bytes']+len(packed) <= 8*1024*1024
        retained = args.retain_references/(name+'-u5.json.xz')
        retained.write_bytes(packed)
        assert lzma.decompress(retained.read_bytes()) == raw
        report['retained_bytes'] += len(packed)
        report['fixtures'][name] = dict(path=str(path), raw_bytes=len(raw), sha256=sha(path),
            retained=str(retained), retained_bytes=len(packed), retained_sha256=sha(retained),
            cases=len(cases), complete=sum(case['complete'] for case in cases),
            selected_oracle_rows=indices, oracle_tool_sha256=sha(module.__file__),
            correspondence_sha256=sha(correspondence))
        args.report.write_text(json.dumps(report, indent=2)+'\n')
        print(json.dumps(dict(fixture=name, cases=len(cases), reference_bytes=len(packed))), flush=True)


if __name__ == '__main__':
    main()
