#!/usr/bin/env python3
"""Reconcile the complete Blender light inventory with retained native inputs.

Run through run_test.py with the pinned Halo Tag Interface. Checks actual saved
authoring tags and final compiled tag reachability. Native render acceptance is
separate; this does not equate the two renderers' photometry or area integrators.
"""
import argparse
import copy
import hashlib
import json
import math
import os
from pathlib import Path
import sys
import struct
from mcc_content import MccCache


def audit(root, interface, report):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    sys.path.insert(0,str(interface));import tag_interface as ti
    g,e,d,groups,extensions,post,_=ti.get_version_args('halo2',0)
    defs=g.generate_defs(d,str(scratch/'defs'),groups,extensions)
    tags=scratch/'tags';folder=tags/'scenarios/multi/dust2_mombasa';folder.parent.mkdir(parents=True)
    folder.symlink_to((root/'h2ek-tags').resolve(),target_is_directory=True)
    def read(name):return ti.read_file(defs,str(tags),str(tags/(name+'.'+('light' if '/lights/' in name else 'shader'))),0,engine_tag=e)['Data']
    def near(a,b,tol=1e-5):
        assert len(a)==len(b) and all(abs(x-y)<tol for x,y in zip(a,b)),(a,b)
    prefix='scenarios/multi/dust2_mombasa/'
    scenario=ti.read_file(defs,str(tags),str(folder/'dust2_mombasa.scenario'),0,engine_tag=e)['Data']
    sky=ti.read_file(defs,str(tags),str(folder/'desert.sky'),0,engine_tag=e)['Data']
    authored=json.loads((root/'model/native-practical-lights.json').read_text())
    inventory_path=root.parent/'blender-light-inventory.json'
    inventory=json.loads(inventory_path.read_text())
    assert hashlib.sha256((root.parent/'source/working/Deserted-polished.blend').read_bytes()).hexdigest()==inventory['blend_sha256']==authored['source_blend_sha256']
    source_lights={r['name']:r for r in inventory['lights']}
    assert len(source_lights)==len(inventory['lights'])==27
    assert set(source_lights)=={r['name'] for r in authored['lights']}|{authored['sun']['name']}
    for source in [*authored['lights'],authored['sun']]:
        original=source_lights[source['name']]
        assert not original['hide_render'] and original['in_view_layer']
        m=original['matrix'];forward=[-m[0][2],-m[1][2],-m[2][2]]
        length=math.sqrt(sum(x*x for x in forward));direction=[-forward[1]/length,forward[0]/length,forward[2]/length]
        near(source['color'],original['color'])
        if original['type']=='SUN':
            near(source['towards_sun'],[-x for x in direction])
            near([source['native_power'],source['diameter_degrees']],
                [original['energy']/6.,math.degrees(original['angle'])])
        else:
            assert source['source_type']==original['type']
            near(source['position_wu'],[-m[1][3]/3.048,m[0][3]/3.048,m[2][3]/3.048])
            near(source['direction'],direction)
            near([source['native_strength']],[.8*original['energy']/450.])
            if original['type']=='SPOT':
                near([source['native_field_of_view_degrees']],[math.degrees(original['spot_size'])])
            else:
                assert original['type']=='AREA' and original['shape']=='RECTANGLE'
                near([source['native_near_width_wu'],source['native_height_scale']],
                    [original['size']/3.048,original['size_y']/original['size']])
    assert authored['world']==inventory['world_backgrounds'][0]
    assert authored['source_light_count']==27 and len(authored['lights'])==26
    assert len(scenario['light volumes'])==len(scenario['light volumes palette'])==26
    expected=[];rows=[]
    for i,(source,instance) in enumerate(zip(authored['lights'],scenario['light volumes'])):
        name=prefix+f'lights/practical_{i:02d}';expected.append(('ligh',name))
        assert instance['type']==i
        assert scenario['light volumes palette'][i]['name']['path']==name
        near(instance['position'],source['position_wu'])
        target=[b-a for a,b in zip(instance['position'],instance['target point'])]
        length=math.sqrt(sum(x*x for x in target));near([x/length for x in target],source['direction'])
        yaw,pitch,roll=map(math.radians,instance['rotation']);assert abs(yaw)<1e-6
        near([math.sin(pitch)*math.cos(roll),-math.sin(roll),math.cos(pitch)*math.cos(roll)],source['direction'])
        assert instance['lightmap type']['value']==3 and instance['type_2']['value']==2
        near([instance['width'],instance['height scale'],instance['field of view']],
            [source['native_near_width_wu'],source['native_height_scale'],source['native_field_of_view_degrees']])
        tag=read(name)
        assert tag['default lightmap setting']['value']==2
        near([tag['diffuse lower bound'][k] for k in ('R','G','B')],source['color'])
        near([tag['diffuse upper bound'][k] for k in ('R','G','B')],source['color'])
        near([tag['brightness bounds']['Min'],tag['brightness bounds']['Max']],[source['native_strength']]*2)
        rows.append(dict(name=source['name'],source_type=source['source_type'],native_tag=name,
            saved_position_direction_color_strength_shape_and_static_mode_verified=True))
    sun=authored['sun'];assert len(sky['lights'])==1
    light=sky['lights'][0];azimuth,elevation=map(math.radians,light['direction'])
    near([math.cos(azimuth)*math.cos(elevation),math.sin(azimuth)*math.cos(elevation),math.sin(elevation)],sun['towards_sun'])
    radio=light['radiosity'][0];near([radio['power'],radio['diameter']],[sun['native_power'],sun['diameter_degrees']])
    near([radio['color'][k] for k in ('R','G','B')],sun['color']);assert radio['flags']&15==15
    presentation=root/'model/native-sky-presentation.json'
    profile=json.loads(presentation.read_text()) if presentation.exists() else {}
    near([sky['global sky rotation']],[profile.get('post_bake_model_yaw_degrees',0.)])
    if 'clear_color' in profile:
        near([sky['clear color'][k] for k in ('R','G','B')],profile['clear_color'])
    world=authored['world'];near([sky['outdoor ambient color'][k] for k in ('R','G','B')],
        [x*world['strength'] for x in world['color'][:3]])
    emission=[]
    for row in json.loads((root/'model/native-emissive-variants.json').read_text())['variants']:
        material=next(r for r in inventory['emissive_materials'] if sorted(r['objects'])==sorted(row['source_objects']))
        near([material['strength']/3.],[row['native_emissive_power']])
        assert material['color_linked'] and not material['strength_linked']
        expected.append(('shad',row['shader']));p={p['name']:p for p in read(row['shader'])['parameters']}
        near([p['emissive_power']['const value']],[row['native_emissive_power']])
        near([p['emissive_color']['const color'][k] for k in ('R','G','B')],row['emissive_color'])
        assert p['self_illum_map']['bitmap']['path']==row['bitmap']
        emission.append(dict(palette=row['palette'],shader=row['shader'],native_emissive_power=row['native_emissive_power'],
            emission_bitmap=row['bitmap'],source_objects=row['source_objects'],saved_parameters_verified=True))
    path=root/'content/halo2/Mods/deserted/deserted.map';cache=MccCache(path)
    present={(t['group'],t['name'].replace('\\','/')) for t in cache.tags.values()}
    compiled_sky=next(t for t in cache.tags.values() if t['group']=='sky ' and
        t['name'].replace('\\','/')==prefix+'desert')
    near([struct.unpack_from('<f',cache.data,compiled_sky['offset']+0x80)[0]],
        [math.radians(sky['global sky rotation'])])
    near(struct.unpack_from('<3f',cache.data,compiled_sky['offset']+0x34),
        [sky['outdoor ambient color'][k] for k in ('R','G','B')])
    near(struct.unpack_from('<3f',cache.data,compiled_sky['offset']+0xa0),
        [sky['clear color'][k] for k in ('R','G','B')])
    assert all(tag in present for tag in expected)
    actual={name for group,name in present if group=='ligh' and name.startswith(prefix+'lights/')}
    assert len(actual)==26
    compiled_emission=[]
    library=root/'content/native-shaders/maps/dust2_mombasa'
    records={(r['group'],r['name']):r for r in json.loads((library/'library.json').read_text())['shader_tags']}
    for row in json.loads((root/'model/native-emissive-variants.json').read_text())['variants']:
        tag=next(t for t in cache.tags.values() if t['group']=='shad' and t['name'].replace('\\','/')==row['shader'])
        # Authoritative shader.xml: cached runtime-properties block at12,
        # stride80 after cached8-byte tag refs; emission RGB/power at16/28.
        # The stock Turf reference was checked against its actual .shader tag.
        props=cache.block(tag['offset']+12,80);assert len(props)==1
        prop=props[0];near(struct.unpack_from('<3f',cache.data,prop+16),row['emissive_color'])
        near([struct.unpack_from('<f',cache.data,prop+28)[0]],[row['native_emissive_power']])
        assert cache.tags[cache.u32(prop+12)]['name'].replace('\\','/')==row['bitmap']
        original=records[('shad',row['source_shader'].replace('/','\\'))]
        expected=copy.deepcopy(original);expected['name']=row['shader'].replace('/','\\');changed=0
        for ref in expected['references']:
            if ref[1]==['bitm',row['source_self_illum_map'].replace('/','\\')]:
                ref[1][1]=row['bitmap'].replace('/','\\');changed+=1
        assert changed==3 and records[('shad',expected['name'])]==expected
        payload=library/'payloads'/expected['payload']
        assert hashlib.sha256(payload.read_bytes()).hexdigest()==payload.stem
        compiled_emission.append(dict(shader=row['shader'],runtime_emission_rgb_power_and_bitmap_verified=True,
            original_native_program_payload=expected['payload'],emission_bitmap_references=changed))
    result=dict(passed=True,source_glb_sha256=hashlib.sha256((root/'model/dust2_mombasa.glb').read_bytes()).hexdigest(),
        source_blend_sha256=authored['source_blend_sha256'],cache_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        actual_blend_inventory_sha256=hashlib.sha256(inventory_path.read_bytes()).hexdigest(),
        actual_blend_source_names_transforms_colors_strengths_and_emissive_objects_reconciled=True,
        source_lights=27,spot_sources=18,rectangular_area_sources=8,sky_suns=1,emissive_material_groups=emission,
        world_ambient_verified=True,sun_direction_color_power_diameter_verified=True,
        compiled_sky_model_yaw_degrees=sky['global sky rotation'],
        compiled_sky_clear_color_and_world_ambient_match_saved_inputs=True,local_sources=rows,
        compiled_emission=compiled_emission,
        all_local_lights_and_emissive_shaders_reachable_in_final_cache=True,
        scope='All27 Blender lights, both emissive material families and constant world represented in saved native inputs and final cache. Native area shapes/brightness are calibrated equivalents. Actual baked rendering and movement are reviewed separately.')
    report.write_text(json.dumps(result,indent=2)+'\n');print('All27 lights,2 emissive material families and world ambient verified in saved native inputs and compiled cache',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for k in ('root','interface','report'):p.add_argument('--'+k,type=Path,required=True)
    audit(**vars(p.parse_args()))
