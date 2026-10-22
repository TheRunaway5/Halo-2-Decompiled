#!/usr/bin/env python3
"""Check authored walk corridors against Deserted's actual solid triangles.

This is a geometry/clearance probe, not native player-controller acceptance.
Run through tools/run_test.py. The finite routes cover the principal map loops.
"""
import argparse
import json
import os
from pathlib import Path
import numpy as np
from shapely.geometry import Polygon
from shapely import points as shapely_points
from shapely.strtree import STRtree
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import dijkstra
from scipy.spatial import cKDTree


ROUTES=[
    ('CT to Long A',-1.025,[(1.14,17.64),(12,17.64),(12,24)]),
    ('A to Short to Mid',.8,[(12,24),(10.8,24),(8,22.2),(5,21.9),(3.2,21.9),(3.2,12.4),(-1.2,12.4),(-1.2,5.4),(-3.2,5.4),(-3.2,18)]),
    ('CT to B',-1.025,[(1.14,19.4),(-3.8,19.4),(-10.8,19.4),(-16.3,19.4),(-16.3,24)]),
    ('Mid to T to Upper Tunnel',0.,[(-3.6,3),(-3.6,-6.5),(-15,-6.5),(-16,-3),(-15,1),(-14.2,5),(-14.2,9.8)]),
    ('Upper Tunnel to B',.28,[(-14.2,9.8),(-16.4,10),(-16.4,17.5)]),
    ('Upper to Lower Tunnel to Mid',.28,[(-14.2,9.8),(-11.8,9.8),(-10.8,10.3),(-10,12.1),(-6,11.8),(-3.4,11.8)]),
    ('T spawn to Long',1.,[(-4.,-7.),(2.5,-7.),(2.5,0.),(6.,5.),(12.,10.)]),
]


def audit(package,report):
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    data=np.load(package/'architecture-solid.npz')
    plan=json.loads((package/'architecture-plan.json').read_text())
    p=data['positions'][data['triangles']]
    normal=np.cross(p[:,1]-p[:,0],p[:,2]-p[:,0])
    area=np.linalg.norm(normal,axis=1)
    top=normal[:,2]>.45*area
    bottom=normal[:,2]<-.45*area
    horizontal=top|bottom
    p=p[horizontal];top=top[horizontal]
    tree=STRtree([Polygon(t[:,:2]) for t in p])
    spacing=.10
    xx,yy=np.meshgrid(np.arange(-19.9,15.5,spacing),np.arange(-9.8,26.6,spacing),indexing='ij')
    xy=np.column_stack((xx.reshape(-1),yy.reshape(-1)))
    pair=tree.query(shapely_points(xy),predicate='intersects')
    query,ids=pair;triangles=p[ids]
    v0=triangles[:,1,:2]-triangles[:,0,:2];v1=triangles[:,2,:2]-triangles[:,0,:2]
    v=xy[query]-triangles[:,0,:2];det=v0[:,0]*v1[:,1]-v0[:,1]*v1[:,0]
    u=(v[:,0]*v1[:,1]-v[:,1]*v1[:,0])/det
    w=(v0[:,0]*v[:,1]-v0[:,1]*v[:,0])/det
    z=triangles[:,0,2]+u*(triangles[:,1,2]-triangles[:,0,2])+w*(triangles[:,2,2]-triangles[:,0,2])
    # Retain separate upper and lower layers at the CT bridge. A single
    # heightfield would incorrectly close that passage.
    floors=np.unique(np.column_stack((query[top[ids]],np.round(z[top[ids]],5))),axis=0)
    floors=floors[(floors[:,1]>=-1.8)&(floors[:,1]<=1.6)]
    ceilings={}
    for cell,zz in zip(query[~top[ids]],z[~top[ids]]):ceilings.setdefault(int(cell),[]).append(zz)
    floors=np.array([r for r in floors if not any(r[1]+.02<c<r[1]+.725 for c in ceilings.get(int(r[0]),[]))])
    cells={}
    for i,(cell,height) in enumerate(floors):cells.setdefault(int(cell),[]).append(i)
    stride=xx.shape[1]
    neighbours=[];safe=np.ones(len(floors),dtype=bool)
    for i,(cell,height) in enumerate(floors):
        for delta in (-stride,stride,-1,1):
            compatible=[j for j in cells.get(int(cell)+delta,[]) if abs(floors[j,1]-height)<=.15]
            if not compatible:safe[i]=False
            neighbours.extend((i,j) for j in compatible)
        for delta in (-2*stride,2*stride,-2,2):
            if not any(abs(floors[j,1]-height)<=.15 for j in cells.get(int(cell)+delta,[])):
                safe[i]=False
    edge=np.array([(a,b) for a,b in neighbours if safe[a] and safe[b]])
    graph=coo_matrix((np.full(len(edge),spacing),(edge[:,0],edge[:,1])),shape=(len(floors),len(floors))).tocsr()
    safe_ids=np.flatnonzero(safe);position=np.column_stack((xy[floors[:,0].astype(int)],floors[:,1]))
    location=cKDTree(position[safe_ids,:2])
    rows=[]
    for name,height,points in ROUTES:
        bad=[];goals=[];paths=[]
        for point in points:
            expected=0.
            for grade in plan['ground_sections']:
                x,y,X,Y=grade['bounds'];a,b,c=grade['plane']
                if x<=point[0]<=X and y<=point[1]<=Y:expected=a*point[0]+b*point[1]+c
            if 2.19<=point[0]<=5.15 and 16.99<=point[1]<=22.65 and height>.3:expected=.8
            distance,index=location.query(point,k=12)
            candidates=safe_ids[index]
            score=distance+2*abs(position[candidates,2]-expected)
            node=int(candidates[np.argmin(score)])
            if np.linalg.norm(position[node,:2]-point)>.4:
                bad.append(dict(position=point,reason='waypoint outside standing corridor'))
            goals.append(node);height=position[node,2]
        for a,b,source,target in zip(points[:-1],points[1:],goals[:-1],goals[1:]):
            distance,previous=dijkstra(graph,indices=source,return_predecessors=True)
            limit=1.75*np.linalg.norm(np.array(b)-a)+1.
            if not np.isfinite(distance[target]) or distance[target]>limit:
                bad.append(dict(from_position=a,to_position=b,reason='blocked or excessive detour',
                    distance=float(distance[target]) if np.isfinite(distance[target]) else None,limit=limit))
            else:
                path=[target]
                while path[-1]!=source:path.append(int(previous[path[-1]]))
                paths.append(position[path[::-1]].tolist())
        rows.append(dict(name=name,passed=not bad,failures=bad,paths=paths))
    result=dict(schema='H2_DESERTED_AUTHORED_ROUTE_PROBE_V1',passed=all(r['passed'] for r in rows),
        scope='triangle geometry, 0.61 meter cardinal wall margin and 2.21 meter standing clearance (actual player capsule radius0.175WU and height0.725WU); bounded route detours, native controller validation separate',routes=rows)
    report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({r['name']:dict(passed=r['passed'],failures=r['failures'][:8]) for r in rows},indent=2))
    return 0 if result['passed'] else 1


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package',required=True,type=Path)
    p.add_argument('--report',required=True,type=Path)
    raise SystemExit(audit(**vars(p.parse_args())))
