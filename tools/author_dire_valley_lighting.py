#!/usr/bin/env python3
"""Prepare an owned Dire Valley ASS for native lighting and tree scenery.

Run after the native scenery import, before structure compilation, through
tools/run_test.py. The installed CE cache and H2 editing kit remain read-only.
"""
import argparse
import json
import os
from pathlib import Path
import shutil

import numpy as np

from port_dire_valley import save, sha
from prepare_dust2_h2ek import source_meshes


def author(package, kit):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if not package.resolve().is_relative_to(scratch) or not kit.resolve().is_relative_to(scratch):
        raise ValueError('Use the owned authoring package and SDK overlay')
    ass = package / 'data/scenarios/multi/dire_valley/structure/dire_valley.ass'
    receipt = package / 'lighting_authoring.json'
    if receipt.exists():
        result = json.loads(receipt.read_text())
        if result['ass_sha256'] != sha(ass):
            raise ValueError('Previously authored lighting geometry changed')
        return result
    before = sha(ass)
    raw = ass.read_bytes()
    tail = raw[raw.index(b';### INSTANCES ###'):].decode().splitlines()
    count = int(tail[1])
    if len(tail) != 2 + count * 11:
        raise ValueError('Unexpected ASS instance layout')
    instances = [tail[2 + i * 11:2 + (i + 1) * 11] for i in range(count)]
    source = json.loads((package / 'scenery_geometry.json').read_text())
    trees = [(i, row) for i, row in enumerate(source['placement_transforms']) if '/trees/' in row['target']]
    if len(trees) != 33 or any(any(row['offset']) for _, row in trees):
        raise ValueError('Native trees need their measured, unshifted source placements')
    removed = {'"%source_render"'} | {f'"%native_h2_{i}"' for i, _ in trees}
    instances = [row for row in instances if row[1] not in removed]
    if len(instances) != count - 34:
        raise ValueError('Missing world or native tree instances')
    for i, row in enumerate(instances):
        row[2] = str(i)

    stream = source_meshes(ass)
    materials, meshes = list(next(stream)), list(stream)
    ground_collision = {i for i, row in enumerate(materials) if 'coag coag_ground@' in row[0]}
    if not ground_collision:
        raise ValueError('Missing original separate ground collision stream')
    omitted = sum(int(face[0]) in ground_collision for face in meshes[0][1])
    # Keep the closed CE collision shell, with the visible ground itself as
    # the colliding/lightmapped surface. Separate @ ground faces have no
    # render attribution, leaving the original object-light query unwritten.
    # Other world render surfaces now belong to real BSP clusters, where
    # original14AC60 can find their collision-leaf render references.
    records0, faces0 = meshes[0]
    records1, faces1 = meshes[1]
    moved = faces1.copy()
    moved[:, 1:] += len(records0)
    meshes[0] = (list(records0) + list(records1), np.concatenate([faces0, moved]))
    meshes[1] = ([], [])
    authored = []
    for name, variant in materials:
        if name.startswith(('"coag ', '"f_')):
            # Normal compiler welding/chopping is needed for the shared
            # collision/render plane graph; preserve the original ASS inputs.
            name = name.replace(')', '').replace('coag coag_ground!', 'coag coag_ground')
        name = name.replace('lm:0.25', 'lm:1.0' if name.startswith('"f_') else 'lm:0.5')
        authored.append((name, variant))
    with ass.open('wb') as output:
        output.write(('2\n"DIRE_VALLEY"\n"1.0"\n"Eric"\n"LOCAL"\n' + str(len(authored)) + '\n').encode())
        for name, variant in authored:
            output.write((name + '\n' + variant + '\n').encode())
        output.write(f';### OBJECTS ###\n{len(meshes)}\n'.encode())
        for records, faces in meshes:
            faces = [row for row in faces if int(row[0]) not in ground_collision]
            output.write(f'"MESH"\n""\n""\n{len(records)}\n'.encode())
            for record in records:
                output.write(record if isinstance(record, bytes) else record.encode())
            output.write(f'{len(faces)}\n'.encode())
            for row in faces:
                output.write(('\n'.join(str(int(value)) for value in row) + '\n').encode())
        output.write((f';### INSTANCES ###\n{len(instances)}\n' + '\n'.join('\n'.join(row) for row in instances) + '\n').encode())
    shutil.copyfile(ass, kit / 'data/scenarios/multi/dire_valley/structure/dire_valley.ass')
    result = dict(schema='H2_DIRE_VALLEY_LIGHTING_AUTHORING_V1', ass_before_sha256=before,
        ass_sha256=sha(ass), world_render_in_primary_bsp=True, compiler_welding_enabled=True,
        omitted_separate_ground_collision_triangles=omitted,
        collision_policy='Original closed CE shell and clips; visible source ground supplies colliding, lightmapped ground surfaces',
        lightmap_density=dict(forerunner=1.0, terrain=0.5, static_props=0.5),
        native_tree_scenery=[row for _, row in trees], native_tree_count=33,
        bsp_prop_instances=len(instances) - 2, world_instance=False,
        compiler_subparts_and_accelerators_preserved=True)
    save(receipt, result)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--kit', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(author(args.package, args.kit)))
