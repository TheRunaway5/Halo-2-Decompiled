"""Qualify literal MCC meter metadata against existing compiled Vulkan kernels."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from mcc_content import MccCache
from native_program_storage_graph import storage_graph

ROOT = Path(__file__).resolve().parents[1]
STEM = r"shaders\shader_templates\transparent\meter_active_camo"
PASS = r"shaders\shader_passes\transparent\meter_active_camo"
VERTEX = r"rasterizer\vertex_shaders_xbox\transparent_generic_reflection"
LIBRARY = 'f523c80325d5fe3f8bff7d4f3768a3b97fb2d9904a9fa455f307757e7436917e'
KERNEL = '5dbdb0d883c442a24c4130b408a748fc0e83ae39929cbe954faaa9ff5d9b7988'
CURRENT_STEM = r"shaders\shader_templates\transparent\plasma_alpha_active_camo"
CURRENT_PASS = r"shaders\shader_passes\plasma\plasma_alpha_active_camo"
CURRENT_KERNEL = '0632caf4684c3014c70612473e728b81d1290f96a03b8d51dcd2c60f74d7afd5'


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()


def sha(value):
    return hashlib.sha256(value).hexdigest()


def generate_one(prepared, stem_name, pass_name, kernel):
    native = prepared / 'native-shaders'
    library = json.loads((native / 'coagulation/library.json').read_bytes())
    assert sha(canonical(library)) == LIBRARY
    records = {(r['group'], r['name']): r for r in library['shader_tags']}
    regions, names = {}, {}
    source = MccCache(prepared / 'halo2/h2_maps_win64_dx11/shared.map')

    def tag(t):
        return {k: t[k] for k in ('group', 'name', 'datum', 'address', 'size')}

    def retain(address, size):
        if not size:
            return b''
        offset = source.pointer(address, source.magic, size)
        raw = bytes(source.data[offset:offset + size])
        for first, old in regions.items():
            low, high = max(first, address), min(first + len(old), address + size)
            assert low >= high or old[low - first:high - first] == raw[low - address:high - address]
        if size > len(regions.get(address, b'')):
            regions[address] = raw
        return raw

    def owner(t):
        names[t['datum']] = tag(t)
        raw = retain(t['address'], t['size'])
        return dict(tag(t), sha256=sha(raw))

    def payload(source_map, record):
        raw = (native / source_map / 'payloads' / record['payload']).read_bytes()
        assert len(raw) == record['bytes'] and sha(raw) + '.bin' == record['payload']
        return raw

    try:
        stem = next(t for t in source.tags.values() if (t['group'], t['name']) == ('stem', stem_name))
        spas = next(t for t in source.tags.values() if (t['group'], t['name']) == ('spas', pass_name))
        setup = {'source_stem': owner(stem), 'source_pass': owner(spas), 'pc_vertices': [], 'external_regions': []}
        pp = source.block(spas['offset'] + 28, 88)
        assert len(pp) == 3
        vertices, external = set(), set()
        for definition in pp:
            for impl in source.block(definition, 330):
                handle = source.u32(impl + 256)
                if handle != 0xffffffff and handle not in vertices:
                    vertices.add(handle)
                    setup['pc_vertices'].append(owner(source.tags[handle]))
                for pixel in source.block(impl + 306, 56):
                    for field, stride in ((24, 4), (32, 1), (40, 1), (48, 1)):
                        count, pointer = source.u32(pixel + field), source.u32(pixel + field + 4)
                        if count and not spas['address'] <= pointer <= pointer + count * stride <= spas['address'] + spas['size']:
                            key = pointer, count * stride
                            if key not in external:
                                external.add(key)
                                setup['external_regions'].append({'address': pointer, 'bytes': key[1],
                                    'sha256': sha(retain(*key))})
        for group, name, label in (('stem', stem_name, 'native_stem'), ('spas', pass_name, 'native_pass')):
            record = records[group, name]
            raw = payload('coagulation', record)
            setup[label] = {'record': record, 'metadata_hex': raw.hex()}
        pass_raw = bytes.fromhex(setup['native_pass']['metadata_hex'])
        u32 = lambda b, n: struct.unpack_from('<I', b, n)[0]
        post = u32(pass_raw, 32)
        assert u32(pass_raw, post) == 1
        impl = u32(pass_raw, post + 4)
        controls = list(struct.unpack_from('<60I', pass_raw, impl + 6))
        for i in [*range(10, 26), 43, 44]:
            controls[i] = 0
        assert sha(struct.pack('<60I', *controls)) == kernel
        catalog = json.loads((ROOT / 'shaders/native-programs/fragments/materials.json').read_bytes())
        assert any(r['family'] == 'world' and r['controls'] == controls for r in catalog['entries'])
        assert all(u32(pass_raw, post + f) == 0 for f in (32, 40, 48))
        setup['existing_fragment_kernel'] = kernel
        hud = json.loads((ROOT / 'tools/data/mcc-native-hud-bootstrap.json').read_bytes())
        sky = json.loads((ROOT / 'tools/data/mcc-native-pc-sky-closure.json').read_bytes())
        world = json.loads((ROOT / 'tools/data/mcc-native-pc-world-closure.json').read_bytes())
        programs = json.loads((ROOT / 'tools/data/mcc-native-pc-world-programs.json').read_bytes())
        resident, fixture_vertices = {}, []
        original = records['vrtx', VERTEX]
        original_graph = storage_graph(original, payload('coagulation', original))['graph']
        for row in programs['maps']:
            s = next(s for s in sky['maps'] if s['map'] == row['map'])
            bank = [(hud['source_map'], r) for r in hud['records']]
            bank += [(s['source_map'], r) for r in s['records']]
            bank += [(world['source_map'], r) for r in world['records']]
            bank += [(r['source_map'], r['record']) for r in row['records']]
            choices = [(source_map, record) for source_map, record in bank
                       if (record['group'], record['name']) == ('vrtx', VERTEX)]
            assert len(choices) == 1
            source_map, record = choices[0]
            raw = payload(source_map, record)
            assert not record['references'] and not record['strings']
            assert storage_graph(record, raw)['graph'] == original_graph
            if record['payload'] not in resident:
                resident[record['payload']] = record
                fixture_vertices.append((record, raw))
        setup['resident_vertex_records'] = list(resident.values())
        policy = {'schema': 'H2_MCC_RESIDENT_METER_METADATA_V1', 'setup': setup,
            'scope': 'MCC source-qualified declarative pass/template metadata only; exact existing compiled Vulkan kernel and existing complete vertex owners; no executable program bank or guessed authored value'}
        record, raw = fixture_vertices[0]
        vertex = {'group': 'vrtx', 'name': VERTEX, 'datum': 0xd000f000,
                  'address': 0xf0000000, 'size': len(raw)}
        relocated = bytearray(raw)
        for field, target in record['pointers']:
            struct.pack_into('<I', relocated, field, vertex['address'] + target)
        names[vertex['datum']] = vertex
        fixture = {'schema': 'H2_MCC_RESIDENT_METER_FIXTURE_V1', 'stem': tag(stem), 'pass': tag(spas),
            'vertex': vertex, 'namespace': list(names.values()),
            'regions': [[at, b.hex()] for at, b in sorted(regions.items())] + [[vertex['address'], relocated.hex()]]}
        return policy, fixture
    finally:
        source.close()


def generate(prepared):
    policy, fixture = generate_one(prepared, STEM, PASS, KERNEL)
    current_policy, current_fixture = generate_one(prepared, CURRENT_STEM, CURRENT_PASS, CURRENT_KERNEL)
    assert policy['setup']['resident_vertex_records'] == current_policy['setup']['resident_vertex_records']
    policy['additional_setups'] = [current_policy['setup']]
    fixture['additional_cases'] = [current_fixture]
    policy['scope'] = ('Two exact MCC source-qualified declarative weapon-pass contracts only; '
                       'existing compiled Vulkan kernels and resident vertex owners; no executable bank or guessed authored value')
    return policy, fixture


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--policy', type=Path)
    parser.add_argument('--fixture', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    policy, fixture = generate(args.cache)
    if args.policy:
        text = json.dumps(policy, indent=2) + '\n'
        if args.check:
            assert args.policy.read_text() == text
        else:
            args.policy.write_text(text)
    if args.fixture:
        args.fixture.write_bytes(canonical(fixture))
    print('Two exact MCC weapon-pass metadata and existing compiled Vulkan/vertex identities authenticated')


if __name__ == '__main__':
    main()
