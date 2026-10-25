#!/usr/bin/env python3
"""Carry the complete verified Blender light rig into a separate native build.

Run through run_test.py. Original Blender/interchange inputs stay unchanged.
Area sources use wide rectangular native projective lights for the static bake;
this preserves source coverage, not Blender's exact area-light integrator.
"""
import argparse
import copy
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import numpy as np
from build_dust2_mombasa import ass_shader_name


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,d):p.write_text(json.dumps(d,indent=2)+'\n')


def stage(source_root, prior, output):
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not output.exists()
    inventory=json.loads((source_root/'blender-light-inventory.json').read_text())
    assert sha(source_root/'source/working/Deserted-polished.blend')==inventory['blend_sha256']
    output.mkdir();shutil.copytree(prior/'model',output/'model');shutil.copytree(prior/'evidence',output/'evidence')
    shutil.copyfile(prior/'native-review-cameras.json',output/'native-review-cameras.json')
    model=output/'model';old=json.loads((model/'native-practical-lights.json').read_text())
    assert old['source_glb_sha256']==sha(model/'dust2_mombasa.glb')
    spots={r['name']:r for r in old['lights']};lights=[];sun=None
    for row in inventory['lights']:
        assert not row['hide_render'] and row['in_view_layer']
        m=np.array(row['matrix']);p=m[:3,3];forward=-m[:3,2];forward/=np.linalg.norm(forward)
        position=np.array([-p[1],p[0],p[2]])/3.048
        direction=np.array([-forward[1],forward[0],forward[2]])
        if row['type']=='SUN':
            assert sun is None
            sun=dict(name=row['name'],towards_sun=(-direction).tolist(),color=row['color'],
                source_energy=row['energy'],native_power=row['energy']/6.,diameter_degrees=math.degrees(row['angle']))
            continue
        if row['type']=='SPOT':
            r=copy.deepcopy(spots.pop(row['name']))
            assert np.max(np.abs(np.array(r['position_wu'])-position))<1e-5
            assert np.max(np.abs(np.array(r['direction'])-direction))<1e-5
            r.update(source_type='SPOT',native_field_of_view_degrees=math.degrees(row['spot_size']),
                native_near_width_wu=old['near_width_wu'],native_height_scale=1.,
                native_strength=.8*row['energy']/450.)
        else:
            assert row['type']=='AREA' and row['shape']=='RECTANGLE'
            r=dict(name=row['name'],source_type='AREA',position_wu=position.tolist(),direction=direction.tolist(),
                color=row['color'],source_energy_w=row['energy'],source_width_m=row['size'],source_height_m=row['size_y'],
                source_spread_radians=row['spread'],native_near_width_wu=row['size']/3.048,
                native_height_scale=row['size_y']/row['size'],native_field_of_view_degrees=170.,
                native_strength=.8*row['energy']/450.,
                approximation='Static wide rectangular frustum, capped at170 degrees to keep native geometry finite; not a physical area emitter')
        lights.append(r)
    assert not spots and sun is not None and len(lights)==26
    old.update(lights=lights,sun=sun,world=inventory['world_backgrounds'][0],
        source_blend_sha256=inventory['blend_sha256'],source_light_count=27,
        scope='All27 active Blender lights represented:26 native bake lights and1 native sky sun. Native brightness is calibrated; rectangular area integration and AgX exposure are not identical.')
    save(model/'native-practical-lights.json',old)
    # Blender emits the base texture from both fixture material families.
    # Use private native shaders so their different strengths survive baking.
    metadata=json.loads((model/'material_mapping.json').read_text());variants=[]
    expected={'Detail | light':('detail_light','polished_practical_emitter',3.),
        'scenarios\\shaders\\multi\\turf\\light_long_a':('light','polished_arch_emitter',.6)}
    for material in inventory['emissive_materials']:
        key,leaf,strength=expected.pop(material['material'])
        assert not material['strength_linked'] and abs(material['strength']-strength)<1e-5
        assert material['color_linked']
        original=metadata['palette'][key]
        shader='scenarios/multi/dust2_mombasa/shaders/'+leaf
        variants.append(dict(source_shader=original['shader'],shader=shader,palette=key,
            source_emission_strength=strength,native_emissive_power=strength/3.,emissive_color=[1.,1.,1.],
            source_self_illum_map='scenarios/bitmaps/multi/turf/light_long_ilum_turf',
            bitmap=original['bitmap'].replace('\\','/'),source_objects=material['objects']))
        original['native_shader_override']=shader
        old_name=ass_shader_name(original['shader'])+('!' if key=='detail_light' else '%')
        new_name=ass_shader_name(shader.replace('/','\\'))+('!' if key=='detail_light' else '%')
        for path in [model/'Deserted.ass',model/'data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass']:
            text=path.read_text();assert text.count('"'+old_name+'"')==1
            path.write_text(text.replace('"'+old_name+'"','"'+new_name+'"'))
    assert not expected
    save(model/'native-emissive-variants.json',dict(schema='H2_DESERTED_EMISSIVE_VARIANTS_V1',variants=variants,
        scope='Native bake emission power uses1/3 of Blender material strength, retaining its5:1 ratio; base texture drives emission. Original native shader programs remain in use.'))
    save(model/'material_mapping.json',metadata)
    save(output/'evidence/all-light-source-staging.json',dict(passed=True,source_glb_sha256=old['source_glb_sha256'],
        source_blend_sha256=inventory['blend_sha256'],lights=27,spots=18,area_fills=8,suns=1,emissive_materials=2,
        native_light_volumes=26,native_sky_suns=1,scope=old['scope'],native_baked=False))
    print('Complete Blender rig staged:18 spots,8 rectangular fills,1 sun,2 emissive material families',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for k in ('source-root','prior','output'):p.add_argument('--'+k,type=Path,required=True)
    stage(**vars(p.parse_args()))
