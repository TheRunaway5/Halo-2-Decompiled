#!/usr/bin/env python3
"""Check that the Classic compiler retained every visible architectural face.

Run through tools/run_test.py. Winding is ignored only for face-presence checks;
the saved source topology audit checks winding and manifold edges separately.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import numpy as np
from scipy.spatial import cKDTree
from deserted_architecture import architecture_sections
from mcc_content import MccCache


def audit(root, report):
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    data=np.load(root/'model/architecture-solid.npz')
    source=data['positions'][data['triangles']]
    path=root/'content/halo2/Mods/deserted/deserted.map'
    c=MccCache(path);b=next(t for t in c.tags.values() if t['group']=='sbsp')
    selected=architecture_sections(c,b);definitions=c.block(b['offset']+312,200)
    triangles=[]
    for ii,inst in enumerate(c.block(b['offset']+320,88)):
        if ii not in selected:continue
        at=definitions[c.u16(inst+52)];base=c.u32(at+40)+c.u32(at+48)+8
        streams={(c.u16(r+4),c.u16(r+6)):(base+c.u32(r+12),c.u32(r+8)) for r in c.block(at+56,16)}
        def read(key,cols,dtype='<f4'):
            off,size=streams[key]
            return np.frombuffer(c.data,dtype=dtype,count=size//np.dtype(dtype).itemsize,offset=off).reshape(-1,cols)
        p=read((56,0),3).astype(float)
        t=read(next(k for k in streams if k[0]==32),3,'<u2')[selected[ii]]
        triangles.append(p[t])
    compiled=np.concatenate(triangles)
    def key(face):
        return tuple(sorted(tuple(v) for v in np.rint(face*1e4).astype(np.int64)))
    exact={key(face) for face in compiled}
    # A compiler may retriangulate a planar polygon. Compare points throughout
    # each source face to nearby actual compiled triangles as well as corners.
    centers=compiled.mean(axis=1);radii=np.linalg.norm(compiled-centers[:,None],axis=2).max(axis=1)
    tree=cKDTree(centers)
    weights=np.array([[1/3]*3,[.8,.1,.1],[.1,.8,.1],[.1,.1,.8]])
    missing=[]
    for i,face in enumerate(source):
        if key(face) in exact:continue
        samples=weights@face
        for q in samples:
            ids=tree.query_ball_point(q,float(radii.max())+.001)
            ids=np.array(ids,dtype=int)
            ids=ids[np.linalg.norm(centers[ids]-q,axis=1)<=radii[ids]+.001]
            p=compiled[ids];a=p[:,1]-p[:,0];z=p[:,2]-p[:,0];v=q-p[:,0]
            aa=(a*a).sum(axis=1);az=(a*z).sum(axis=1);zz=(z*z).sum(axis=1)
            va=(v*a).sum(axis=1);vz=(v*z).sum(axis=1);det=aa*zz-az*az
            valid=det>1e-18
            u=np.divide(va*zz-vz*az,det,out=np.zeros(len(ids)),where=valid)
            w=np.divide(vz*aa-va*az,det,out=np.zeros(len(ids)),where=valid)
            error=np.linalg.norm(p[:,0]+u[:,None]*a+w[:,None]*z-q,axis=1)
            distance=np.where(valid&(u>=0)&(w>=0)&(u+w<=1),error,np.inf)
            for start,end in ((0,1),(1,2),(2,0)):
                edge=p[:,end]-p[:,start]
                scale=np.divide(((q-p[:,start])*edge).sum(axis=1),(edge*edge).sum(axis=1),
                    out=np.zeros(len(ids)),where=(edge*edge).sum(axis=1)>1e-20).clip(0,1)
                distance=np.minimum(distance,np.linalg.norm(p[:,start]+scale[:,None]*edge-q,axis=1))
            if not (distance<.001).any():
                missing.append(dict(source_face=i,point=q.tolist(),triangle=face.tolist(),
                    area=float(np.linalg.norm(np.cross(face[1]-face[0],face[2]-face[0]))/2),
                    material_role=int(data['roles'][i])))
                break
    missing.sort(key=lambda r:r['area'],reverse=True)
    result=dict(passed=not missing,source_faces=len(source),compiled_architecture_faces=len(compiled),
        missing_face_samples=len(missing),missing_surface_area=sum(r['area'] for r in missing),examples=missing[:30],
        cache_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        scope='Four interior samples per source architectural triangle within0.001WU (3.048mm) of actual compiled render faces, including compiler weld/plane rounding; native walk remains separate')
    report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='examples'}),flush=True)
    return 0 if result['passed'] else 1


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',required=True,type=Path)
    p.add_argument('--report',required=True,type=Path)
    raise SystemExit(audit(**vars(p.parse_args())))
