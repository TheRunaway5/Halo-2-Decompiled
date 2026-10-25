#!/usr/bin/env python3
"""Configure an imported Dust II BSP's spawns, sky, rock shader and walk variant.

Requires a private extracted H2EK tagset and General-101's tag_interface module.
Use tools/run_test.py. Stock kit installation files need not be edited: direct
--kit-root to the owned build copy. Final tags are explicit map deliverables.
"""
from __future__ import annotations

import argparse
import base64
import copy
import json
import math
import os
from pathlib import Path
import re
import sys
import tempfile

from build_dust2_mombasa import save_json, sha


def configure(kit,package,interface,report,unlimited,materials_only,style='new-mombasa',post_bake_presentation_only=False):
    sys.path.insert(0,str(interface.resolve()))
    import tag_interface as ti
    game,engine,definitions,groups,extensions,postprocess,_=ti.get_version_args('halo2',0)
    with tempfile.TemporaryDirectory(prefix='dust2-tag-defs-',dir=os.environ.get('HALO2_TEST_SCRATCH')) as owned:
        merged=game.generate_defs(definitions,owned,groups,extensions)
        tags=kit/'tags'
        def read(relative):
            return ti.read_file(merged,str(tags),str(tags/relative),0,engine_tag=engine)
        def write(relative,value):
            path=tags/relative;path.parent.mkdir(parents=True,exist_ok=True)
            # Atomic replacement also keeps a private hard-linked kit copy from
            # modifying the original installed tag through its shared inode.
            with tempfile.TemporaryDirectory(prefix='dust2-tag-',dir=path.parent) as tmp:
                stage=Path(tmp)/path.name
                ti.write_file(merged,value,ti.obfuscation_buffer_prepare(),str(stage),engine_tag=engine,
                    tag_groups=groups,tag_extensions=extensions,postprocess_functions=postprocess)
                stage.replace(path)
            return dict(path=relative,sha256=sha(path),bytes=path.stat().st_size)
        result={}
        if post_bake_presentation_only:
            # The native sky model owns this visual rotation independently of
            # the individual sun direction. Apply it after radiosity so the
            # compiler never receives an unqualified lighting transform.
            profile=json.loads((package/'native-sky-presentation.json').read_text())
            degrees=profile['post_bake_model_yaw_degrees']
            assert math.isfinite(degrees) and -360<=degrees<=360
            relative='scenarios/multi/dust2_mombasa/desert.sky'
            sky=read(relative);before=copy.deepcopy(sky['Data'])
            sky['Data']['global sky rotation']=degrees
            pin=write(relative,sky)
            back=read(relative)['Data']
            assert abs(back['global sky rotation']-degrees)<1e-5,(degrees,back['global sky rotation'])
            before.pop('global sky rotation');after=copy.deepcopy(back);after.pop('global sky rotation')
            assert before==after,'Post-bake presentation changed lighting or other sky fields'
            save_json(report,dict(schema='H2_DESERTED_POST_BAKE_SKY_PRESENTATION_V1',
                sky=pin,model_yaw_degrees=degrees,saved_model_yaw_degrees=back['global sky rotation'],
                all_other_sky_fields_unchanged=True,
                scope='Visual sky model rotation after lightmaps; individual SUN direction and radiosity fields preserved'))
            return
        emissive=package/'native-emissive-variants.json'
        if emissive.exists():
            result['emissive_materials']=[]
            for row in json.loads(emissive.read_text())['variants']:
                shader=read(row['source_shader'].replace('\\','/')+'.shader')
                parameters={p['name']:p for p in shader['Data']['parameters']}
                parameters['emissive_power']['const value']=row['native_emissive_power']
                parameters['emissive_color']['const color']=dict(zip(('R','G','B'),row['emissive_color']))
                parameters['self_illum_map']['bitmap'].update(path=row['bitmap'],length=len(row['bitmap']))
                relative=row['shader']+'.shader';pin=write(relative,shader)
                back={p['name']:p for p in read(relative)['Data']['parameters']}
                assert abs(back['emissive_power']['const value']-row['native_emissive_power'])<1e-6
                assert back['self_illum_map']['bitmap']['path']==row['bitmap']
                result['emissive_materials'].append(dict(**pin,native_emissive_power=row['native_emissive_power'],readback_passed=True))
        variants=package/'native-material-variants.json'
        if variants.exists():
            result['candidate_material_variants']=[]
            for row in json.loads(variants.read_text())['variants']:
                relative=row['shader'].replace('\\','/')+'.shader'
                assert relative.startswith('scenarios/multi/dust2_mombasa/shaders/')
                shader=read(row['source_shader'].replace('\\','/')+'.shader')
                bitmap=next(p for p in shader['Data']['parameters'] if p['name']=='base_map')['bitmap']
                bitmap['path']=row['bitmap'].replace('\\','/');bitmap['length']=len(bitmap['path'])
                result['candidate_material_variants'].append(write(relative,shader))
        material=read('scenarios/shaders/solo/earthcity/ec_ocean_floor.shader')
        base=next(p for p in material['Data']['parameters'] if p['name']=='base_map')['bitmap']
        base['path']='scenarios/bitmaps/detail_maps/nature/rock/sedimentary_rock_a'
        base['length']=len(base['path'])
        result['rock']=write('scenarios/multi/dust2_mombasa/shaders/desert_sedimentary.shader',material)
        plan=package/'architecture-plan.json'
        if plan.exists() and json.loads(plan.read_text()).get('name')=='Deserted':
            sand=read('scenarios/shaders/solo/earthcity/ec_ocean_floor.shader')
            bitmap=next(p for p in sand['Data']['parameters'] if p['name']=='base_map')['bitmap']
            bitmap['path']='scenarios/bitmaps/detail_maps/nature/ground/sand'
            bitmap['length']=len(bitmap['path'])
            result['sand']=write('scenarios/multi/dust2_mombasa/shaders/desert_sand.shader',sand)

        sky=read('scenarios/skies/multi/halo/coagulation/coagulation.sky')
        light=sky['Data']['lights'][0]
        matrix=json.loads((package/'lighting.json').read_text())['sun_direction_node']['matrix']
        towards_sun=[matrix[10],matrix[8],matrix[9]]
        light['direction vector']=towards_sun
        light['direction']=[math.degrees(math.atan2(towards_sun[1],towards_sun[0])),
            math.degrees(math.asin(towards_sun[2]/math.sqrt(sum(v*v for v in towards_sun))))]
        light['radiosity'][0]['color']={'R':1.,'G':.85,'B':.69}
        light['radiosity'][0]['power']=.5
        light['radiosity'][0]['diameter']=.8
        sky['Data']['outdoor ambient color']={'R':.48,'G':.52,'B':.60}
        sky['Data']['indoor ambient color']={'R':0.,'G':0.,'B':0.}
        sky['Data']['atmospheric fog'][0].update({'color':{'R':.72,'G':.62,'B':.45},
            'maximum density':.16,'start distance':100.,'opaque distance':700.})
        sky['Data']['flags'] |= 32 # Use Clear Color; original render-sky light query.
        sky['Data']['clear color']=copy.deepcopy(sky['Data']['atmospheric fog'][0]['color'])
        if style=='district':
            sky=read('scenarios/skies/multi/street_sweeper/street_sweeper.sky')
            sky['Data']['flags'] |= 32
            sky['Data']['clear color']=copy.deepcopy(sky['Data']['atmospheric fog'][0]['color'])
        lighting_path=package/'native-practical-lights.json'
        if lighting_path.exists():
            authored_lighting=json.loads(lighting_path.read_text())
            if 'sun' in authored_lighting:
                authored_sun=authored_lighting['sun'];direction=authored_sun['towards_sun']
                light=sky['Data']['lights'][0];sky['Data']['lights']=[light]
                light['direction vector']=direction
                light['direction']=[math.degrees(math.atan2(direction[1],direction[0])),math.degrees(math.asin(direction[2]))]
                light['radiosity'][0].update(color=dict(zip(('R','G','B'),authored_sun['color'])),
                    power=authored_sun['native_power'],diameter=authored_sun['diameter_degrees'],flags=15)
                sky['Data']['global sky rotation']=0.
                world=authored_lighting['world'];assert not world['color_linked']
                ambient={k:c*world['strength'] for k,c in zip(('R','G','B'),world['color'][:3])}
                sky['Data']['outdoor ambient color']=ambient
                sky['Data']['clear color']=copy.deepcopy(ambient)
                result['blender_sun']=authored_sun
                result['blender_world']=dict(source=world,native_outdoor_ambient=ambient,
                    scope='Constant source world converted to native outdoor ambient; native indoor occlusion and GI remain engine-specific')
        presentation=package/'native-sky-presentation.json'
        if presentation.exists():
            authored_presentation=json.loads(presentation.read_text())
            clear=authored_presentation['clear_color']
            assert len(clear)==3 and all(math.isfinite(v) and 0<=v<=1 for v in clear)
            sky['Data']['clear color']=dict(zip(('R','G','B'),clear))
            result['authored_sky_presentation']=authored_presentation
        result['sky']=write('scenarios/multi/dust2_mombasa/desert.sky',sky)
        if presentation.exists():
            saved=read('scenarios/multi/dust2_mombasa/desert.sky')['Data']['clear color']
            assert all(abs(saved[k]-v)<1e-6 for k,v in zip(('R','G','B'),clear))
            result['authored_sky_clear_color_readback']=saved

        if not materials_only:
            scenario=read('scenarios/multi/dust2_mombasa/dust2_mombasa.scenario')
            data=scenario['Data']
            assert data['type']['value']==1
            sky_ref=data['skies'][0]['sky']
            sky_ref['path']='scenarios/multi/dust2_mombasa/desert';sky_ref['length']=len(sky_ref['path'])
            template=read('scenarios/multi/example_pi/example_pi.scenario')['Data']['player starting locations'][0]
            data['player starting locations']=[]
            for entity in json.loads((package/'source_entities.json').read_text()):
                kind=entity['classname']
                if kind not in ('info_player_terrorist','info_player_counterterrorist'):continue
                start=copy.deepcopy(template)
                start['position']=entity['halo_world_position']
                angles=[float(v) for v in re.findall(r'[-+]?\d*\.?\d+(?:[eE][-+]?\d+)?',entity['angles'])]
                start['facing']=angles[1]
                start['team designator']['value']=0 if kind=='info_player_terrorist' else 1
                start['type 0']['value']=12 # all games, from the kit's template
                data['player starting locations'].append(start)
            assert len(data['player starting locations'])==30
            level=copy.deepcopy(read('scenarios/multi/halo/coagulation/coagulation.scenario')['Data']['level data'])
            entry=level[0]['multiplayer level data'][0]
            entry.update({'map id':20261021,'path':'scenarios/multi/dust2_mombasa/dust2_mombasa','sort order':100})
            title='Deserted'
            description='Dust II layout, Halo 2 District art, retail palms and a permanent Warthog wreck.' if style=='district' else 'CS2 geometry with New Mombasa materials and desert terrain.'
            for key,text,width in [('Skip',title,32),('Skip_1',description,128)]:
                entry[key]=base64.b64encode(text.encode('utf-16le')[:width*2-2].ljust(width*2,b'\0')*9).decode()
            data['level data']=level
            practicals=package/'native-practical-lights.json'
            if practicals.exists():
                authored=json.loads(practicals.read_text())
                assert authored['schema']=='H2_DESERTED_PRACTICAL_LIGHTS_V1'
                assert authored['source_glb_sha256']==sha(package/'dust2_mombasa.glb')
                source=read('scenarios/solo/03a_oldmombasa/03a_oldmombasa.scenario')['Data']
                template=source['light volumes'][0]
                base_light=read('scenarios/objects/lights/solo/earthcity/ec_frustum_dim_yellow.light')
                data['light volumes']=[];data['light volumes palette']=[]
                result['practical_lights']=[]
                for i,row in enumerate(authored['lights']):
                    relative=f'scenarios/multi/dust2_mombasa/lights/practical_{i:02d}'
                    light=copy.deepcopy(base_light);ld=light['Data']
                    ld['default lightmap setting']['value']=2 # lightmaps only; no added dynamic lights
                    color=dict(zip(('R','G','B'),row['color']))
                    ld['diffuse lower bound']=copy.deepcopy(color);ld['diffuse upper bound']=color
                    strength=row.get('native_strength')
                    if strength is None:
                        strength=authored['native_strength_at_reference']*row['intensity_candela']/authored['native_strength_reference_candela']
                    assert 0<strength<=2
                    ld['brightness bounds']={'Min':strength,'Max':strength}
                    fov=row.get('native_field_of_view_degrees')
                    if fov is None:fov=math.degrees(2*row['outer_cone_radians'])
                    width=row.get('native_near_width_wu',authored['near_width_wu'])
                    height=row.get('native_height_scale',1.)
                    ld.update({'near width':width,'height stretch':height,
                        'field of view':fov,'falloff distance':authored['falloff_wu'],'cutoff distance':authored['cutoff_wu']})
                    pin=write(relative+'.light',light)
                    palette=copy.deepcopy(source['light volumes palette'][0])
                    palette['name'].update(path=relative,length=len(relative))
                    data['light volumes palette'].append(palette)
                    instance=copy.deepcopy(template)
                    direction=row['direction'];position=row['position_wu']
                    # Stock EarthCity zero-rotation custom frusta target +Z;
                    # a positive roll tilts that target toward -Y. Convert
                    # glTF's -Z direction to that native +Z convention.
                    pitch=math.atan2(direction[0],direction[2])
                    roll=-math.asin(max(-1.,min(1.,direction[1])))
                    reconstructed=[math.sin(pitch)*math.cos(roll),-math.sin(roll),math.cos(pitch)*math.cos(roll)]
                    assert max(abs(a-b) for a,b in zip(direction,reconstructed))<1e-6
                    instance.update({'type':i,'name':-1,'position':position,
                        'rotation':[0.,math.degrees(pitch),math.degrees(roll)],'scale':1.,
                        'placement flags':0,'unique id':0x44000000+i,'origin bsp index':0,
                        'WordFlags':0,'manual bsp flags':0,'flags':0,'flags_1':1,
                        'lightmap flags':0,'lightmap half life':0.,'lightmap light scale':0.,
                        'target point':[p+d*authored['cutoff_wu'] for p,d in zip(position,direction)],
                        'width':width,'height scale':height,'field of view':fov,
                        'falloff distance':authored['falloff_wu'],'cutoff distance':authored['cutoff_wu']})
                    instance['lightmap type']['value']=3 # lightmaps only
                    instance['type_2']['value']=2 # projective frustum
                    data['light volumes'].append(instance)
                    result['practical_lights'].append(dict(name=row['name'],tag=pin,position_wu=position,
                        direction=direction,brightness=strength,field_of_view=fov,mode='lightmaps only',
                        source_type=row.get('source_type','SPOT'),near_width_wu=width,height_scale=height,
                        inner_cone_note='Native stock circular soft-fade gel; glTF inner cone is not exactly represented'))
            result['scenario']=write('scenarios/multi/dust2_mombasa/dust2_mombasa.scenario',scenario)
            if practicals.exists():
                back=read('scenarios/multi/dust2_mombasa/dust2_mombasa.scenario')['Data']
                assert len(back['light volumes'])==len(authored['lights'])
                for original,reloaded in zip(data['light volumes'],back['light volumes']):
                    for key in ('position','rotation','target point'):
                        assert max(abs(a-b) for a,b in zip(original[key],reloaded[key]))<1e-5,(key,original[key],reloaded[key])
                    assert reloaded['lightmap type']['value']==3
                result['practical_light_readback_passed']=True
            result['spawn_count']=30
        if unlimited:
            path='multiplayer/game_variant_settings/multiplayer_game_settings.multiplayer_variant_settings_interface_definition'
            variants=read(path)
            slayer=next(v for v in variants['Data']['default variants'] if v['variant name']=='slayer')
            slayer['settings']=[v for v in slayer['settings'] if v['setting category']['value']!=10]
            slayer['settings'].append({'setting category':{'type':'LongEnum','value':10,'value name':''},'value':0})
            result['unlimited_slayer']=write(path,variants)
            result['time_limit_seconds']=0
    save_json(report,dict(schema='H2_DUST2_SCENARIO_AUTHORING_V1',**result))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kit-root',required=True,type=Path)
    parser.add_argument('--package',required=True,type=Path)
    parser.add_argument('--tag-interface',required=True,type=Path)
    parser.add_argument('--report-output',required=True,type=Path)
    parser.add_argument('--unlimited-time',action='store_true')
    parser.add_argument('--materials-only',action='store_true')
    parser.add_argument('--post-bake-presentation-only',action='store_true')
    parser.add_argument('--style',choices=('new-mombasa','district'),default='new-mombasa')
    a=parser.parse_args()
    configure(a.kit_root,a.package,a.tag_interface,a.report_output,a.unlimited_time,a.materials_only,a.style,a.post_bake_presentation_only)


if __name__=='__main__':main()
