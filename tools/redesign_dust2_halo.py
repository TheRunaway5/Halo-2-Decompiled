#!/usr/bin/env python3
"""Rebuild the Dust II art layer with District-era materials and retail props.

Run under tools/run_test.py. The immutable full-density package is the layout
reference. Meshoptimizer v1.3 performs bounded QEM reduction; replacement
positions come from the actual source meshes, not screenshots.
"""
import argparse
import ctypes as ct
import io
import json
import os
from pathlib import Path
import shutil

import numpy as np
from PIL import Image
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import connected_components
from scipy.spatial import cKDTree

from build_dust2_mombasa import Glb, dds, save_json, sha, write_ass
from mcc_content import MccCache
from refine_dust2_environment import SceneWriter

EARTH='scenarios\\shaders\\solo\\earthcity\\'
TURF='scenarios\\shaders\\multi\\turf\\'
ZANZIBAR='scenarios\\shaders\\multi\\zanzibar\\'
PALETTE={
 'wall':('03a_oldmombasa',EARTH+'base_stucco3'),
 'white':('03a_oldmombasa',EARTH+'base_stucco1_grim'),
 'stucco':('03a_oldmombasa',EARTH+'base_stucco4'),
 'floor':('turf',TURF+'turf_ground_unwrap_road'),
 'walkway':('03a_oldmombasa',EARTH+'base_sidewalk'),
 'stairs':('03a_oldmombasa',EARTH+'base_concrete1'),
 'concrete':('zanzibar',ZANZIBAR+'massive_concrete'),
 'brick':('turf',TURF+'brick_oldworld_02'),
 'stone':('03a_oldmombasa',EARTH+'base_stucco5_brick'),
 'blue':('03a_oldmombasa',EARTH+'base_metal'),
 'metal':('zanzibar',ZANZIBAR+'steel_weathered'),
 'trim':('zanzibar',ZANZIBAR+'panel_ripple_rusted'),
 'door':('03a_oldmombasa',EARTH+'base_stucco1_door'),
 'roof':('zanzibar',ZANZIBAR+'metal_roof'),
 'glass':('turf',TURF+'windows_storefront'),
 'light':('turf',TURF+'light_long_a'),
 'terrain':('zanzibar',ZANZIBAR+'zanzibar_ground_dirt'),
 'plant':('zanzibar','scenarios\\shaders\\nature\\trees\\palm_frond_a'),
 'bark':('zanzibar','scenarios\\shaders\\nature\\trees\\palmtree_bark_a'),
 'fence':('03a_oldmombasa',EARTH+'base_chainlink'),
 'desert_sand':('zanzibar',ZANZIBAR+'zanzibar_ground_beach'),
 'desert_rock':('zanzibar',ZANZIBAR+'zanzibar_cliff1'),
}


class Simplifier:
 def __init__(self,path):
  self.library=ct.CDLL(str(path.resolve()))
  self.call=self.library.meshopt_simplifyWithUpdate
  self.call.argtypes=[ct.c_void_p,ct.c_size_t,ct.c_void_p,ct.c_size_t,ct.c_size_t,
    ct.c_void_p,ct.c_size_t,ct.c_void_p,ct.c_size_t,ct.c_void_p,ct.c_size_t,
    ct.c_float,ct.c_uint,ct.c_void_p]
  self.call.restype=ct.c_size_t
 def simplify(self,positions,triangles,target,error,prune=True):
  positions,remap=np.unique(np.round(positions,5).astype('<f4'),axis=0,return_inverse=True)
  triangles=remap[triangles].astype('<u4').reshape(-1)
  measured=ct.c_float()
  count=self.call(triangles.ctypes.data,len(triangles),positions.ctypes.data,len(positions),12,
    None,0,None,0,None,max(3,int(target)*3),error,4|(8 if prune else 0),ct.byref(measured))
  triangles=triangles[:count].reshape(-1,3)
  keep=(triangles[:,0]!=triangles[:,1])&(triangles[:,1]!=triangles[:,2])&(triangles[:,2]!=triangles[:,0])
  triangles=triangles[keep]
  area=np.linalg.norm(np.cross(positions[triangles[:,1]]-positions[triangles[:,0]],positions[triangles[:,2]]-positions[triangles[:,0]]),axis=1)
  triangles=triangles[area>1e-12]
  used,inverse=np.unique(triangles,return_inverse=True)
  return positions[used],inverse.astype('<u4').reshape(-1,3),float(measured.value)


def components(positions,triangles):
 points,index=np.unique(np.round(positions,4),axis=0,return_inverse=True)
 faces=index[triangles]
 edges=np.concatenate([faces[:,[0,1]],faces[:,[1,2]],faces[:,[2,0]]])
 graph=coo_matrix((np.ones(len(edges)),(edges[:,0],edges[:,1])),shape=(len(points),len(points)))
 _,labels=connected_components(graph,directed=False)
 return [points[labels==n] for n in np.unique(labels)]


def palette_textures(imported,output,extra_shaders):
 inventory=json.loads((imported/'bitmap-resources.json').read_text())
 banks={(b['map'],b['tag']):b for b in inventory['banks']}
 caches={};result={}
 choices=dict(PALETTE)
 for name,shader in extra_shaders.items():choices[name]=('zanzibar',shader)
 (output/'textures').mkdir()
 for key,(map_name,shader) in choices.items():
  if map_name not in caches:caches[map_name]=MccCache(imported/f'halo2/h2_maps_win64_dx11/{map_name}.map')
  cache=caches[map_name]
  matches=[t for t in cache.tags.values() if t['group']=='shad' and t['name']==shader]
  assert len(matches)==1,(map_name,shader)
  tag=matches[0]
  if not tag['address']:
   if 'shared' not in caches:caches['shared']=MccCache(imported/'halo2/h2_maps_win64_dx11/shared.map')
   cache=caches['shared'];tag=next(t for t in cache.tags.values() if t['group']=='shad' and t['name']==shader)
  props=cache.block(tag['offset']+12,80);assert props,shader
  candidates=[cache.u32(p+4) for p in props if cache.u32(p+4)!=0xffffffff]
  if candidates:bitmap=cache.tags[candidates[0]]
  else:
   # Transparent two-alpha/environment shaders have no direct diffuse slot.
   # Use the exact named image from their preserved native reference bank for
   # the editing preview; gameplay retains the complete original program.
   library=json.loads((imported/f'native-shaders/{map_name}/library.json').read_text())
   native=next(r for r in library['shader_tags'] if r['group']=='shad' and r['name']==shader)
   leaf=shader.split('\\')[-1]
   names={r[1][1] for r in native['references'] if r[1][0]=='bitm' and r[1][1].endswith('\\'+leaf)}
   assert len(names)==1,('No unique authored transparent image',shader,names)
   bitmap=next(t for t in cache.tags.values() if t['group']=='bitm' and t['name']==next(iter(names)))
  identity=(map_name+'.map',bitmap['name'])
  if identity not in banks:identity=('shared.map',bitmap['name'])
  frame=banks[identity]['frames'][0]
  resource=inventory['resources'][frame['partitions'][0]]
  path=imported/resource['path'];assert sha(path)==resource['sha256']
  width,height,depth=frame['dimensions'];assert depth==1
  blob=dds(width,height,frame['format'],path.read_bytes(),frame['additional_mips'])
  image=Image.open(io.BytesIO(blob)).convert('RGBA')
  if key not in ('plant','fence') and 'frond' not in shader:image.putalpha(255)
  image.save(output/f'textures/{key}.png');image.save(output/f'textures/{key}.tga')
  (output/f'textures/{key}.dds').write_bytes(blob)
  result[key]=dict(shader=shader,bitmap=bitmap['name'],map=map_name,source_sha256=resource['sha256'],
    png=f'textures/{key}.png',tga=f'textures/{key}.tga')
 for cache in caches.values():cache.close()
 return result


def redesign(args):
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
 output=args.output.resolve();assert output.is_relative_to(scratch) and not output.exists()
 output.mkdir()
 source=Glb(args.package/'dust2_mombasa.glb')
 mapping=json.loads((args.package/'material_mapping.json').read_text())
 old_palette=list(mapping['palette'])+['desert_sand','desert_rock']
 assert len(old_palette)==len(source.doc['materials'])
 props=json.loads((args.props/'props.json').read_text())['models']
 extra={}
 for prop in props:
  used=np.unique(np.fromfile(args.props/prop['geometry'],dtype=np.dtype([('values','<f4',8),('material','<u4')]))['material'])
  for n in used:
   shader=prop['materials'][int(n)]
   assert shader,(prop['name'],int(n))
   extra[prop['name']+'_'+str(n)]=shader
 palette=palette_textures(args.imported,output,extra)
 writer=SceneWriter();writer.doc['asset']['generator']='Halo 2 District art rebuild'
 materials={}
 for key,row in palette.items():
  materials[key]=len(writer.doc['materials']);tex=writer.texture(output/row['png'])
  material=dict(name=row['shader'],pbrMetallicRoughness=dict(baseColorTexture=dict(index=tex),
     metallicFactor=0,roughnessFactor=.9),extras=dict(halo2_shader=row['shader']))
  if key in ('plant','fence') or 'frond' in row['shader']:material.update(alphaMode='MASK',alphaCutoff=.5,doubleSided=True)
  writer.doc['materials'].append(material)

 def emit(name,p,t,key,normals=None,uv=None):
  # Test the serialized float coordinates: unit conversion can collapse a
  # near-collinear triangle which was nonzero in the simplifier's coordinates.
  quantized=(p[:,[1,2,0]]*3.048).astype('<f4').astype(float)
  area=np.linalg.norm(np.cross(quantized[t[:,1]]-quantized[t[:,0]],quantized[t[:,2]]-quantized[t[:,0]]),axis=1)
  t=t[area>1e-12]
  if not len(t):return
  if normals is None:
   p=p[t.reshape(-1)];t=np.arange(len(p),dtype='<u4').reshape(-1,3)
   normal=np.cross(p[t[:,1]]-p[t[:,0]],p[t[:,2]]-p[t[:,0]])
   normal/=np.maximum(np.linalg.norm(normal,axis=1,keepdims=True),1e-20)
   normals=np.repeat(normal,3,axis=0)
  if uv is None:
   axes=np.argmax(abs(normals),axis=1);uv=np.empty((len(p),2))
   for axis,plane in enumerate(([1,2],[0,2],[0,1])):uv[axes==axis]=p[axes==axis][:,plane]/.8
  used=np.unique(t)
  assert np.isfinite(p[used]).all() and np.isfinite(normals[used]).all(),name
  assert (np.linalg.norm(normals[used],axis=1)>.5).all(),('Invalid vertex normals',name)
  primitive=dict(attributes=dict(POSITION=writer.array(p[:,[1,2,0]]*3.048,'VEC3'),
     NORMAL=writer.array(normals[:,[1,2,0]],'VEC3'),TEXCOORD_0=writer.array(uv,'VEC2')),
     indices=writer.array(t.reshape(-1),'SCALAR'),material=materials[key])
  writer.doc['meshes'].append(dict(name=name,primitives=[primitive]))
  writer.doc['nodes'].append(dict(name=name,mesh=len(writer.doc['meshes'])-1))
  writer.doc['scenes'][0]['nodes'].append(len(writer.doc['nodes'])-1)

 groups={};removed={'palm':[],'car':[]};nodes=[]
 for index,node,mesh,world in source.nodes():
  for prim in mesh['primitives']:
   p=source.array(prim['attributes']['POSITION']).astype(float)@world[:3,:3].T+world[:3,3]
   p=p[:,[2,0,1]]/3.048;t=source.array(prim['indices'])[:,0].reshape(-1,3)
   name=mesh.get('name','')
   replacement='palm' if 'palm_bark' in name else 'car' if 'dust_car_wreck_color' in name else None
   if replacement:removed[replacement].append((p,t));continue
   if 'palm_frond' in name or 'dust_car_wreck_interior' in name:continue
   key=old_palette[prim['material']]
   if key=='plant':continue # Replace the dense CS2 foliage art with retail palms.
   if key=='bark':key='metal' # Old classifier also matched the word "street".
   nodes.append((name,p,t,key))
 for kind,meshes in removed.items():
  p=np.concatenate([p for p,t in meshes]);offset=np.cumsum([0]+[len(p) for p,t in meshes])
  t=np.concatenate([t+offset[i] for i,(p,t) in enumerate(meshes)])
  pieces=components(p,t)
  if kind=='palm':
   pieces=[v for v in pieces if np.ptp(v[:,2])>.7]
   roots=[v[v[:,2]<v[:,2].min()+.15].mean(axis=0) for v in pieces]
   # Source aggregate fragments can split a trunk at a batching boundary.
   merged=[]
   for root in roots:
    if not any(np.linalg.norm(root[:2]-other[:2])<.15 for other in merged):merged.append(root)
   palm_roots=merged
  else:
   pieces=[v for v in pieces if np.ptp(v,axis=0).max()>.5]
   centers=np.array([(v.min(axis=0)+v.max(axis=0))*.5 for v in pieces])
   pairs=cKDTree(centers[:,:2]).query_pairs(1.2,output_type='ndarray')
   graph=coo_matrix((np.ones(len(pairs)),(pairs[:,0],pairs[:,1])),shape=(len(centers),len(centers)))
   _,labels=connected_components(graph,directed=False)
   car_groups=[np.concatenate([pieces[i] for i in np.flatnonzero(labels==n)]) for n in np.unique(labels)]
   car_groups=[v for v in car_groups if np.ptp(v,axis=0).max()>1]
   cars=[dict(position=((v.min(axis=0)+v.max(axis=0))*.5).tolist(),minimum=v.min(axis=0).tolist(),maximum=v.max(axis=0).tolist()) for v in car_groups]
 for name,p,t,key in nodes:
  if 'dust_vehicle_glass' in name:
   center=p[t].mean(axis=1);keep=np.ones(len(t),dtype=bool)
   for car in cars:
    lo=np.array(car['minimum'])-.1;hi=np.array(car['maximum'])+.1
    keep&=~((center>=lo)&(center<=hi)).all(axis=1)
   t=t[keep]
  if not len(t):continue
  groups.setdefault(key,[]).append((p,t))
 simplify=Simplifier(args.meshoptimizer);report=[]
 for key,meshes in groups.items():
  p=np.concatenate([p for p,t in meshes]);offset=np.cumsum([0]+[len(p) for p,t in meshes])
  t=np.concatenate([t+offset[i] for i,(p,t) in enumerate(meshes)])
  before=len(t);target=350 if key.startswith('desert_') else max(12,round(before/4500000*args.building_triangles))
  error_limit=2. if key.startswith('desert_') else min(args.max_error,.05) if key in ('floor','stairs','walkway') else min(args.max_error,.15) if key=='door' else args.max_error
  p,t,error=simplify.simplify(p,t,target,error_limit)
  emit('district_'+key,p,t,key)
  row=dict(material=key,before=before,after=len(t),qem_error_world_units=error)
  report.append(row);print(row,flush=True)
 placements=[]
 for kind,positions in [('palm',palm_roots),('warthog',[c['position'] for c in cars])]:
  for i,position in enumerate(positions):
   prop=props[i%3] if kind=='palm' else next(p for p in props if p['name']=='warthog_destroyed')
   raw=np.fromfile(args.props/prop['geometry'],dtype=np.dtype([('p','<f4',3),('n','<f4',3),('uv','<f4',2),('m','<u4')]))
   p=raw['p'].astype(float);n=raw['n'].astype(float);p[:,2]-=p[:,2].min()
   origin=np.array(position);origin[2]=min(origin[2],cars[i]['minimum'][2]) if kind=='warthog' else origin[2]
   angle=0.
   if kind=='warthog':
    extent=np.array(cars[i]['maximum'])-cars[i]['minimum'];angle=0. if extent[0]>=extent[1] else np.pi/2
   rotation=np.array([[np.cos(angle),-np.sin(angle),0],[np.sin(angle),np.cos(angle),0],[0,0,1]])
   p=p@rotation.T+origin;n=n@rotation.T;t=np.arange(len(p),dtype='<u4').reshape(-1,3)
   for material in np.unique(raw['m']):
    part=t[raw['m'][::3]==material];assert (raw['m'][part]==material).all()
    key=prop['name']+'_'+str(material)
    emit(f'{prop["name"]}_{i}_{material}',p,part,key,n,raw['uv'])
   placements.append(dict(kind=kind,source=prop['tag'],position=origin.tolist(),yaw_radians=angle,triangles=len(t),static=True))
 writer.write(output/'dust2_mombasa.glb',generator='Halo 2 District art rebuild')
 shutil.copyfile(args.package/'dust2_collision.glb',output/'dust2_collision.glb')
 shutil.copyfile(args.package/'dust2_collision.ass',output/'dust2_collision.ass')
 for name in ('source_entities.json','provenance.json','lighting.json'):shutil.copyfile(args.package/name,output/name)
 ass=output/'data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass';ass.parent.mkdir(parents=True)
 final=Glb(output/'dust2_mombasa.glb');rows=[dict(palette=k,reason='District art rebuild') for k in palette]
 geometry=write_ass(final,ass,palette,rows)
 background=SceneWriter(final)
 background.doc['scenes'][0]['nodes']=[i for i,n in enumerate(final.doc['nodes']) if n['name'].startswith('district_desert_')]
 background.write(output/'desert_backdrop.glb',generator='Halo 2 District desert background')
 write_ass(Glb(output/'desert_backdrop.glb'),ass.parent/'desert_backdrop.ass',palette,rows)
 save_json(output/'material_mapping.json',dict(palette=palette,materials=rows))
 save_json(output/'redesign.json',dict(schema='H2_DUST2_DISTRICT_REDESIGN_V1',source_sha256=sha(args.package/'dust2_mombasa.glb'),
   material_reduction=report,placements=placements,geometry=geometry,collision='original reference pending replacement collision authoring',
   geometry_reference='Original Xbox: Zanzibar 23397; Headlong 28513; Lockout 14463 BSP triangles',
   simplifier='meshoptimizer v1.3 9e1f07b159d3cb777f1c67ed31fc11fd117986f4',runtime_verified=False))
 print('Output',output,flush=True)


if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 for name in ('package','imported','props','meshoptimizer','output'):parser.add_argument('--'+name,type=Path,required=True)
 parser.add_argument('--building-triangles',type=int,default=12000)
 parser.add_argument('--max-error',type=float,default=.35)
 redesign(parser.parse_args())
