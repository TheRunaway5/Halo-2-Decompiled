#!/usr/bin/env python3
"""Assemble the Dust II model, collision and sealed sky shell for Classic Tool.

Run through tools/run_test.py. The output is an explicit authoring deliverable;
intermediate mesh batches are owned by a TemporaryDirectory. Triangle corners,
normals and UV text are preserved exactly while instance boundaries change.
"""
from __future__ import annotations

import argparse
import json
import mmap
import os
from pathlib import Path
import re
import shutil
import tempfile

import numpy as np

from build_dust2_mombasa import save_json, sha

IDENTITY = '0\t0\t0\t1\n0\t0\t0\n1\n'*2


def source_meshes(path):
    with path.open('rb') as f, mmap.mmap(f.fileno(),0,access=mmap.ACCESS_READ) as raw:
        positions=[m.start() for m in re.finditer(b'"MESH"\n""\n""\n',raw)]
        assert positions
        header=[line.strip() for line in raw[:positions[0]].decode().splitlines()
                if line.strip() and not line.startswith(';')]
        assert header[0]=='2'
        count=int(header[5])
        materials=[tuple(header[6+i*2:8+i*2]) for i in range(count)]
        assert int(header[6+count*2])==len(positions)
        # The writer emits a delimited instance section. No triangle scan is
        # needed to distinguish the final mesh from its instance records.
        end=raw.find(b';### INSTANCES ###',positions[-1])
        assert end>positions[-1]
        yield materials
        for i,start in enumerate(positions):
            finish=positions[i+1] if i+1<len(positions) else end
            lines=[line for line in raw[start:finish].splitlines(keepends=True)
                   if line.strip() and not line.startswith(b';')]
            vertices=int(lines[3]);at=4+vertices*5
            assert all(lines[4+j*5+2].strip()==b'0' and
                       lines[4+j*5+3].strip()==b'1' for j in range(vertices))
            records=[b''.join(lines[4+j*5:9+j*5]) for j in range(vertices)]
            triangles=np.fromstring(b''.join(lines[at+1:]).decode(),dtype=np.int64,sep=' ').reshape(-1,4)
            assert len(triangles)==int(lines[at])
            yield records,triangles


def shell(path,bounds,material):
    lower,upper=np.asarray(bounds,dtype=float)
    lower-=500;upper+=500
    points=np.asarray([[x,y,z] for x in [lower[0],upper[0]]
                       for y in [lower[1],upper[1]] for z in [lower[2],upper[2]]])
    faces=[(0,1,3),(0,3,2),(4,6,7),(4,7,5),(0,4,5),(0,5,1),
           (2,3,7),(2,7,6),(0,2,6),(0,6,4),(1,5,7),(1,7,3)]
    with path.open('w') as f:
        f.write('"MESH"\n""\n""\n36\n')
        for face in faces:
            v=points[list(face)]
            n=np.cross(v[1]-v[0],v[2]-v[0]);n/=np.linalg.norm(n)
            if n@((lower+upper)/2-v.mean(axis=0))<0:
                v=v[[0,2,1]];n=-n
            for pos in v:
                f.write('\t'.join(map(str,pos))+'\n'+'\t'.join(map(str,n))+'\n0\n1\n0\t0\n')
        f.write('12\n')
        for i in range(12):f.write(f'{material}\n{i*3}\n{i*3+1}\n{i*3+2}\n')


def background_selection(package):
    """Keep finite primary-cluster bounds without exceeding global BSP limits."""
    source=source_meshes(package/'data/scenarios/multi/dust2_mombasa/structure/desert_backdrop.ass')
    next(source)
    selected=set();extrema={};offset=0
    for records,triangles in source:
        coords=np.asarray([[float(v) for v in record.splitlines()[0].split()] for record in records])
        corners=coords[triangles[:,1:]]
        for axis in range(3):
            values=corners[:,:,axis]
            for maximum in (False,True):
                at=int(values.argmax() if maximum else values.argmin())
                value=float(values.flat[at]);key=(axis,maximum)
                if key not in extrema or (value>extrema[key][0] if maximum else value<extrema[key][0]):
                    extrema[key]=(value,offset+at//3)
        offset+=len(triangles)
    if offset<=5276:
        selected.update(range(offset))
    else:
        selected.update(range(0,offset,8))
    selected.update(face for _,face in extrema.values())
    assert offset==42160 or (package/'redesign.json').is_file()
    return selected


def assemble(package,output,report,limit,precise,split_render_batches,collision_plan,primary_background):
    materials=[];objects=[];stats=[]
    selected_background=background_selection(package) if primary_background else set()
    background_at=0;primary_vertices=[];primary_triangles=[]
    with tempfile.TemporaryDirectory(prefix='dust2-ass-',dir=os.environ.get('HALO2_TEST_SCRATCH')) as owned:
        temporary=Path(owned)
        for filename,role in [('data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass','render'),
                              ('dust2_collision.ass','collision')]:
            source=source_meshes(package/filename)
            palette=next(source);offset=len(materials)
            background_materials={i for i,(name,_) in enumerate(palette)
                if any(s in name for s in ('city ec_ocean_floor','dust2 desert_sand','dust2 desert_sedimentary',
                    'zanzibar zanzibar_ground_beach','zanzibar zanzibar_cliff1'))}
            for name,variant in palette:
                if role=='render' and precise:name=name[:-1]+')"'
                if role=='collision' and '%' not in name:name=name[:-1]+'%"'
                if role=='render' and any(s in name for s in ('city ec_ocean_floor','dust2 desert_sand','dust2 desert_sedimentary',
                        'zanzibar zanzibar_ground_beach','zanzibar zanzibar_cliff1')):
                    # Kilometres of distant terrain do not need the playable
                    # streets' texel density. Keep their lightmaps bounded.
                    name=name[:-1]+' lm:0.01"'
                materials.append((name,variant))
            vertex_records=[];triangle_records=[];first=len(objects);total=0;batch_number=0;piece_number=0
            def flush():
                nonlocal batch_number,piece_number
                if not vertex_records:return
                rows=np.asarray(triangle_records,dtype=np.int64)
                # Two-sided materials create backfaces during Tool import.
                # Charge both sides to the strip budget, including mixed batches.
                weights=np.asarray([2 if '%' in materials[int(m)][0] else 1 for m in rows[:,0]])
                cumulative=np.cumsum(weights)
                parts=[];begin=0;before=0
                while begin<len(rows):
                    end=int(np.searchsorted(cumulative,before+limit,side='right'))
                    assert end>begin
                    part=rows[begin:end]
                    if role=='render' and batch_number in split_render_batches:
                        parts.extend(np.array_split(part,2))
                    else:parts.append(part)
                    begin=end;before=int(cumulative[end-1])
                pieces=[]
                for part in parts:
                    if role=='collision':
                        pieces.extend(np.array_split(part,int(collision_plan.get(str(piece_number),1))))
                        piece_number+=1
                    else:pieces.append(part)
                for part in pieces:
                    used,inverse=np.unique(part[:,1:],return_inverse=True)
                    faces=inverse.reshape(-1,3)
                    path=temporary/f'batch-{len(objects)}.ass'
                    with path.open('wb') as f:
                        f.write(f'"MESH"\n""\n""\n{len(used)}\n'.encode())
                        f.writelines(vertex_records[i] for i in used)
                        f.write(f'{len(part)}\n'.encode())
                        for m,(a,b,c) in zip(part[:,0],faces):f.write(f'{m}\n{a}\n{b}\n{c}\n'.encode())
                    objects.append((path,role))
                batch_number+=1
                vertex_records.clear();triangle_records.clear()
            for records,triangles in source:
                total+=len(triangles)
                if role=='render' and selected_background:
                    locations=np.flatnonzero(np.isin(triangles[:,0],list(background_materials)))
                    selected=np.zeros(len(triangles),dtype=bool)
                    selected[locations]=[background_at+i in selected_background for i in range(len(locations))]
                    background_at+=len(locations)
                    if selected.any():
                        part=triangles[selected]
                        used,inverse=np.unique(part[:,1:],return_inverse=True)
                        base=len(primary_vertices)
                        primary_vertices.extend(records[i] for i in used)
                        primary_triangles.extend((int(m)+offset,int(a)+base,int(b)+base,int(c)+base)
                            for m,(a,b,c) in zip(part[:,0],inverse.reshape(-1,3)))
                        triangles=triangles[~selected]
                begin=0
                while begin<len(triangles):
                    end=min(begin+limit-len(triangle_records),len(triangles))
                    group=triangles[begin:end]
                    used,inverse=np.unique(group[:,1:],return_inverse=True)
                    assert len(vertex_records)+len(used)<=60000
                    base=len(vertex_records)
                    vertex_records.extend(records[i] for i in used)
                    faces=inverse.reshape(-1,3)+base
                    triangle_records.extend((int(m)+offset,int(a),int(b),int(c))
                        for m,(a,b,c) in zip(group[:,0],faces))
                    begin=end
                    if len(triangle_records)==limit:flush()
            flush()
            stats.append(dict(role=role,triangles=total,instances=len(objects)-first,source_sha256=sha(package/filename)))
        path=temporary/'sky.ass'
        bounds=json.loads((package/'provenance.json').read_text())['render']['bounds_ass']
        shell(path,bounds,len(materials));materials.append(('"+sky"','""'))
        if primary_background:
            assert (background_at==42160 or (package/'redesign.json').is_file()) and len(primary_triangles)==len(selected_background)
            lines=path.read_bytes().splitlines(keepends=True);count=int(lines[3]);at=4+count*5
            records=[b''.join(lines[4+i*5:9+i*5]) for i in range(count)]
            triangles=np.fromstring(b''.join(lines[at+1:]).decode(),dtype=np.int64,sep=' ').reshape(-1,4)
            base=len(primary_vertices);primary_vertices.extend(records)
            primary_triangles.extend((int(m),int(a)+base,int(b)+base,int(c)+base) for m,a,b,c in triangles)
            assert len(primary_vertices)<65535 and len(primary_triangles)*3<65535
            path=temporary/'primary.ass'
            with path.open('wb') as f:
                f.write(f'"MESH"\n""\n""\n{len(primary_vertices)}\n'.encode());f.writelines(primary_vertices)
                f.write(f'{len(primary_triangles)}\n'.encode())
                for m,a,b,c in primary_triangles:f.write(f'{m}\n{a}\n{b}\n{c}\n'.encode())
        objects.append((path,'primary'))
        assert len(objects)-1<=512,'Classic Tool supports at most 512 instance definitions'
        output.parent.mkdir(parents=True,exist_ok=True)
        with output.open('w') as out:
            out.write('2\n"DUST2_MOMBASA"\n"1.0"\n"Eric"\n"LOCAL"\n'+str(len(materials))+'\n')
            for name,variant in materials:out.write(name+'\n'+variant+'\n')
            out.write(';### OBJECTS ###\n'+str(len(objects))+'\n')
            for path,_ in objects:
                with path.open() as f:shutil.copyfileobj(f,out)
            out.write(';### INSTANCES ###\n'+str(len(objects)+1)+'\n-1\n"Scene Root"\n-1\n-1\n0\n'+IDENTITY)
            for i,(_,role) in enumerate(objects):
                name='level' if role=='primary' else f'%{role}_{i}'
                out.write(f'{i}\n"{name}"\n{i+1}\n0\n0\n'+IDENTITY)
    save_json(report,dict(schema='H2_DUST2_CLASSIC_ASSEMBLY_V1',output=str(output),sha256=sha(output),
        objects=len(objects),materials=len(materials),batches=stats,max_triangles_per_instance=limit,
        triangle_corners_preserved=True,render_marked_precise=precise,sky_shell_triangles=12,
        two_sided_faces_count_against_strip_budget=True,
        collision_surface_policy=('two-sided authored solids and retail prop hulls; no Source collision reused'
            if (package/'collision-authoring.json').is_file() else
            'two-sided original Source surfaces; no filled or invented solid volume'),
        collision_split_plan=collision_plan,
        primary_background_triangles=len(selected_background),
        primary_background_strategy=('complete reduced desert terrain' if (package/'redesign.json').is_file()
            and len(selected_background)<=5276 else 'every eighth original background triangle plus six extrema; all remaining triangles instanced'),
        split_two_sided_render_batches=sorted(split_render_batches)))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package',required=True,type=Path)
    parser.add_argument('--output-ass',required=True,type=Path)
    parser.add_argument('--report-output',required=True,type=Path)
    parser.add_argument('--max-triangles',type=int,default=12000)
    parser.add_argument('--precise-render',action='store_true')
    parser.add_argument('--split-render-batch',type=int,action='append',default=[])
    parser.add_argument('--collision-split-plan',type=Path)
    parser.add_argument('--primary-background',action='store_true')
    args=parser.parse_args()
    if not 1<=args.max_triangles<=12000:raise ValueError('Invalid safe strip budget')
    plan={}
    if args.collision_split_plan:
        receipt=json.loads(args.collision_split_plan.read_text())
        assert receipt['passed'],'Cannot use a failed collision preflight'
        plan=receipt['split_plan']
        assert all(int(v) in (1,2,4,8) for v in plan.values())
    assemble(args.package,args.output_ass,args.report_output,args.max_triangles,args.precise_render,set(args.split_render_batch),plan,args.primary_background)


if __name__=='__main__':main()
