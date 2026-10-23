#!/usr/bin/env python3
"""Adapt a verified Blender delivery to the native Classic compiler package.

Run through run_test.py. Preserve the delivered source and published baseline;
only a new candidate is written. Triangle data and explicit collision roles
are retained. Native compilation and visual acceptance remain separate.
"""
import argparse
import copy
import json
import os
from pathlib import Path
import shutil
import numpy as np
from PIL import Image
from build_dust2_mombasa import Glb, ass_shader_name, sha, write_ass
from prepare_dust2_h2ek import source_meshes, IDENTITY
from audit_dust2_architecture import audit
from audit_deserted_routes import audit as audit_routes


def save(path, value):
    path.write_text(json.dumps(value, indent=2)+'\n')


def stage(source, baseline, output):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not output.exists(), 'A candidate must have a new output root'
    output.mkdir();evidence=output/'evidence';evidence.mkdir()
    model=output/'model'
    shutil.copytree(baseline/'model',model)
    metadata=json.loads((source/'material_mapping.json').read_text())
    provenance=json.loads((source/'provenance.json').read_text())
    assert sha(baseline/'model/Deserted.glb')==provenance['baseline']['glb_sha256']
    assert sha(source/'Deserted-polished.glb')==metadata['source_glb_sha256']==provenance['candidate']['glb_sha256']
    palette=metadata['palette'];shared=set(metadata['shared_collision_palette_keys'])
    aliases=set(metadata['render_only_detail_palette_keys'])
    assert not aliases & shared
    g=Glb(source/'Deserted-polished.glb');rows=[]
    for material in g.doc['materials']:
        extra=material['extras'];key=extra['halo_palette']
        assert key in palette and extra['halo2_shader']==palette[key]['shader']
        assert (extra.get('halo_render_only') is True)==(key in aliases)
        rows.append(dict(palette=key))
    reconstructed=scratch/'roundtrip.ass'
    conversion=write_ass(g,reconstructed,palette,rows,shared_collision_materials=shared)
    assert sha(reconstructed)==sha(source/'Deserted-polished.ass')==provenance['candidate']['ass_sha256']
    save(evidence/'ass-source-readback.json',dict(passed=True,source_ass_sha256=sha(reconstructed),**conversion))
    for leaf in ('dust2_mombasa.glb','Deserted.glb'):
        (model/leaf).unlink();shutil.copyfile(source/'Deserted-polished.glb',model/leaf)
    shutil.copytree(source/'textures',model/'textures',dirs_exist_ok=True)
    for key,row in palette.items():
        image=Image.open(model/row['png']);row['tga']='textures/'+key+'.tga'
        image.save(model/row['tga'])
    # A source glTF color multiplier has no ASS equivalent. Author a private
    # road bitmap/shader variant; never replace the installed retail assets.
    road=next(m for m in g.doc['materials'] if m['extras']['halo_palette']=='road')
    factor=np.array(road['pbrMetallicRoughness']['baseColorFactor'][:3])
    assert np.allclose(factor,[.43,.45,.46])
    original_road=copy.deepcopy(palette['road'])
    pixels=np.array(Image.open(model/original_road['png']).convert('RGBA'))
    srgb=pixels[:,:,:3]/255.
    linear=np.where(srgb<=.04045,srgb/12.92,((srgb+.055)/1.055)**2.4)*factor
    encoded=np.where(linear<=.0031308,linear*12.92,1.055*linear**(1/2.4)-.055)
    pixels[:,:,:3]=np.rint(np.clip(encoded,0,1)*255).astype(np.uint8)
    bitmap='scenarios/multi/dust2_mombasa/bitmaps/polished_road'
    shader='scenarios/multi/dust2_mombasa/shaders/polished_road'
    bitmap_file=model/'data'/(bitmap+'.tif');bitmap_file.parent.mkdir(parents=True,exist_ok=True)
    Image.fromarray(pixels).save(bitmap_file)
    save(model/'native-material-variants.json',dict(variants=[dict(source_shader=original_road['shader'],
        shader=shader,bitmap=bitmap,source_bitmap=original_road['bitmap'],linear_base_color_factor=factor.tolist(),
        source_png_sha256=sha(model/original_road['png']),source_image_sha256=sha(bitmap_file),
        scope='Authored darker road using sRGB decode, linear source multiplier and sRGB encode; actual native appearance needs review')]))
    palette['road']['native_shader_override']=shader
    metadata['native_material_variants']='native-material-variants.json'
    save(model/'material_mapping.json',metadata)
    structure=model/'data/scenarios/multi/dust2_mombasa/structure'
    text=reconstructed.read_text();old=ass_shader_name(original_road['shader']);new=ass_shader_name(shader.replace('/','\\'))
    assert text.count('"'+old+'%"')==1
    text=text.replace('"'+old+'%"','"'+new+'%"')
    (structure/'dust2_mombasa.ass').write_text(text);(model/'Deserted.ass').write_text(text)
    prop=source/'Deserted-polished-prop-collision.ass'
    assert sha(prop)==provenance['candidate']['prop_collision_ass_sha256']
    shutil.copyfile(prop,model/'dust2_collision.ass')
    # The old prop GLB no longer represents the five translated hulls.
    (model/'dust2_collision.glb').unlink()
    shutil.copyfile(source/'palm-translation-manifest.json',model/'palm-translation-manifest.json')
    collision=json.loads((source/'collision-notes.json').read_text());save(model/'collision-authoring.json',collision)
    # Select terrain by explicit palette identity, so decorative cliff pebbles
    # cannot accidentally enter the reduced-density distant backdrop batch.
    indices={list(palette).index(k) for k in ('desert_sand','desert_rock')}
    meshes=source_meshes(structure/'dust2_mombasa.ass');materials=next(meshes);background=[];count=0
    for records,triangles in meshes:
        tri=triangles[np.isin(triangles[:,0],list(indices))]
        if not len(tri):continue
        used,inv=np.unique(tri[:,1:],return_inverse=True)
        background.append(([records[i] for i in used],np.column_stack((tri[:,0],inv.reshape(-1,3)))))
        count+=len(tri)
    assert 0<count<=5276
    with (structure/'desert_backdrop.ass').open('wb') as out:
        out.write(('2\n"DUST2_MOMBASA"\n"1.0"\n"Eric"\n"LOCAL"\n'+str(len(materials))+'\n').encode())
        for name,variant in materials:out.write((name+'\n'+variant+'\n').encode())
        out.write((';### OBJECTS ###\n'+str(len(background))+'\n').encode())
        for i,(records,tri) in enumerate(background):
            out.write((f';OBJECT {i}\n"MESH"\n""\n""\n{len(records)}\n').encode());out.writelines(records)
            out.write((str(len(tri))+'\n').encode())
            for m,a,b,c in tri:out.write(f'{m}\n{a}\n{b}\n{c}\n'.encode())
        out.write((f';### INSTANCES ###\n{len(background)+1}\n-1\n"Scene Root"\n-1\n-1\n0\n'+IDENTITY).encode())
        for i in range(len(background)):out.write((f'{i}\n"%backdrop_{i}"\n{i+1}\n0\n0\n'+IDENTITY).encode())
    (model/'desert_backdrop.glb').unlink()
    save(model/'assembly-background.json',dict(material_indices=sorted(indices),triangles=count,
        exclude_render_only_detail_indices=sorted(list(palette).index(k) for k in aliases)))
    # Rebuild the route solid from the actual delivered mesh, with wide indices.
    ps=[];ts=[];roles=[];offset=0;decorations=0
    for _,node,mesh,w in g.nodes():
        name=node.get('name','');architecture=name.startswith('architecture_')
        for prim in mesh['primitives']:
            key=rows[prim['material']]['palette']
            if name.startswith('decorative_'):
                assert key in aliases and node.get('extras',{}).get('halo_role')=='decoration'
                decorations+=g.doc['accessors'][prim['indices']]['count']//3
            if not architecture:continue
            assert key in shared and key not in aliases
            p=g.array(prim['attributes']['POSITION']).astype(float);p=p@w[:3,:3].T+w[:3,3]
            t=g.array(prim['indices'])[:,0].astype(np.int64).reshape(-1,3)
            if np.linalg.det(w[:3,:3])<0:t=t[:,[0,2,1]]
            ps.append(p[:,[2,0,1]]/3.048);ts.append(t+offset);roles.extend([list(palette).index(key)]*len(t));offset+=len(p)
    p,remap=np.unique(np.round(np.concatenate(ps),8),axis=0,return_inverse=True);t=remap[np.concatenate(ts)]
    np.savez_compressed(model/'architecture-solid.npz',positions=p,triangles=t,roles=np.array(roles))
    assert len(t)==provenance['candidate']['architecture_triangles'] and decorations==16469
    base_provenance=json.loads((baseline/'model/provenance.json').read_text())
    save(model/'provenance.json',dict(schema='H2_DESERTED_BLENDER_COMPILER_PACKAGE_V1',name='Deserted',
        blender_delivery=provenance,render=dict(conversion,bounds_ass=base_provenance['render']['bounds_ass']),
        sources=base_provenance['sources'],architecture_rebuilt=True,certified_one_to_one_runtime=False))
    save(model/'redesign.json',dict(name='Deserted',state='Blender polish staged; native compile/rebake/review pending',
        architecture_triangles=len(t),triangles=conversion['triangles'],runtime_verified=False))
    assert audit(model/'dust2_mombasa.glb',evidence/'geometry-final.json')==0
    assert audit_routes(model,evidence/'routes-final.json')==0
    prop_source=source_meshes(model/'dust2_collision.ass');next(prop_source);pp=[];tt=[];at=0
    for records,tri in prop_source:
        q=np.array([[float(x) for x in row.splitlines()[0].split()] for row in records])/100
        pp.append(q);tt.append(tri[:,1:]+at);at+=len(q)
    assert sum(map(len,tt))==766
    props=scratch/'props-route';props.mkdir();shutil.copyfile(model/'architecture-plan.json',props/'architecture-plan.json')
    np.savez_compressed(props/'architecture-solid.npz',positions=np.concatenate([p,*pp]),
        triangles=np.concatenate([t,*[x+len(p) for x in tt]]))
    assert audit_routes(props,evidence/'routes-with-props-final.json')==0
    save(evidence/'blender-staging.json',dict(passed=True,source_glb_sha256=sha(source/'Deserted-polished.glb'),
        staged_glb_sha256=sha(model/'dust2_mombasa.glb'),architectural_triangles=len(t),render_only_decoration_triangles=decorations,
        backdrop_triangles=count,prop_hull_triangles=766,compiler_sky_bounds_preserved=True,
        native_road_variant='model/native-material-variants.json',scope='Native compiler package contract and fresh source checks; compilation/rebake/playtest pending'))
    print('Blender delivery staged and source gates passed',flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('source','baseline','output'):p.add_argument('--'+key,required=True,type=Path)
    stage(**vars(p.parse_args()))
