#!/usr/bin/env python3
"""Build an explicit campaign visual acceptance ledger from an Xbox asset census.

Historical material admission is recorded separately from live rendering and
matched-frame acceptance. Missing maps, modified reports, unknown references or
unmatched admission rows are errors; none can silently reduce the scope.
"""
import argparse
import hashlib
import json
from pathlib import Path

from audit_campaign import MAPS

NONE = 0xFFFFFFFF
GATES = (
    'native_birth_and_retirement', 'original_visibility_and_draw_order',
    'live_regions_permutations_and_six_lods', 'completed_world_pose_and_palette',
    'ordinary_lighting_colors_and_shader_functions', 'prt_and_effect_lighting',
    'shields_camouflage_and_transparent_passes', 'held_weapons_and_attachments',
    'matched_tu15_color_depth_textures_and_composition',
    'resolution_resize_and_split_screen', 'combined_runtime_delivery',
)


def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def build(census, maps, admission, supplemental=None):
    manifest_path = census / 'manifest.json'
    manifest = json.loads(manifest_path.read_text())
    expected = {row['map']: row for row in manifest['maps']}
    require(len(expected) == len(manifest['maps']) and set(expected) == set(MAPS),
            'Census must contain each of the 15 campaign maps exactly once')
    old = json.loads(admission.read_text())
    admitted = {row['map']: row for row in old['maps']}
    require(len(admitted) == len(old['maps']) and set(admitted) == set(MAPS),
            'Admission report must contain each campaign map exactly once')
    output = dict(authority='OG Xbox TU1.5 code and original Xbox cache assets',
                  boundary='Asset coverage and historical admission only; all live/frame gates remain open.',
                  census_manifest_sha256=sha(manifest_path),
                  historical_admission_sha256=sha(admission),
                  gate_definitions=list(GATES), maps=[], full_visual_parity_verified=False)
    for name in MAPS:
        path = census / (name + '.json')
        require(sha(path) == expected[name]['report_sha256'], f'{name}: census report changed')
        asset = json.loads(path.read_text())
        cache_sha = sha(maps / (name + '.map'))
        require(asset['map'] == name and asset['cache_sha256'] == cache_sha == expected[name]['cache_sha256'],
                f'{name}: original cache identity differs from census')
        for field in ('characters', 'units', 'models', 'shaders', 'passes', 'vertex_shaders'):
            require(len(asset[field]) == expected[name][field], f'{name}: {field} extent differs')
        prior = admitted[name]
        require(prior['failed'] == 0 and all(row['accepted'] for row in prior['materials']),
                f'{name}: historical material admission has a failure')
        materials = {}
        for row in prior['materials']:
            key = (int(row['shader'], 16), row['lod'])
            require(key not in materials, f'{name}: duplicate admission row')
            materials[key] = row
        shader_rows = []
        for key, shader in asset['shaders'].items():
            datum = int(key, 16)
            templates = []
            for post in shader['postprocess']:
                template = asset['templates'][post['template']]
                templates.append(dict(datum=post['template'], name=template['name']))
            lods = [row for (owner, _), row in sorted(materials.items()) if owner == datum]
            require(lods and all(row['name'] == shader['name'] for row in lods),
                    f'{name}: missing or mismatched historical shader admission {key}')
            shader_rows.append(dict(datum=datum, name=shader['name'], templates=templates,
                                    historical_material_lod_admission=lods,
                                    matched_original_frames=[]))
        require({owner for owner, _ in materials} == {int(key, 16) for key in asset['shaders']},
                f'{name}: historical shader set differs')
        model_admission = {row['unit']: row for row in prior['models']}
        require(len(model_admission) == len(prior['models']), f'{name}: duplicate model admission')
        owners = []
        for unit in asset['units']:
            if unit['model'] is None:
                owners.append(dict(**unit, renderable_model=False, gates={g: 'open' for g in GATES}))
                continue
            model = asset['models'][str(unit['model'])]
            prior_model = model_admission.get(unit['datum'])
            require(prior_model and prior_model['accepted'] and prior_model['name'] == unit['name'],
                    f'{name}: missing or mismatched historical model admission {unit["name"]}')
            require(all(shader == NONE or format(shader, '08x') in asset['shaders']
                        for shader in model['materials']), f'{name}: unknown model shader')
            for region in model['regions']:
                for permutation in region['permutations']:
                    require(len(permutation['sections']) == 6 and all(
                        s == 0xFFFF or 0 <= s < len(model['sections']) for s in permutation['sections']),
                        f'{name}: invalid authored six-LOD section reference')
            owners.append(dict(**unit, renderable_model=True, model_name=model['name'],
                               material_slots=model['materials'], regions=model['regions'],
                               sections=model['sections'], historical_model_admission=prior_model,
                               gates={g: 'open' for g in GATES}, matched_original_frames=[]))
        require(set(model_admission) == {u['datum'] for u in asset['units'] if u['model'] is not None},
                f'{name}: historical model set differs')
        output['maps'].append(dict(map=name, cache_sha256=cache_sha,
                                  census_report_sha256=sha(path), owners=owners, shaders=shader_rows,
                                  character_inheritance=asset['characters']))
    if supplemental is not None:
        extra_manifest_path = supplemental / 'manifest.json'
        extra_manifest = json.loads(extra_manifest_path.read_text())
        extra_expected = {row['map']: row for row in extra_manifest['maps']}
        require(extra_manifest.get('supplemental_character_roots') is True and
                len(extra_expected) == len(extra_manifest['maps']) and set(extra_expected) == set(MAPS),
                'Supplemental census must declare its root discovery and all15 maps')
        output['supplemental_census_manifest_sha256'] = sha(extra_manifest_path)
        for row in output['maps']:
            name = row['map']
            path = supplemental / (name + '.json')
            require(sha(path) == extra_expected[name]['report_sha256'], f'{name}: supplemental report changed')
            asset = json.loads(path.read_text())
            require(asset['map'] == name and asset['cache_sha256'] == row['cache_sha256'] ==
                    extra_expected[name]['cache_sha256'], f'{name}: supplemental cache identity differs')
            require({u['datum'] for u in row['owners']}.isdisjoint(u['datum'] for u in asset['units']),
                    f'{name}: supplemental root duplicates base unit')
            require(asset['root_discovery']['arbitrary_attachment_dependency_closure'] is False,
                    f'{name}: unsupported claim of complete arbitrary attachment coverage')
            for field in ('characters', 'units', 'models', 'shaders', 'passes', 'vertex_shaders'):
                require(len(asset[field]) == extra_expected[name][field], f'{name}: supplemental {field} extent differs')
            row['supplemental_root_discovery'] = asset['root_discovery']
            row['supplemental_owners'] = []
            for unit in asset['units']:
                model = asset['models'][str(unit['model'])]
                require(all(shader == NONE or format(shader, '08x') in asset['shaders']
                            for shader in model['materials']), f'{name}: unknown supplemental model shader')
                for region in model['regions']:
                    for permutation in region['permutations']:
                        require(len(permutation['sections']) == 6 and all(
                            s == 0xFFFF or 0 <= s < len(model['sections']) for s in permutation['sections']),
                            f'{name}: invalid supplemental six-LOD section reference')
                row['supplemental_owners'].append(dict(**unit, model_name=model['name'],
                    material_slots=model['materials'], regions=model['regions'], sections=model['sections'],
                    historical_model_admission=None, gates={g: 'open' for g in GATES}, matched_original_frames=[]))
            row['supplemental_shaders'] = asset['shaders']
            row['supplemental_templates'] = asset['templates']
        output['arbitrary_attachment_dependency_closure_verified'] = False
    output['totals'] = dict(maps=len(output['maps']),
        map_local_owners=sum(len(m['owners']) for m in output['maps']),
        unique_unit_paths=len({u['name'] for m in output['maps'] for u in m['owners']}),
        material_lod_admissions=sum(len(s['historical_material_lod_admission'])
                                    for m in output['maps'] for s in m['shaders']),
        matched_original_character_frames=0)
    if supplemental is not None:
        output['totals']['supplemental_map_local_roots'] = sum(len(m['supplemental_owners']) for m in output['maps'])
        output['totals']['supplemental_unique_paths'] = len({u['name'] for m in output['maps'] for u in m['supplemental_owners']})
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--census', type=Path, required=True)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--historical-admission', type=Path, required=True)
    parser.add_argument('--supplemental-census', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = build(args.census, args.maps, args.historical_admission, args.supplemental_census)
    with args.output.open('x') as stream:
        json.dump(result, stream, indent=2)
        stream.write('\n')
    print(json.dumps(result['totals'], sort_keys=True))


if __name__ == '__main__':
    main()
