#!/usr/bin/env python3
"""Author Dire Valley's Halo 2 scenario from extracted CE placements.

Use a private H2EK overlay and run through tools/run_test.py. Angles from the
cache are radians; Halo-Tag-Interface's editable dictionaries use degrees.
"""
import argparse
import base64
import copy
import json
import math
import os
from pathlib import Path
import sys
import tempfile

import numpy as np

from port_dire_valley import save, sha


def configure(kit, package, interface):
    sys.path.insert(0,str(interface.resolve()))
    import tag_interface as ti
    g,engine,defs,groups,ext,post,_=ti.get_version_args('halo2',0)
    with tempfile.TemporaryDirectory(dir=os.environ['HALO2_TEST_SCRATCH']) as temp:
        merged=g.generate_defs(defs,temp,groups,ext)
        tags=kit/'tags'
        def read(rel):return ti.read_file(merged,str(tags),str(tags/rel),0,engine_tag=engine)
        def write(rel,value):
            dest=tags/rel;dest.parent.mkdir(parents=True,exist_ok=True)
            with tempfile.TemporaryDirectory(dir=dest.parent) as owned:
                tmp=Path(owned)/dest.name
                ti.write_file(merged,value,ti.obfuscation_buffer_prepare(),str(tmp),engine_tag=engine,
                    tag_groups=groups,tag_extensions=ext,postprocess_functions=post)
                tmp.replace(dest)
        def ref(original,path,group=None):
            result=copy.deepcopy(original)
            result.update(path=path,length=len(path))
            if group:result['group name']=group
            return result
        source=json.loads((package/'source_placements.json').read_text())
        scenario=read('scenarios/multi/dire_valley/dire_valley.scenario')
        data=scenario['Data']
        coag=read('scenarios/multi/halo/coagulation/coagulation.scenario')['Data']
        sky=read('scenarios/skies/multi/halo/coagulation/coagulation.sky')
        # Authored native-map flag: keep Coagulation's exact ambient/sun and
        # visible sky model, and admit the original render-sky light query for
        # dynamic receivers. This does not change the stock sky tag.
        sky['Data']['flags'] |= 32
        write('scenarios/multi/dire_valley/valley.sky',sky)
        data['skies'][0]['sky']=ref(data['skies'][0]['sky'],'scenarios/multi/dire_valley/valley','sky ')
        data['player starting locations']=[]
        for row in source['starts']:
            v=copy.deepcopy(coag['player starting locations'][0])
            v.update(position=row['position'],facing=math.degrees(row['facing']))
            v['team designator']['value']=row['team']
            for i,kind in enumerate(row['types']):v[f'type {i}']['value']=kind
            data['player starting locations'].append(v)
        data['netgame flags']=[]
        for row in source['netgame_flags']:
            kind=row['type'];team=row['identifier']
            if kind==0:types=[0,1,2,3] # Matching CTF and Assault endpoints.
            elif kind==2:types=[4]
            elif kind==8:types=[11+row['identifier']]
            elif kind in (6,7):types=[kind+1];team=-1 # Original wildcard selector; identifier owns link.
            else:continue # CE race/vehicle hints have no Halo 2 objective.
            for kind2 in types:
                v=copy.deepcopy(coag['netgame flags'][0])
                v.update(position=[row[k] for k in ('x','y','z')],facing=math.degrees(row['facing']),
                    identifier=row['identifier'],flags=3 if kind==0 else 0)
                v['type']['value']=kind2;v['team designator']['value']=team
                data['netgame flags'].append(v)

        data['scenery']=[];data['scenery palette']=[]
        art=[]
        def scenery(path,position,rotation,variant='',scale=1.,source_name='',prototype=None,unique_id=None):
            if not (tags/(path+'.scenery')).is_file():raise ValueError('Missing scenery '+path)
            existing=[x['name']['path'] for x in data['scenery palette']]
            if path not in existing:
                data['scenery palette'].append(dict(name=ref(coag['scenery palette'][0]['name'],path,'scen')))
                existing.append(path)
            v=copy.deepcopy(coag['scenery'][0] if prototype is None else prototype);v.update(type=existing.index(path),name=-1,position=position,
                rotation=rotation,scale=scale,**{'unique id':0x50000000+len(data['scenery']) if unique_id is None else unique_id,
                    'variant name':variant,'variant name_pad':0,'valid multiplayer games':0})
            data['scenery'].append(v)
            art.append(dict(source=source_name,target=path,variant=variant,position=position,scale=scale))
        if not (package/'scenery_geometry.json').is_file():
            raise ValueError('Import the original static scenery geometry before authoring the scenario')
        # Original channel and destinations are exact; real H2 base/receiver
        # variants carry their stock plasma programs, markers and animation.
        geometry=np.load(package/'source_geometry.npz')
        corners=geometry['vertices'][geometry['triangles'][:,1:],:3]
        def floor_below(row):
            xy=np.array([row['x'],row['y']]);p=corners
            ab=p[:,1,:2]-p[:,0,:2];ac=p[:,2,:2]-p[:,0,:2];q=xy-p[:,0,:2]
            det=ab[:,0]*ac[:,1]-ab[:,1]*ac[:,0];valid=abs(det)>1e-12
            u=np.zeros(len(p));v=np.zeros(len(p))
            u[valid]=(q[valid,0]*ac[valid,1]-q[valid,1]*ac[valid,0])/det[valid]
            v[valid]=(ab[valid,0]*q[valid,1]-ab[valid,1]*q[valid,0])/det[valid]
            z=p[:,0,2]+u*(p[:,1,2]-p[:,0,2])+v*(p[:,2,2]-p[:,0,2])
            valid &= (u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7)&(z<=row['z']+.01)
            if not valid.any():raise ValueError('Teleporter has no original floor underneath')
            return float(z[valid].max())
        # Fit the stock sender into the measured CE opening. Its complete H2
        # model is taller/deeper than the CE shield; a floor-only placement
        # would bury its emitters in the existing wall. Keep the logical link.
        provenance=json.loads((package/'provenance.json').read_text())
        opening=provenance['removed_ce_teleporter_surfaces']
        if len(opening)!=1:raise ValueError('Expected the original single CE portal opening')
        lo,hi=np.asarray(opening[0]['bounds_world'])
        bounds=read('objects/multi/2way_teleporter/2way_teleporter.render_model')['Data']['compression info'][0]
        h2lo=np.array([bounds['position bounds '+a]['Min'] for a in 'xyz'])
        h2hi=np.array([bounds['position bounds '+a]['Max'] for a in 'xyz'])
        sender_scale=float(min(1.,(hi[0]-lo[0])/(h2hi[0]-h2lo[0]),(hi[2]-lo[2])/(h2hi[2]-h2lo[2])))
        sender_position=[float((lo[0]+hi[0])/2-(h2lo[0]+h2hi[0])*sender_scale/2),
            float(lo[1]-h2hi[1]*sender_scale),float(lo[2]-h2lo[2]*sender_scale)]
        for row in source['netgame_flags']:
            if row['type'] not in (6,7):continue
            if row['type']==6 and row['facing']!=0.:raise ValueError('Sender fit requires its original axis-aligned opening')
            scenery('objects/multi/2way_teleporter/2way_teleporter',
                sender_position if row['type']==6 else [row['x'],row['y'],floor_below(row)],
                [math.degrees(row['facing']),0.,0.],variant='base' if row['type']==6 else 'receiver',
                scale=sender_scale if row['type']==6 else 1.,
                source_name='CE teleport '+('from' if row['type']==6 else 'to'))

        lighting_authoring=package/'lighting_authoring.json'
        if lighting_authoring.exists():
            lighting=json.loads(lighting_authoring.read_text())
            placements=json.loads((package/'scenery_geometry.json').read_text())['placement_transforms']
            for tree in lighting['native_tree_scenery']+lighting.get('native_prop_scenery',[]):
                index=placements.index(tree)
                row=tree['source']
                scenery(tree['target'],row['position'],[math.degrees(v) for v in row['rotation']],
                    scale=tree['scale'],source_name=row['source'],prototype=data['scenery'][0],
                    unique_id=0x51000000+index)

        weapons={'pistol':'magnum','assault rifle':'battle_rifle','plasma_cannon':'fuel_rod_gun',
            'flame thrower':'brute_shot','sniper rifle':'sniper_rifle','rocket launcher':'rocket_launcher',
            'frag grenades':'frag_grenades','plasma grenades':'plasma_grenades','plasma rifle':'plasma_rifle',
            'plasma pistol':'plasma_pistol','shotgun':'shotgun','needler':'needler'}
        powers={'powerup super shield':'powerup_super_shield','powerup health pack':'powerup_health_pack',
            'shield-invisibility':'powerup_invisibility'}
        data['netgame equipment']=[];pickups=[];omitted=[]
        def equipment(path,group,row,classification):
            extension='item_collection' if group=='itmc' else 'vehicle_collection'
            if not (tags/(path+'.'+extension)).is_file():raise ValueError('Missing collection '+path)
            v=copy.deepcopy(coag['netgame equipment'][0])
            v.update(position=row['position'],orientation=[math.degrees(row.get('facing',0)),0.,0.])
            v['item/vehicle collection']=ref(v['item/vehicle collection'],path,group)
            v['classification']['value']=classification
            for i,kind in enumerate(row.get('types',[12,0,0,0])):v[f'type {i}']['value']=kind
            v['spawn time (in seconds, 0 = default)']=row.get('spawn_seconds',0)
            data['netgame equipment'].append(v)
            pickups.append(dict(source=row['source'],target=path,position=row['position']))
        for row in source['equipment']:
            leaf=row['source'].rsplit('\\',1)[-1]
            if leaf=='powerup health pack':
                omitted.append(dict(row,reason='The unused H2 healthpack shader has unresolved retained native inputs; use normal H2 health regeneration'))
                continue
            power=leaf in powers
            name=powers[leaf] if power else weapons[leaf]
            equipment('multiplayer/'+('powerups/' if power else 'single_weapons/')+name,'itmc',row,
                9 if power else 8 if 'grenades' in name else 0)
        for row in source['vehicles']:
            old=row['source'];name=('warthog_gauss' if 'rwarthog' in old else 'warthog' if 'warthog' in old
                else 'ghost' if 'ghost' in old else 'scorpion')
            item=dict(row,facing=row['rotation'][0])
            equipment('multiplayer/single_vehicles/'+name,'vehc',item,3 if name=='scorpion' else 1)
        level=copy.deepcopy(coag['level data']);entry=level[0]['multiplayer level data'][0]
        entry.update({'map id':20261022,'path':'scenarios/multi/dire_valley/dire_valley','sort order':101})
        for key,text,width in [('Skip','Dire Valley',32),('Skip_1','Danger Canyon reborn with Halo 2 materials and teleporters.',128)]:
            entry[key]=base64.b64encode(text.encode('utf-16le')[:width*2-2].ljust(width*2,b'\0')*9).decode()
        data['level data']=level
        write('scenarios/multi/dire_valley/dire_valley.scenario',scenario)
        save(package/'scenario_authoring.json',dict(schema='H2_DIRE_VALLEY_SCENARIO_V1',
            starts=len(data['player starting locations']),objectives=len(data['netgame flags']),
            equipment=pickups,omitted_equipment=omitted,scenery=art,teleporter_channel=0,
            ce_teleporter_geometry_retained=False,halo2_teleporter_variants=['base','receiver'],
            sender_fit=dict(opening_bounds_world=[lo.tolist(),hi.tolist()],
                h2_model_bounds=[h2lo.tolist(),h2hi.tolist()],position=sender_position,scale=sender_scale),
            scenario_sha256=sha(tags/'scenarios/multi/dire_valley/dire_valley.scenario')))
        # Read after writing; no silent dropped or truncated scenario records.
        verified=read('scenarios/multi/dire_valley/dire_valley.scenario')['Data']
        for key in ['player starting locations','netgame flags','scenery','netgame equipment']:
            assert len(verified[key])==len(data[key]),key
        print(json.dumps(dict(starts=len(data['player starting locations']),scenery=len(data['scenery']),
            equipment=len(data['netgame equipment']),objectives=len(data['netgame flags']))))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ['kit','package','interface']:p.add_argument('--'+flag,required=True,type=Path)
    configure(**vars(p.parse_args()))
