#!/usr/bin/env python3
"""Author a portable Blender lighting scene and render the Dust II package.

Requires Blender's bpy Python module. Run through tools/run_test.py, with
--package pointing at the owned staging package. Renders are explicitly
requested model previews; they do not qualify Halo 2 runtime lightmaps.
"""
import argparse
import json
import math
from pathlib import Path
import time

import bpy
from mathutils import Vector


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package",required=True,type=Path)
    parser.add_argument("--samples",default=48,type=int)
    args=parser.parse_args()
    package=args.package.resolve()
    profile=json.loads((package/"lighting.json").read_text())
    config=profile["blender"]
    started=time.monotonic()
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(package/"dust2_mombasa.glb"))
    scene=bpy.context.scene
    scene.render.engine="CYCLES"
    scene.cycles.samples=args.samples
    scene.cycles.use_adaptive_sampling=True
    scene.cycles.adaptive_threshold=.035
    scene.cycles.use_denoising=True
    scene.cycles.max_bounces=config["total_bounces"]
    scene.cycles.diffuse_bounces=config["diffuse_bounces"]
    scene.cycles.glossy_bounces=3
    scene.cycles.transmission_bounces=2
    devices=bpy.context.preferences.addons['cycles'].preferences
    devices.compute_device_type='OPTIX'
    devices.get_devices()
    selected=[]
    for device in devices.devices:
        device.use=device.type=='OPTIX'
        if device.use:selected.append(device.name)
    scene.cycles.device='GPU' if selected else 'CPU'
    print('Render devices:',selected or ['CPU'],flush=True)
    suns=[o for o in scene.objects if o.type=='LIGHT' and o.data.type=='SUN']
    if len(suns)!=1:raise ValueError('Expected the original Dust II sun')
    sun=suns[0]
    sun.name="Dust II reference sun / desert daylight"
    sun.data.energy=config["sun_energy"]
    sun.data.angle=math.radians(config["sun_angle_degrees"])
    toward_sun=-(sun.matrix_world.to_quaternion()@Vector((0,0,-1)))
    toward_sun.normalize()
    world=bpy.data.worlds.new('Desert sky / indirect lighting')
    world.use_nodes=True
    scene.world=world
    nodes=world.node_tree.nodes
    sky=nodes.new('ShaderNodeTexSky')
    sky.sky_type='NISHITA'
    sky.sun_disc=False
    sky.sun_elevation=math.asin(max(-1,min(1,toward_sun.z)))
    sky.sun_rotation=math.atan2(toward_sun.y,toward_sun.x)
    sky.altitude=100
    sky.air_density=config['air_density']
    sky.dust_density=config['dust_density']
    nodes['Background'].inputs['Strength'].default_value=config['sky_strength']
    world.node_tree.links.new(sky.outputs['Color'],nodes['Background'].inputs['Color'])
    # Preserve the imported geometry and UVs; texture luminance controls the
    # fluorescent material's emission rather than adding arbitrary point lamps.
    emissive=[]
    for material in bpy.data.materials:
        if not material.use_nodes:continue
        images=[n for n in material.node_tree.nodes if n.type=='TEX_IMAGE' and n.image]
        if not any(n.image.name.split('.')[0]=='light' for n in images):continue
        principled=next((n for n in material.node_tree.nodes if n.type=='BSDF_PRINCIPLED'),None)
        if principled:
            texture=next(n for n in images if n.image.name.split('.')[0]=='light')
            material.node_tree.links.new(texture.outputs['Color'],principled.inputs['Emission Color'])
            principled.inputs['Emission Strength'].default_value=config['emissive_fixture_strength']
            emissive.append(material.name)
    scene.view_settings.view_transform=config['view_transform']
    scene.view_settings.exposure=config['exposure']
    scene.render.resolution_x=1400
    scene.render.resolution_y=1000
    scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.render.image_settings.color_mode='RGB'
    scene.render.image_settings.compression=50
    scene.render.film_transparent=False
    scene.render.use_persistent_data=True
    scene.unit_settings.system='METRIC'
    camera_data=bpy.data.cameras.new('Dust II review camera')
    camera=bpy.data.objects.new('Dust II review camera',camera_data)
    scene.collection.objects.link(camera)
    scene.camera=camera
    camera.data.clip_end=5000
    camera.data.lens=38
    camera.data.sensor_width=36
    def view(eye,target):
        camera.location=Vector(eye)
        camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler()
    views=[('preview-overview.png',[180,-165,150],[22,20,0])]
    views.append(('preview-desert-lighting.png',[150,-125,65],[20,30,12]))
    for filename,eye,target in views:
        view(eye,target)
        scene.render.filepath=str(package/filename)
        print('Rendering',filename,flush=True)
        bpy.ops.render.render(write_still=True)
    # Save an editable scene with embedded textures and actual sun/sky/bounce
    # settings, so lighting work survives beyond a preview image.
    view(views[0][1],views[0][2])
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(package/'dust2_mombasa_lighting.blend'),compress=True)
    report=dict(schema='H2_DUST2_CYCLES_LIGHTING_REVIEW_V1',blender_version=bpy.app.version_string,
        devices=selected or ['CPU'],samples=args.samples,diffuse_bounces=scene.cycles.diffuse_bounces,
        max_bounces=scene.cycles.max_bounces,sun_direction_preserved=True,
        sun_energy=sun.data.energy,sun_angle_degrees=math.degrees(sun.data.angle),
        sky_model='Nishita',sky_strength=config['sky_strength'],
        fixture_emission_materials=emissive,raytraced_sun_shadows=True,indirect_sky_bounce=True,
        editable_scene='dust2_mombasa_lighting.blend',embedded_textures=True,
        renders=[v[0] for v in views],elapsed_seconds=round(time.monotonic()-started,2),
        halo2_runtime_lightmaps_baked=False,halo2_runtime_tested=False)
    (package/'lighting_validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2),flush=True)


if __name__=='__main__':main()
