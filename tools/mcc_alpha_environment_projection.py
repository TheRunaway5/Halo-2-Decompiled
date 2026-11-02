"""Prove alpha/environment metadata conversion from complete named bindings.

Every original template and paired material is checked. UV roles are linked by
parameter name, typed bitmap property and the unchanged TU1.5 component masks;
no role or new scale is inferred from equal default values. Static conversion
does not establish animation support, E3 rendering or mission acceptance.
Run through run_test.py; retain only bounded metadata profiles and reports.
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

from audit_h2ek_shader_bindings import BindingReader, psh_bindings
from mcc_env_illum_projection import FIELDS, banks, u16
from mcc_env_illum_projection import native_graph
from mcc_tiling_specular_projection import pixel_roles, vertex_roles


TEMPLATE = r'shaders\shader_templates\opaque\tex_bump_env_alpha_test'


def need(ok, why):
    if not ok:
        raise ValueError('Alpha/environment projection: ' + why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def derive(source, native, metadata, masks):
    sp, np = pixel_roles(source, metadata), pixel_roles(native, metadata, True)
    sv, su = vertex_roles(source, metadata)
    nv, nu = vertex_roles(native, metadata)
    need(set(sp) == set(np) == set(range(6)) and set(sv) == set(range(8)) and
         set(nv) == set(range(9)), 'complete named main parameter ownership changed')
    pixels, vectors = [], []
    for slot in range(6):
        candidates = [i for i, role in sp.items() if role == np[slot]]
        need(len(candidates) == 1, 'original packed color has no unique named source')
        pixels.append(candidates[0])
    for slot in range(9):
        name, target_mode = nv[slot]
        candidates = [i for i, role in sv.items() if role[0] == name]
        need(len(candidates) == 1, 'original vector has no unique named source')
        source_slot = candidates[0]
        source_mode = sv[source_slot][1]
        if source_mode == target_mode:
            lanes = [0, 1, 2, 3]
        elif (source_mode, target_mode) == (13, 9):
            need(masks['13'] == 15 and masks['9'] == 3, 'original XY/full component contract changed')
            lanes = [0, 1, None, None]
        elif (source_mode, target_mode) == (9, 10):
            need(masks['9'] == 3 and masks['10'] == 12, 'original XY/ZW component contract changed')
            lanes = [None, None, 0, 1]
        else:
            raise ValueError('Unproved named vector component conversion')
        if source_slot in su or slot in nu:
            need(source_slot in su and slot in nu and su[source_slot][0] == nu[slot][0] and
                 su[source_slot][1:] == (source_mode, 0) and nu[slot][1:] == (target_mode, 0),
                 'named transform escaped its typed bitmap/component owner')
        need(all(lane is not None or not (masks[str(target_mode)] & (1 << i))
                 for i, lane in enumerate(lanes)), 'canonical zero would replace a consumed lane')
        vectors.append(dict(source=source_slot, parameter=name, source_mode=source_mode,
                            native_mode=target_mode, lanes=lanes))
    return dict(pixel_sources=pixels, vector_sources=vectors,
                source_vector_roles=sv, native_vector_roles=nv,
                source_pixel_roles=sp, native_pixel_roles=np)


def project(row, source_arrays, native_arrays, roles):
    data = {key: bytes.fromhex(value['hex']) for key, value in row['source_banks'].items()}
    source, native = banks(source_arrays), banks(native_arrays)
    need(all(value['count'] == roles['source_counts'][key] for key, value in row['source_banks'].items()) and
         all(len(data[str(field)]) == roles['source_counts'][str(field)] * stride for field, stride in FIELDS),
         'complete source bank counts or extents differ')
    root = row['source_root']
    need(root['definition_count'] == 3 and root['trailing_definitions_zero'] and root['named_parameter_count'] == 0,
         'additional platform/named authoring requires an owner')
    need(len(data['4']) == 84 and len(data['12']) == 24 and len(data['20']) == 128,
         'main parameter bank shape differs from named roles')
    need(not row['source_overlays'] and all(not data[str(f)] for f in (60, 68, 76, 84)) and
         not any(data['52']) and len(data['52']) == len(source['24']) // 6 * 10,
         'animation requires a complete native mapping')
    need(data['36'] == source['8'] and data['44'] == b''.join(source['16'][i:i+2]
         for i in range(8, len(source['16']), 10)), 'authored scheduling differs from complete compiler graph')
    need(len(data['28']) == 12 and all(u16(data['28'], i * 6 + 4) == u16(source['0'], i * 10) ==
         u16(native['0'], i * 10) for i in range(2)), 'authored LOD pass ranges differ')
    need(all(math.isfinite(x) for x, in struct.iter_unpack('<f', data['20'])), 'nonfinite authored vector')
    result = dict(data)
    result['12'] = b''.join(data['12'][i * 4:i * 4 + 4] for i in roles['pixel_sources'])
    result['20'] = b''.join(bytes(4) if lane is None else
        data['20'][role['source'] * 16 + lane * 4:role['source'] * 16 + lane * 4 + 4]
        for role in roles['vector_sources'] for lane in role['lanes'])
    result['36'] = native['8']
    result['44'] = b''.join(native['16'][i:i+2] for i in range(8, len(native['16']), 10))
    result['52'] = bytes(len(native['24']) // 6 * 10)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority', type=Path, required=True)
    parser.add_argument('--bindings', type=Path, required=True)
    parser.add_argument('--mask-proof', type=Path, required=True)
    parser.add_argument('--profile-output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    packed = args.authority.read_bytes()
    data, metadata = json.loads(lzma.decompress(packed)), json.loads(args.bindings.read_bytes())
    mask = json.loads(args.mask_proof.read_bytes())
    need(mask['passed'] and mask['entry'] == '0x16fe0' and mask['instruction_sites'] == 101 and
         mask['all_sites_observed'] and not mask['original_code_patched'] and
         not mask['original_calls_intercepted'] and mask['mixed_and_boundary_cases'] == 1185 and
         mask['helper_sha256'] == sha(Path(__file__).with_name('audit_shader_vertex_masks_tu15.py').read_bytes()),
         'static component masks lack their complete unchanged TU1.5 execution proof')
    need(data['template'] == TEMPLATE and not data['errors'] and len(data['materials']) == 88 and
         len(data['stems']) == 1 and metadata['passed'] and not metadata['missing_editable_passes'] and
         metadata['authority_sha256'] == sha(packed), 'complete authored cohort or metadata differs')
    for item in metadata['textual'].values():
        need(psh_bindings(Path(item['path'])) == item, 'complete textual binding input changed')
    for item in metadata['editable'].values():
        fresh = BindingReader(Path(item['path']), 'spas').shader_pass()
        need(json.dumps(fresh, sort_keys=True) == json.dumps(item, sort_keys=True), 'complete editable binding input changed')
        need(all(v['scale_by_texture_stage'] == 0 for impl in fresh['metadata']['implementations']
                 for v in impl['vertex_constants']), 'texture-stage scaling requires an additional owner')
    graph = next(iter(data['stems'].values()))['graph']
    source = graph['definitions'][0]['arrays']
    native, roles, variants = None, None, []
    for name, info in data['native_libraries'].items():
        path = Path(info['path'])
        raw = path.read_bytes()
        need(sha(raw) == info['sha256'], 'original native library changed')
        for record in json.loads(raw)['shader_tags']:
            if record['group'] != 'stem' or record['name'] != TEMPLATE:
                continue
            arrays = native_graph(path.parent, record)
            derived = derive(source, arrays, metadata, mask['masks'])
            need(native is None or native == arrays, 'original native variants disagree on complete graph')
            need(roles is None or roles == derived, 'original native variants disagree on named parameter roles')
            native, roles = arrays, derived
            variants.append(dict(map=name, payload=record['payload'], bytes=record['bytes'], fnv64=record['fnv64'],
                                 source_xbe_sha256=info['source_xbe_sha256']))
    need(len(variants) == 38 and roles is not None, 'complete original template cohort changed')
    roles['source_counts'] = {key: value['count'] for key, value in data['materials'][0]['source_banks'].items()}
    paired, output_hash = 0, hashlib.sha256()
    for row in data['materials']:
        output = project(row, source, native, roles)
        for field, _ in FIELDS:
            output_hash.update(output[str(field)])
        if row['native_pair']:
            need(row['source_images'] == row['native_images'], 'complete paired typed bitmap/frame bank differs')
            for field, _ in FIELDS:
                if field != 4:
                    need(output[str(field)].hex() == row['native_banks'][str(field)]['hex'],
                         'complete original bank differs: ' + row['map'] + ':' + row['shader'] + ':' + str(field))
            paired += 1
    need(paired == 68, 'complete original material controls changed')
    changed = copy.deepcopy(data['materials'][0])
    vectors = bytearray.fromhex(changed['source_banks']['20']['hex'])
    # Independent nondefault control exercises both relocation and duplicated
    # bump ownership; translations must survive only in the full transform.
    struct.pack_into('<4f', vectors, 0, 2.25, 3.5, -0.75, 0.625)
    struct.pack_into('<4f', vectors, 48, 5.25, 7.5, 19, 23)
    changed['source_banks']['20']['hex'] = vectors.hex()
    output = project(changed, source, native, roles)['20']
    need(struct.unpack_from('<4f', output, 16) == (2.25, 3.5, 0, 0) and
         struct.unpack_from('<4f', output, 48) == (0, 0, 5.25, 7.5) and
         struct.unpack_from('<4f', output, 64) == (2.25, 3.5, -0.75, 0.625),
         'nondefault authored transform was replaced by a literal/default')
    profile = dict(schema='H2_MCC_NATIVE_ALPHA_ENVIRONMENT_PROFILES_V1', template=TEMPLATE,
        source_graph=graph, native_arrays=native, native_records=variants, roles=roles,
        authority=dict(path=str(args.authority), sha256=sha(packed), materials=88, paired_controls=paired),
        named_bindings_sha256=sha(args.bindings.read_bytes()), mask_proof_sha256=sha(args.mask_proof.read_bytes()))
    profile_raw = (json.dumps(profile, indent=2) + '\n').encode()
    need(len(profile_raw) < 1024 * 1024, 'profile exceeds bounded metadata budget')
    args.profile_output.write_bytes(profile_raw)
    report = dict(scope=__doc__, passed=True, authored_materials=88, original_pairs=paired,
        complete_original_banks_checked=paired * 14, native_template_variants=len(variants),
        roles=roles, all_authored_rows_convert=True, outputs_sha256=output_hash.hexdigest(),
        nondefault_alpha_scale_and_bump_translation_preserved=True,
        profile_path=str(args.profile_output), profile_sha256=sha(profile_raw),
        helper_sha256=sha(Path(__file__).read_bytes()), native_projection_admitted=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'authored_materials', 'original_pairs', 'native_template_variants')}))


if __name__ == '__main__':
    main()
