#!/usr/bin/env python3
"""Constrain real baked texels to agree across planar chart seams.

The least-squares correction uses only texels sampled along actual coincident
compiled edges. Geometry, UVs, collision, shader code and interior texels keep
their authored bytes. Run through tools/run_test.py.
"""
import argparse
from collections import defaultdict
import hashlib
import json
import os
from pathlib import Path
import struct
import numpy as np
from PIL import Image
from scipy.sparse import coo_matrix
from scipy.sparse.linalg import lsmr
from mcc_content import MccCache,texture_resource
from prepare_dust2_native import publish
from deserted_architecture import architecture_sections


def solve(root,imported,main_hud):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=root/'content/halo2/Mods/deserted/deserted.map';c=MccCache(source)
    l=next(t for t in c.tags.values() if t['group']=='ltmp');b=next(t for t in c.tags.values() if t['group']=='sbsp')
    bsp_sha=hashlib.sha256(c.data[b['offset']:b['offset']+b['size']]).hexdigest()
    g=c.block(l['offset']+128,104)[0];records=c.block(c.tags[c.u32(g+28)]['offset']+68,168)
    images=[];offsets=[0]
    with source.open('rb') as stream:
        for a in records:
            w,h,fmt=(c.u16(a+x) for x in (4,6,12));assert fmt==11
            raw=texture_resource(stream,c.u32(a+56),c.u32(a+80))
            images.append(np.asarray(Image.frombytes('RGBA',(w,h),raw,'raw','BGRA')).copy())
            offsets.append(offsets[-1]+w*h)
    def texels(ii,uv):
        h,w=images[ii].shape[:2];p=uv*np.array([w,h])-.5;ij=np.floor(p).astype(int);f=p-ij
        x,y=ij.T;fx,fy=f.T
        ids=np.column_stack((y.clip(0,h-1)*w+x.clip(0,w-1),(y+1).clip(0,h-1)*w+x.clip(0,w-1),
            y.clip(0,h-1)*w+(x+1).clip(0,w-1),(y+1).clip(0,h-1)*w+(x+1).clip(0,w-1)))+offsets[ii]
        weights=np.column_stack(((1-fx)*(1-fy),(1-fx)*fy,fx*(1-fy),fx*fy))
        return ids,weights
    bindings=c.block(g+72,4);definitions=c.block(b['offset']+312,200)
    selected=architecture_sections(c,b)
    edges=defaultdict(list)
    for ii,inst in enumerate(c.block(b['offset']+320,88)):
        if ii not in selected:continue
        at=definitions[c.u16(inst+52)];base=c.u32(at+40)+c.u32(at+48)+8
        streams={(c.u16(r+4),c.u16(r+6)):(base+c.u32(r+12),c.u32(r+8)) for r in c.block(at+56,16)}
        def read(key,cols,dtype='<f4'):
            off,size=streams[key];return np.frombuffer(c.data,dtype=dtype,count=size//np.dtype(dtype).itemsize,offset=off).reshape(-1,cols).copy()
        p=read((56,0),3).astype(float);uv=read((56,3),2).astype(float)
        t=read(next(k for k in streams if k[0]==32),3,'<u2')[selected[ii]];tri=p[t];n=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);n/=np.maximum(np.linalg.norm(n,axis=1)[:,None],1e-30)
        image=c.u16(bindings[ii])
        for ti,ids in enumerate(t):
            for a,z in ((0,1),(1,2),(2,0)):
                keys=[tuple(v) for v in np.rint(tri[ti,[a,z]]*1e4).astype(int)]
                if keys[0]>keys[1]:a,z=z,a;keys.reverse()
                edges[tuple(keys)].append((n[ti],image,uv[ids[[a,z]]]))
    indices=[];coefficients=[]
    for faces in edges.values():
        for ai,(na,ia,ua) in enumerate(faces):
            for nb,ib,ub in faces[ai+1:]:
                if na@nb<.99999 or (ia==ib and np.array_equal(ua,ub)):continue
                sizea=np.array(images[ia].shape[1::-1]);sizeb=np.array(images[ib].shape[1::-1])
                length=max(np.linalg.norm((ua[1]-ua[0])*sizea),np.linalg.norm((ub[1]-ub[0])*sizeb))
                ts=np.linspace(0,1,min(129,max(17,int(length*2)+1)))[:,None]
                a,wa=texels(ia,ua[0]*(1-ts)+ua[1]*ts);z,wz=texels(ib,ub[0]*(1-ts)+ub[1]*ts)
                indices.append(np.column_stack((a,z)));coefficients.append(np.column_stack((wa,-wz)))
    ids=np.concatenate(indices);values=np.concatenate(coefficients);unique,inverse=np.unique(ids,return_inverse=True)
    rows=np.repeat(np.arange(len(ids)),8)
    matrix=coo_matrix((values.reshape(-1),(rows,inverse.reshape(-1))),shape=(len(ids),len(unique))).tocsr();matrix.eliminate_zeros()
    pixels=np.concatenate([im[:,:,:3].reshape(-1,3) for im in images]).astype(float);selected=pixels[unique].copy()
    before=matrix@selected;iterations=[]
    print('Solving',matrix.shape,'seam samples / actual texels',flush=True)
    for outer in range(2):
        residual=matrix@selected
        for channel in range(3):
            result=lsmr(matrix,-residual[:,channel],atol=1e-6,btol=1e-6,maxiter=250)
            selected[:,channel]+=result[0]
            iterations.append(dict(round=outer,channel=channel,iterations=result[2],residual_norm=float(result[3])))
        selected=np.clip(selected,0,255)
    selected=np.rint(selected);after=matrix@selected;pixels[unique]=selected
    for i,(im,a) in enumerate(zip(images,records)):
        if not np.any((unique>=offsets[i])&(unique<offsets[i+1])):continue
        im[:,:,:3]=pixels[offsets[i]:offsets[i+1]].reshape(im.shape[0],im.shape[1],3).astype('u1')
        raw=im[:,:,[2,1,0,3]].tobytes();payload=struct.pack('<ii',1,-len(raw))+raw
        at,size=c.u32(a+56),c.u32(a+80)
        assert size==len(payload) and struct.unpack_from('<ii',c.data,at)==(1,-len(raw)), 'Expected the owned stitch candidate uncompressed partition'
        c.data[at:at+size]=payload
    struct.pack_into('<I',c.data,8,len(c.data));private=scratch/'seams.map';private.write_bytes(c.data)
    report=dict(schema='H2_DESERTED_BAKED_SEAM_SOLVE_V1',source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
        constraints=matrix.shape[0],boundary_texels=matrix.shape[1],before_mean_rgb_jump=float(abs(before).mean()),
        after_mean_rgb_jump=float(abs(after).mean()),after_max_rgb_jump=float(abs(after).max()),iterations=iterations,
        scope='Baked edge texel continuity; native visual review remains separate')
    publish(private,None,imported,scratch/'content',scratch/'publication.json',main_hud,compiled_lighting=True,output_name='deserted')
    verified=MccCache(scratch/'content/halo2/Mods/deserted/deserted.map')
    final_bsp=next(t for t in verified.tags.values() if t['group']=='sbsp')
    assert bsp_sha==hashlib.sha256(verified.data[final_bsp['offset']:final_bsp['offset']+final_bsp['size']]).hexdigest()
    report['compiled_bsp_metadata_preserved']=True
    report['compiled_bsp_sha256']=bsp_sha
    (root/'content').rename(scratch/'previous-owned-content');(scratch/'content').rename(root/'content')
    (root/'evidence/seam-solve.json').write_text(json.dumps(report,indent=2)+'\n');print(report,flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('root','imported','main-hud'):p.add_argument('--'+name,required=True,type=Path)
    solve(**vars(p.parse_args()))
