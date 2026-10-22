#!/usr/bin/env python3
"""Render quick architectural review views with bpy; not runtime evidence.

Run under tools/run_test.py with bpy 4.5.3. The camera fill is an inspection
light, not part of the authored map lighting. Keep outputs in owned scratch.
"""
import argparse
import json
import math
import os
from pathlib import Path
import sys

import bpy
from mathutils import Vector
import numpy as np
from build_dust2_mombasa import Glb


def preview(package, output):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert output.resolve().is_relative_to(scratch)
    output.mkdir(exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 16
    scene.cycles.use_denoising = True
    preferences = bpy.context.preferences.addons['cycles'].preferences
    try:
        preferences.compute_device_type = 'OPTIX'
        preferences.get_devices()
        for device in preferences.devices:
            device.use = device.type == 'OPTIX'
        scene.cycles.device = 'GPU'
    except Exception:
        scene.cycles.device = 'CPU'
    scene.render.resolution_x = 1024
    scene.render.resolution_y = 576
    scene.render.resolution_percentage = 100
    scene.world = bpy.data.worlds.new('Inspection ambient')
    scene.world.use_nodes = True
    scene.world.node_tree.nodes['Background'].inputs['Color'].default_value = (.5, .6, .7, 1)
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = .5
    sun = bpy.data.lights.new('Inspection sun', 'SUN')
    sun.energy = 2
    sun.angle = math.radians(2)
    light = bpy.data.objects.new('Inspection sun', sun)
    scene.collection.objects.link(light)
    light.rotation_euler = (.4, -.6, -.8)
    palette = json.loads((package/'material_mapping.json').read_text())['palette']
    materials = []
    for key, row in palette.items():
        material = bpy.data.materials.new(key)
        material.use_nodes = True
        nodes = material.node_tree.nodes
        shader = nodes.get('Principled BSDF')
        shader.inputs['Roughness'].default_value = .9
        image = nodes.new('ShaderNodeTexImage')
        image.image = bpy.data.images.load(str(package/row['png']), check_existing=True)
        material.node_tree.links.new(image.outputs['Color'], shader.inputs['Base Color'])
        if key in ('plant', 'fence') or 'frond' in row['shader']:
            material.node_tree.links.new(image.outputs['Alpha'], shader.inputs['Alpha'])
        materials.append(material)
    source = Glb(package/'dust2_mombasa.glb')
    for _, node, mesh, world in source.nodes():
        for primitive in mesh['primitives']:
            p = source.array(primitive['attributes']['POSITION']).astype(float)
            p = (p @ world[:3, :3].T + world[:3, 3])[:, [2, 0, 1]]
            t = source.array(primitive['indices'])[:, 0].reshape(-1, 3)
            uv = source.array(primitive['attributes']['TEXCOORD_0'])
            data = bpy.data.meshes.new(node['name'])
            data.from_pydata(p.tolist(), [], t.tolist())
            data.materials.append(materials[primitive['material']])
            layer = data.uv_layers.new()
            ids = np.empty(len(data.loops), dtype=np.int32)
            data.loops.foreach_get('vertex_index', ids)
            layer.data.foreach_set('uv', uv[ids].reshape(-1))
            obj = bpy.data.objects.new(node['name'], data)
            scene.collection.objects.link(obj)
    camera = bpy.data.cameras.new('Review camera')
    camera.angle = math.radians(70)
    obj = bpy.data.objects.new('Review camera', camera)
    scene.collection.objects.link(obj)
    scene.camera = obj
    fill = bpy.data.lights.new('Inspection camera fill', 'AREA')
    fill.energy = 180
    fill.size = 3
    fill_obj = bpy.data.objects.new('Inspection camera fill', fill)
    scene.collection.objects.link(fill_obj)
    views = [
        ('ct-floor', (1.14, 17.64, -.43), (1, 0, -.45)),
        ('ct-exit', (9, 17, .6), (1, 0, -.15)),
        ('ground-ahead', (13.74, 16.31, .6), (1, 0, -.6)),
        ('wreck', (-10.5, 15, .8), (-1, .2, -.18)),
    ]
    plan_path = package/'architecture-plan.json'
    if plan_path.exists():
        plan = json.loads(plan_path.read_text())
        views.extend((v['name'], v['position'], v['forward']) for v in plan['inspection_views'])
    views.append(('overview', (0, 8, 80), (0, 0, -1)))
    for name, position, direction in views:
        obj.location = Vector(position)*3.048
        obj.rotation_euler = Vector(direction).to_track_quat('-Z', 'Y').to_euler()
        camera.type = 'ORTHO' if name == 'overview' else 'PERSP'
        if name == 'overview':
            camera.ortho_scale = 220
        fill_obj.location = obj.location
        fill_obj.rotation_euler = obj.rotation_euler
        scene.render.filepath = str(output/(name+'.png'))
        bpy.ops.render.render(write_still=True)
    (output/'scope.json').write_text(json.dumps(dict(
        runtime_evidence=False, camera_fill_is_inspection_only=True,
        views=[v[0] for v in views]), indent=2)+'\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    preview(**vars(parser.parse_args()))
