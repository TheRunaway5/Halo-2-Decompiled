#!/usr/bin/env python3
"""Inspect real baked colors at coplanar triangle boundaries in Deserted."""
from collections import defaultdict
import argparse
import hashlib
import json
import os
from pathlib import Path
import numpy as np
from PIL import Image
from mcc_content import MccCache,texture_resource
from deserted_architecture import architecture_sections

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('root',type=Path)
p.add_argument('--report',type=Path)
p.add_argument('--retain-atlases',action='store_true')
args=p.parse_args()
root=args.root;scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
c=MccCache(root/'content/halo2/Mods/deserted/deserted.map')
l=next(t for t in c.tags.values() if t['group']=='ltmp');b=next(t for t in c.tags.values() if t['group']=='sbsp')
g=c.block(l['offset']+128,104)[0];bitm=c.tags[c.u32(g+28)]
images=[]
with (root/'content/halo2/h2_maps_win64_dx11/textures.dat').open('rb') as stream:
    for i,a in enumerate(c.block(bitm['offset']+68,168)):
        w,h,fmt=(c.u16(a+x) for x in (4,6,12));assert fmt==11
        raw=texture_resource(stream,c.u32(a+56),c.u32(a+80))
        im=Image.frombytes('RGBA',(w,h),raw,'raw','BGRA')
        if args.retain_atlases and i in (1,6,10):im.save(root/'evidence'/f'lightmap-atlas-{i}.png')
        images.append(np.asarray(im).astype(float)[:,:,:3])
bindings=c.block(g+72,4);rows=[]
offset_probe=defaultdict(list)
definitions=c.block(b['offset']+312,200)
selected=architecture_sections(c,b)
def sample(image,uv):
    h,w=image.shape[:2];xy=uv*np.array([w,h])-.5
    x,y=xy.T;ix=np.floor(x).astype(int);iy=np.floor(y).astype(int);fx=(x-ix)[:,None];fy=(y-iy)[:,None]
    return (image[iy.clip(0,h-1),ix.clip(0,w-1)]*(1-fx)*(1-fy)+image[(iy+1).clip(0,h-1),ix.clip(0,w-1)]*(1-fx)*fy+
        image[iy.clip(0,h-1),(ix+1).clip(0,w-1)]*fx*(1-fy)+image[(iy+1).clip(0,h-1),(ix+1).clip(0,w-1)]*fx*fy)
for ii,inst in enumerate(c.block(b['offset']+320,88)):
    if ii not in selected:continue
    at=definitions[c.u16(inst+52)];base=c.u32(at+40)+c.u32(at+48)+8
    streams={(c.u16(r+4),c.u16(r+6)):(base+c.u32(r+12),c.u32(r+8)) for r in c.block(at+56,16)}
    def read(key,cols,dtype='<f4'):
        off,size=streams[key];return np.frombuffer(c.data,dtype=dtype,count=size//np.dtype(dtype).itemsize,offset=off).reshape(-1,cols)
    p=read((56,0),3).astype(float);uv=read((56,3),2).astype(float)
    t=read(next(k for k in streams if k[0]==32),3,'<u2')[selected[ii]];tri=p[t];cross=np.cross(tri[:,1]-tri[:,0],tri[:,2]-tri[:,0]);n=cross/np.maximum(np.linalg.norm(cross,axis=1)[:,None],1e-30)
    image=images[c.u16(bindings[ii])];edges=defaultdict(list)
    for ti in range(len(t)):
        for a,z in ((0,1),(1,2),(2,0)):
            vertices=tri[ti,[a,z]];keys=[tuple(v) for v in np.rint(vertices*1e4).astype(int)]
            if keys[0]>keys[1]:a,z=z,a;keys.reverse()
            edges[tuple(keys)].append((ti,t[ti,[a,z]]))
    for key,faces in edges.items():
        for ai,(ta,va) in enumerate(faces):
            for tb,vb in faces[ai+1:]:
                if n[ta]@n[tb]<.99999:continue
                # Restrict to broad flat walls/floor, away from sharp corners.
                midpoint=p[va].mean(axis=0)
                if np.linalg.norm(p[va[0]]-p[va[1]])<.25:continue
                ts=np.linspace(.15,.85,5)[:,None]
                ua=uv[va[0]]*(1-ts)+uv[va[1]]*ts;ub=uv[vb[0]]*(1-ts)+uv[vb[1]]*ts
                delta=float(abs(sample(image,ua)-sample(image,ub)).mean())
                if delta>8:
                    rows.append(dict(instance=ii,faces=[int(ta),int(tb)],midpoint=midpoint.tolist(),
                        normal=n[ta].tolist(),mean_rgb_discontinuity=delta,uv_a=uv[va].tolist(),uv_b=uv[vb].tolist()))
                    for dx in (-.5,0,.5):
                        for dy in (-.5,0,.5):
                            shift=np.array([dx/image.shape[1],dy/image.shape[0]])
                            offset_probe[f'{dx},{dy}'].append(float(abs(sample(image,ua+shift)-sample(image,ub+shift)).mean()))
rows.sort(key=lambda r:r['mean_rgb_discontinuity'],reverse=True)
report=dict(passed=len(rows)==0,bad_coplanar_lightmap_edges=len(rows),examples=rows[:24],
    half_texel_probe={k:dict(mean_rgb_jump=float(np.mean(v)),remaining_jumps_over8=int((np.array(v)>8).sum())) for k,v in offset_probe.items()},
    cache_sha256=hashlib.sha256((root/'content/halo2/Mods/deserted/deserted.map').read_bytes()).hexdigest(),
    scope='Coplanar compiled edges longer than0.25WU; five actual atlas samples along each edge; RGB discontinuity threshold8/255')
(args.report or root/'evidence/atlas-seams-current.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ('examples','half_texel_probe')}))
if rows:print('Largest RGB jump:',rows[0]['mean_rgb_discontinuity'],'at',rows[0]['midpoint'])
raise SystemExit(0 if report['passed'] else 1)
