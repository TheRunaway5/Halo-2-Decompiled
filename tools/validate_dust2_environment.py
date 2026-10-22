#!/usr/bin/env python3
"""Validate additive Dust II scenery against the previous model package.

Run through tools/run_test.py. Inputs are read-only; the only retained output
is the explicitly requested small validation report in the model package.
"""
import argparse
import copy
import hashlib
import json
import mmap
import re
from pathlib import Path

import numpy as np

from build_dust2_mombasa import Glb, ass_shader_name, save_json, sha


def ass_records(path):
    """Canonical text permits only the verified delimiter/reference fix."""
    digest=hashlib.sha256()
    with path.open() as f:
        for line in f:
            match=re.fullmatch(r'"(scenarios\\.*?)([!@%]*)"\n',line)
            if match:
                line='"'+ass_shader_name(match[1])+match[2]+'"\n'
            digest.update(line.replace(' ','\t').encode())
    return digest.hexdigest()


def body_hash(path, end_marker):
    with path.open('rb') as f, mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as raw:
        start=raw.find(b';OBJECT 0\n')
        end=raw.find(end_marker,start)
        if not 0<=start<end:raise ValueError('Missing ASS object boundary')
        # The Classic Tool correction changes vector delimiters from spaces
        # to tabs. Compare the exact numeric text after delimiter normalization.
        return hashlib.sha256(raw[start:end].replace(b' ', b'\t')).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline',required=True,type=Path)
    parser.add_argument('--package',required=True,type=Path)
    args=parser.parse_args()
    old=Glb(args.baseline/'dust2_mombasa.glb')
    new=Glb(args.package/'dust2_mombasa.glb')
    assert old.raw[old.bin_at:old.bin_at+old.bin_size]==new.raw[new.bin_at:new.bin_at+old.bin_size]
    expected=copy.deepcopy(old.doc['meshes'])
    removed_tints=0
    for mesh in expected:
        for prim in mesh['primitives']:
            if prim['attributes'].pop('COLOR_0',None) is not None:removed_tints+=1
    assert expected==new.doc['meshes'][:len(expected)]
    assert old.doc['nodes']==new.doc['nodes'][:len(old.doc['nodes'])]
    assert old.doc['materials']==new.doc['materials'][:len(old.doc['materials'])]
    original=json.loads((args.baseline/'provenance.json').read_text())
    objects=original['render']['objects']
    relative='data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass'
    before=body_hash(args.baseline/relative,b';### INSTANCES ###')
    after=body_hash(args.package/relative,f';OBJECT {objects}\n'.encode())
    assert before==after,'Original ASS numeric records changed'
    preserved=['dust2_collision.glb','source_entities.json']
    hashes={name:sha(args.package/name) for name in preserved}
    assert all(hashes[name]==sha(args.baseline/name) for name in preserved)
    for name in ['dust2_collision.ass','dust2_source_helpers.ass']:
        assert ass_records(args.baseline/name)==ass_records(args.package/name),name
    backdrop=Glb(args.package/'desert_backdrop.glb')
    env=json.loads((args.package/'desert_environment.json').read_text())
    lower,upper=np.asarray(env['profile']['protected_collision_bounds_meters'])
    triangles=0;vertices=0;minimum_area=np.inf
    for _,node,mesh,world in backdrop.nodes():
        assert node['extras']['collision'] is False
        assert np.allclose(world,np.eye(4))
        for prim in mesh['primitives']:
            p=backdrop.array(prim['attributes']['POSITION'])
            n=backdrop.array(prim['attributes']['NORMAL'])
            ids=backdrop.array(prim['indices'])[:,0].reshape(-1,3)
            assert np.isfinite(p).all() and np.isfinite(n).all()
            assert ids.min()>=0 and ids.max()<len(p)
            assert np.max(np.abs(np.linalg.norm(n,axis=1)-1))<1e-5
            tri=p[ids]
            areas=np.linalg.norm(np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]),axis=1)/2
            assert (areas>1e-6).all()
            minimum_area=min(minimum_area,float(areas.min()))
            # Bounding boxes are conservative: any triangle which can overlap
            # the protected XY footprint must lie entirely beneath its floor.
            lo=tri.min(axis=1);hi=tri.max(axis=1)
            overlap=(hi[:,0]>=lower[0])&(lo[:,0]<=upper[0])&(hi[:,2]>=lower[2])&(lo[:,2]<=upper[2])
            assert (hi[overlap,1] <= lower[1]-3.9).all(),'Backdrop intrudes into map bounds'
            triangles+=len(ids);vertices+=len(p)
    assert triangles==sum(r['triangles'] for r in env['geometry'])
    assert triangles==42160
    lighting=json.loads((args.package/'lighting_validation.json').read_text())
    assert lighting['raytraced_sun_shadows'] and lighting['indirect_sky_bounce']
    assert not lighting['halo2_runtime_lightmaps_baked']
    report=dict(schema='H2_DUST2_ENVIRONMENT_VALIDATION_V1',passed=True,
        baseline_model_sha256=sha(args.baseline/'dust2_mombasa.glb'),
        source_geometry_buffer_byte_identical=True,source_geometry_attributes_unchanged=True,
        original_ass_numeric_records_unchanged=True,original_ass_normalized_sha256=before,
        ass_vector_delimiters='tabs required by Classic Tool',
        ass_material_references='exact shader collection and leaf names',
        original_materials_unchanged=True,source_node_placements_unchanged=True,
        inherited_cs2_appearance_tints_unbound=removed_tints,preserved_file_sha256=hashes,
        backdrop_triangles=triangles,backdrop_vertices=vertices,minimum_triangle_area_m2=minimum_area,
        backdrop_intrudes_into_playable_bounds=False,backdrop_has_collision=False,
        lighting_scene_present=(args.package/'dust2_mombasa_lighting.blend').is_file(),
        halo2_bsp_compiled=False,halo2_runtime_tested=False)
    save_json(args.package/'environment_validation.json',report)
    print(json.dumps(report,indent=2),flush=True)


if __name__=='__main__':main()
