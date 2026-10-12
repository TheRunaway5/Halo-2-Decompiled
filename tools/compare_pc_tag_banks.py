#!/usr/bin/env python3
"""Audit named schema-readable fields of the available multiplayer tag banks.

Compares exact scalar bits and normalized references/strings. Unknown layouts,
external resource payloads and invalid schema reads stay explicit exclusions.
Xbox and Vista combat fields are processed by their unchanged original routines.
"""
import argparse
from collections import Counter, defaultdict
import copy
import gzip
import hashlib
import json
from pathlib import Path
import struct
import xml.etree.ElementTree as ET

from inspect_multiplayer import Cache, MAPS
from inspect_pc_combat_tag_sample import PcCache, sha
from oracle_mp_combat_postprocess_updates import run_one, IMAGES

ROOT = Path(__file__).resolve().parents[1]
PLUGINS = ROOT / 'tools/reference/assembly/src/Assembly/Plugins'
MCC_SCHEMAS = ROOT / 'analysis/multiplayer/pc-gameplay-tag-comparison-20261011/schemas/Halo2MCC'
MCC = Path('/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/h2_maps_win64_dx11')
VISTA = ROOT / 'Halo 2 Vista installer/maps'
DLC = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c')
VISTA_ORACLES = ROOT / 'analysis/multiplayer/pc-executable-postprocess-20261011/vista-allmaps-measured'
SIZES = dict(int8=1, uint8=1, enum8=1, flags8=1,
             int16=2, uint16=2, enum16=2, flags16=2,
             int32=4, uint32=4, enum32=4, flags32=4, float32=4, degree=4, int64=8, uint64=8,
             datum=4, color32=4, range16=4, point16=4,
             rangef=8, ranged=8, point2=8, vector2=8, degree2=8,
             point3=12, vector3=12, degree3=12, colorf=12,
             plane2=12, plane3=16, vector4=16, rect16=8, quat16=8)
GAMEPLAY = {'weap', 'proj', 'jpt!', 'vehi', 'bipd', 'crea', 'eqip', 'itmc', 'vehc',
            'hlmt', 'matg', 'mulg', 'scnr', 'phmo', 'coll', 'bloc', 'mach', 'ctrl',
            'lifi', 'ssce', 'scen', 'goof', 'sily', 'gldf'}


def encoded(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=True, separators=(',', ':'))


class Schema:
    def __init__(self):
        self.parsed = {}
        self.pins = {}

    def get(self, group, xbox, mcc=False):
        banks = ([PLUGINS / 'Halo2Xbox', PLUGINS / 'Halo2'] if xbox else
                 [MCC_SCHEMAS, PLUGINS / 'Halo2'] if mcc else [PLUGINS / 'Halo2'])
        path = next((b / (group + '.xml') for b in banks
                     if (b / (group + '.xml')).is_file()), None)
        if path is None:
            return None
        if path not in self.parsed:
            self.parsed[path] = ET.parse(path).getroot()
            self.pins[str(path.relative_to(ROOT))] = sha(path)
        return self.parsed[path]


class Bank:
    def __init__(self, cache, schema, shared=None, xbox=False):
        self.cache, self.schema, self.shared, self.xbox = cache, schema, shared, xbox
        self.results = {}
        self.tags = defaultdict(list)
        self.index = {}
        for t in cache.tags.values():
            index = int(t['datum'], 16) & 0xffff
            self.index[index] = t
            self.tags[t['group'], t['name']].append(t)
        self.strings = self.read_strings()

    def read_strings(self):
        c = self.cache
        start = 356 if self.xbox else 48 if c.mcc else 368
        if self.xbox:
            count, size, indices, data = struct.unpack_from('<4I', c.data, start)
        elif c.mcc:
            count, data, size, indices = struct.unpack_from('<4I', c.data, start)
        else:
            count, size, indices, data = struct.unpack_from('<4I', c.data, start)
        result = {}
        if count > 65536 or indices + count * 4 > len(c.data) or data + size > len(c.data):
            return result
        for i in range(count):
            off = c.u(indices + i * 4)
            if off == 0xffffffff or off >= size:
                continue
            end = c.data.find(b'\0', data + off, data + size)
            if end >= data + off:
                result[i] = bytes(c.data[data + off:end]).decode('utf-8', errors='replace')
        return result

    def resolve(self, datum):
        if datum == 0xffffffff:
            return None
        tag = self.index.get(datum & 0xffff)
        if tag is None or tag['datum'] != f'{datum:08x}':
            return {'unresolved_datum': f'{datum:08x}'}
        return [tag['group'], tag['name']]

    def owner(self, tag):
        if tag['offset'] is not None:
            return self, tag
        if self.shared is None:
            return None, None
        other = self.shared.index.get(int(tag['datum'], 16) & 0xffff)
        if other is None or any(other[k] != tag[k] for k in ('datum', 'group', 'name')):
            return None, None
        if other['offset'] is None:
            return None, None
        return self.shared, other

    def extract(self, tag):
        owner, tag = self.owner(tag)
        if owner is None:
            return {'fields': {}, 'issues': [{'reason': 'unresolved_shared_owner'}], 'skipped': {}}
        if owner is not self:
            return owner.extract(tag)
        key = tag['datum']
        if key in self.results:
            return self.results[key]
        node = self.schema.get(tag['group'], self.xbox, getattr(self.cache, 'mcc', False))
        result = {'fields': {}, 'issues': [], 'skipped': Counter()}
        self.results[key] = result
        if node is None:
            result['issues'].append({'reason': 'missing_schema'})
            return result
        c, data = self.cache, self.cache.data
        low, high = tag['offset'], tag['offset'] + tag['size']

        def read(at, size):
            if not 0 <= low <= at <= at + size <= high <= len(data):
                raise ValueError('schema_field_outside_tag')
            return bytes(data[at:at + size])

        def add(path, kind, value, at, size):
            assert path not in result['fields'], (tag, path)
            result['fields'][path] = {'type': kind, 'value': value, 'offset': at, 'size': size}

        def walk(elements, base, prefix='', depth=0):
            if depth > 32:
                result['issues'].append({'path': prefix, 'reason': 'schema_depth_limit'})
                return
            children = [e for e in elements if 'offset' in e.attrib]
            counts = Counter(e.get('name', '') for e in children)
            ordinals = Counter()
            for e in children:
                name, kind = e.get('name', ''), e.tag.lower()
                ordinals[name] += 1
                identity = name + (f'#{ordinals[name]}' if counts[name] > 1 else '')
                path = prefix + identity
                at = base + int(e.get('offset'), 0)
                if kind == 'undefined' or 'unknown' in name.lower():
                    result['skipped']['opaque_or_unnamed_schema_field'] += 1
                    continue
                try:
                    if kind == 'tagblock':
                        count, pointer = struct.unpack('<iI', read(at, 8))
                        if count < 0 or count > 65536:
                            raise ValueError('invalid_schema_block_count')
                        add(path, 'tagblock_count', count, at, 4)
                        if not count:
                            continue
                        start = pointer - c.magic
                        stride = int(e.get('elementSize'), 0)
                        read(start, count * stride)
                        for index in range(count):
                            walk(e, start + index * stride, path + f'[{index}]/', depth + 1)
                    elif kind == 'tagref':
                        with_group = e.get('withGroup', 'true').lower() != 'false'
                        size = 8 if with_group else 4
                        raw = read(at, size)
                        datum = struct.unpack_from('<I', raw, 4 if with_group else 0)[0]
                        value = {'target': self.resolve(datum)}
                        if with_group:
                            value['declared_group_hex'] = raw[:4].hex()
                        add(path, kind, value, at, size)
                    elif kind == 'stringid':
                        raw = read(at, 4)
                        value = struct.unpack('<I', raw)[0]
                        add(path, kind, self.strings.get(value & 0xffff,
                                                       {'unresolved_stringid': f'{value:08x}'}), at, 4)
                    elif kind in ('ascii', 'utf16'):
                        size = int(e.get('size'), 0)
                        raw = read(at, size)
                        value = raw.decode('utf-16-le' if kind == 'utf16' else 'ascii', errors='replace').split('\0')[0]
                        add(path, kind, value, at, size)
                    elif kind == 'dataref':
                        size, pointer = struct.unpack('<iI', read(at, 8))
                        add(path, 'dataref_size', size, at, 4)
                        result['skipped']['opaque_dataref_payload'] += 1
                    elif kind in SIZES:
                        size = SIZES[kind]
                        add(path, kind, read(at, size).hex(), at, size)
                    else:
                        result['skipped']['unsupported_type_' + kind] += 1
                except (ValueError, struct.error) as error:
                    result['issues'].append({'path': path, 'offset': at, 'reason': str(error)})
        walk(node, tag['offset'])
        return result


def pc_bank(path, schema, shared):
    return Bank(PcCache(path), schema, shared=shared)


def vista_processed_bank(level, raw_shared, schema, name, shared_views):
    """Apply only ordered measured original instructions to private cache views."""
    path = VISTA_ORACLES / name / 'report.json'
    oracle = json.loads(path.read_text())
    assert oracle['all_41_ordered_destination_values_equal_to_xbox']
    owners = {str(level.path): level, str(raw_shared.cache.path): raw_shared.cache}
    for source in oracle['cache_inputs']:
        assert owners[source['path']].provenance == source
    assert sha(Path(oracle['executable_path'])) == oracle['executable_sha256']
    stores = [s for s in oracle['ordered_stores'] if s['region'] == 'actual_resident_metadata']
    assert len(stores) == 41
    grouped = defaultdict(list)
    for store in stores:
        assert store['owner'] in owners
        grouped[store['owner']].append(store)

    def process(cache, writes):
        view = copy.copy(cache)
        view.data = bytearray(cache.data)
        for store in writes:
            at, size = store['file_offset'], store['size']
            assert view.data[at:at + size].hex() == store['before_hex']
            view.data[at:at + size] = bytes.fromhex(store['after_hex'])
        return view

    shared_writes = grouped.get(str(raw_shared.cache.path), [])
    signature = encoded([(s['file_offset'], s['size'], s['before_hex'], s['after_hex']) for s in shared_writes])
    if signature not in shared_views:
        shared_views[signature] = Bank(process(raw_shared.cache, shared_writes), schema)
    level_writes = grouped.get(str(level.path), [])
    view = process(level, level_writes) if level_writes else level
    bank = Bank(view, schema, shared=shared_views[signature])
    evidence = {'report_sha256': sha(path), 'report_path': str(path),
                'executable_sha256': oracle['executable_sha256'],
                'metadata_store_count': len(stores),
                'actual_changed_store_count': oracle['actual_changed_store_count'],
                'level_view_sha256': hashlib.sha256(view.data).hexdigest(),
                'shared_view_sha256': hashlib.sha256(bank.shared.cache.data).hexdigest()}
    return bank, evidence


def inventory():
    paths = [ROOT / 'assets/disc/maps' / (name + '.map') for name in MAPS]
    paths += sorted((ROOT / 'assets/multiplayer-map-pack/demos/dlc').rglob('*.map'))
    paths += sorted(DLC.rglob('*.map'))
    originals, entries, hashes = {}, [], {}
    for path in paths:
        digest = sha(path)
        logical = 'highplains' if path.stem == 'highplains2' else path.stem
        entry = {'path': str(path), 'sha256': digest, 'logical_name': logical}
        if digest in hashes:
            entry['duplicate_of'] = hashes[digest]
        elif '00000004old' in str(path):
            entry['older_revision'] = True
        else:
            assert logical not in originals, (path, originals.get(logical))
            originals[logical] = path
        hashes[digest] = str(path)
        entries.append(entry)
    return originals, entries


def compare(xbox, pc, output, summary):
    xkeys, pkeys = set(xbox.tags), set(pc.tags)
    summary.update(xbox_tag_count=sum(map(len, xbox.tags.values())),
                   pc_tag_count=sum(map(len, pc.tags.values())),
                   xbox_only_tags=[list(k) for k in sorted(xkeys - pkeys)],
                   pc_only_tags=[list(k) for k in sorted(pkeys - xkeys)],
                   groups={}, schema_issues=[], skipped=Counter())
    differences = 0
    with output.open('wb') as raw, gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as gz:
        def emit(row):
            nonlocal differences
            gz.write((encoded(row) + '\n').encode())
            differences += 1
        for key in sorted(xkeys & pkeys):
            group, name = key
            stats = summary['groups'].setdefault(group, Counter())
            stats['common_tags'] += 1
            xtags, ptags = xbox.tags[key], pc.tags[key]
            if len(xtags) != 1 or len(ptags) != 1:
                summary['schema_issues'].append({'tag': list(key), 'reason': 'duplicate_tag_identity',
                                                  'xbox_count': len(xtags), 'pc_count': len(ptags)})
                continue
            xs, ps = xbox.extract(xtags[0]), pc.extract(ptags[0])
            for label, fields in [('xbox', xs), ('pc', ps)]:
                summary['skipped'].update(fields['skipped'])
                for issue in fields['issues']:
                    summary['schema_issues'].append({'tag': list(key), 'platform': label, **issue})
            xf, pf = xs['fields'], ps['fields']
            for path in sorted(set(xf) | set(pf)):
                left, right = xf.get(path), pf.get(path)
                if left is None or right is None:
                    reason = 'pc_only_field' if left is None else 'xbox_only_field'
                elif left['type'] != right['type']:
                    reason = 'field_type_difference'
                elif left['value'] != right['value']:
                    reason = 'field_value_difference'
                else:
                    stats['equal_fields'] += 1
                    continue
                stats[reason] += 1
                emit({'tag_group': group, 'tag_name': name, 'path': path, 'reason': reason,
                      'xbox': left, 'pc': right})
    summary['difference_records'] = differences
    summary['differences_sha256'] = sha(output)
    summary['difference_file'] = output.name
    summary['gameplay_totals'] = dict(sum((v for g, v in summary['groups'].items() if g in GAMEPLAY), Counter()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--map', action='append', dest='maps')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    schema = Schema()
    originals, original_inventory = inventory()
    pc_roots = {'vista': VISTA, 'mcc': MCC}
    shared = {label: Bank(PcCache(root / 'shared.map'), schema) for label, root in pc_roots.items()}
    vista_shared_views = {}
    result = {'tool_sha256': sha(Path(__file__)),
              'reader_sha256': sha(ROOT / 'tools/inspect_pc_combat_tag_sample.py'),
              'original_oracle_sha256': sha(ROOT / 'tools/oracle_mp_combat_postprocess_updates.py'),
              'mcc_schema_provenance_sha256': sha(MCC_SCHEMAS.parent / 'provenance.json'),
              'scope': 'All named schema-readable resident/shared fields of matching tag identities in available original multiplayer maps. Exact numeric bits; normalized tag references and string IDs. Xbox and Vista views include only their unchanged measured original combat postprocessors. MCC uses stored fields. Other loader fixups and complete runtime behavior are outside this audit.',
              'excluded': ['Opaque or Unknown schema fields', 'External resource/dataref payload contents',
                           'Missing schemas and out-of-tag schema reads, retained as explicit issues',
                           'Full native or PC gameplay, complete loader postprocessing and engine equivalence'],
              'gameplay_groups': sorted(GAMEPLAY), 'original_map_inventory': original_inventory,
              'shared_inputs': {k: v.cache.provenance for k, v in shared.items()},
              'maps': {}, 'pc_unmatched_maps': {}, 'pc_map_inventory': {}, 'schemas': {}}
    pc_names = {label: {p.stem for p in root.glob('*.map') if p.stem in set(originals) | {'needle', 'street_sweeper'}}
                for label, root in pc_roots.items()}
    for label in pc_roots:
        result['pc_unmatched_maps'][label] = sorted(pc_names[label] - set(originals))
        result['pc_map_inventory'][label] = [{'name': name, 'path': str((pc_roots[label] / (name + '.map')).resolve()),
                                            'sha256': sha(pc_roots[label] / (name + '.map'))}
                                           for name in sorted(pc_names[label])]
    selected = sorted(args.maps or originals)
    for name in selected:
        source = Cache(originals[name])
        try:
            assert source.h(source.scenario + 0x10) == 1
            oracle = run_one(source, IMAGES[1], args.output / 'xbox-original-postprocess' / name)
            processed = copy.copy(source)
            processed.data = bytearray(source.data)
            for store in oracle['ordered_stores']:
                if store['region'] != 'original_cache_metadata':
                    continue
                at, size = int(store['file_offset'], 16), store['size']
                assert processed.data[at:at + size].hex() == store['before_hex']
                processed.data[at:at + size] = bytes.fromhex(store['after_hex'])
            assert hashlib.sha256(processed.data).hexdigest() == oracle['complete_cache_output_sha256']
            oracle_path = args.output / 'xbox-original-postprocess' / name / 'report.json'
            oracle_path.write_text(json.dumps(oracle, indent=2) + '\n')
            xb = Bank(processed, schema, xbox=True)
            comparisons = {}
            result['maps'][name] = {'xbox_path': str(source.path), 'xbox_sha256': sha(source.path),
                                    'postprocess_report_sha256': sha(oracle_path), 'comparisons': comparisons}
            for label, root in pc_roots.items():
                path = root / (name + '.map')
                if not path.is_file():
                    comparisons[label] = {'missing_map': True}
                    continue
                level = PcCache(path)
                evidence = None
                if label == 'vista':
                    bank, evidence = vista_processed_bank(level, shared[label], schema, name, vista_shared_views)
                else:
                    bank = Bank(level, schema, shared=shared[label])
                comparisons[label] = {'input': bank.cache.provenance}
                if evidence:
                    comparisons[label]['original_vista_postprocess'] = evidence
                compare(xb, bank, args.output / (name + '-' + label + '-differences.jsonl.gz'), comparisons[label])
            print(encoded({'map': name, 'comparisons': {k: v.get('gameplay_totals', {'missing': True})
                                                       for k, v in comparisons.items()}}), flush=True)
        finally:
            source.data.close()
            source.stream.close()
    result['schemas'] = schema.pins
    result['totals'] = {}
    for label in pc_roots:
        records = [m['comparisons'][label] for m in result['maps'].values()]
        available = [r for r in records if not r.get('missing_map')]
        result['totals'][label] = {'compared_maps': len(available),
                                   'missing_original_maps': [n for n, m in result['maps'].items()
                                                             if m['comparisons'][label].get('missing_map')],
                                   'gameplay': dict(sum((Counter(r['gameplay_totals']) for r in available), Counter())),
                                   'all_groups': dict(sum((Counter(g) for r in available for g in r['groups'].values()), Counter())),
                                   'schema_issue_count': sum(len(r['schema_issues']) for r in available)}
    (args.output / 'report.json').write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
    print(encoded({'totals': result['totals'], 'report_sha256': sha(args.output / 'report.json')}), flush=True)


if __name__ == '__main__':
    main()
