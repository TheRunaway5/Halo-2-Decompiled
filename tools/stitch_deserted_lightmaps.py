#!/usr/bin/env python3
"""Stitch the actual Classic bake across coplanar architectural triangle charts.

Run through run_test.py. Positions, indices, UVs, collision, lighting directions
and shader programs stay byte-identical. RGB texels of the authored bake are
reconciled across shared planar edges, with larger filtering gutters.
"""
from collections import defaultdict
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import numpy as np
from PIL import Image
from scipy.ndimage import distance_transform_edt
from mcc_content import MccCache,texture_resource
from prepare_dust2_native import publish
from deserted_architecture import architecture_sections
from deserted_collision import validate_cache


def sample(image,uv):
    h,w=image.shape[:2];xy=uv*np.array([w,h])-.5
    x,y=xy.T;ix=np.floor(x).astype(int);iy=np.floor(y).astype(int);fx=(x-ix)[:,None];fy=(y-iy)[:,None]
    return (image[iy.clip(0,h-1),ix.clip(0,w-1)]*(1-fx)*(1-fy)+image[(iy+1).clip(0,h-1),ix.clip(0,w-1)]*(1-fx)*fy+
        image[iy.clip(0,h-1),(ix+1).clip(0,w-1)]*fx*(1-fy)+image[(iy+1).clip(0,h-1),(ix+1).clip(0,w-1)]*fx*fy)


def stitch(root,runtime,imported,main_hud):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=root/'content/halo2/Mods/deserted/deserted.map';c=MccCache(source)
    validate_cache(c,root/'evidence/collision-cache.json')
    l=next(t for t in c.tags.values() if t['group']=='ltmp');b=next(t for t in c.tags.values() if t['group']=='sbsp')
    g=c.block(l['offset']+128,104)[0];bitm=c.tags[c.u32(g+28)]
    selected=architecture_sections(c,b)
    bindings=c.block(g+72,4)
    selected_images={c.u16(bindings[ii]) for ii in selected}
    records=c.block(bitm['offset']+68,168);images=[]
    with source.open('rb') as stream:
        for a in records:
            w,h,fmt=(c.u16(a+x) for x in (4,6,12));assert fmt==11 and c.u16(a+20)==0
            raw=texture_resource(stream,c.u32(a+56),c.u32(a+80))
            image=Image.frombytes('RGBA',(w,h),raw,'raw','BGRA')
            if len(images) in selected_images:
                image=image.resize((w*4,h*4),Image.Resampling.BILINEAR)
                struct.pack_into('<HH',c.data,a+4,w*4,h*4)
            images.append(np.asarray(image).astype(float))
    bindings=c.block(g+72,4);definitions=c.block(b['offset']+312,200)
    faces=[];corners=defaultdict(list)
    for ii,inst in enumerate(c.block(b['offset']+320,88)):
        if ii not in selected:continue
        at=definitions[c.u16(inst+52)];base=c.u32(at+40)+c.u32(at+48)+8
        streams={(c.u16(r+4),c.u16(r+6)):(base+c.u32(r+12),c.u32(r+8)) for r in c.block(at+56,16)}
        def read(key,cols,dtype='<f4'):
            off,size=streams[key];return np.frombuffer(c.data,dtype=dtype,count=size//np.dtype(dtype).itemsize,offset=off).reshape(-1,cols).copy()
        p=read((56,0),3).astype(float);uv=read((56,3),2).astype(float)
        t=read(next(k for k in streams if k[0]==32),3,'<u2')[selected[ii]];tri=p[t];cross=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);length=np.linalg.norm(cross,axis=1);normals=cross/np.maximum(length[:,None],1e-30)
        image_index=c.u16(bindings[ii]);image=images[image_index]
        for ti,ids in enumerate(t):
            if length[ti]<1e-8:continue
            color=sample(image[:,:,:3],uv[ids]);keys=[]
            for j,v in enumerate(tri[ti]):
                key=tuple(np.rint(v*1e4).astype(int))+tuple(np.rint(normals[ti]*1000).astype(int))
                keys.append(key);corners[key].append(color[j])
            faces.append((image_index,uv[ids],color,keys))
    target={key:np.mean(values,axis=0) for key,values in corners.items()}
    edge_samples=defaultdict(list);edge_t=np.linspace(0,1,17)[:,None]
    for image_index,uv,color,keys in faces:
        for a,z in ((0,1),(1,2),(2,0)):
            if keys[a]>keys[z]:a,z=z,a
            edge_samples[(keys[a],keys[z])].append(sample(images[image_index][:,:,:3],uv[a]*(1-edge_t)+uv[z]*edge_t))
    edge_targets={key:np.mean(values,axis=0) for key,values in edge_samples.items()}
    for (a,z),values in edge_targets.items():values[0]=target[a];values[-1]=target[z]
    del edge_samples
    accum=[np.zeros_like(im[:,:,:3]) for im in images];weights=[np.zeros(im.shape[:2]) for im in images]
    for image_index,uv,color,keys in faces:
        h,w=images[image_index].shape[:2];q=uv*np.array([w,h])-.5
        low=np.floor(q.min(axis=0)-1).astype(int).clip(0,[w-1,h-1]);high=np.ceil(q.max(axis=0)+1).astype(int).clip(0,[w-1,h-1])
        x,y=np.meshgrid(np.arange(low[0],high[0]+1),np.arange(low[1],high[1]+1))
        matrix=np.column_stack((q[1]-q[0],q[2]-q[0]))
        if abs(np.linalg.det(matrix))<1e-8:continue
        ab=np.stack((x-q[0,0],y-q[0,1]),axis=-1)@np.linalg.inv(matrix).T
        bary=np.stack((1-ab.sum(axis=-1),ab[:,:,0],ab[:,:,1]),axis=-1)
        # Rasterize the triangle plus its one-texel filtering gutter. Nearest
        # barycentric coordinates extrapolate corrected border samples.
        inside=(bary>=-.03).all(axis=-1)
        if not inside.any():continue
        correction=-(bary@np.array([target[k]-color[j] for j,k in enumerate(keys)]))
        for a,z in ((0,1),(1,2),(2,0)):
            if keys[a]>keys[z]:a,z=z,a
            pair=bary[:,:,a]+bary[:,:,z]
            parameter=np.clip(bary[:,:,z]/np.maximum(pair,1e-10),0,1)
            key=(keys[a],keys[z]);curve=edge_targets[key]-sample(images[image_index][:,:,:3],uv[a]*(1-edge_t)+uv[z]*edge_t)
            interpolated=np.stack([np.interp(parameter,np.linspace(0,1,17),curve[:,channel]) for channel in range(3)],axis=-1)
            correction+=pair[:,:,None]*interpolated
        yy,xx=y[inside],x[inside]
        accum[image_index][yy,xx]+=correction[inside];weights[image_index][yy,xx]+=1
    modified=[]
    for i,(image,a) in enumerate(zip(images,records)):
        covered=weights[i]>0
        if not covered.any():continue
        correction=accum[i]/np.maximum(weights[i][:,:,None],1)
        distance,nearest=distance_transform_edt(~covered,return_indices=True)
        gutter=(~covered)&(distance<=1.5)
        image[:,:,:3]=np.clip(image[:,:,:3]+correction,0,255)
        image[gutter,:3]=image[nearest[0][gutter],nearest[1][gutter],:3]
        rgba=np.rint(image).astype('u1');raw=rgba[:,:,[2,1,0,3]].tobytes()
        payload=struct.pack('<ii',1,-len(raw))+raw
        c.data.extend(bytes((-len(c.data))%16));offset=len(c.data);c.data.extend(payload)
        struct.pack_into('<I',c.data,a+56,offset);struct.pack_into('<I',c.data,a+80,len(payload))
        modified.append(dict(image=i,texels=int(covered.sum()),mean_abs_rgb_correction=float(abs(correction[covered]).mean())))
    struct.pack_into('<I',c.data,8,len(c.data));private=scratch/'stitched.map';private.write_bytes(c.data)
    content=scratch/'content';publish(private,None,imported,content,scratch/'publication.json',main_hud,compiled_lighting=True,output_name='deserted')
    candidate=root/'lighting-candidate'
    if candidate.exists():
        assert json.loads((candidate/'evidence/stitch.json').read_text())['schema']=='H2_DESERTED_LIGHTMAP_STITCH_CANDIDATE_V1'
        candidate.rename(scratch/'previous-owned-candidate')
    candidate.mkdir();(candidate/'evidence').mkdir()
    content.rename(candidate/'content')
    report=dict(schema='H2_DESERTED_LIGHTMAP_STITCH_CANDIDATE_V1',source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        scope='Candidate only; native visual and compiled edge review pending',images=modified,faces=len(faces),shared_corners=sum(len(v)>1 for v in corners.values()))
    (candidate/'evidence/stitch.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report),flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('root','imported','main-hud'):p.add_argument('--'+name,required=True,type=Path)
    p.add_argument('--runtime',type=Path)
    stitch(**vars(p.parse_args()))
