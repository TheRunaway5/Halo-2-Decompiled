#!/usr/bin/env python3
"""Author Dire Valley's native material polish in an owned H2EK overlay.

The CE world positions/normals, collision and instance transforms stay exact.
Only semantic material choices and render UVs change. Run after lighting
geometry authoring, before structure/high-lightmap compilation, via run_test.py.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct

import numpy as np
from port_dire_valley import CeCache, save, sha
from prepare_dust2_h2ek import source_meshes

SHIELD = r'scenarios\objects\covenant\military\cov_portable_shield\shaders\shield_energy'
OFFSET = r'scenarios\objects\covenant\military\cov_portable_shield\bitmaps\shield_offset'
ARROW = 'scenarios/shaders/multi/halo/beavercreek/bc_floor_arrow'


def owned(path):
    if not path.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)):
        raise ValueError('Polish writes require owned test scratch')


def author(package, kit):
    owned(package); owned(kit)
    ass = package/'data/scenarios/multi/dire_valley/structure/dire_valley.ass'
    receipt = package/'material_polish.json'
    if receipt.exists():
        result=json.loads(receipt.read_text())
        if result['ass_sha256'] != sha(ass): raise ValueError('Polished authoring changed')
        return result
    before=sha(ass)
    source=json.loads((package/'provenance.json').read_text())
    c=CeCache(Path(source['source']))
    if sha(Path(source['source'])) != source['source_sha256']: raise ValueError('CE source changed')
    def record(v):
        return ('%.9g\t%.9g\t%.9g\n%.9g\t%.9g\t%.9g\n0\n1\n%.9g\t%.9g\n'%(*(v[:3].astype(float)*100),*v[3:6],*v[12:14])).encode()
    targets={};surfaces=[]
    for lm in c.block(c.root+260,32,True):
        for at in c.block(lm+20,256,True):
            name=c.tagref(at)['name'];leaf=name.rsplit('\\',1)[-1]
            kind='arrow' if 'arrow_' in leaf else 'glyph' if any(x in leaf for x in ['decor_strip','symbol_']) else 'lamp' if 'lights_' in leaf else None
            if kind is None: continue
            n,off=c.unpack('<II',at+180);vb=c.u32(c.bsp+8)+off
            raw=np.ndarray((n,14),'<f4',c.data,vb,strides=(56,4)).copy()
            uv=raw[:,12:14];lo=uv.min(0);span=uv.max(0)-lo
            if np.any(span<1e-7): raise ValueError('Degenerate source marker UV')
            unit=(uv-lo)/span
            if kind=='arrow': new=unit
            elif kind=='glyph':
                # One complete circuit glyph from the actual Coagulation atlas,
                # with a texel gutter. Never map the whole atlas onto a sign.
                new=(np.array([87.,3.])+unit*np.array([69.,54.]))/256.
            else:
                # One complete native luminous panel. Original CE UVs selected
                # unrelated atlas cells, producing dotted door jambs.
                new=(np.array([192.,4.])+unit*np.array([45.,10.]))/256.
            for v,replacement in zip(raw,new):
                key=record(v);value=(kind,tuple(replacement))
                if key in targets and targets[key]!=value: raise ValueError('Ambiguous marker vertex')
                targets[key]=value
            surfaces.append(dict(source=name,kind=kind,vertices=n,triangles=c.u32(at+24),
                source_uv=[lo.tolist(),(lo+span).tolist()],native_uv=[new.min(0).tolist(),new.max(0).tolist()]))
    stream=source_meshes(ass);materials=list(next(stream));meshes=list(stream)
    arrow_index=len(materials);materials.append((json.dumps('dvarrow bc_floor_arrow! lm:1.0'),'""'))
    materials=[(name.replace('dv1 invalid','dv0 granite_large'),variant) for name,variant in materials]
    records,faces=meshes[0];records=list(records);faces=faces.copy();modified={};changed=0;arrows=0
    for face in faces:
        if '!' not in materials[int(face[0])][0]: continue
        original=[records[int(v)] for v in face[1:]]
        if not all(v in targets for v in original): continue
        kinds={targets[v][0] for v in original}
        if len(kinds)!=1: raise ValueError('Mixed marker triangle')
        kind=kinds.pop()
        if kind=='arrow':face[0]=arrow_index;arrows+=1
        for j,old in enumerate(original,1):
            key=(old,kind)
            if key not in modified:
                uv=targets[old][1];lines=old.splitlines(keepends=True)
                lines[4]=('%.9g\t%.9g\n'%uv).encode();modified[key]=len(records);records.append(b''.join(lines))
            face[j]=modified[key]
        changed+=1
    expected=sum(r['triangles'] for r in surfaces)
    if changed!=expected or not arrows: raise ValueError(f'Marker triangle coverage {changed}/{expected}')
    meshes[0]=(records,faces)
    # H2EK's ASS reader maps exported V to native 1-V. CE render vertices and
    # H2 SDK raw model vertices already contain native top-origin UVs. Convert
    # them once at the ASS boundary, including the newly fitted atlas cells.
    # Native SCEN trees/teleporters do not pass through this authoring path.
    for index,(vertices,triangles) in enumerate(meshes):
        converted=[]
        for vertex in vertices:
            lines=vertex.splitlines(keepends=True)
            u,v=map(float,lines[4].split())
            lines[4]=('%.9g\t%.9g\n'%(u,1.-v)).encode()
            converted.append(b''.join(lines))
        meshes[index]=(converted,triangles)
    tail=ass.read_bytes().split(b';### INSTANCES ###',1)[1]
    scenery_source=json.loads((package/'scenery_geometry.json').read_text())
    native_props=[(i,row) for i,row in enumerate(scenery_source['placement_transforms'])
                  if 'cov_portable_shield' in row['target']]
    if len(native_props)!=4:raise ValueError('Expected the four source shield placements')
    lines=tail.decode().splitlines();count=int(lines[1])
    instances=[lines[2+i*11:2+(i+1)*11] for i in range(count)]
    removed={f'"%native_h2_{i}"' for i,_ in native_props}
    instances=[row for row in instances if row[1] not in removed]
    if len(instances)!=count-4:raise ValueError('Shield BSP instances were not unique')
    for i,row in enumerate(instances):row[2]=str(i)
    tail=('\n'+str(len(instances))+'\n'+'\n'.join('\n'.join(row) for row in instances)+'\n').encode()
    with ass.open('wb') as f:
        f.write(('2\n"DIRE_VALLEY"\n"1.0"\n"Eric"\n"LOCAL"\n'+str(len(materials))+'\n').encode())
        for name,variant in materials:f.write((name+'\n'+variant+'\n').encode())
        f.write(f';### OBJECTS ###\n{len(meshes)}\n'.encode())
        for vertices,triangles in meshes:
            f.write(f'"MESH"\n""\n""\n{len(vertices)}\n'.encode());f.writelines(vertices)
            f.write(f'{len(triangles)}\n'.encode())
            for row in triangles:f.write(('\n'.join(str(int(v)) for v in row)+'\n').encode())
        f.write(b';### INSTANCES ###'+tail)
    shutil.copyfile(ass,kit/'data/scenarios/multi/dire_valley/structure/dire_valley.ass')
    collection=kit/'tags/scenarios/shaders/shader_collections.shader_collections'
    text=collection.read_text();collection.unlink()
    collection.write_text(text+'\ndvarrow scenarios\\shaders\\multi\\halo\\beavercreek\n')
    lighting_path=package/'lighting_authoring.json';lighting=json.loads(lighting_path.read_text())
    lighting.update(ass_sha256=sha(ass),production_ass_sha256=sha(ass),
        native_prop_scenery=[row for _,row in native_props],native_prop_count=4,
        bsp_prop_instances=len(instances)-2)
    save(lighting_path,lighting)
    mapping_path=package/'material_mapping.json';mapping=json.loads(mapping_path.read_text())
    for key in mapping:
        if 'arrow_' in key:mapping[key]='dvarrow bc_floor_arrow'
    save(mapping_path,mapping)
    scenery_path=package/'scenery_geometry.json';scenery=json.loads(scenery_path.read_text())
    scenery['material_overrides']={'shaders/invalid':'scenarios/objects/nature/boulders/granite/shaders/granite_large'}
    for _,row in native_props:
        rel=row['target']+'.scenery';scenery['asset_sha256'][rel]=sha(kit/'tags'/rel)
    scenery['material_mapping']={k.replace('dv1 invalid','dv0 granite_large'):v.replace('shaders/invalid','scenarios/objects/nature/boulders/granite/shaders/granite_large') for k,v in scenery['material_mapping'].items()}
    save(scenery_path,scenery)
    result=dict(schema='H2_DIRE_VALLEY_MATERIAL_POLISH_V1',ass_before_sha256=before,ass_sha256=sha(ass),
        world_positions_normals_collision_and_transforms_preserved=True,marker_surfaces=len(surfaces),
        marker_triangles=changed,native_arrow_triangles=arrows,marker_vertices=len(modified),surfaces=surfaces,
        giant_rock_placeholder_replacements=6,native_arrow_shader=ARROW,
        native_to_ass_uv='U unchanged; ASS V = 1 - native V; H2EK compiles native V = 1 - ASS V',
        native_shield_scenery=4,bsp_shield_instances_removed=4,
        shield_policy='Native scenery model/LOD/function inputs; no BSP small-instance coverage fade on energy fields',
        source_texture_policy='Only original Halo 2 bitmap pixels and native shader programs; marker UVs fit actual atlas cells.')
    save(receipt,result);return result


def repair_native_bank(bank):
    """Use the SDK-authored shield texture in its original native program slot.

    This is a custom-map asset binding correction. No shader instructions,
    blend states, masks, colors, curves, or other native owner are replaced.
    """
    owned(bank)
    library=bank/'library.json';lib=json.loads(library.read_text())
    row=next(r for r in lib['shader_tags'] if r['group']=='shad' and r['name']==SHIELD)
    payload=(bank/'payloads'/row['payload']).read_bytes();ptr=dict(row['pointers'])
    definition=ptr[36];bitmaps=ptr[definition+8]
    if struct.unpack_from('<I',payload,definition+4)[0]!=4:raise ValueError('Unexpected native shield bitmap layout')
    fix=next(r for r in row['references'] if r[0]==bitmaps+24)
    if fix[1]!=['bitm',r'rasterizer\invalid']:raise ValueError('Native shield beforeimage changed')
    fix[1]=['bitm',OFFSET]
    lib.setdefault('custom_authoring',{})['material_polish']=dict(shader=SHIELD,postprocess_bitmap_slot=2,
        before=r'rasterizer\invalid',after=OFFSET,program_payload_sha256=hashlib.sha256(payload).hexdigest(),
        authority='Installed H2EK shield_energy.shader plasma_offset_map parameter')
    save(library,lib)
    return lib['custom_authoring']['material_polish']


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package',type=Path,required=True);p.add_argument('--kit',type=Path,required=True)
    a=p.parse_args();print(json.dumps(author(a.package,a.kit)))
