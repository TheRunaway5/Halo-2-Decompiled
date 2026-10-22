#!/usr/bin/env python3
"""Author Deserted collision from its new solids and the real retail props.

No Source collision triangles are carried into this map. Run in run_test.py;
the model package must be inside its active HALO2_TEST_SCRATCH owner.
"""
import argparse
import json
import os
from pathlib import Path
import numpy as np
from build_dust2_mombasa import Glb, write_ass, save_json
from refine_dust2_environment import SceneWriter


def author(package, props):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert package.resolve().is_relative_to(scratch)
    palette=json.loads((package/'material_mapping.json').read_text())['palette']
    materials={k:palette[k] for k in ('concrete','metal','bark')}
    writer=SceneWriter()
    writer.doc['materials']=[dict(name=r['shader']) for r in materials.values()]
    counts={}

    def emit(name,p,t,material):
        p=p[t.reshape(-1)]
        t=np.arange(len(p),dtype='<u4').reshape(-1,3)
        normal=np.cross(p[t[:,1]]-p[t[:,0]],p[t[:,2]]-p[t[:,0]])
        size=np.linalg.norm(normal,axis=1)
        assert (size>1e-12).all(),name
        normal=np.repeat(normal/size[:,None],3,axis=0)
        primitive=dict(attributes=dict(POSITION=writer.array(p[:,[1,2,0]]*3.048,'VEC3'),
            NORMAL=writer.array(normal[:,[1,2,0]],'VEC3'),TEXCOORD_0=writer.array(np.zeros((len(p),2)),'VEC2')),
            indices=writer.array(t.reshape(-1),'SCALAR'),material=material)
        writer.doc['meshes'].append(dict(name=name,primitives=[primitive]))
        writer.doc['nodes'].append(dict(name=name,mesh=len(writer.doc['meshes'])-1))
        writer.doc['scenes'][0]['nodes'].append(len(writer.doc['nodes'])-1)
        counts[name]=len(t)

    solid=np.load(package/'architecture-solid.npz')
    # The architecture itself now collides through its rendered, lightmapped
    # faces. A collision-only copy has no render-strip references for the
    # original object-lighting query and would leave the player/weapon black.
    placements=json.loads((package/'redesign.json').read_text())['placements']
    prop=next(r for r in json.loads((props/'props.json').read_text())['models'] if r['name']=='warthog_destroyed')
    raw=np.fromfile(props/prop['geometry'],dtype=np.dtype([('p','<f4',3),('rest','<f4',5),('m','<u4')]))
    hull=np.array(prop['collision_triangles']).reshape(-1,3)
    hull[:,2]-=raw['p'][:,2].min()
    hog=next(r for r in placements if r['kind']=='warthog')
    angle=hog['yaw_radians'];c,s=np.cos(angle),np.sin(angle)
    hull=hull@np.array([[c,-s,0],[s,c,0],[0,0,1]]).T+hog['position']
    emit('permanent_retail_warthog_hull',hull,np.arange(len(hull)).reshape(-1,3),1)
    render=Glb(package/'dust2_mombasa.glb')
    keys=list(palette)
    for _,node,mesh,world in render.nodes():
        if not node['name'].startswith('palmtree_'):
            continue
        for prim in mesh['primitives']:
            if 'bark' not in palette[keys[prim['material']]]['shader']:
                continue
            p=render.array(prim['attributes']['POSITION']).astype(float)
            p=(p@world[:3,:3].T+world[:3,3])[:,[2,0,1]]/3.048
            emit(node['name']+'_retail_trunk',p,render.array(prim['indices'])[:,0].reshape(-1,3),2)
    writer.write(package/'dust2_collision.glb',generator='Deserted authored collision')
    write_ass(Glb(package/'dust2_collision.glb'),package/'dust2_collision.ass',materials,
        [dict(palette=k) for k in materials],collision=True)
    save_json(package/'collision-authoring.json',dict(schema='H2_DESERTED_COLLISION_V1',
        source_collision_triangles_reused=0,render_solids_match_collision=True,
        architecture_shares_render_and_collision=True,shared_architecture_triangles=len(solid['triangles']),
        warthog_is_permanent_bsp=True,groups=counts,extra_collision_triangles=sum(counts.values()),
        triangles=len(solid['triangles'])+sum(counts.values())))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package',type=Path,required=True)
    p.add_argument('--props',type=Path,required=True)
    author(**vars(p.parse_args()))
