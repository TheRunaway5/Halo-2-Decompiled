"""Prove complete tiling-specular data projection from named compiler roles.

Uses all captured authored rows, complete paired original banks, named H2EK
constant declarations and exact original template records. No role is selected
by equality of default values. This data proof does not admit an engine feature
or establish rendered parity. Run through run_test.py.
"""
import argparse
import copy
import hashlib
import json
import lzma
import math
import os
from pathlib import Path
import struct

from mcc_env_illum_projection import FIELDS, banks, extent, native_graph, u16
from audit_h2ek_shader_bindings import BindingReader, psh_bindings


TEMPLATE = r'shaders\shader_templates\opaque\tex_bump_env_tiling_specular'


def need(ok, why):
    if not ok:
        raise ValueError('Tiling-specular projection: ' + why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def pack_channel(value):
    # This integer-ratio form is separately compared with unchanged TU1.5
    # FISTP. All main vectors admitted here must also be finite.
    real = struct.unpack('<f', struct.pack('<I', value))[0]
    need(math.isfinite(real), 'nonfinite authored vector')
    numerator, denominator = real.as_integer_ratio()
    quotient, remainder = divmod(abs(numerator) * 255, denominator)
    quotient += 2 * remainder > denominator or (2 * remainder == denominator and quotient & 1)
    result = -quotient if numerator < 0 else quotient
    return result & 0xffffffff if -0x80000000 <= result <= 0x7fffffff else 0x80000000


def pack_vector(raw, component):
    r, g, b, a = struct.unpack('<4I', raw)
    if component == 'a':
        return struct.pack('<I', (pack_channel(a) << 24) & 0xffffffff)
    need(component == 'rgb', 'unknown packed component')
    return struct.pack('<I', ((pack_channel(r) & 255) << 16) |
                       ((pack_channel(g) & 255) << 8) | (pack_channel(b) & 255))


def instances(graph):
    data = banks(graph)
    for index, (_, name) in enumerate(graph[2]['typed_references']):
        for local, impl in enumerate(extent(u16(data['16'], index * 10 + 8), len(data['24']) // 6)):
            yield name, local, impl


def remaps(data, impl, field):
    return [struct.unpack_from('<HBB', data['32'], i * 4)
            for i in extent(u16(data['24'], impl * 6 + field), len(data['32']) // 4)]


def assign(roles, index, value):
    need(index not in roles or roles[index] == value, 'one bank slot has conflicting named roles')
    roles[index] = value


def vertex_roles(graph, metadata):
    data = banks(graph)
    named, uv = {}, {}
    for name, local, impl in instances(graph):
        constants = (metadata['editable'][name]['metadata']['implementations'][local]['vertex_constants']
                     if name in metadata['editable'] else [])
        textures = remaps(data, impl, 0)
        for dest, component, source in remaps(data, impl, 4):
            need(component == 0, 'unexpected vertex remap component selector')
            candidates = [item for item in constants if (dest & 1023) == item['register_index'] +
                          (item['register_bank'] << 4) + (item['component_mask'] << 5)]
            if candidates:
                need(len(candidates) == 1, 'named vertex declaration is ambiguous')
                mask = candidates[0]['component_mask']
                assign(named, source, (candidates[0]['parameter']['name'], mask))
            mode = (dest >> 5) & 31
            if mode >= 5:
                bitmap = [slot for target, channel, slot in textures if target & 255 == dest >> 10]
                need(len(bitmap) == 1, 'UV remap lost its complete bitmap-property owner')
                assign(uv, source, (bitmap[0], mode, component))
    return named, uv


def pixel_roles(graph, metadata, original=False):
    data, roles = banks(graph), {}
    for name, local, impl in instances(graph):
        mappings = remaps(data, impl, 2)
        if original:
            if name.endswith(r'\tiling_specular_zwrite'):
                declared = metadata['textual'][name + ':' + str(local)]['declarations']
                need(len(declared) == len(mappings) == 1 and declared[0]['component'] == 'rgb' and
                     mappings[0][:2] == (512, 4), 'zwrite sole final RGB parameter contract changed')
                assign(roles, mappings[0][2], (declared[0]['parameter'], 'rgb'))
                continue
            if name not in metadata['editable']:
                continue
            bindings = metadata['editable'][name]['metadata']['implementations'][local][
                'unused_shader_code_metadata']['named_parameter_bindings']
            for dest, component, source in mappings:
                candidates = [item for item in bindings if item['prefix_combiner_index'] is not None and
                    dest == (item['prefix_combiner_index'] << 6) | (item['constant_bank'] << 10) and
                    component == (3 if item['component'] == 'alpha' else 4)]
                if candidates:
                    need(len(candidates) == 1, 'original pixel declaration is ambiguous')
                    assign(roles, source, (candidates[0]['parameter'], 'a' if component == 3 else 'rgb'))
        else:
            declared = metadata['textual'][name + ':' + str(local)]['declarations']
            for dest, component, source in mappings:
                need(component == 0 and dest < 256, 'MCC pixel remap escaped named register encoding')
                candidates = [item for item in declared if item['component'] in ('rgb', 'a') and dest == item['register'] |
                              ((3 if item['component'] == 'a' else 4) << 5)]
                need(len(candidates) == 1, 'MCC pixel declaration has no unique named binding')
                assign(roles, source, (candidates[0]['parameter'], candidates[0]['component']))
    return roles


def derive(source, native, metadata):
    sp, np = pixel_roles(source, metadata), pixel_roles(native, metadata, True)
    sv, suv = vertex_roles(source, metadata)
    nv, nuv = vertex_roles(native, metadata)
    need(set(sp) == set(range(5)) and set(np) == set(range(7)) and set(sv) == set(range(12)),
         'named main parameter ownership is incomplete')
    vectors = []
    for slot in range(8):
        candidates = [index for index in sv if
            ((slot in nv and sv[index] == nv[slot]) or (slot in nuv and suv.get(index) == nuv[slot]))]
        need(len(candidates) == 1, 'native vector has no unique complete semantic source')
        vectors.append(candidates[0])
    pixels = []
    for slot in range(7):
        packed = [index for index, role in sp.items() if role == np[slot]]
        if packed:
            need(len(packed) == 1, 'packed color has multiple named source parameters')
            pixels.append(dict(kind='pixel', source=packed[0], parameter=np[slot][0], component=np[slot][1]))
        else:
            candidates = [index for index, role in sv.items() if role ==
                          (np[slot][0], 3 if np[slot][1] == 'a' else 4)]
            need(len(candidates) == 1, 'packed original color has no unique named float source')
            pixels.append(dict(kind='vector', source=candidates[0], parameter=np[slot][0], component=np[slot][1]))
    unused = {index: role for index, role in sp.items() if role not in np.values()}
    need(unused == {3: ('env_midrange_tint_factor', 'a'), 4: ('env_midrange_tint_color', 'rgb')},
         'unowned residual parameter is not the explicit midrange pair')
    return dict(vector_sources=vectors, pixel_sources=pixels,
                source_vector_roles=sv, native_vector_roles=nv,
                source_pixel_roles=sp, native_pixel_roles=np,
                inactive_midrange_factor_pixel=3, inactive_midrange_color_pixel=4)


def project(row, source_arrays, native_arrays, roles):
    data = {key: bytes.fromhex(value['hex']) for key, value in row['source_banks'].items()}
    source, native = banks(source_arrays), banks(native_arrays)
    need(all(value['count'] == roles['source_counts'][key] for key, value in row['source_banks'].items()) and
         all(len(data[str(field)]) == roles['source_counts'][str(field)] * stride for field, stride in FIELDS),
         'complete source bank counts or extents differ')
    need(row['source_root']['definition_count'] == 3 and row['source_root']['trailing_definitions_zero'] and
         row['source_root']['named_parameter_count'] == 0, 'additional authored platform/named parameters')
    need(len(data['4']) == 84 and len(data['12']) == 20 and len(data['20']) == 192,
         'main bank shape differs from paired compiler roles')
    need(not row['source_overlays'] and all(not data[str(field)] for field in (60, 68, 76, 84)) and
         not any(data['52']) and len(data['52']) == len(source['24']) // 6 * 10,
         'authored animation requires complete native mapping')
    need(data['36'] == source['8'] and data['44'] == b''.join(source['16'][i:i+2]
         for i in range(8, len(source['16']), 10)), 'material schedules differ from its compiler graph')
    need(len(data['28']) == 18 and all(u16(data['28'], i * 6 + 4) == u16(source['0'], i * 10) ==
         u16(native['0'], i * 10) for i in range(3)), 'authored LOD pass ranges differ')
    need(data['12'][12:16] == bytes(4), 'active MCC midrange tint requires an additional native owner')
    need(all(math.isfinite(value) for value, in struct.iter_unpack('<f', data['20'])), 'nonfinite authored vector')
    result = dict(data)
    result['12'] = b''.join(data['12'][item['source'] * 4:item['source'] * 4 + 4]
        if item['kind'] == 'pixel' else pack_vector(data['20'][item['source'] * 16:item['source'] * 16 + 16],
        item['component']) for item in roles['pixel_sources'])
    result['20'] = b''.join(data['20'][index * 16:index * 16 + 16] for index in roles['vector_sources'])
    result['36'] = native['8']
    result['44'] = b''.join(native['16'][i:i+2] for i in range(8, len(native['16']), 10))
    # Every source animation range is empty. The complete original controls
    # independently specify the native count and its canonical empty records.
    result['52'] = bytes(len(native['24']) // 6 * 10)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority', type=Path, required=True)
    parser.add_argument('--bindings', type=Path, required=True)
    parser.add_argument('--profile-output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--packing-proof', type=Path, required=True)
    parser.add_argument('--inactive-midrange-proof', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    raw = args.authority.read_bytes()
    data, metadata = json.loads(lzma.decompress(raw)), json.loads(args.bindings.read_text())
    packing = json.loads(args.packing_proof.read_text())
    inactive = json.loads(args.inactive_midrange_proof.read_text())
    need(packing['passed'] and packing['original_comparisons'] == 14756 and
         packing['finite_channel_proposal_comparisons'] > 14000 and
         packing['channel_proposal']['sha256'] == sha(Path(__file__).read_bytes()),
         'actual projection packer lacks its original execution proof')
    need(inactive['passed'] and inactive['bindings_sha256'] == sha(args.bindings.read_bytes()) and
         inactive['zero_factor']['intermediate_tint_weight_zero'] and
         not inactive['zero_factor']['tint_reaches_color'] and inactive['nonzero_factor_control']['tint_reaches_color'],
         'inactive residual parameter lacks its complete dependency proof')
    need(data['template'] == TEMPLATE and not data['errors'] and len(data['materials']) == 14 and
         len(data['stems']) == 1 and metadata['passed'], 'authority cohort differs')
    for item in metadata['textual'].values():
        need(psh_bindings(Path(item['path'])) == item, 'named source declaration differs from its complete input')
    for item in metadata['editable'].values():
        fresh = BindingReader(Path(item['path']), 'spas').shader_pass()
        need(json.dumps(fresh, sort_keys=True) == json.dumps(item, sort_keys=True),
             'editable named bindings differ from their complete input')
    source_graph = next(iter(data['stems'].values()))['graph']
    source = source_graph['definitions'][0]['arrays']
    variants, roles, native_arrays = [], None, None
    for name, info in data['native_libraries'].items():
        path = Path(info['path'])
        library_raw = path.read_bytes()
        need(sha(library_raw) == info['sha256'], 'original library changed')
        library = json.loads(library_raw)
        records = [record for record in library['shader_tags'] if record['group'] == 'stem' and record['name'] == TEMPLATE]
        for record in records:
            graph = native_graph(path.parent, record)
            derived = derive(source, graph, metadata)
            need(roles is None or roles == derived, 'original variants disagree about semantic roles')
            need(native_arrays is None or graph == native_arrays, 'original variants have different complete graphs')
            roles, native_arrays = derived, graph
            variants.append(dict(map=name, payload=record['payload'], bytes=record['bytes'], fnv64=record['fnv64'],
                                 source_xbe_sha256=info['source_xbe_sha256']))
    need(len(variants) == 2 and roles is not None, 'complete original template controls changed')
    roles['source_counts'] = {key: value['count'] for key, value in data['materials'][0]['source_banks'].items()}
    paired = 0
    outputs_hash = hashlib.sha256()
    for row in data['materials']:
        output = project(row, source, native_arrays, roles)
        for field, _ in FIELDS:
            outputs_hash.update(output[str(field)])
        if row['native_pair']:
            need(row['source_images'] == row['native_images'], 'typed bitmap/frame roles differ')
            for field, _ in FIELDS:
                if field != 4:
                    need(output[str(field)].hex() == row['native_banks'][str(field)]['hex'],
                         'complete original bank differs: ' + row['map'] + ':' + str(field))
            paired += 1
    need(paired == 2, 'paired original materials changed')
    # An active residual parameter is refused rather than silently dropped.
    changed = copy.deepcopy(data['materials'][0])
    values = bytearray.fromhex(changed['source_banks']['12']['hex'])
    values[15] = 1
    changed['source_banks']['12']['hex'] = values.hex()
    try:
        project(changed, source, native_arrays, roles)
    except ValueError as error:
        need('midrange' in str(error), 'residual-parameter control failed at another guard')
    else:
        raise ValueError('Active residual parameter was accepted')
    profile = dict(schema='H2_MCC_NATIVE_TILING_SPECULAR_PROFILES_V1', template=TEMPLATE,
        source_graph=source_graph, native_arrays=native_arrays, native_records=variants, roles=roles,
        authority=dict(path=str(args.authority), sha256=sha(raw), materials=14, paired_controls=paired),
        named_bindings_sha256=sha(args.bindings.read_bytes()))
    profile['packing_proof_sha256'] = sha(args.packing_proof.read_bytes())
    profile['inactive_midrange_proof_sha256'] = sha(args.inactive_midrange_proof.read_bytes())
    profile_raw = json.dumps(profile, indent=2) + '\n'
    need(len(profile_raw.encode()) < 1024 * 1024, 'profile exceeds metadata budget')
    args.profile_output.write_text(profile_raw)
    report = dict(scope=__doc__, passed=True, authored_materials=14, original_pairs=paired,
        complete_original_banks_checked=paired * 14, native_template_variants=len(variants), roles=roles,
        all_authored_rows_convert=True, outputs_sha256=outputs_hash.hexdigest(),
        active_midrange_factor_rejected=True, midrange_factor_zero_contribution_qualified=True,
        packing_proof_sha256=profile['packing_proof_sha256'],
        inactive_midrange_proof_sha256=profile['inactive_midrange_proof_sha256'],
        profile_path=str(args.profile_output), profile_sha256=sha(profile_raw.encode()),
        helper_sha256=sha(Path(__file__).read_bytes()), native_projection_admitted=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('passed', 'authored_materials', 'original_pairs', 'native_template_variants')}))


if __name__ == '__main__':
    main()
