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


def configure(kit,package,interface,report,unlimited,materials_only,style='new-mombasa'):
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
        result['sky']=write('scenarios/multi/dust2_mombasa/desert.sky',sky)

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
            result['scenario']=write('scenarios/multi/dust2_mombasa/dust2_mombasa.scenario',scenario)
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
    parser.add_argument('--style',choices=('new-mombasa','district'),default='new-mombasa')
    a=parser.parse_args()
    configure(a.kit_root,a.package,a.tag_interface,a.report_output,a.unlimited_time,a.materials_only,a.style)


if __name__=='__main__':main()
