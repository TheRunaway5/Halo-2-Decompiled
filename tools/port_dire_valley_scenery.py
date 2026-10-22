#!/usr/bin/env python3
"""Author actual H2 SDK prop meshes at Danger Canyon's source placements.

Run through tools/run_test.py. Highest intact render/collision sections become
static ASS instances. Layout and strip references: General-101's tag interface
and Halo Asset Blender toolset. No CE scenery geometry or textures are retained.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import numpy as np
from port_dire_valley import CeCache, IDENTITY, save, sha
from prepare_dust2_h2ek import source_meshes


def replacement(name):
    leaf=name.rsplit('\\',1)[-1]
    if leaf.startswith('boulder_granite_'):
        size=leaf.removeprefix('boulder_granite_').replace('gigantic','giant')
        return 'scenarios/objects/nature/boulders/granite/'+size+'/boulder_granite_'+size,'extent'
    if leaf.startswith('mp_tree_pine_'):
        model='redcedar_l' if leaf.endswith('tall') else 'redcedar_m'
        return 'scenarios/objects/nature/trees/tree_redcedar/'+model+'/'+model,'height'
    if leaf in ('mp_beacon_red','mp_beacon_blue'):
        color=leaf.removeprefix('mp_beacon_')
        return 'scenarios/objects/multi/headlong/'+color+'_bridge_light/'+color+'_bridge_light','floor-footprint'
    models={'c_storage':'objects/gear/covenant/military/crate_space/crate_space',
        'c_storage_large':'objects/gear/covenant/military/crate_space_b/crate_space_b',
        'c_uplink':'objects/gear/covenant/military/antennae_comm/antennae_comm',
        'c_field_generator':'scenarios/objects/covenant/military/cov_portable_shield/cov_portable_shield',
        'flag_base':'objects/multi/flag_base/flag_base'}
    return models[leaf],'floor-footprint' if leaf=='flag_base' else 'height'


def ce_bounds(c,obj):
    root=c.tagref(obj['offset']+40)['offset'];sections=c.block(root+208,48);chosen=set();vertices=[]
    for region in c.block(root+196,76):
        permutations=c.block(region+64,88)
        if permutations:chosen.add(next(i for i in reversed(c.unpack('<5h',permutations[0]+64)) if i>=0))
    for index in chosen:
        for at in c.block(sections[index]+36,132):
            count=c.u32(at+88);offset=c.u32(c.index+20)+c.u32(at+100);c.require(offset,count*68)
            vertices.extend(np.ndarray((count,17),'<f4',c.data,offset,strides=(68,4))[:,:3].tolist())
    p=np.asarray(vertices);return np.array([p.min(0),p.max(0)])


def rotation(q):
    x,y,z,w=q
    return np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],
        [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],
        [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])


def node_transforms(nodes):
    matrices=[]
    for node in nodes:
        m=np.eye(4);m[:3,:3]=rotation(node['default rotation']);m[:3,3]=node['default translation']
        parent=node['parent node']
        if parent>=0:
            if parent>=len(matrices):raise ValueError('H2 node hierarchy is not parent first')
            m=matrices[parent]@m
        matrices.append(m)
    return matrices


def triangulate(p,normal):
    if len(p)>3:
        quality=[]
        for start in range(len(p)):
            q=np.roll(p,-start,axis=0);a=q[1:-1]-q[0];b=q[2:]-q[0]
            quality.append(np.min(np.linalg.norm(np.cross(a,b),axis=1)/
                np.maximum(np.linalg.norm(a,axis=1)*np.linalg.norm(b,axis=1),1e-30)))
        p=np.roll(p,-int(np.argmax(quality)),axis=0)
    faces=np.array([(0,i,i+1) for i in range(1,len(p)-1)])
    if sum(np.cross(p[b]-p[a],p[d]-p[a])@normal for a,b,d in faces)<0:faces=faces[:,::-1]
    return p,faces


def port(source,package,kit,interface):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if not kit.resolve().is_relative_to(scratch):raise ValueError('Use the owned SDK overlay')
    sys.path.insert(0,str(interface.resolve()));import tag_interface as ti
    g,engine,defs,groups,ext,post,_=ti.get_version_args('halo2',0)
    c=CeCache(source);objects={t['name']:t for t in c.tags.values() if t['group']=='scen'}
    rows=json.loads((package/'source_placements.json').read_text())['scenery']
    ass=package/'data/scenarios/multi/dire_valley/structure/dire_valley.ass'
    stream=source_meshes(ass);materials=list(next(stream));base=list(stream)
    if len(base)!=2:raise ValueError('Start from the extracted world before scenery import')
    meshes=[];placements=[];inventory={};collections={};material_mapping={};tags=kit/'tags'
    with tempfile.TemporaryDirectory(dir=scratch) as temp:
        merged=g.generate_defs(defs,temp,groups,ext);asset_hashes={}
        def read(path,kind):
            full=tags/(path+'.'+kind);asset_hashes[path+'.'+kind]=sha(full)
            return ti.read_file(merged,str(tags),str(full),0,engine_tag=engine)['Data']
        def material(path,flags):
            directory,leaf=path.rsplit('/',1)
            if directory not in collections:collections[directory]='dv'+str(len(collections))
            name=collections[directory]+' '+leaf+flags
            if '!' in flags:name+=' lm:0.25'
            item=(json.dumps(name),'""')
            if item not in materials:materials.append(item)
            material_mapping[name]=path
            return materials.index(item)
        def record(p,n,uv):
            return ('%.9g\t%.9g\t%.9g\n%.9g\t%.9g\t%.9g\n0\n1\n%.9g\t%.9g\n'%(*(p*100),*n,*uv)).encode()
        for name in dict.fromkeys(row['source'] for row in rows):
            if 'teleporter_base' in name:continue
            target,policy=replacement(name);old=ce_bounds(c,objects[name]);model=read(target,'model')
            render_path=model['render model']['path'];render=read(render_path,'render_model')
            selected=set();permutations={}
            for region in render['regions']:
                perm=region['permutations'][0];permutations[region['name']]=perm['name']
                selected.add(next(perm['L'+str(i)+' section index'] for i in range(6,0,-1) if perm['L'+str(i)+' section index']>=0))
            vr=[];faces=[];render_positions=[];render_count=0;two_sided=0;normal_samples=[]
            shaders=[m['shader']['path'] for m in render['materials']]
            for index in sorted(selected):
                for data in render['sections'][index]['section data']:
                    raw=data['raw vertices'];vbase=len(vr)
                    vr.extend(record(np.asarray(v['position']),v['normal'],v['texcoord']) for v in raw)
                    render_positions.extend(v['position'] for v in raw)
                    for part in data['parts']:
                        path=shaders[part['material']];sided=any(token in path for token in ['bough','shield_energy'])
                        mat=material(path,'!)'+('%' if sided else ''))
                        ids=[v['index'] for v in data['strip indices'][part['strip start index']:part['strip start index']+part['strip length']]]
                        if part['flags']&8:
                            if len(ids)%3:raise ValueError('Malformed H2 triangle list')
                            triangles=[ids[i:i+3] for i in range(0,len(ids),3)]
                        else:triangles=[ids[i:i+3] if i%2==0 else ids[i:i+3][::-1] for i in range(len(ids)-2)]
                        for tri in triangles:
                            if len(set(tri))!=3:continue
                            if min(tri)<0 or max(tri)>=len(raw):raise ValueError('H2 model strip outside vertices')
                            a,b,d=[np.asarray(raw[i]['position']) for i in tri]
                            normal_samples.append(float(np.cross(b-a,d-a)@np.mean([raw[i]['normal'] for i in tri],axis=0)))
                            faces.append((mat,*(i+vbase for i in tri)));render_count+=1;two_sided+=int(sided)
            p=np.asarray(render_positions);new=np.array([p.min(0),p.max(0)])
            if not len(faces) or not np.isfinite(p).all():raise ValueError('H2 model lacks finite geometry')
            if sum(v>1e-10 for v in normal_samples)<sum(v< -1e-10 for v in normal_samples):
                raise ValueError('H2 strip winding differs from authored normals: '+target)
            if policy=='height':scale=float(old[1,2]/new[1,2])
            elif policy=='floor-footprint':scale=float(max(old[1,:2]-old[0,:2])/max(new[1,:2]-new[0,:2]))
            else:scale=float(max(old[1]-old[0])/max(new[1]-new[0]))
            if not 0<scale<100:raise ValueError('Invalid model fit scale')
            offset=np.zeros(3)
            if policy=='floor-footprint':offset[2]=old[0,2]-new[0,2]*scale
            collision_count=0;collision_path=model['collision model']['path']
            if collision_path:
                coll=read(collision_path,'collision_model');transforms=node_transforms(render['nodes'])
                by_name={n['name']:m for n,m in zip(render['nodes'],transforms)}
                for region in coll['regions']:
                    desired=permutations.get(region['name'])
                    perm=next((p for p in region['permutations'] if p['name']==desired),region['permutations'][0])
                    for bsp in perm['bsps']:
                        node=coll['nodes'][bsp['node index']]['name'];transform=by_name[node]
                        for si,surface in enumerate(bsp['surfaces']):
                            if surface['flags']&16:continue
                            polygon=[];edge=surface['first edge'];visited=set()
                            while edge not in visited:
                                visited.add(edge);e=bsp['edges'][edge]
                                if e['left surface']==si:point=e['start vertex'];edge=e['forward edge']
                                elif e['right surface']==si:point=e['end vertex'];edge=e['reverse edge']
                                else:raise ValueError('Native collision edge lost its surface')
                                polygon.append((transform@np.array([*bsp['vertices'][point]['point'],1]))[:3])
                            if edge!=surface['first edge'] or len(polygon)<3:raise ValueError('Invalid native collision loop')
                            plane=surface['plane'];normal=np.asarray(bsp['planes'][plane&0x7fff]['plane'][:3])*(-1 if plane&0x8000 else 1)
                            normal=transform[:3,:3]@normal;points,triangles=triangulate(np.asarray(polygon),normal)
                            mat=material(shaders[0],'@'+('%' if surface['flags']&1 else ''));vbase=len(vr)
                            vr.extend(record(point,normal,(0,0)) for point in points)
                            faces.extend((mat,*(i+vbase for i in tri)) for tri in triangles);collision_count+=len(triangles)
            mesh_index=len(base)+len(meshes);meshes.append((vr,np.asarray(faces)))
            for row in rows:
                if row['source']==name:placements.append(dict(mesh=mesh_index,source=row,scale=scale,offset=offset.tolist(),target=target))
            inventory[name]=dict(target=target,render_model=render_path,collision_model=collision_path,
                source_bounds_world=old.tolist(),native_bounds_world=new.tolist(),scale=scale,local_offset=offset.tolist(),fit=policy,
                render_triangles=render_count,two_sided_backfaces=two_sided,collision_triangles=collision_count,
                placements=sum(row['source']==name for row in rows))
        tmp=ass.with_suffix('.staged.ass')
        with tmp.open('wb') as f:
            f.write(('2\n"DIRE_VALLEY"\n"1.0"\n"Eric"\n"LOCAL"\n'+str(len(materials))+'\n').encode())
            for name,variant in materials:f.write((name+'\n'+variant+'\n').encode())
            all_meshes=base+meshes;f.write(f';### OBJECTS ###\n{len(all_meshes)}\n'.encode())
            for records,triangles in all_meshes:
                f.write(f'"MESH"\n""\n""\n{len(records)}\n'.encode());f.writelines(records)
                f.write(f'{len(triangles)}\n'.encode())
                for m,a,b,d in triangles:f.write(f'{m}\n{a}\n{b}\n{d}\n'.encode())
            f.write((f';### INSTANCES ###\n{3+len(placements)}\n-1\n"Scene Root"\n-1\n-1\n0\n'+IDENTITY+
                '0\n"level"\n1\n0\n0\n'+IDENTITY+'1\n"%source_render"\n2\n0\n0\n'+IDENTITY).encode())
            for i,item in enumerate(placements):
                row=item['source'];yaw,pitch,roll=np.asarray(row['rotation'])/2
                cr,sr=np.cos(roll),np.sin(roll);cp,sp=np.cos(pitch),np.sin(pitch);cy,sy=np.cos(yaw),np.sin(yaw)
                q=[sr*cp*cy-cr*sp*sy,-cr*sp*cy-sr*cp*sy,cr*cp*sy-sr*sp*cy,cr*cp*cy+sr*sp*sy]
                pos=np.asarray(row['position'])+rotation(q)@np.asarray(item['offset'])
                trans='\t'.join('%.9g'%v for v in pos*100);rot='\t'.join('%.9g'%v for v in q)
                f.write((f'{item["mesh"]}\n"%native_h2_{i}"\n{i+3}\n0\n0\n{rot}\n{trans}\n{item["scale"]:.9g}\n'+
                    '0\t0\t0\t1\n0\t0\t0\n1\n').encode())
        tmp.replace(ass);shutil.copyfile(ass,kit/'data/scenarios/multi/dire_valley/structure/dire_valley.ass')
        collections_file=tags/'scenarios/shaders/shader_collections.shader_collections';text=collections_file.read_text()
        for directory,alias in collections.items():text+='\n'+alias+' '+directory.replace('/','\\')+'\n'
        collections_file.unlink();collections_file.write_text(text)
        save(package/'scenery_geometry.json',dict(schema='H2_DIRE_VALLEY_NATIVE_SCENERY_V2',definitions=inventory,
            placements=len(placements),placement_transforms=placements,material_mapping=material_mapping,asset_sha256=asset_hashes,
            ass_sha256=sha(ass),default_intact_collision=True,ce_teleporter_base_excluded=True,
            ce_scenery_geometry_retained=False,native_tree_placements=33,native_rock_placements=61,
            policy='Highest intact H2 render/collision meshes authored as static instances; source positions/rotations retained, fit scales recorded. No destructible prop lifecycle.'))
        print(json.dumps(dict(definitions=len(meshes),placements=len(placements),materials=len(materials),source='Actual Halo 2 SDK models')),flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for flag in ['source','package','kit','interface']:p.add_argument('--'+flag,required=True,type=Path)
    port(**vars(p.parse_args()))
