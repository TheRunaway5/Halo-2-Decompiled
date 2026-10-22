#!/usr/bin/env python3
"""Replace the source car's metal collision with the actual destroyed Hog hull.

Only contained metal/vehicle/glass/rubber triangles are removed for the asset
swap. Optional bounded QEM reduces the remaining collision without component
pruning. All outputs must remain in the active tools/run_test.py scratch owner.
"""
import argparse
import json
import os
from pathlib import Path
import numpy as np
from build_dust2_mombasa import Glb,save_json,write_ass
from refine_dust2_environment import SceneWriter
from redesign_dust2_halo import Simplifier

def author(source,package,props,meshoptimizer=None):
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 assert package.resolve().is_relative_to(scratch)
 placements=[p for p in json.loads((package/'redesign.json').read_text())['placements'] if p['kind']=='warthog']
 assert len(placements)==1,'This authored replacement requires the single source car'
 render=Glb(source/'dust2_mombasa.glb');body=[]
 for _,node,mesh,world in render.nodes():
  if 'dust_car_wreck_color' not in mesh.get('name',''):continue
  for primitive in mesh['primitives']:
   p=render.array(primitive['attributes']['POSITION']).astype(float)@world[:3,:3].T+world[:3,3]
   body.append(p[:,[2,0,1]]/3.048)
 body=np.concatenate(body);low=body.min(axis=0)-.025;high=body.max(axis=0)+.025
 collision=Glb(source/'dust2_collision.glb');writer=SceneWriter()
 palette=json.loads((package/'material_mapping.json').read_text())['palette']
 materials={key:palette[key] for key in ('concrete','metal')}
 writer.doc['materials']=[dict(name=row['shader']) for row in materials.values()]
 report=[];simplifier=Simplifier(meshoptimizer) if meshoptimizer else None
 def emit(name,p,t,material):
  quantized=(p[:,[1,2,0]]*3.048).astype('<f4').astype(float)
  area=np.linalg.norm(np.cross(quantized[t[:,1]]-quantized[t[:,0]],quantized[t[:,2]]-quantized[t[:,0]]),axis=1)
  t=t[area>1e-12]
  if not len(t):return
  p=p[t.reshape(-1)];t=np.arange(len(p),dtype='<u4').reshape(-1,3)
  n=np.cross(p[t[:,1]]-p[t[:,0]],p[t[:,2]]-p[t[:,0]])
  n/=np.maximum(np.linalg.norm(n,axis=1,keepdims=True),1e-20);n=np.repeat(n,3,axis=0)
  primitive=dict(attributes=dict(POSITION=writer.array(p[:,[1,2,0]]*3.048,'VEC3'),
    NORMAL=writer.array(n[:,[1,2,0]],'VEC3'),TEXCOORD_0=writer.array(np.zeros((len(p),2)),'VEC2')),
    indices=writer.array(t.reshape(-1),'SCALAR'),material=material)
  writer.doc['meshes'].append(dict(name=name,primitives=[primitive]))
  writer.doc['nodes'].append(dict(name=name,mesh=len(writer.doc['meshes'])-1))
  writer.doc['scenes'][0]['nodes'].append(len(writer.doc['nodes'])-1)
 for _,node,mesh,world in collision.nodes():
  for primitive in mesh['primitives']:
   p=collision.array(primitive['attributes']['POSITION']).astype(float)@world[:3,:3].T+world[:3,3]
   p=p[:,[2,0,1]]/3.048;t=collision.array(primitive['indices'])[:,0].reshape(-1,3)
   name=node['name'];remove=np.zeros(len(t),dtype=bool)
   eligible=name.startswith('physics_group_') and any(word in name for word in ('metal','glass','rubber','plastic'))
   if eligible:remove=((p[t]>=low)&(p[t]<=high)).all(axis=(1,2))
   keep=t[~remove];error=0
   if simplifier and len(keep):p,keep,error=simplifier.simplify(p,keep,max(4,len(keep)//20),.005,prune=False)
   emit(name,p,keep,0)
   report.append(dict(group=name,triangles=len(t),removed=int(remove.sum()),authored_triangles=len(keep),qem_error_world_units=error))
 native=next(p for p in json.loads((props/'props.json').read_text())['models'] if p['name']=='warthog_destroyed')
 raw=np.fromfile(props/native['geometry'],dtype=np.dtype([('p','<f4',3),('rest','<f4',5),('material','<u4')]))
 hull=np.array(native['collision_triangles']);assert hull.shape[1:]==(3,3)
 for placement in placements:
  p=hull.reshape(-1,3).copy();p[:,2]-=raw['p'][:,2].min()
  angle=placement['yaw_radians'];c,s=np.cos(angle),np.sin(angle)
  p=p@np.array([[c,-s,0],[s,c,0],[0,0,1]]).T+placement['position']
  emit('static_destroyed_warthog_collision',p,np.arange(len(p)).reshape(-1,3),1)
 writer.write(package/'dust2_collision.glb')
 write_ass(Glb(package/'dust2_collision.glb'),package/'dust2_collision.ass',materials,
    [dict(palette=key) for key in materials],collision=True)
 save_json(package/'replacement_collision.json',dict(schema='H2_DUST2_WRECK_COLLISION_V1',
   source_car_bounds=[low.tolist(),high.tolist()],original_groups=report,removed=sum(r['removed'] for r in report),
   wreck_triangles=len(hull),wreck_source=native['tag'],permanent_bsp_geometry=True,
   road_wall_stair_clip_triangles_removed_for_asset_replacement=0,
   collision_simplification='bounded QEM without isolated-component pruning' if simplifier else 'none'))

if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 for name in ('source','package','props'):parser.add_argument('--'+name,type=Path,required=True)
 parser.add_argument('--meshoptimizer',type=Path)
 author(**vars(parser.parse_args()))
