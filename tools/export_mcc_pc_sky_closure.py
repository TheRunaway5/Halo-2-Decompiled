#!/usr/bin/env python3
"""Export the finite, paired native sky closures for the two PC-only maps.

Run through tools/run_test.py. Only bounded policy/evidence files are written;
original maps, retained Xbox programs and imported resources remain read-only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_rendering import Cache
from mcc_content import MccCache
from export_mcc_elongation_glass_tu15 import extract

ROOT = Path(__file__).resolve().parents[1]
PAIRS = {
    'needle': ('dune', ROOT/'assets/map-pack/maps/dune.map', (
        r'scenarios\shaders\multi\dune\dune_island',
        r'scenarios\shaders\multi\dune\dune_planet',
        r'scenarios\shaders\multi\dune\dune_ring',
        r'scenarios\skies\multi\zanzibar\shaders\clouds_low',
        r'scenarios\skies\multi\zanzibar\shaders\sky_dome')),
    'street_sweeper': ('03a_oldmombasa', ROOT/'assets/disc/maps/03a_oldmombasa.map', (
        r'scenarios\skies\solo\earthcity\shaders\clouds_low2',
        r'scenarios\skies\solo\earthcity\shaders\ec_sky_dome2',
        r'scenarios\skies\solo\earthcity\shaders\ec_sky_tether_faded',
        r'scenarios\skies\solo\earthcity\shaders\ec_sky_tether_tile')),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()


def normalized_root(value):
    data = bytearray.fromhex(value['root_hex'])
    # Typed identities and owned block edges are compared separately.
    for at in (4, 16, 36, 48):
        struct.pack_into('<I', data, at, 0)
    struct.pack_into('<I', data, 32, 1)
    return data


def overlays(cache, value):
    raw = bytearray.fromhex(value['banks']['60']['hex'])
    functions = []
    for at in range(0, len(raw), 20):
        size, address = struct.unpack_from('<II', raw, at + 12)
        start = address - cache.magic
        cache.require(start, size)
        data = bytes(cache.data[start:start + size])
        functions.append((address, data))
        struct.pack_into('<I', raw, at + 16, 0)
    return bytes(raw), functions


def prove_pair(mcc, xbox, name):
    source, original = extract(mcc, name), extract(xbox, name)
    source_stem = mcc.tags[struct.unpack_from('<I', bytes.fromhex(source['root_hex']), 4)[0]]
    original_stem = xbox.tags[struct.unpack_from('<I', bytes.fromhex(original['root_hex']), 4)[0]]
    if (source_stem['group'], source_stem['name']) != (original_stem['group'], original_stem['name']):
        raise ValueError('Changed complete typed template identity: ' + name)
    source['template_owner'] = source_stem
    if normalized_root(source) != normalized_root(original):
        raise ValueError('Changed complete material root: ' + name)
    if source['runtime']['count'] != original['runtime']['count'] or source['normalized_runtime'] != original['normalized_runtime']:
        raise ValueError('Changed complete runtime properties: ' + name)
    if source['normalized_predictions'] != original['normalized_predictions']:
        raise ValueError('Changed complete predictions: ' + name)
    # MCC carries two additional empty platform definitions, never programs.
    if source['definitions']['count'] != 3 or original['definitions']['count'] != 1:
        raise ValueError('Changed platform definition cardinality: ' + name)
    if any(bytes.fromhex(source['definitions']['hex'])[124:]):
        raise ValueError('Nonempty additional platform definition: ' + name)
    xdef, ydef = bytearray.fromhex(source['definitions']['hex'])[:124], bytearray.fromhex(original['definitions']['hex'])
    for data in (xdef, ydef):
        struct.pack_into('<I', data, 0, 0)
        for at in range(8, 116, 8):
            struct.pack_into('<I', data, at, 0)
    if xdef != ydef:
        raise ValueError('Changed complete cooked bank descriptors: ' + name)
    typed = lambda value: [(row['tag']['group'], row['tag']['name'], row['frame']) for row in value['bitmaps']]
    if typed(source) != typed(original):
        raise ValueError('Changed complete typed resource tuple: ' + name)
    for field in source['banks']:
        if field in ('4', '60'):
            continue
        a, b = bytes.fromhex(source['banks'][field]['hex']), bytes.fromhex(original['banks'][field]['hex'])
        if field == '12' and a:
            # Measured complete four-word tuple for these exact finite inputs.
            words = struct.unpack('<4I', a)
            a = struct.pack('<4I', *(words[index] for index in (0, 2, 1, 3)))
        if a != b:
            raise ValueError('Changed complete cooked bank ' + field + ': ' + name)
    xoverlay, xfunctions = overlays(mcc, source)
    yoverlay, yfunctions = overlays(xbox, original)
    if xoverlay != yoverlay or [data for _, data in xfunctions] != [data for _, data in yfunctions]:
        raise ValueError('Changed complete animation functions: ' + name)
    # Bitmap words 0/4 are typed references and frame ordinals, not pointers.
    # Log-dimension beforeimages are retained and recorded separately;
    # native instances keep the literal original values for restored source images.
    source['regions'].extend((address, data.hex()) for address, data in xfunctions if data)
    return source, {'shader': name, 'original_root_sha256': sha(bytes.fromhex(original['root_hex'])),
                    'bitmap_tuple': typed(source), 'resource_log_words': [[a['log_dimension_word'], b['log_dimension_word']] for a, b in zip(source['bitmaps'], original['bitmaps'])], 'pixel_permutation': [0, 2, 1, 3] if source['banks']['12']['count'] else [],
                    'animation_function_sha256': [sha(data) for _, data in xfunctions]}


def export(args):
    policy = {'schema': 'H2_MCC_NATIVE_PC_SKY_CLOSURE_V1', 'maps': []}
    fixtures, evidence = [], []
    for name, (native_map, original_path, identities) in PAIRS.items():
        path = args.cache/'halo2/h2_maps_win64_dx11'/(name + '.map')
        native_dir = args.cache/'native-shaders'/native_map
        library = json.loads((native_dir/'library.json').read_bytes())
        if library['source_sha256'] != file_sha(original_path):
            raise ValueError('Native library does not come from the paired original map')
        mcc, xbox = MccCache(path), Cache(original_path)
        shared = MccCache(args.cache/'halo2/h2_maps_win64_dx11/shared.map')
        try:
            owners, regions, comparisons = {}, {}, []
            def owner(tag):
                cache = mcc
                if not tag['address']:
                    resolved = shared.tags.get(tag['datum'])
                    if not resolved or (resolved['group'], resolved['name']) != (tag['group'], tag['name']) or not resolved['address'] or not resolved['size']:
                        raise ValueError('Shared owner is missing or has a different salted identity')
                    tag, cache = resolved, shared
                key = tag['datum']
                record = {field: tag[field] for field in ('group', 'name', 'datum', 'address', 'size')}
                if key in owners and owners[key] != record:
                    raise ValueError('Conflicting full typed source owner')
                owners[key] = record
                if tag['address'] and tag['size']:
                    start = tag['address'] - cache.magic
                    cache.require(start, tag['size'])
                    # Only shad roots are schema-sized, their tag extent can contain
                    # unrelated/shared allocations. Bitmap metadata is complete.
                    if tag['group'] == 'bitm':
                        regions[(tag['address'], tag['size'])] = bytes(cache.data[start:start + tag['size']])
            for identity in identities:
                value, comparison = prove_pair(mcc, xbox, identity)
                comparisons.append(comparison)
                owner(value['shader'])
                owner(value['template_owner'])
                for image in value['bitmaps']:
                    owner(image['tag'])
                for address, raw in value['regions']:
                    data = bytes.fromhex(raw)
                    regions[(address, len(data))] = data
            by_identity = {(r['group'], r['name']): r for r in library['shader_tags']}
            wanted = {('shad', identity) for identity in identities}
            pending = list(wanted)
            while pending:
                identity = pending.pop()
                record = by_identity[identity]
                if record['strings']:
                    raise ValueError('Unexpected StringID dependency in PC sky closure')
                for _, target in record['references']:
                    key = tuple(target)
                    if key[0] == 'bitm':
                        if not any((t['group'], t['name']) == key for t in owners.values()):
                            raise ValueError('Native material escapes proved bitmap namespace')
                    elif key not in wanted:
                        wanted.add(key)
                        pending.append(key)
            records = [by_identity[key] for key in sorted(wanted)]
            for record in records:
                raw = (native_dir/'payloads'/record['payload']).read_bytes()
                if len(raw) != record['bytes'] or sha(raw) + '.bin' != record['payload']:
                    raise ValueError('Native payload no longer matches its complete body')
            region_rows = [{'address': address, 'bytes': size, 'sha256': sha(data)}
                           for (address, size), data in sorted(regions.items())]
            row = {'map': name, 'bytes': path.stat().st_size, 'sha256': file_sha(path),
                   'source_map': native_map, 'source_canonical_sha256': sha(canonical(library)),
                   'owners': sorted(owners.values(), key=lambda row: row['datum']),
                   'regions': region_rows, 'records': records}
            policy['maps'].append(row)
            fixtures.append({'map': name, 'source_library': library,
                             'regions': [{'address': address, 'hex': data.hex()} for (address, _), data in sorted(regions.items())]})
            evidence.append({'map': name, 'mcc_map_sha256': row['sha256'],
                             'original_map': str(original_path.relative_to(ROOT)),
                             'original_map_sha256': file_sha(original_path),
                             'source_xbe_sha256': library['source_xbe_sha256'],
                             'source_library_canonical_sha256': row['source_canonical_sha256'],
                             'native_records': len(records), 'materials': comparisons})
        finally:
            mcc.close()
            xbox.close()
            shared.close()
    raw = json.dumps(policy, indent=2) + '\n'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(raw)
    value = canonical(policy).decode()
    args.include.parent.mkdir(parents=True, exist_ok=True)
    args.include.write_text('// Generated by tools/export_mcc_pc_sky_closure.py; do not edit.\n'
                            '// Policy SHA256: ' + sha(raw.encode()) + '\n'
                            'constexpr std::string_view native_pc_sky_closure_policy=R"H2SKY(' + value + ')H2SKY";\n')
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    # Keep only closure records, rather than retaining unrelated material metadata.
    for row, fixture in zip(policy['maps'], fixtures):
        fixture['source_library'] = {'canonical_sha256': row['source_canonical_sha256']}
    args.fixture.write_text(json.dumps({'schema': policy['schema'], 'maps': fixtures}, indent=2) + '\n')
    args.evidence.parent.mkdir(parents=True, exist_ok=True)
    args.evidence.write_text(json.dumps({'scope': 'Finite paired cooked data/program closure; PC maps have no original Xbox map framebuffer oracle',
                                        'exporter_sha256': file_sha(Path(__file__)), 'maps': evidence}, indent=2) + '\n')
    print(json.dumps({'maps': len(policy['maps']), 'materials': sum(len(row['materials']) for row in evidence),
                      'native_records': sum(row['native_records'] for row in evidence),
                      'policy_bytes': len(raw), 'region_bytes': sum(sum(len(bytes.fromhex(r['hex'])) for r in f['regions']) for f in fixtures)}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT/'tools/data/mcc-native-pc-sky-closure.json')
    parser.add_argument('--include', type=Path, default=ROOT/'src/runtime/mcc_native_pc_sky_closure.inc')
    parser.add_argument('--fixture', type=Path, default=ROOT/'tests/fixtures/mcc-pc-sky-closure-regions.json')
    parser.add_argument('--evidence', type=Path, default=ROOT/'analysis/sky-water-tu15-20261024/pc-sky-native-closure-evidence.json')
    export(parser.parse_args())


if __name__ == '__main__':
    main()
