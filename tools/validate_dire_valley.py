#!/usr/bin/env python3
"""Check the compiled Dire Valley cache against its extracted CE placements.

This is asset/metadata validation. Actual movement, rendering and teleporter
activation require separate native-runtime checks.
"""
import argparse
import json
import math
from pathlib import Path
import struct

import numpy as np

from mcc_content import MccCache
from port_dire_valley import save, sha
from prepare_dust2_h2ek import source_meshes


def validate(cache,package,report):
    c=MccCache(cache);scenario=c.tags[c.u32(c.index+12)];s=scenario['offset']
    assert scenario['name']=='scenarios\\multi\\dire_valley\\dire_valley'
    source=json.loads((package/'source_placements.json').read_text())
    starts=c.block(s+0x100,0x34)
    assert len(starts)==len(source['starts'])==103
    error=0.
    for at,row in zip(starts,source['starts']):
        xyz=struct.unpack_from('<3f',c.data,at);yaw=struct.unpack_from('<f',c.data,at+12)[0]
        error=max(error,max(abs(a-b) for a,b in zip(xyz,row['position'])))
        assert abs(yaw-row['facing'])<2e-6
        assert struct.unpack_from('<h',c.data,at+16)[0]==row['team']
        assert list(struct.unpack_from('<4h',c.data,at+20))==row['types']
    assert error<2e-5
    portals=[]
    for at in c.block(s+0x118,32):
        kind,selector,channel=struct.unpack_from('<3h',c.data,at+16)
        if kind not in (7,8):continue
        xyz=struct.unpack_from('<3f',c.data,at)
        original=next(v for v in source['netgame_flags'] if v['type']==kind-1)
        assert xyz==tuple(original[k] for k in ('x','y','z'))
        assert channel==original['identifier']==0 and selector==-1
        portals.append(dict(type=kind,channel=channel,position=xyz))
    assert len(portals)==2
    lighting_authoring=package/'lighting_authoring.json'
    lighting_policy=json.loads(lighting_authoring.read_text()) if lighting_authoring.exists() else None
    native_trees=lighting_policy['native_tree_scenery'] if lighting_policy else []
    native_props=lighting_policy.get('native_prop_scenery',[]) if lighting_policy else []
    scenery=c.block(s+0x50,0x5c)
    assert len(scenery)==2+len(native_trees)+len(native_props)
    authoring=json.loads((package/'scenario_authoring.json').read_text())
    fitted=authoring['sender_fit']
    sender=c.block(s+0x50,0x5c)[0]
    assert max(abs(a-b) for a,b in zip(struct.unpack_from('<3f',c.data,sender+8),fitted['position']))<2e-6
    assert abs(struct.unpack_from('<f',c.data,sender+32)[0]-fitted['scale'])<2e-6
    lo,hi=np.asarray(fitted['h2_model_bounds'])*fitted['scale']+np.asarray(fitted['position'])
    opening_lo,opening_hi=np.asarray(fitted['opening_bounds_world'])
    assert lo[0]>=opening_lo[0]-2e-6 and hi[0]<=opening_hi[0]+2e-6
    assert lo[2]>=opening_lo[2]-2e-6 and hi[2]<=opening_hi[2]+2e-6
    assert hi[1]<=opening_lo[1]+2e-6
    palette=c.block(s+0x58,40)
    for at,tree in zip(scenery[2:],native_trees+native_props):
        definition=c.tags[c.u32(palette[c.u16(at)]+4)]
        assert definition['group']=='scen' and definition['name']==tree['target'].replace('/','\\')
        assert max(abs(a-b) for a,b in zip(struct.unpack_from('<3f',c.data,at+8),tree['source']['position']))<2e-6
        assert max(abs(a-b) for a,b in zip(struct.unpack_from('<3f',c.data,at+20),tree['source']['rotation']))<2e-6
        assert abs(struct.unpack_from('<f',c.data,at+32)[0]-tree['scale'])<2e-6
    scenery_report=json.loads((package/'scenery_geometry.json').read_text())
    assert scenery_report['schema']=='H2_DIRE_VALLEY_NATIVE_SCENERY_V2'
    assert scenery_report['ce_scenery_geometry_retained'] is False
    assert scenery_report['native_tree_placements']==33 and scenery_report['native_rock_placements']==61
    assert len(scenery_report['definitions'])==13 and scenery_report['placements']==163
    expected_equipment=len(source['equipment'])+len(source['vehicles'])-len(authoring['omitted_equipment'])
    assert len(c.block(s+0x120,0x90))==expected_equipment==77
    assert any(t['group']=='scen' and t['name']=='objects\\multi\\2way_teleporter\\2way_teleporter' for t in c.tags.values())
    forbidden=[t['name'] for t in c.tags.values() if any(x in t['name'].lower() for x in ['dangercanyon','teleporter_shield'])]
    assert not forbidden,forbidden
    refs=c.block(s+0x210,68);assert len(refs)==1
    bsp=c.tags[c.u32(refs[0]+20)];ltmp=c.tags[c.u32(refs[0]+28)]
    assert bsp['group']=='sbsp' and ltmp['group']=='ltmp' and ltmp['address']
    instances=c.block(bsp['offset']+320,88)
    assert len(instances)==(lighting_policy['bsp_prop_instances'] if lighting_policy else 164)
    # Observe the compiler's real transform records, independent of ASS order.
    # The only unmatched instance must be the unchanged world render stream.
    stream=source_meshes(package/'data/scenarios/multi/dire_valley/structure/dire_valley.ass')
    ass_materials=next(stream);prototype_bounds={}
    for index,(records,faces) in enumerate(stream):
        if not len(faces):continue
        vertices=np.asarray([[float(x) for x in row.splitlines()[0].split()] for row in records])/100
        # This bound encloses the render mesh and its collision mesh. The
        # shield's native collision extends beyond its visible energy sheet.
        points=vertices[np.unique(faces[:,1:])]
        prototype_bounds[index]=(points.min(0),points.max(0))
    definitions=c.block(bsp['offset']+312,200)
    available=list(instances);max_instance_error=0.;max_prototype_error=0.
    for item in scenery_report['placement_transforms']:
        if native_trees and '/trees/' in item['target']:continue
        if item in native_props:continue
        yaw,pitch,roll=item['source']['rotation']
        cy,sy=math.cos(yaw),math.sin(yaw);cp,sp=math.cos(pitch),math.sin(pitch);cr,sr=math.cos(roll),math.sin(roll)
        rx=np.array([[1,0,0],[0,cr,-sr],[0,sr,cr]])
        ry=np.array([[cp,0,-sp],[0,1,0],[sp,0,cp]])
        rz=np.array([[cy,-sy,0],[sy,cy,0],[0,0,1]])
        orientation=rx@ry@rz
        position=np.asarray(item['source']['position'])+orientation@np.asarray(item['offset'])
        # Tool applies each ASS scale to the shared prototype's vertices,
        # leaving an orthonormal placement. Check both observations.
        expected=np.r_[orientation.T.reshape(-1),position]
        def affine(at):
            values=np.asarray(struct.unpack_from('<13f',c.data,at))
            return np.r_[values[0]*values[1:10],values[10:13]]
        best=min(available,key=lambda at:np.max(np.abs(affine(at)-expected)))
        delta=float(np.max(np.abs(affine(best)-expected)))
        assert delta<3e-5,(item['target'],delta)
        max_instance_error=max(max_instance_error,delta);available.remove(best)
        lo,hi=prototype_bounds[item['mesh']]
        expected_bounds=np.r_[(lo+hi)*item['scale']/2,np.linalg.norm(hi-lo)*item['scale']/2]
        actual_bounds=np.asarray(struct.unpack_from('<4f',c.data,definitions[c.u16(best+52)]+96))
        bounds_delta=float(np.max(np.abs(actual_bounds-expected_bounds)))
        bounds_tolerance=3e-5 # Float bounds reduction; observed error is retained below.
        assert bounds_delta<bounds_tolerance,(item['target'],'compiled prototype bounds',bounds_delta,bounds_tolerance)
        max_prototype_error=max(max_prototype_error,bounds_delta)
    assert len(available)==(0 if lighting_policy else 1)
    if available:
        assert np.max(np.abs(np.asarray(struct.unpack_from('<13f',c.data,available[0]))-np.r_[1.,np.eye(3).reshape(-1),[0,0,0]]))<2e-6
    groups=c.block(ltmp['offset']+128,104);assert len(groups)==1
    assert c.u32(groups[0]+4)==c.u32(bsp['offset']+8)
    result=dict(schema='H2_DIRE_VALLEY_COMPILED_VALIDATION_V1',passed=True,cache_sha256=sha(cache),
        starts=len(starts),max_start_position_error=error,teleporters=portals,
        halo2_teleporter_scenery=2,ce_teleporter_tags=forbidden,netgame_equipment=expected_equipment,
        halo2_sender_fits_original_opening=True,
        halo2_tree_placements=33,ce_tree_geometry_retained=False,
        native_tree_scenery=len(native_trees),world_render_in_primary_bsp=bool(lighting_policy),
        native_prop_scenery=len(native_props),
        omitted_health_pack_spawns=len(authoring['omitted_equipment']),
        source_static_scenery_placements=163,native_halo2_scenery_placements=163,compiled_bsp_instances=len(instances),
        max_native_scenery_transform_error=max_instance_error,
        max_native_prototype_bounds_error=max_prototype_error,
        lighting_tag=ltmp['name'],lighting_groups=len(groups),runtime_verified=False)
    save(report,result);print(json.dumps(result))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ['cache','package','report']:p.add_argument('--'+flag,required=True,type=Path)
    validate(**vars(p.parse_args()))
