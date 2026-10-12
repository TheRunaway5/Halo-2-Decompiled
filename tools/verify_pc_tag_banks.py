#!/usr/bin/env python3
"""Verify retained tag audit repeats, source pins, counts and sampled raw bytes.

This does not use the audit's schema walker to check reported byte locations.
It checks every difference's group/reason accounting and selected actual source
bytes in each map/group/reason. Complete loader/gameplay parity is not asserted.
"""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import struct

from inspect_multiplayer import Cache
from inspect_pc_combat_tag_sample import PcCache, sha, locate_original_leaf

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/pc-gameplay-tag-comparison-20261011'
NUMERIC_TYPES = {'int8', 'uint8', 'enum8', 'flags8', 'int16', 'uint16', 'enum16', 'flags16',
                 'int32', 'uint32', 'enum32', 'flags32', 'float32', 'degree', 'int64', 'uint64',
                 'datum', 'color32', 'range16', 'point16', 'rangef', 'ranged', 'point2', 'vector2',
                 'degree2', 'point3', 'vector3', 'degree3', 'colorf', 'plane2', 'plane3', 'vector4',
                 'rect16', 'quat16'}


def tags(cache):
    result = {}
    for t in cache.tags.values():
        key = (t['group'], t['name'])
        assert key not in result
        result[key] = t
    return result


def private_view(cache, stores, region):
    data = bytearray(cache.data)
    for s in stores:
        if s['region'] != region or (region == 'actual_resident_metadata' and s['owner'] != str(cache.path)):
            continue
        at = s['file_offset']
        at = int(at, 16) if isinstance(at, str) else at
        assert data[at:at + s['size']].hex() == s['before_hex']
        data[at:at + s['size']] = bytes.fromhex(s['after_hex'])
    return data


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--measured', type=Path, default=BASE / 'measured')
    ap.add_argument('--repeat', type=Path, default=BASE / 'repeat')
    ap.add_argument('--output', type=Path, default=BASE / 'verification.json')
    ap.add_argument('--source-only', action='store_true', help='Verify canonical data while a separate repeat is running.')
    ap.add_argument('--finish-repeat', type=Path, help='Complete a pinned source-only verification with exact repeat checks.')
    a = ap.parse_args()
    if a.finish_repeat:
        result = json.loads(a.finish_repeat.read_text())
        assert result['all_source_checks_passed'] and not result['all_checks_passed']
        assert result['verifier_sha256'] == sha(Path(__file__))
        assert result['source_report_sha256'] == sha(a.measured / 'report.json')
        assert result['coverage_sha256'] == sha(BASE / 'coverage.json')
        paths = sorted(p.relative_to(a.measured) for p in a.measured.rglob('*') if p.is_file())
        assert list(map(str, paths)) == sorted(result['retained_file_hashes'])
        assert paths == sorted(p.relative_to(a.repeat) for p in a.repeat.rglob('*') if p.is_file())
        for rel in paths:
            assert sha(a.measured / rel) == result['retained_file_hashes'][str(rel)] == sha(a.repeat / rel)
        for path, digest in result['source_hashes'].items():
            assert sha(Path(path)) == digest
        result.update(all_checks_passed=True, retained_repeat_files=len(paths),
                      source_verification_sha256=sha(a.finish_repeat))
        a.output.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
        print(json.dumps({'all_checks_passed': True, 'retained_repeat_files': len(paths),
                          'difference_records_counted': result['difference_records_counted'],
                          'independently_read_field_locations': result['independently_read_field_locations'],
                          'mcc_stored_original_combat_destinations_matched': result['mcc_stored_original_combat_destinations_matched']}))
        return
    report = json.loads((a.measured / 'report.json').read_text())
    coverage = json.loads((BASE / 'coverage.json').read_text())
    assert coverage['source_report_sha256'] == sha(a.measured / 'report.json')
    for platform, totals in report['totals'].items():
        for key, value in totals.items():
            assert coverage['platforms'][platform][key] == value
    paths = sorted(p.relative_to(a.measured) for p in a.measured.rglob('*') if p.is_file())
    if not a.source_only:
        assert paths == sorted(p.relative_to(a.repeat) for p in a.repeat.rglob('*') if p.is_file())
    pins = {}
    for rel in paths:
        digest = sha(a.measured / rel)
        if not a.source_only:
            assert digest == sha(a.repeat / rel), rel
        pins[str(rel)] = digest
    source_pins = {str(ROOT / rel): digest for rel, digest in report['schemas'].items()}
    for key, path in [('tool_sha256', ROOT / 'tools/compare_pc_tag_banks.py'),
                      ('reader_sha256', ROOT / 'tools/inspect_pc_combat_tag_sample.py'),
                      ('original_oracle_sha256', ROOT / 'tools/oracle_mp_combat_postprocess_updates.py'),
                      ('mcc_schema_provenance_sha256', BASE / 'schemas/provenance.json')]:
        source_pins[str(path)] = report[key]
    for r in report['original_map_inventory']:
        source_pins[r['path']] = r['sha256']
    for rows in report['pc_map_inventory'].values():
        for r in rows:
            source_pins[r['path']] = r['sha256']
    for r in report['shared_inputs'].values():
        source_pins[r['path']] = r['sha256']
    for path, digest in source_pins.items():
        assert sha(Path(path)) == digest, path
    assert len(report['maps']) == 23
    assert report['totals']['mcc']['compared_maps'] == 23
    assert report['totals']['vista']['compared_maps'] == 21
    assert report['totals']['vista']['missing_original_maps'] == ['derelict', 'highplains']
    assert all(v == ['needle', 'street_sweeper'] for v in report['pc_unmatched_maps'].values())
    shared = {label: PcCache(Path(r['path'])) for label, r in report['shared_inputs'].items()}
    records = samples = mcc_destinations = 0
    checks = []
    documented_examples = {}
    for name, row in report['maps'].items():
        xbox = Cache(Path(row['xbox_path']))
        try:
            xreport = a.measured / 'xbox-original-postprocess' / name / 'report.json'
            assert sha(xreport) == row['postprocess_report_sha256']
            xr = json.loads(xreport.read_text())
            xbe = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
            assert sha(xbe) == xr['xbe_sha256']
            source_pins[str(xbe)] = xr['xbe_sha256']
            xdata = private_view(xbox, xr['ordered_stores'], 'original_cache_metadata')
            assert hashlib.sha256(xdata).hexdigest() == xr['complete_cache_output_sha256']
            xtags = tags(xbox)
            for label, c in row['comparisons'].items():
                if c.get('missing_map'):
                    continue
                pc = PcCache(Path(c['input']['path']))
                assert pc.provenance == c['input']
                pdata, sdata = pc.data, shared[label].data
                if label == 'vista':
                    pr = c['original_vista_postprocess']
                    ppath = Path(pr['report_path'])
                    assert sha(ppath) == pr['report_sha256']
                    oracle = json.loads(ppath.read_text())
                    assert sha(Path(oracle['executable_path'])) == pr['executable_sha256']
                    source_pins[oracle['executable_path']] = pr['executable_sha256']
                    pdata = private_view(pc, oracle['ordered_stores'], 'actual_resident_metadata')
                    sdata = private_view(shared[label], oracle['ordered_stores'], 'actual_resident_metadata')
                    assert hashlib.sha256(pdata).hexdigest() == pr['level_view_sha256']
                    assert hashlib.sha256(sdata).hexdigest() == pr['shared_view_sha256']
                ptags, stags = tags(pc), tags(shared[label])
                if name == 'ascension':
                    specs = [('matg', 'globals\\globals', 'Materials', 0x150, 4),
                             ('weap', 'objects\\weapons\\melee\\energy_blade\\energy_blade',
                              'Damage Pyramid Angles first word', 0x1b0, 4),
                             ('weap', 'objects\\weapons\\rifle\\battle_rifle\\battle_rifle',
                              'Zoom-In Sound', 0x26c, 8),
                             ('weap', 'objects\\weapons\\multiplayer\\ball\\ball',
                              'Weapon flags', 0x12c, 4)]
                    for source_label, cache, data, index in [('xbox', xbox, xdata, xtags),
                                                             (label, pc, pdata, ptags)]:
                        if source_label in documented_examples:
                            continue
                        example_rows = []
                        for group, tagname, field, offset, size in specs:
                            owner, view, tag = cache, data, index[group, tagname]
                            if tag['offset'] is None:
                                owner, view, tag = shared[label], sdata, stags[group, tagname]
                                assert tag['datum'] == index[group, tagname]['datum']
                            at = tag['offset'] + offset
                            raw = bytes(view[at:at + size])
                            assert at + size <= tag['offset'] + tag['size']
                            value = raw.hex()
                            if field == 'Materials':
                                value = struct.unpack('<I', raw)[0]
                                assert value == (182 if source_label == 'xbox' else 183)
                            elif field == 'Damage Pyramid Angles first word':
                                assert value == ('e3dfd63e' if source_label == 'xbox' else 'e4dfd63e')
                            elif field == 'Zoom-In Sound':
                                datum = f'{struct.unpack_from("<I", raw, 4)[0]:08x}'
                                targets = [t for t in cache.tags.values() if t['datum'] == datum]
                                assert len(targets) == 1 and targets[0]['group'] == 'snd!'
                                value = targets[0]['name']
                                assert value == ('sound\\weapons\\battle_rifle\\zoom_in' if source_label == 'mcc'
                                                 else 'sound\\weapons\\sniper_rifle\\zoom_in')
                            else:
                                assert value == ('58003400' if source_label == 'mcc' else 'd8003400')
                            example_rows.append({'tag_group': group, 'tag_name': tagname, 'field': field,
                                                 'owner': str(owner.path), 'offset': at, 'size': size,
                                                 'raw_hex': raw.hex(), 'value': value})
                        documented_examples[source_label] = example_rows
                if label == 'mcc':
                    measured = [s for s in xr['ordered_stores'] if s['region'] == 'original_cache_metadata']
                    assert len(measured) == 41
                    for store in measured:
                        assert len(store['schema_annotations']) == 1
                        annotation = locate_original_leaf(xbox, store['tag'], store['schema_annotations'][0])
                        identity = (store['tag']['group'], store['tag']['name'])
                        owner, tag = pc, ptags[identity]
                        if tag['offset'] is None:
                            owner, tag = shared[label], stags[identity]
                            assert tag['datum'] == ptags[identity]['datum']
                        at, raw = owner.field(tag, annotation)
                        assert raw.hex() == store['after_hex'], (name, identity, annotation, raw.hex())
                        mcc_destinations += 1
                group_counts, selected = {}, set()
                path = a.measured / c['difference_file']
                assert sha(path) == c['differences_sha256']
                local_records = local_samples = 0
                with gzip.open(path, 'rt') as stream:
                    for line in stream:
                        d = json.loads(line)
                        group_counts.setdefault(d['tag_group'], Counter())[d['reason']] += 1
                        local_records += 1
                        # Independently read at least the first numeric/count
                        # difference for every group/reason pair in each map.
                        key = (d['tag_group'], d['reason'])
                        if key in selected:
                            continue
                        identity = (d['tag_group'], d['tag_name'])
                        did_check = False
                        for side in ('xbox', 'pc'):
                            f = d[side]
                            if f is None or f['type'] not in NUMERIC_TYPES | {'tagblock_count', 'dataref_size'}:
                                continue
                            cache, data, tag = xbox, xdata, xtags[identity]
                            if side == 'pc':
                                cache, data, tag = pc, pdata, ptags[identity]
                                if tag['offset'] is None:
                                    cache, data, tag = shared[label], sdata, stags[identity]
                                    assert tag['datum'] == ptags[identity]['datum']
                            at, size = f['offset'], f['size']
                            assert tag['offset'] <= at <= at + size <= tag['offset'] + tag['size'] <= len(data)
                            raw = bytes(data[at:at + size])
                            value = raw.hex() if f['type'] in NUMERIC_TYPES else struct.unpack('<i', raw)[0]
                            assert value == f['value'], (name, label, d, side)
                            local_samples += 1
                            did_check = True
                        if did_check:
                            selected.add(key)
                assert local_records == c['difference_records']
                for group, counts in group_counts.items():
                    assert counts == Counter({k: v for k, v in c['groups'][group].items()
                                              if k not in ('common_tags', 'equal_fields')})
                records += local_records
                samples += local_samples
                checks.append({'map': name, 'platform': label, 'difference_records': local_records,
                               'independently_read_field_locations': local_samples})
                print(json.dumps(checks[-1]), flush=True)
        finally:
            xbox.data.close()
            xbox.stream.close()
    sample = BASE / 'corrected-sample'
    assert (sample / 'report.json').read_bytes() == (sample / 'repeat.json').read_bytes()
    result = {'all_checks_passed': not a.source_only, 'all_source_checks_passed': True,
              'retained_canonical_files': len(paths), 'retained_repeat_files': 0 if a.source_only else len(paths),
              'source_report_sha256': sha(a.measured / 'report.json'), 'coverage_sha256': sha(BASE / 'coverage.json'),
              'difference_records_counted': records, 'independently_read_field_locations': samples,
              'original_maps': 23, 'platform_map_comparisons': len(checks),
              'mcc_stored_original_combat_destinations_matched': mcc_destinations,
              'corrected_sample_repeat_equal': True, 'checks': checks,
              'documented_examples': documented_examples,
              'retained_file_hashes': pins, 'source_hashes': source_pins,
              'verifier_sha256': sha(Path(__file__)),
              'scope': 'Exact retained repeat files; current input/schema/tool hashes; every difference group/reason count; selected reported numeric/count locations per map/group/reason checked against private original-instruction-processed cache bytes independently of the schema walker. Unknown schema semantics, opaque payloads, other loader fixups and complete gameplay remain excluded.'}
    assert mcc_destinations == 23 * 41
    a.output.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k not in ('checks', 'retained_file_hashes', 'source_hashes', 'documented_examples')}))


if __name__ == '__main__':
    main()
