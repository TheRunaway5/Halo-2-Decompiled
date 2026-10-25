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


def closed_architecture(package):
    """Admit a source-bound partition of the repaired exterior volume."""
    manifest=package/'closed-architecture-manifest.json'
    source=package/'closed-architecture.ass'
    if not manifest.exists():
        assert not source.exists(),'Closed architecture requires its validation manifest'
        return None
    proof=json.loads(manifest.read_text())
    assert proof['schema']=='DESERTED_CLOSED_ARCHITECTURE_PIECES_V1'
    assert proof['passed'] and proof['ass_independent_parse']['passed']
    assert sha(source)==proof['ass_sha256']
    from deserted_architecture_digest import architecture_digest
    assert architecture_digest(package/'dust2_mombasa.glb')==proof['architecture_corner_digest'],\
        'Closed pieces do not belong to the staged visible architecture'
    mapping=json.loads((package/'material_mapping.json').read_text())
    assert list(mapping['palette'])==proof['material_palette_keys']
    shared={list(mapping['palette']).index(key) for key in mapping['shared_collision_palette_keys']}
    assert shared==set(proof['architecture_material_ids'])
    assert proof['outside_internal_cap_samples']==0
    assert proof['external_corner_provenance_failures']==0
    assert proof['source_surface_failed_samples']==0
    assert all(row['mismatch_count']==0 for row in proof['routes'])
    return dict(path=source,manifest=manifest,proof=proof,material_ids=shared)


def collision_proxy(package):
    """Admit separately compiled collision without replacing visible corners."""
    manifest=package/'collision-proxy-manifest.json'
    source=package/'collision-proxy.ass'
    if not manifest.exists():
        assert not source.exists(),'Collision proxy requires its source-bound manifest'
        return None
    proof=json.loads(manifest.read_text())
    assert proof['schema']=='DESERTED_CLOSED_COLLISION_PROXY_V1'
    assert proof['passed'] and proof['ass_independent_parse']['passed']
    assert proof['route_validation']['passed']
    assert proof['tool_receipt']['passed']
    assert len(proof['tool_receipt']['pieces'])==proof['piece_count']
    assert all(row['passed'] and row['compiled_collision']['nonempty'] and
               row['compiled_collision']['deleted_vertex_references']==0
               for row in proof['tool_receipt']['pieces'])
    assert all(row['mismatch_count']==0 for row in proof['route_validation']['routes'])
    assert proof['route_validation_boundary_band_wu']<=1e-4
    assert proof['compiled_proxy_render_and_shadow_triangle_counts_zero']
    assert all(proof['assembly_contract'][key] for key in (
        'preserve_all_original_render_triangle_corner_records',
        'architecture_visible_materials_percent_to_exclamation',
        'append_proxy_palette_independently',
        'preserve_each_proxy_object_as_one_closed_instance',
        'props_collision_and_other_render_faces_unchanged',
        'source_visible_shadow_occluder_geometry_preserved'))
    assert proof['assembly_contract']['proxy_material_flags']=='@)'
    assert not proof['assembly_contract']['proxy_render_or_lightmap_participation']
    assert sha(source)==proof['ass_sha256']
    from deserted_architecture_digest import architecture_digest
    assert architecture_digest(package/'dust2_mombasa.glb')==proof['architecture_corner_digest']
    mapping=json.loads((package/'material_mapping.json').read_text())
    assert list(mapping['palette'])==proof['material_palette_keys']
    shared={list(mapping['palette']).index(key) for key in mapping['shared_collision_palette_keys']}
    assert shared==set(proof['architecture_material_ids'])
    return dict(path=source,manifest=manifest,proof=proof,material_ids=shared)


def static_scenery(package):
    """Validate the source-bound, independently qualified permanent wreck."""
    recipe=package/'native-static-scenery.json'
    if not recipe.exists():return None
    proof=json.loads(recipe.read_text())
    assert proof['schema']=='DESERTED_NATIVE_STATIC_SCENERY_V1' and proof['passed']
    render=package/'data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass'
    assert proof['source_render_ass_sha256']==sha(render)
    assert proof['source_collision_ass_sha256']==sha(package/'dust2_collision.ass')
    paths={}
    for key in ('prefab_manifest','authoring_helper','native_pilot'):
        path=(package/proof[key]).resolve(strict=True)
        assert path.is_relative_to(package.resolve())
        assert sha(path)==proof[key+'_sha256']
        paths[key]=path
    prefab=json.loads(paths['prefab_manifest'].read_text())
    pilot=json.loads(paths['native_pilot'].read_text())
    assert prefab['schema']=='DESERTED_STATIC_WRECK_PREFAB_SOURCE_V1' and prefab['passed']
    assert pilot['schema']=='DESERTED_STATIC_WRECK_NATIVE_PILOT_V1' and pilot['passed']
    assert pilot['compiled_factory_inputs']['passed']
    source_policy=prefab['source_definition']['lightmapping_policy']
    assert source_policy in (0,2), 'Qualified wreck requires a supported authored lighting policy'
    pilot_policy=pilot['compiled_factory_inputs'].get('lightmapping_policy')
    if pilot_policy is None:
        # The frozen original dynamic pilot predates the explicit policy field.
        assert source_policy==2, 'Baked scenery requires an explicit matching native pilot policy'
    else:
        assert pilot_policy==source_policy, 'Source scenery policy differs from its actual native pilot'
    assert pilot['native_scenery_factory_and_render_resolved']
    assert pilot['collision_control_separation_wu']>2
    assert all(row['passed'] for row in pilot['native']) and len(pilot['native'])==4
    assert prefab['source_render_triangles']==1770 and prefab['source_collision_triangles']==270
    assert prefab['qualified_parent_render_ass_sha256']==proof['source_render_ass_sha256']
    assert prefab['qualified_parent_collision_ass_sha256']==proof['source_collision_ass_sha256']
    assert prefab['native_pilot']['proof_sha256']==proof['native_pilot_sha256']
    assert all(prefab['native_acceptance'].values())
    mixed_manifest=package/'mixed-geometry-manifest.json'
    assert mixed_manifest.is_file(), 'Static wreck requires a qualified BSP omission owner'
    mixed=json.loads(mixed_manifest.read_text());assert mixed['passed']
    props_report=(package/mixed['props']['report']).resolve(strict=True)
    assert props_report.is_relative_to(package.resolve())
    assert sha(props_report)==mixed['props']['report_sha256']
    props=json.loads(props_report.read_text())
    assert props['passed'] and props.get('static_wreck_omitted') is True
    assert props['excluded_scenery_render_object_ids']==proof['excluded_source_render_object_ids']
    helper=Path(__file__).resolve().parent/'deserted_static_scenery.py'
    assert sha(helper)==proof['authoring_helper_sha256']
    return dict(recipe=recipe,proof=proof,**paths)


def mixed_geometry(package, proxy=None):
    """Admit authored shared render/collision owners before Classic compilation.

    BspSurfaceLighting follows the hit collision instance's render references.
    Consequently its closed collision solid and visible exterior must belong
    to the same ASS object. The authoring proof covers exact halfspace splits;
    final compiler references and native routes remain separate required gates.
    """
    manifest=package/'mixed-geometry-manifest.json'
    if not manifest.exists():return None
    assert proxy is not None,'Mixed geometry requires its source-bound collision proxy'
    proof=json.loads(manifest.read_text())
    assert proof['schema']=='DESERTED_MIXED_GEOMETRY_V1' and proof['passed']
    render=package/'data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass'
    assert sha(render)==proof['source_render_ass_sha256']
    assert sha(package/'dust2_collision.ass')==proof['source_collision_ass_sha256']
    assert sha(proxy['manifest'])==proof['collision_proxy_manifest_sha256']
    inputs={}
    for role in ('architecture','props'):
        row=proof[role]
        path=package/row['ass'];report=package/row['report']
        assert path.resolve().is_relative_to(package.resolve())
        assert report.resolve().is_relative_to(package.resolve())
        assert sha(path)==row['ass_sha256'] and sha(report)==row['report_sha256']
        authoring=json.loads(report.read_text())
        assert authoring['passed'] and authoring['output_sha256']==sha(path)
        assert authoring['source_ass_sha256' if role=='architecture' else 'source_render_ass_sha256']==sha(render)
        if role=='architecture':
            assert authoring['proxy_ass_sha256']==proxy['proof']['ass_sha256']
            assert authoring['all_visible_source_surface_area_preserved']
            assert authoring['proxy_collision_byte_identity']
            assert set(authoring['material_ids'])==proxy['material_ids']
            assert authoring['maximum_barycentric_position_error_wu']<1e-10
            assert authoring['maximum_relative_area_delta']<=1e-7
            assert authoring.get('collision_two_sided',False)==proof.get('collision_two_sided',False)
            if proof.get('collision_two_sided'):
                assert authoring['visible_material_policy_unchanged']
                assert authoring['architecture_render_reverse_faces_requested'] is False
                assert authoring['proxy_geometry_material_ids_corner_records_indices_exact']
                proxy_palette=next(source_meshes(proxy['path']))
                assert len(authoring['collision_material_policy_delta'])==len(proxy_palette)
                for index,row in enumerate(authoring['collision_material_policy_delta']):
                    assert row['proxy_palette_index']==index and row['source']==proxy_palette[index][0]
                    assert row['authored']==row['source'][:-1]+'%"'
        else:
            assert authoring['source_collision_ass_sha256']==proof['source_collision_ass_sha256']
            omitted=bool(authoring.get('static_wreck_omitted'))
            if authoring.get('shared_palm_trunks'):
                assert authoring['all_retained_source_render_corners_normals_uv_indices_preserved' if omitted else 'all_source_render_corners_normals_uv_indices_preserved']
                assert authoring['all_retained_source_hull_geometry_preserved' if omitted else 'all_source_hull_geometry_preserved']
                assert authoring['source_hull_geometry_maximum_error_wu']<=2e-7
                # Shared visible trunk faces replace only the geometrically
                # identical collision-only duplicates. Actual Classic/native
                # reference coverage is qualified separately.
            else:
                assert authoring['all_source_corners_normals_uv_indices_preserved']
                assert authoring['all_source_hulls_preserved']
            if omitted:
                assert authoring['retained_palms_records_and_indices_exact']
                assert authoring['removed_wreck_render_triangles']==1770
                assert authoring['removed_wreck_collision_triangles']==270
                scenery=static_scenery(package)
                assert scenery is not None
                scenery_proof=scenery['proof']
                assert scenery_proof['excluded_source_render_object_ids']==authoring['excluded_scenery_render_object_ids']
        inputs[role]=dict(path=path,report=report,proof=authoring)
    if (package/'native-static-scenery.json').exists():
        assert inputs['props']['proof'].get('static_wreck_omitted') is True
    return dict(manifest=manifest,proof=proof,**inputs)


def assemble(package,output,report,limit,precise,split_render_batches,collision_plan,primary_background):
    materials=[];objects=[];stats=[]
    partition=closed_architecture(package)
    proxy=collision_proxy(package)
    mixed=mixed_geometry(package,proxy)
    assert not (partition and proxy),'Choose one architecture collision owner'
    replaced_architecture=0
    background_policy=package/'assembly-background.json'
    explicit_background=json.loads(background_policy.read_text()) if background_policy.exists() else None
    selected_background=background_selection(package) if primary_background else set()
    background_at=0;primary_vertices=[];primary_triangles=[]
    with tempfile.TemporaryDirectory(prefix='dust2-ass-',dir=os.environ.get('HALO2_TEST_SCRATCH')) as owned:
        temporary=Path(owned)
        for filename,role in [('data/scenarios/multi/dust2_mombasa/structure/dust2_mombasa.ass','render'),
                              ('dust2_collision.ass','collision')]:
            if mixed and role=='collision':continue
            source=source_meshes(package/filename)
            palette=next(source);offset=len(materials)
            background_materials={i for i,(name,_) in enumerate(palette)
                if any(s in name for s in ('city ec_ocean_floor','dust2 desert_sand','dust2 desert_sedimentary',
                    'zanzibar zanzibar_ground_beach','zanzibar zanzibar_cliff1'))}
            if role=='render' and explicit_background is not None:
                background_materials=set(explicit_background['material_indices'])
                assert all(0<=i<len(palette) for i in background_materials)
            for mi,(name,variant) in enumerate(palette):
                if role=='render' and proxy and mi in proxy['material_ids']:
                    assert '%' in name and '!' not in name and '@' not in name
                    name=name.replace('%','!')
                if role=='render' and partition and mi in partition['material_ids']:
                    assert '%' in name and '!' not in name
                    name=name.replace('%','') # Whole closed pieces own ordinary single-sided collision.
                if role=='render' and precise:name=name[:-1]+')"'
                if role=='collision' and '%' not in name:name=name[:-1]+'%"'
                if role=='render' and mi in background_materials:
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
            for source_object,(records,triangles) in enumerate(source):
                total+=len(triangles)
                if mixed:
                    props=mixed['props']['proof']
                    if source_object in props['source_render_object_ids']+props.get('excluded_scenery_render_object_ids',[]):continue
                    architecture=np.isin(triangles[:,0],list(proxy['material_ids']))
                    replaced_architecture+=int(architecture.sum())
                    triangles=triangles[~architecture]
                if role=='render' and partition:
                    architecture=np.isin(triangles[:,0],list(partition['material_ids']))
                    replaced_architecture+=int(architecture.sum())
                    triangles=triangles[~architecture]
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
            if role=='render' and partition:
                assert replaced_architecture==partition['proof']['source_triangles']
                closed_source=source_meshes(partition['path'])
                closed_palette=next(closed_source)
                assert len(closed_palette)==len(palette)
                assert [variant for _,variant in closed_palette]==[variant for _,variant in palette]
                count=0;faces=0
                for records,triangles in closed_source:
                    assert 0<len(triangles)<=limit and len(records)<65535
                    assert set(map(int,triangles[:,0]))<=partition['material_ids']
                    assert triangles[:,1:].min()>=0 and triangles[:,1:].max()<len(records)
                    path=temporary/f'closed-{count}.ass'
                    with path.open('wb') as f:
                        f.write(f'"MESH"\n""\n""\n{len(records)}\n'.encode());f.writelines(records)
                        f.write(f'{len(triangles)}\n'.encode())
                        for m,a,b,c in triangles:f.write(f'{m+offset}\n{a}\n{b}\n{c}\n'.encode())
                    objects.append((path,'architecture'))
                    count+=1;faces+=len(triangles)
                assert count==partition['proof']['piece_count']
                stats.append(dict(role='closed architecture',triangles=faces,instances=count,
                    source_sha256=partition['proof']['ass_sha256'],
                    replaced_visible_source_triangles=replaced_architecture,
                    internal_caps_are_compiler_artifacts=True))
        if mixed:
            assert replaced_architecture==mixed['architecture']['proof']['source_triangles']
            for mixed_role in ('architecture','props'):
                entry=mixed[mixed_role]
                source=source_meshes(entry['path']);palette=next(source);offset=len(materials)
                materials.extend(palette);count=0;faces=0
                for records,triangles in source:
                    assert len(records)<65535 and len(triangles)>0
                    assert triangles[:,1:].min()>=0 and triangles[:,1:].max()<len(records)
                    render_triangles=sum('@' not in palette[int(m)][0] for m in triangles[:,0])
                    assert render_triangles*3<65535,'Classic render strip/index bound'
                    path=temporary/f'mixed-{mixed_role}-{count}.ass'
                    with path.open('wb') as f:
                        f.write(f'"MESH"\n""\n""\n{len(records)}\n'.encode());f.writelines(records)
                        f.write(f'{len(triangles)}\n'.encode())
                        for m,a,b,c in triangles:f.write(f'{m+offset}\n{a}\n{b}\n{c}\n'.encode())
                    objects.append((path,'mixed_'+mixed_role));count+=1;faces+=len(triangles)
                stats.append(dict(role='shared render/collision '+mixed_role,triangles=faces,instances=count,
                    source_sha256=sha(entry['path']),authoring_report_sha256=sha(entry['report']),
                    closed_collision_and_visible_geometry_share_instance=True))
        elif proxy:
            source=source_meshes(proxy['path']);palette=next(source);offset=len(materials)
            assert len(palette)==len(proxy['proof']['material_palette_keys'])
            assert all('@' in name and ')' in name and '%' not in name and '!' not in name
                       for name,_ in palette),'Proxy must remain precise collision-only geometry'
            materials.extend(palette)
            count=0;faces=0
            for records,triangles in source:
                assert 0<len(triangles)<=limit and len(records)<65535
                assert set(map(int,triangles[:,0]))<=proxy['material_ids']
                assert triangles[:,1:].min()>=0 and triangles[:,1:].max()<len(records)
                path=temporary/f'proxy-{count}.ass'
                with path.open('wb') as f:
                    f.write(f'"MESH"\n""\n""\n{len(records)}\n'.encode());f.writelines(records)
                    f.write(f'{len(triangles)}\n'.encode())
                    for m,a,b,c in triangles:f.write(f'{m+offset}\n{a}\n{b}\n{c}\n'.encode())
                objects.append((path,'architecture_collision'));count+=1;faces+=len(triangles)
            assert count==proxy['proof']['piece_count']
            stats.append(dict(role='separate architecture collision',triangles=faces,instances=count,
                source_sha256=proxy['proof']['ass_sha256'],visible_source_corners_unchanged=True))
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
        triangle_corners_preserved=not bool(mixed),render_marked_precise=precise,sky_shell_triangles=12,
        original_surfaces_and_barycentric_attributes_preserved=bool(mixed),
        two_sided_faces_count_against_strip_budget=True,
        collision_surface_policy=('source-bound architecture solids and retail hulls grouped with their visible geometry for original surface-lighting references'
            if mixed else 'source-bound conditioned architecture collision solids and original retail prop hulls; visible shell is render-only'
            if proxy else ('two-sided authored solids and retail prop hulls; no Source collision reused'
                if (package/'collision-authoring.json').is_file() else
                'two-sided original Source surfaces; no filled or invented solid volume')),
        collision_split_plan=collision_plan,
        mixed_geometry_manifest_sha256=sha(mixed['manifest']) if mixed else None,
        source_architecture_split_by_authenticated_planes=bool(mixed),
        final_compiler_reference_and_native_route_validation_required=bool(mixed),
        architecture_collision_proxy=(dict(manifest_sha256=sha(proxy['manifest']),
            ass_sha256=proxy['proof']['ass_sha256'],source_architecture=proxy['proof']['architecture_corner_digest'],
            pieces=proxy['proof']['piece_count'],visible_geometry_render_only=True,
            proxy_geometry_collision_only=True) if proxy else None),
        closed_architecture_partition=(dict(manifest_sha256=sha(partition['manifest']),
            ass_sha256=partition['proof']['ass_sha256'],
            source_architecture=partition['proof']['architecture_corner_digest'],
            pieces=partition['proof']['piece_count'],
            source_triangles_replaced=replaced_architecture,
            single_sided_closed_pieces=True) if partition else None),
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
