#!/usr/bin/env python3
"""Compile the finite complete-original vehicle metadata restoration registry."""
import argparse
from collections import Counter
import copy
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SOURCES = {
    'coagulation': 'f523c80325d5fe3f8bff7d4f3768a3b97fb2d9904a9fa455f307757e7436917e',
    '01b_spacestation': 'db415f7973f0cc7a966148db98fd2837c8dfd3c4c320e534eec00ae7e11beb41',
}
MAPS = {
    'needle': (9902592, 'bdf79ad0880e0ded968745bc102dcadaeb2ac690dd0eab42eb994f6f4f1d5fb7',
               '8698adc13e378c5220b18699471a47758b96ff68d04ffe1dc66cfddc8030a7de', 92, 68, 42),
    'street_sweeper': (7628288, 'a607587a4d4cf64a5fc52471810d292fbefaff06cd1aae09ee7fc24cacab67e6',
                       '7f9222998d0752232147a2c20278140a3d622c3d09d7ea2004e4c99b98537b10', 81, 61, 38),
}
SHARED_AUTHORITY = ('Exact consumed typed allocations/opaque regions and bitmap metadata/frame ownership; '
                    'whole-file shared fingerprint is preparation provenance only')
ROW_KEYS = {'map', 'bytes', 'sha256', 'preceding_canonical_sha256',
            'source_shared_preparation_provenance_bytes', 'source_shared_preparation_provenance_sha256',
            'runtime_shared_authority', 'sources', 'records', 'namespace_component_prefix',
            'declared_program_namespace', 'source_string_count', 'source_string_ids',
            'material_source_profiles', 'source_tags', 'source_regions', 'bitmap_roles'}
RECORD_KEYS = {'group', 'name', 'bytes', 'payload', 'fnv64', 'pointers', 'references', 'strings'}
GROUPS = {'shad', 'stem', 'spas', 'vrtx'}
ALLOCATION_COUNTS = {'needle': (126, 527), 'street_sweeper': (114, 511)}
PROGRAM_GROUP_COUNTS = {
    'needle': {'shad': 24, 'spas': 23, 'stem': 13, 'vrtx': 32},
    'street_sweeper': {'shad': 20, 'spas': 20, 'stem': 10, 'vrtx': 31},
}


def require(value, message):
    if not value:
        raise ValueError('Vehicle registry: ' + message)


def uint(value, maximum=0xffffffff):
    return type(value) is int and 0 <= value <= maximum


def fingerprint(value, count=64):
    return isinstance(value, str) and re.fullmatch('[0-9a-f]{' + str(count) + '}', value) is not None


def label(value):
    return isinstance(value, str) and 0 < len(value.encode('utf-8')) <= 4096 and '\0' not in value


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()


def no_duplicates(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, 'duplicate JSON member')
        result[key] = value
    return result


def material_names(name):
    values = {'objects\\vehicles\\ghost\\shaders\\' + leaf for leaf in
              ('ghost_damage', 'ghost_dash', 'ghost_metal', 'ghost_torn', 'plasma_vent')}
    values |= {'objects\\vehicles\\warthog\\shaders\\' + leaf for leaf in
               ('chassis', 'console', 'glass', 'glass_damage', 'glass_damage_red', 'glass_damage_white',
                'lights', 'metal', 'radio', 'scanner', 'speedo', 'tires', 'tires_hubless')}
    values |= {'objects\\vehicles\\warthog\\turrets\\chaingun\\shaders\\' + leaf
               for leaf in ('chaingun', 'chaingun_ammo')}
    if name == 'needle':
        values |= {'objects\\vehicles\\h_turret_ap\\shaders\\h_turret_ap_decal',
                   'objects\\vehicles\\warthog\\turrets\\chaingun\\shaders\\barrels_blur'}
        values |= {'objects\\weapons\\fixed\\plasma_cannon\\shaders\\' + leaf
                   for leaf in ('plasma_cannon_luminous', 'plasma_illum')}
    return values


def identity(record):
    return record['group'], record['name']


def descriptor(record):
    require(isinstance(record, dict) and set(record) == RECORD_KEYS and
            record['group'] in GROUPS and label(record['name']) and uint(record['bytes'], 16 * 1024 * 1024) and
            record['bytes'] >= 4 and isinstance(record['payload'], str) and record['payload'].endswith('.bin') and
            fingerprint(record['payload'][:-4]) and fingerprint(record['fnv64'], 16), 'invalid complete TU descriptor')
    seen = set()
    for kind in ('pointers', 'references', 'strings'):
        require(isinstance(record[kind], list), 'descriptor fixup bank is not a list')
        for fixup in record[kind]:
            require(isinstance(fixup, list) and len(fixup) == 2, 'malformed typed relocation')
            field, value = fixup
            require(uint(field) and field <= record['bytes'] - 4 and field not in seen, 'overlapping/outside typed relocation')
            seen.add(field)
            if kind == 'pointers':
                require(uint(value) and value < record['bytes'], 'pointer target outside complete TU payload')
            elif kind == 'references':
                require(isinstance(value, list) and len(value) == 2 and value[0] in GROUPS | {'bitm'} and
                        label(value[1]), 'untyped/unresolved TU reference')
            else:
                require(label(value) and len(value.encode('utf-8')) <= 255, 'invalid original StringID label')


def validate_map(row):
    require(isinstance(row, dict) and set(row) == ROW_KEYS and row['map'] in MAPS, 'unreviewed finite map schema')
    name = row['map']; size, digest, preceding, records, programs, bitmaps = MAPS[name]
    require(row['bytes'] == size and row['sha256'] == digest and row['preceding_canonical_sha256'] == preceding and
            row['namespace_component_prefix'] == '__tu15_vehicle_', 'finite input/preceding bank changed')
    expected_sources = SOURCES if name == 'needle' else {'coagulation': SOURCES['coagulation']}
    require(row['sources'] == expected_sources and row['runtime_shared_authority'] == SHARED_AUTHORITY and
            row['source_shared_preparation_provenance_bytes'] == 27908864 and
            fingerprint(row['source_shared_preparation_provenance_sha256']), 'original donor/shared authority changed')
    require(isinstance(row['records'], list) and len(row['records']) == records and
            isinstance(row['declared_program_namespace'], list) and len(row['declared_program_namespace']) == programs,
            'finite record/program census changed')
    names, published_names = {}, set()
    for entry in row['declared_program_namespace']:
        require(set(entry) == {'source_map', 'group', 'original_name', 'published_name'} and
                entry['source_map'] in expected_sources and entry['group'] in GROUPS - {'shad'} and
                label(entry['original_name']) and label(entry['published_name']), 'invalid original program namespace')
        parent, separator, leaf = entry['original_name'].rpartition('\\')
        require(separator and entry['published_name'] == parent + '\\__tu15_vehicle_' + entry['source_map'] + '\\' + leaf,
                'program namespace changed authored parent/leaf identity')
        key = entry['source_map'], entry['group'], entry['original_name']
        target = entry['group'], entry['published_name']
        require(key not in names and target not in published_names, 'duplicate original/storage program identity')
        names[key] = target; published_names.add(target)
    materials = material_names(name)
    require(len(materials) == (24 if name == 'needle' else 20), 'internal finite material census differs')
    def mapped(source, group, value):
        if group == 'bitm':
            return group, value
        if group == 'shad':
            require(value in materials, 'material reference escaped finite source owners')
            return group, value
        require((source, group, value) in names, 'reference has no declared donor storage identity')
        return names[source, group, value]
    source_tags, typed_names = {}, {}
    require(len(row['source_tags']) == ALLOCATION_COUNTS[name][0] and
            len(row['source_regions']) == ALLOCATION_COUNTS[name][1], 'complete consumed source allocation census changed')
    for tag in row['source_tags']:
        require(set(tag) == {'datum', 'group', 'name', 'address', 'size', 'sha256'} and
                uint(tag['datum']) and tag['datum'] != 0xffffffff and tag['group'] in GROUPS | {'bitm'} and
                label(tag['name']) and uint(tag['address']) and tag['address'] > 0 and
                uint(tag['size'], 16 * 1024 * 1024) and tag['size'] > 0 and
                tag['address'] + tag['size'] <= 2**32 and fingerprint(tag['sha256']), 'invalid complete source allocation')
        require(tag['datum'] not in source_tags and identity(tag) not in typed_names, 'duplicate complete source allocation')
        source_tags[tag['datum']] = tag; typed_names[identity(tag)] = tag
    regions = {}
    for region in row['source_regions']:
        require(set(region) == {'address', 'bytes', 'sha256'} and uint(region['address']) and
                uint(region['bytes'], 16 * 1024 * 1024) and region['bytes'] > 0 and
                region['address'] + region['bytes'] <= 2**32 and fingerprint(region['sha256']) and
                region['address'] not in regions, 'invalid/duplicate complete source region')
        regions[region['address']] = region
    for tag in source_tags.values():
        region = regions.get(tag['address'])
        require(region and region['bytes'] >= tag['size'] and
                (region['bytes'] != tag['size'] or region['sha256'] == tag['sha256']), 'complete source tag lacks its hashed region')
    roles, role_names = set(), set()
    require(isinstance(row['bitmap_roles'], list) and len(row['bitmap_roles']) == bitmaps, 'finite bitmap role census changed')
    for role in row['bitmap_roles']:
        require(set(role) == {'group', 'name', 'frame', 'datum', 'address', 'size'} and role['group'] == 'bitm' and
                label(role['name']) and uint(role['frame']) and (role['group'], role['name']) in typed_names,
                'bitmap role lacks its complete hashed typed source owner')
        tag = typed_names[role['group'], role['name']]
        require(all(role[k] == tag[k] for k in ('datum', 'group', 'name', 'address', 'size')) and
                (role['name'], role['frame']) not in roles, 'bitmap role allocation/frame identity differs')
        roles.add((role['name'], role['frame'])); role_names.add(role['name'])
    actual_materials, actual_programs, seen, strings = {}, set(), set(), set()
    for item in row['records']:
        require(set(item) == {'source_map', 'original_record', 'record', 'semantic_name'} and
                item['source_map'] in expected_sources, 'invalid complete original publication row')
        original, published = item['original_record'], item['record']; descriptor(original); descriptor(published)
        require(item['semantic_name'] == original['name'], 'semantic name differs from complete original descriptor')
        source = item['source_map']; target = mapped(source, original['group'], original['name'])
        expected = copy.deepcopy(original); expected['name'] = target[1]
        for reference in expected['references']:
            reference[1] = list(mapped(source, *reference[1]))
        require(expected == published and target not in seen, 'publication changed more than declared names/references')
        seen.add(target)
        if original['group'] == 'shad': actual_materials[original['name']] = original
        else: actual_programs.add(target)
        for _, value in original['strings']: strings.add(value)
        for _, (group, value) in original['references']:
            if group == 'bitm': require(value in role_names, 'original bitmap reference lacks a typed/frame profile')
    require(set(actual_materials) == materials and actual_programs == published_names, 'incomplete bidirectional program/material closure')
    require(Counter(item['record']['group'] for item in row['records']) == PROGRAM_GROUP_COUNTS[name],
            'complete native program group census changed')
    for item in row['records']:
        for _, target in item['record']['references']:
            require(target[0] == 'bitm' or tuple(target) in seen, 'dangling published shader reference')
    profiles = {}
    for profile in row['material_source_profiles']:
        require(set(profile) == {'name', 'datum', 'bytes', 'sha256', 'source_template', 'native_template'} and
                profile['name'] in materials and profile['name'] not in profiles and
                ('shad', profile['name']) in typed_names, 'material profile lacks its exact source allocation')
        tag = typed_names['shad', profile['name']]
        require(profile['datum'] == tag['datum'] and profile['bytes'] == tag['size'] and profile['sha256'] == tag['sha256'] and
                ('stem', profile['source_template']) in typed_names and
                [reference for field, reference in actual_materials[profile['name']]['references'] if field == 4] ==
                [['stem', profile['native_template']]], 'material/source/native template identity changed')
        profiles[profile['name']] = profile
    require(set(profiles) == materials and set(row['source_string_ids']) == strings and
            uint(row['source_string_count'], 65536), 'complete material/StringID census differs')
    for text, value in row['source_string_ids'].items():
        require(uint(value) and value & 0xffffff < row['source_string_count'] and
                value >> 24 == len(text.encode('utf-8')), 'original StringID must already exist with its exact length/index')


def generate(raw):
    require(isinstance(raw, bytes) and len(raw) <= 16 * 1024 * 1024, 'invalid source policy extent')
    value = json.loads(raw, object_pairs_hook=no_duplicates)
    require(set(value) == {'schema', 'maps'} and value['schema'] == 'H2_MCC_NATIVE_VEHICLE_RESTORATION_V1' and
            isinstance(value['maps'], list) and [row['map'] for row in value['maps']] == list(MAPS), 'unreviewed finite map census')
    for row in value['maps']: validate_map(row)
    encoded = canonical(value).decode()
    require(')H2VEHICLE"' not in encoded, 'registry escapes its compiled literal')
    return ('// Generated by tools/generate_mcc_native_vehicle_restoration.py; do not edit.\n'
            '// Source data SHA256: ' + hashlib.sha256(raw).hexdigest() + '\n'
            'constexpr char native_vehicle_restoration_storage[]=R"H2VEHICLE(' + encoded + ')H2VEHICLE";\n'
            'constexpr std::string_view native_vehicle_restoration_policy{native_vehicle_restoration_storage,sizeof(native_vehicle_restoration_storage)-1};\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    text = generate((ROOT / 'tools/data/mcc-native-vehicle-restoration.json').read_bytes())
    output = ROOT / 'src/runtime/mcc_native_vehicle_restoration.inc'
    if args.check:
        require(output.is_file() and output.read_text() == text, 'stale finite original vehicle registry')
    else:
        output.write_text(text)


if __name__ == '__main__':
    main()
