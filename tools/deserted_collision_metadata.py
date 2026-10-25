"""Source-bound Deserted collision/render reference authoring.

The ASS authors real shared render/collision owners. This module writes only
render leaves and surface references after Classic lightmapping. Original
collision, geometry, indices, material/lightmap fields and scenery stay exact.
Qualifier exercises the unchanged native collision/attribution implementations.
"""
import copy
from collections import defaultdict
import numpy as np
import base64
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

_LOADED_HELPER_SHA256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()


def _sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def _semantic(value):
    if isinstance(value,dict):
        return {k:_semantic(v) for k,v in value.items() if not k.startswith(('TagBlock','StructHeader'))}
    if isinstance(value,list):return [_semantic(v) for v in value]
    return value


def _stable(value):
    return json.dumps(_semantic(value),sort_keys=True,separators=(',',':'))


def _canonical(definition,name):
    def flat(value):
        if isinstance(value,dict):
            if 'value' in value:return [value['value']]
            return [v for k,x in value.items() if not k.startswith(('TagBlock','StructHeader')) for v in flat(x)]
        if isinstance(value,(list,tuple)):return [v for x in value for v in flat(x)]
        assert isinstance(value,(int,float)),(type(value),value)
        return [value]
    fields={'planes':'planes','collision_leaves':'leaves','refs2':'bsp2d references','nodes2':'bsp2d nodes','surfaces':'surfaces','edges':'edges','vertices':'vertices','leaves':'render leaves','refs':'surface references'}
    row={key:[flat(x) for x in definition[field]] for key,field in fields.items()};row['name']=name;row['nodes']=[]
    for node in definition['bsp3d nodes']:
        if set(node)=={'Skip'}:
            raw=base64.b64decode(node['Skip']);assert len(raw)==8
            row['nodes'].append([int.from_bytes(raw[:2],'little'),int.from_bytes(raw[2:5],'little'),int.from_bytes(raw[5:8],'little')])
        else:row['nodes'].append(flat(node))
    row['vertices']=[p[:3] for p in row['vertices']]
    assert len(definition['render data'])==1,'Mixed source definition must have one authored section'
    section=definition['render data'][0]
    row['position']=[p['position'] for p in section['raw vertices']]
    row['indices']=[x['index']&65535 for x in section['strip indices']]
    row['parts']=[[p['strip start index']&65535,p['strip length']&65535,p['material']&65535,p['type']['value']] for p in section['parts']]
    # Source is a literal triangle list; the raw source reference offsets and
    # lightmap-triangle identity are checked again by the author below.
    assert len(row['indices'])%3==0
    for ref in row['refs']:ref[0]&=65535;ref[1]&=65535
    for leaf in row['leaves']:leaf[1]&=65535
    unsigned={'refs2':(0,1),'nodes2':(3,4),'surfaces':(0,1,2,3),'edges':(0,1,2,3),'collision_leaves':(0,1,2)}
    for field,columns in unsigned.items():
        for record in row[field]:
            for col in columns:record[col]&=(255 if (field=='surfaces' and col in (2,3)) or (field=='collision_leaves' and col in (0,1)) else 65535)
    return row


def _selection(data,package):
    package=Path(package);manifest=json.loads((package/'mixed-geometry-manifest.json').read_text())
    assert manifest['passed'] and manifest['collision_two_sided']
    architecture=json.loads((package/manifest['architecture']['report']).read_text())
    props=json.loads((package/manifest['props']['report']).read_text())
    assert architecture['collision_two_sided'] and architecture['visible_material_policy_unchanged']
    assert props['shared_palm_trunks'] and len(props['rows'])==8
    expected={'mixed_architecture_':len(architecture['pieces']),'mixed_props_':len(props['rows'])}
    groups={key:[] for key in expected};selected={};all_owners=defaultdict(list)
    for ordinal,instance in enumerate(data['instanced geometry instances']):
        di=int(instance['instance definition'])&65535;name=instance['name'];all_owners[di].append(name)
        role=next((key for key in expected if name.startswith(key)),None)
        if role is None:continue
        assert name[len(role):].isdigit(),'Mixed instance name does not match the source assembly role'
        assert instance['scale']==1 and tuple(instance['forward'])==(1,0,0) and tuple(instance['left'])==(0,1,0) and tuple(instance['up'])==(0,0,1) and tuple(instance['position'])==(0,0,0), ('Mixed source world geometry must keep its identity placement',name,{k:instance[k] for k in ('scale','forward','left','up','position')})
        policy=int(instance['lightmapping policy']['value'])
        assert policy in (0,1),'Selected source instance has an unsupported lightmapping policy'
        groups[role].append(dict(instance=ordinal,definition=di,name=name,lightmapping_policy=policy))
        selected.setdefault(di,[]).append(dict(instance=ordinal,name=name,role=role,lightmapping_policy=policy))
    assert all(len(groups[k])==expected[k] for k in groups),(expected,{k:len(v) for k,v in groups.items()})
    for di,owners in selected.items():
        assert set(all_owners[di])=={v['name'] for v in owners},'A selected definition is shared with a source-unrelated instance'
        definition=data['instanced geometries definitions'][di]
        assert definition['surfaces'] and definition['render leaves'] and definition['surface references'] and definition['render data']
    return selected,groups,manifest


def _interface(interface,temp):
    interface=Path(interface).resolve(strict=True)
    revision=subprocess.run(['git','-C',str(interface),'rev-parse','HEAD'],check=True,stdout=subprocess.PIPE,text=True,timeout=30).stdout.strip()
    assert revision=='7b01076b485cb851277c14defb71c8f88e214de4','Unqualified tag-interface revision'
    sys.path.insert(0,str(interface))
    import tag_interface as ti
    game,engine,definitions,groups,extensions,postprocess,_=ti.get_version_args('halo2',0)
    merged=game.generate_defs(definitions,str(Path(temp)/'definitions'),groups,extensions)
    return ti,engine,groups,extensions,postprocess,merged


def _bsp(kit):
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);kit=Path(kit).resolve(strict=True)
    assert kit.is_relative_to(scratch),'Saved reference authoring requires a current owned private kit'
    tags=kit/'tags';paths=list((tags/'scenarios/multi/dust2_mombasa').glob('*.scenario_structure_bsp'))
    assert len(paths)==1,'Expected the single staged Deserted BSP'
    assert paths[0].resolve(strict=True).is_relative_to(kit) and not paths[0].is_symlink()
    return tags,paths[0]


def _lightmap(ti,merged,tags,path,engine):
    paths=list(path.parent.glob('*.scenario_structure_lightmap'))
    if not paths:return None,None
    assert len(paths)==1
    data=ti.read_file(merged,str(tags),str(paths[0]),0,engine_tag=engine)['Data']
    groups=data['lightmap groups']
    return (groups[0] if groups else None),paths[0]


def _with_baked_geometry(row,group,definition):
    if not group or definition>=len(group.get('poop definitions',[])):return row
    sections=group['poop definitions'][definition]['cache data']
    if not sections or not sections[0]['raw vertices']:return row
    assert len(sections)==1
    section=sections[0];row=copy.deepcopy(row)
    row['position']=[v['position'] for v in section['raw vertices']]
    row['indices']=[v['index']&65535 for v in section['strip indices']]
    row['parts']=[[p['strip start index']&65535,p['strip length']&65535,p['material']&65535,p['type']['value']] for p in section['parts']]
    return row


def _source_lighting_categories(data,selected,group):
    """Admit actual pervertex instances only with their complete source bucket.

    Reference authoring is independent of pixel/vertex lighting. This check
    does not convert policies, synthesize atlas ownership, or change payloads.
    """
    result=[]
    for di,owners in sorted(selected.items()):
        for owner in owners:
            policy=owner['lightmapping_policy']
            record=dict(instance=owner['instance'],definition=di,name=owner['name'],policy=policy)
            if policy==1:
                assert group is not None,'Pervertex source owner has no saved lightmap group'
                refs=group['instance bucket refs'];buckets=group['geometry buckets']
                assert len(refs)==len(data['instanced geometry instances'])
                ref=refs[owner['instance']];bi=int(ref['bucket index']);assert 0<=bi<len(buckets)
                offsets=ref['section offsets'];assert len(offsets)==1
                first=int(offsets[0]['section offset'])&65535;assert first!=65535
                bucket=buckets[bi];assert int(bucket['flags'])&3==3,'Pervertex owner requires incident and radiance streams'
                sections=group['poop definitions'][di]['cache data'];assert len(sections)==1
                count=len(sections[0]['raw vertices']);assert count>0
                raw=bucket['raw vertices'];assert first+count<=len(raw)
                incident=np.asarray([v['primary lightmap incident direction'] for v in raw[first:first+count]],dtype='<f4')
                colors=np.asarray([[v['primary lightmap color'][k] for k in ('R','G','B')] for v in raw[first:first+count]],dtype='<f4')
                assert incident.shape==colors.shape==(count,3) and np.isfinite(incident).all() and np.isfinite(colors).all()
                assert ((colors>=0)&(colors<=1)).all(),'Pervertex source RGB exceeds its normalized color domain'
                record.update(bucket=bi,section_offset=first,vertices=count,bucket_flags=int(bucket['flags']),incident_float3_sha256=hashlib.sha256(incident.tobytes()).hexdigest(),source_color_float3_sha256=hashlib.sha256(colors.tobytes()).hexdigest(),source_color_minimum=colors.min(0).tolist(),source_color_maximum=colors.max(0).tolist(),source_payload_complete=True)
            result.append(record)
    return result


def author_saved_references(kit,interface,package,report_path):
    """Atomically author source-bound search metadata, preserving all other data."""
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    tags,path=_bsp(kit);before_hash=_sha(path)
    with tempfile.TemporaryDirectory(prefix='deserted-references-',dir=scratch) as temporary:
        ti,engine,groups,extensions,postprocess,merged=_interface(interface,temporary)
        document=ti.read_file(merged,str(tags),str(path),0,engine_tag=engine);original=copy.deepcopy(document['Data'])
        selected,instances,manifest=_selection(original,package);ids=sorted(selected)
        lightmap,lightmap_path=_lightmap(ti,merged,tags,path,engine)
        lighting_categories=_source_lighting_categories(original,selected,lightmap)
        lightmap_hash=_sha(lightmap_path) if lightmap_path else None
        rows=[_with_baked_geometry(_canonical(original['instanced geometries definitions'][di],','.join(x['name'] for x in selected[di])),lightmap,di) for di in ids]
        bound,bindings=bind_existing_references(rows)
        authored,changes=author_leaf_references(bound,common_area=True)
        for di,row in zip(ids,authored):
            definition=document['Data']['instanced geometries definitions'][di]
            definition['render leaves']=[{'cluster':int(cluster),'surface reference count':int(count),'first surface reference index':int(first)} for cluster,count,first in row['leaves']]
            definition['surface references']=[{'strip index':int(strip) if strip<32768 else int(strip)-65536,'lightmap triangle index':int(triangle) if triangle<32768 else int(triangle)-65536,'bsp node index':int(node)} for strip,triangle,node in row['refs']]
        protected=copy.deepcopy(document['Data'])
        for di in ids:
            for key in ('render leaves','surface references'):
                protected['instanced geometries definitions'][di][key]=original['instanced geometries definitions'][di][key]
        assert _stable(protected)==_stable(original),'Reference authoring changed a protected saved field'
        stage=Path(temporary)/path.name
        ti.write_file(merged,document,ti.obfuscation_buffer_prepare(),str(stage),engine_tag=engine,tag_groups=groups,tag_extensions=extensions,postprocess_functions=postprocess)
        back=ti.read_file(merged,str(tags),str(stage),0,engine_tag=engine)['Data']
        assert _stable(back)==_stable(document['Data']),'Typed saved-BSP reference roundtrip changed authored or protected fields'
        for di,row in zip(ids,authored):assert _stable(_with_baked_geometry(_canonical(back['instanced geometries definitions'][di],row['name']),lightmap,di))==_stable(row)
        # Never modify an installed/shared hard-link through its inode.
        replacement=path.with_name(path.name+'.deserted-reference-stage');replacement.write_bytes(stage.read_bytes());replacement.replace(path)
        assert (not lightmap_path) or _sha(lightmap_path)==lightmap_hash,'Reference authoring changed source lighting'
    summary=dict(schema='DESERTED_SAVED_COLLISION_REFERENCES_V1',passed=True,helper_sha256=_LOADED_HELPER_SHA256,source_bsp_sha256=before_hash,authored_bsp_sha256=_sha(path),bsp_path=str(path),mixed_manifest_sha256=_sha(Path(package)/'mixed-geometry-manifest.json'),selected_definitions=ids,source_instances=instances,definition_deduplication_supported=True,only_render_leaves_and_surface_references_changed=True,all_geometry_collision_bsp2d_bsp3d_material_lighting_and_nonselected_fields_exact=True,typed_readback_exact=True,source_geometry_lightmap_triangle_identity_exact=True,definitions=[dict(definition=di,source_names=[x['name'] for x in selected[di]],valid_original_reference_nodes_preserved=bindings[i]['valid_original_nodes_preserved'],existing_nodes_bound=bindings[i]['qualified_bindings'],added_references=changes[i]['added_references'],common_area=changes[i]['common_area_stats']) for i,di in enumerate(ids)],owned_scratch=str(scratch),compiled_and_native_qualification_pending=True)
    summary.update(source_lighting_categories=lighting_categories,source_lightmap_sha256=lightmap_hash,all_source_lighting_policies_and_payloads_unchanged=True)
    Path(report_path).write_text(json.dumps(summary,indent=2)+'\n')
    return summary


def _reference_block(cache,at,expected):
    # load_bsp_surface_lighting admits up to one million source references.
    # The generic inspection helper's 65,536-element default does not apply
    # to this long-indexed array. Require the exact authenticated saved count.
    count=cache.u32(at);assert count==expected and 0<=count<=1_000_000
    if not count:return []
    base=cache.pointer(cache.u32(at+4),cache.magic,count*8)
    return range(base,base+count*8,8)


def _compiled_rows(cache,source,selected):
    bsp=next(t for t in cache.tags.values() if t['group']=='sbsp');defs=cache.block(bsp['offset']+312,200);instances=cache.block(bsp['offset']+320,88)
    count,size,indices,strings=cache.u32(48),cache.u32(56),cache.u32(60),cache.u32(52)
    def name(value):
        index=value&0xffffff;assert index<count
        off=cache.u32(indices+index*4);assert off<size
        return cache.string(strings+off,size-off)
    names=defaultdict(list)
    for i,at in enumerate(instances):names[name(cache.u32(at+80))].append((i,at))
    assert len(instances)==len(source['instanced geometry instances']), 'Compiled source instance count changed'
    mappings={};rows=[];streams_by_definition={}
    for di in sorted(selected):
        owners=selected[di];compiled_ids=set()
        for owner in owners:
            matches=names[owner['name']];assert len(matches)==1, ('Selected source instance name is not unique in compiled cache',owner['name'],matches)
            ii,at=matches[0];compiled_ids.add(cache.u16(at+52));raw=source['instanced geometry instances'][owner['instance']]
            transform=np.asarray([raw['scale'],*raw['forward'],*raw['left'],*raw['up'],*raw['position']],dtype='<f4').tobytes()
            assert bytes(cache.data[at:at+52])==transform and cache.u16(at+86)==raw['lightmapping policy']['value']
        assert len(compiled_ids)==1;ci=compiled_ids.pop();mappings[di]=ci;at=defs[ci]
        base=cache.u32(at+40)+cache.u32(at+48)+8;resources=[(cache.u16(r+4),cache.u16(r+6),base+cache.u32(r+12),cache.u32(r+8)) for r in cache.block(at+56,16)];streams={(a,b):(p,n) for a,b,p,n in resources};streams_by_definition[di]=streams
        p,n=streams[(56,0)];positions=np.frombuffer(cache.data,dtype='<f4',offset=p,count=n//4).reshape(-1,3).copy().tolist();ix=next(r for r in resources if r[0]==32);indices2=np.frombuffer(cache.data,dtype='<u2',offset=ix[2],count=ix[3]//2).copy().tolist();pr=resources[0];parts=[[cache.u16(p+6),cache.u16(p+8),cache.u16(p+4),cache.u16(p)] for p in range(pr[2],pr[2]+pr[3],72)];c=at+0x70
        def block(offset,stride,fmt):return [list(struct.unpack_from(fmt,cache.data,p)) for p in cache.block(c+offset,stride)]
        nodes=[[cache.u16(p),int.from_bytes(cache.data[p+2:p+5],'little'),int.from_bytes(cache.data[p+5:p+8],'little')] for p in cache.block(c,8)]
        rows.append(dict(name=','.join(o['name'] for o in owners),position=positions,indices=indices2,parts=parts,nodes=nodes,planes=block(8,16,'<4f'),collision_leaves=block(16,4,'<BBH'),refs2=block(24,4,'<HH'),nodes2=block(32,16,'<3fHH'),surfaces=block(40,8,'<HHBBh'),edges=block(48,12,'<4H2h'),vertices=[v[:3] for v in block(56,16,'<3fI')],leaves=[list(struct.unpack_from('<hHI',cache.data,p)) for p in cache.block(at+0xb8,8)],refs=[list(struct.unpack_from('<HHi',cache.data,p)) for p in _reference_block(cache,at+0xc0,len(source['instanced geometries definitions'][di]['surface references']))]))
    return rows,mappings,streams_by_definition


def _compiled_lighting_categories(cache,data,group,categories):
    """Compare real pervertex bindings/streams to their saved source owners."""
    wanted=[r for r in categories if r['policy']==1]
    if not wanted:return []
    bsp=next(t for t in cache.tags.values() if t['group']=='sbsp')
    ltmp=next(t for t in cache.tags.values() if t['group']=='ltmp')
    groups=cache.block(ltmp['offset']+128,104);assert len(groups)==1
    root=groups[0];assert cache.u32(root+4)==cache.u32(bsp['offset']+8)
    refs=cache.block(root+80,12);buckets=cache.block(root+64,56);instances=cache.block(bsp['offset']+320,88)
    count,size,index_table,strings=cache.u32(48),cache.u32(56),cache.u32(60),cache.u32(52)
    names=defaultdict(list)
    for i,at in enumerate(instances):
        sid=cache.u32(at+80)&0xffffff;assert sid<count
        off=cache.u32(index_table+sid*4);assert off<size
        names[cache.string(strings+off,size-off)].append((i,at))
    result=[]
    for record in wanted:
        matches=names[record['name']];assert len(matches)==1
        ii,at=matches[0];assert cache.u16(at+86)==1
        ref=refs[ii];bi=struct.unpack_from('<h',cache.data,ref+2)[0]
        offsets=[cache.u16(x) for x in cache.block(ref+4,2)]
        assert bi==record['bucket'] and offsets==[record['section_offset']]
        bucket=buckets[bi];assert cache.u16(bucket)==record['bucket_flags']
        base=cache.u32(bucket+12)+cache.u32(bucket+20)+8;streams={};declarations=[]
        for resource in cache.block(bucket+28,16):
            if cache.u16(resource+4)!=0:continue
            if int(cache.data[resource])==0 and cache.u16(resource+6)==32:
                off=base+cache.u32(resource+12);length=cache.u32(resource+8);cache.require(off,length)
                declarations.append(bytes(cache.data[off:off+length]));continue
            if int(cache.data[resource])!=2:continue
            channel=cache.u16(resource+6);assert channel not in streams
            off=base+cache.u32(resource+12);length=cache.u32(resource+8);cache.require(off,length)
            streams[channel]=bytes(cache.data[off:off+length])
        assert 0 in streams and 1 in streams
        assert len(declarations)==1 and len(declarations[0])==64
        assert declarations[0][0:2]==bytes((47,12)) and declarations[0][32:34]==bytes((46,4)), 'Unsupported original PC lightmap vertex declarations'
        incident=np.frombuffer(streams[0],'<f4').reshape(-1,3)
        rgba=np.frombuffer(streams[1],np.uint8).reshape(-1,4)
        first=record['section_offset'];end=first+record['vertices']
        assert len(incident)==len(rgba) and end<=len(incident)
        raw=group['geometry buckets'][record['bucket']]['raw vertices'][first:end]
        source_incident=np.asarray([v['primary lightmap incident direction'] for v in raw],dtype='<f4')
        source_color=np.asarray([[v['primary lightmap color'][k] for k in ('R','G','B')] for v in raw],dtype=np.float32)
        # Actual installed Classic cache-only oracle on all26 v7 vertices
        # proves truncating FLOAT32*255 RGB8 conversion, including values
        # which disagree with round-to-nearest. Alpha is not a light channel.
        expected_rgb=np.floor(source_color*np.float32(255)).astype(np.uint8)
        assert incident[first:end].tobytes()==source_incident.tobytes(),'Pervertex incident bytes changed while caching'
        assert np.array_equal(rgba[first:end,:3],expected_rgb),'Pervertex RGB8 differs from authenticated Classic source conversion'
        result.append(dict(**record,compiled_instance=ii,incident_float3_exact=True,radiance_rgb8_exact=True,radiance_conversion='floor(float32(sourceRGB)*float32(255))',incident_resource_sha256=hashlib.sha256(streams[0]).hexdigest(),radiance_resource_rgba8_sha256=hashlib.sha256(streams[1]).hexdigest()))
    return result


def qualify_compiled_references(kit,interface,package,cache_path,report_path):
    """Read back actual saved→cache ownership and exercise original native rays."""
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);tags,path=_bsp(kit)
    from mcc_content import MccCache
    with tempfile.TemporaryDirectory(prefix='deserted-reference-cache-',dir=scratch) as temporary:
        ti,engine,groups,extensions,postprocess,merged=_interface(interface,temporary)
        data=ti.read_file(merged,str(tags),str(path),0,engine_tag=engine)['Data'];selected,owners,manifest=_selection(data,package)
        lightmap,lightmap_path=_lightmap(ti,merged,tags,path,engine)
        lighting_categories=_source_lighting_categories(data,selected,lightmap)
        expected=[_with_baked_geometry(_canonical(data['instanced geometries definitions'][di],','.join(o['name'] for o in selected[di])),lightmap,di) for di in sorted(selected)]
        cache=MccCache(cache_path)
        try:
            actual,mappings,streams=_compiled_rows(cache,data,selected);comparisons=[]
            compiled_lighting_categories=_compiled_lighting_categories(cache,data,lightmap,lighting_categories)
            # The source is signed short where the runtime is unsigned; all
            # other numeric values are identical serialized source fields.
            unsigned={'refs2':(0,1),'nodes2':(3,4),'surfaces':(0,1,2,3),'edges':(0,1,2,3),'collision_leaves':(0,1,2)}
            for row in expected:
                for field,columns in unsigned.items():
                    for record in row[field]:
                        for col in columns:record[col]&=(255 if (field=='surfaces' and col in (2,3)) or (field=='collision_leaves' and col in (0,1)) else 65535)
            for row,back,di in zip(expected,actual,sorted(selected)):
                assert _stable(row)==_stable(back),('Compiled collision/render/reference readback mismatch',di,[k for k in row if _stable(row[k])!=_stable(back[k])])
                attributes={};raw=(lightmap['poop definitions'][di]['cache data'][0]['raw vertices'] if lightmap and lightmap['poop definitions'][di]['cache data'] else data['instanced geometries definitions'][di]['render data'][0]['raw vertices'])
                for key,field,columns in [((56,0),'position',3),((56,1),'texcoord',2),((56,3),'primary lightmap texcoord',2)]:
                    if key not in streams[di]:
                        assert key==(56,3) and all(o['lightmapping_policy']==1 for o in selected[di])
                        assert not np.asarray([v[field] for v in raw],dtype=np.float32).any(),'Missing pervertex UV stream has nonzero source UVs'
                        attributes[field]=dict(storage='Absent in compiled pervertex section',source_values_all_zero=True,policy_preserved=True)
                        continue
                    off,size=streams[di][key];stored=bytes(cache.data[off:off+size]);source=np.asarray([v[field] for v in raw],dtype='<f4').tobytes();assert stored==source,('Compiled vertex attribute differs',di,field);attributes[field]=hashlib.sha256(stored).hexdigest()
                if (56,4) in streams[di]:
                    off,size=streams[di][(56,4)];stored=bytes(cache.data[off:off+size]);source=np.asarray([v['primary lightmap incident direction'] for v in raw],dtype='<f4').tobytes();assert stored==source, ('Compiled incident vertex stream differs',di);attributes['geometry_incident_sha256']=hashlib.sha256(stored).hexdigest()
                off,size=streams[di][(56,2)];basis=np.frombuffer(cache.data,dtype='<f4',offset=off,count=size//4).reshape(-1,3,3).copy();source_basis={name:np.asarray([v[name] for v in raw],dtype='<f4') for name in ('normal','binormal','tangent')};matches=[[name for name,v in source_basis.items() if np.array_equal(v,basis[:,slot])] for slot in range(3)];assert all(matches) and all(any(name in m for m in matches) for name in source_basis),('Basis vectors changed during cache compilation',di,matches);attributes['basis_slot_source_fields']=matches;attributes['basis_sha256']=hashlib.sha256(basis.tobytes()).hexdigest()
                comparisons.append(dict(source_definition=di,compiled_definition=mappings[di],all_canonical_geometry_collision_indices_parts_and_references_exact=True,vertex_attributes=attributes))
        finally:cache.close()
        routes_path=Path(package).parent/'evidence/routes-final.json';routes=json.loads(routes_path.read_text());assert routes['passed'];queries=[]
        for route in routes['routes']:
            for points in route['paths']:
                for p in points:queries.append(dict(origin=[p[0],p[1],p[2]+.3625],delta=[0,0,-10],group=route['name']))
        assert queries
        fixture=Path(temporary)/'banks.json';fixture.write_text(json.dumps(actual,separators=(',',':')));query_path=Path(temporary)/'queries.json';query_path.write_text(json.dumps(queries,separators=(',',':')));probe=Path(temporary)/'probe.cpp';probe.write_text(_NATIVE_CONTACT_DRIVER);exe=Path(temporary)/'probe';repo=Path(__file__).resolve().parents[1]
        command=['c++','-std=c++20','-O1','-ffunction-sections','-fdata-sections','-fno-fast-math','-ffp-contract=off','-I',str(repo/'include'),str(probe),str(repo/'src/rendering/world/bsp_collision_vector.cpp'),str(repo/'src/rendering/world/bsp_surface_lighting.cpp'),'-Wl,--gc-sections','-o',str(exe)];subprocess.run(command,check=True,timeout=180)
        run=subprocess.run([str(exe),str(fixture),'.75','combined',str(query_path)],check=True,stdout=subprocess.PIPE,text=True,timeout=120);contacts=json.loads(run.stdout)
        assert contacts['rays']==contacts['hits']==len(queries) and contacts['unattributed']==0 and contacts['ambiguous_attribution_ties']==0,contacts
    report=dict(schema='DESERTED_COMPILED_COLLISION_REFERENCES_V1',passed=True,helper_sha256=_LOADED_HELPER_SHA256,native_probe_sha256=hashlib.sha256(_NATIVE_CONTACT_DRIVER.encode()).hexdigest(),source_bsp_sha256=_sha(path),cache_sha256=_sha(cache_path),source_ltmp_sha256=_sha(lightmap_path) if lightmap_path else None,instance_identity_and_transforms_exact=True,source_instance_groups=owners,definitions=comparisons,combined_native_route_contacts=contacts,all_source_geometry_collision_bsp2d_bsp3d_materials_and_references_exact=True,only_authorized_saved_reference_fields_changed=True,owned_scratch=str(scratch),scope='Actual compiled cache compared to typed saved BSP/LTMP; unchanged original native BSP vector and render attribution on every source-author route origin with explicit.3625WU diagnostic offset. Native actor/light-sphere scheduling, MOPP callback order and visible weapon/full-map review remain separate.')
    report.update(source_lighting_categories=lighting_categories,compiled_pervertex_lighting=compiled_lighting_categories,all_source_lighting_policies_and_payloads_preserved=True)
    Path(report_path).write_text(json.dumps(report,indent=2)+'\n');return report


def bind_existing_references(rows, max_plane_distance=2e-4):
    assert 0<max_plane_distance<=2e-4
    result=copy.deepcopy(rows);reports=[]
    for data in result:
        nodes=data['nodes'];planes=np.asarray(data['planes'],dtype=float)
        positions=np.asarray(data['position'],dtype=float)
        indices=np.asarray(data['indices'],dtype=int)
        leaf_paths=defaultdict(set);stack=[(0,frozenset())];visits=0
        while stack:
            node,path=stack.pop();node=int(node)&0xffffff;visits+=1
            assert visits<=max(1,len(nodes))*16,'Unbounded or cyclic BSP tree'
            if node==0xffffff:continue
            if node&0x800000:
                leaf_paths[node&0x7fffff].update(path);continue
            assert node<len(nodes) and node not in path,'Invalid BSP tree'
            _,negative,positive=nodes[node]
            new_path=path|{node}
            stack.extend(((negative,new_path),(positive,new_path)))
        owners=defaultdict(list)
        for li,(_,count,first) in enumerate(data['leaves']):
            assert first>=0 and first+count<=len(data['refs'])
            for ri in range(first,first+count):owners[ri].append(li)
        changed=[];unresolved=defaultdict(int);preserved=0
        for ri,ref in enumerate(data['refs']):
            if 0<=ref[2]<len(nodes):preserved+=1;continue
            if not owners[ri]:unresolved['no_original_leaf_membership']+=1;continue
            strip=int(ref[0])
            if strip<0 or strip+2>=len(indices):unresolved['invalid_original_strip']+=1;continue
            vi=indices[strip:strip+3]
            if (vi<0).any() or (vi>=len(positions)).any():unresolved['invalid_original_vertex']+=1;continue
            tri=positions[vi];normal=np.cross(tri[1]-tri[0],tri[2]-tri[0]);length=np.linalg.norm(normal)
            if length<=1e-14:unresolved['degenerate_render_triangle']+=1;continue
            normal/=length
            memberships=[]
            for leaf in owners[ri]:
                by_plane=defaultdict(list)
                for node in leaf_paths[leaf]:by_plane[int(nodes[node][0])].append(node)
                memberships.append(by_plane)
            common=set(memberships[0])
            for membership in memberships[1:]:common.intersection_update(membership)
            candidates=[]
            for pi in sorted(common):
                if not 0<=pi<len(planes):continue
                p=planes[pi];size=np.linalg.norm(p[:3])
                if size<=1e-14:continue
                direction=p[:3]/size
                if abs(float(normal@direction))<.99999:continue
                error=float(np.max(np.abs(tri@p[:3]-p[3]))/size)
                if error>max_plane_distance:continue
                # Keep the node on every owner path; geometrically duplicate
                # nodes with the same plane index are interchangeable only
                # after their membership is independently established.
                common_nodes=set(memberships[0][pi])
                for membership in memberships[1:]:common_nodes.intersection_update(membership[pi])
                if common_nodes:candidates.append((pi,min(common_nodes),error))
            if len(candidates)!=1:
                unresolved['no_exact_coplanar_owner' if not candidates else 'ambiguous_plane_owner']+=1;continue
            plane,node,error=candidates[0];before=int(ref[2]);ref[2]=node
            changed.append(dict(reference=ri,old_node=before,node=node,plane=plane,
                leaves=owners[ri],maximum_corner_plane_error_wu=error))
        reports.append(dict(name=data['name'],references=len(data['refs']),
            valid_original_nodes_preserved=preserved,qualified_bindings=len(changed),
            unresolved=dict(unresolved),bindings=changed,
            maximum_plane_distance_wu=max_plane_distance,
            source_geometry_strips_leaf_ranges_lightmap_triangle_indices_exact=True,
            native_contact_qualification_required=True))
    return result,reports


def _common_area_collision_points(triangle, polygon, plane):
    """Exact projected convex intersection lifted to the real collision plane.

    No polygon expansion is permitted. A line/point contact is not an area
    owner, and a nonconvex saved collision polygon is left unresolved.
    """
    normal=np.asarray(plane[:3],float);dominant=int(np.argmax(np.abs(normal)))
    axes=[i for i in range(3) if i!=dominant]
    boundary=np.asarray(polygon,float)[:,axes]
    signed=sum(float(boundary[i,0]*boundary[(i+1)%len(boundary),1]-
                     boundary[(i+1)%len(boundary),0]*boundary[i,1])
               for i in range(len(boundary)))
    if signed==0:return None
    winding=1 if signed>0 else -1
    # Float64 round-off allowance only; it never moves a clipping boundary.
    for i in range(len(boundary)):
        a,b=boundary[i],boundary[(i+1)%len(boundary)]
        cross=(b[0]-a[0])*(boundary[:,1]-a[1])-(b[1]-a[1])*(boundary[:,0]-a[0])
        if np.min(winding*cross)<-1e-10:return None
    points=np.asarray(triangle,float)[:,axes].copy()
    for i in range(len(boundary)):
        a,b=boundary[i],boundary[(i+1)%len(boundary)]
        edge=b-a;result=[]
        for p,q in zip(points,np.roll(points,-1,axis=0)):
            dp=winding*(edge[0]*(p[1]-a[1])-edge[1]*(p[0]-a[0]))
            dq=winding*(edge[0]*(q[1]-a[1])-edge[1]*(q[0]-a[0]))
            if dp>=0:result.append(p)
            if (dp>=0)!=(dq>=0):result.append(p+(q-p)*(dp/(dp-dq)))
        if len(result)<3:return None
        points=np.asarray(result)
    area=abs(sum(points[i,0]*points[(i+1)%len(points),1]-
                 points[(i+1)%len(points),0]*points[i,1] for i in range(len(points))))*.5
    if area<=1e-14:return None
    lifted=np.empty((len(points),3));lifted[:,axes]=points
    lifted[:,dominant]=(float(plane[3])-lifted[:,axes]@normal[axes])/normal[dominant]
    return lifted,area


def author_leaf_references(rows, max_plane_distance=2e-4, *, common_area=False):
    """Compile conservative leaf search lists from literal render geometry.

    Only real collision-surface planes, real ancestor nodes and actual render
    strips are admitted. Broad-phase polygon bounds may conservatively admit
    an extra search candidate; the unchanged original barycentric routine is
    the final intersection test. This produces metadata, never new geometry,
    fitted normals, lighting samples, projected triangles or replacement UVs.
    """
    result=copy.deepcopy(rows);reports=[]
    for data in result:
        nodes=data['nodes'];planes=np.asarray(data['planes'],float)
        positions=np.asarray(data['position'],float);indices=np.asarray(data['indices'],int)
        assert len(indices)%3==0
        triangles=positions[indices.reshape(-1,3)]
        normals=np.cross(triangles[:,1]-triangles[:,0],triangles[:,2]-triangles[:,0])
        norm=np.linalg.norm(normals,axis=1);normals/=np.where(norm>1e-14,norm,1)[:,None]
        bounds=np.stack((triangles.min(1),triangles.max(1)),axis=1)
        assert data['refs'] and all(int(r[0])%3==0 and int(r[1])==int(r[0])//3 for r in data['refs']),\
            'Literal source bank does not certify render-strip/lightmap-triangle identity'
        paths=defaultdict(set);stack=[(0,frozenset())];visits=0
        while stack:
            node,path=stack.pop();node=int(node)&0xffffff;visits+=1
            assert visits<=max(1,len(nodes))*16
            if node==0xffffff:continue
            if node&0x800000:paths[node&0x7fffff].update(path);continue
            assert node<len(nodes) and node not in path
            stack.extend(((nodes[node][1],path|{node}),(nodes[node][2],path|{node})))
        surface_points={};surface_memo={};plane_triangles={};overlap_memo={}
        overlap_stats=dict(qualified_pairs=0,rejected_pairs=0,maximum_local_render_plane_error_wu=0.,
                           maximum_whole_triangle_collision_plane_error_wu=0.,minimum_common_projected_area_wu2=None)
        def polygon(surface):
            if surface in surface_points:return surface_points[surface]
            at=int(data['surfaces'][surface][1]);first=at;seen=set();points=[]
            while True:
                assert at not in seen and 0<=at<len(data['edges'])
                seen.add(at);e=data['edges'][at];right=int(int(e[5])==surface)
                assert int(e[4+right])==surface
                points.append(data['vertices'][int(e[right])]);at=int(e[2+right])
                if at==first:break
            value=np.asarray(points,float);assert len(value)>=3
            surface_points[surface]=value;return value
        def surfaces(node,active=frozenset()):
            node=int(node)&0xffff
            if node==0xffff:return frozenset()
            if node&0x8000:
                surface=node&0x7fff;assert surface<len(data['surfaces']);return frozenset((surface,))
            assert node<len(data['nodes2']) and node not in active
            if node not in surface_memo:
                a=data['nodes2'][node];surface_memo[node]=surfaces(a[3],active|{node})|surfaces(a[4],active|{node})
            return surface_memo[node]
        def candidates(pi):
            if pi not in plane_triangles:
                p=planes[pi];size=np.linalg.norm(p[:3]);assert size>1e-14
                if common_area:
                    # Source-distance band plus exact local overlap. Merely
                    # touching near a perpendicular wall edge is not a common
                    # surface owner, even if a tiny projected overlap exists.
                    signed=(triangles@p[:3]-p[3])/size
                    plane_triangles[pi]=np.flatnonzero((np.max(np.abs(signed),axis=1)<.01)&(norm>1e-14))
                else:
                    selected=np.flatnonzero((np.abs(normals@(p[:3]/size))>=.99999)&(norm>1e-14))
                    error=np.max(np.abs(triangles[selected]@p[:3]-p[3]),axis=1)/size
                    plane_triangles[pi]=selected[error<=max_plane_distance]
            return plane_triangles[pi]
        def common_owner(pi,surface,ti):
            key=(pi,surface,int(ti))
            if key not in overlap_memo:
                overlap=_common_area_collision_points(triangles[ti],polygon(surface),planes[pi])
                eligible=False
                if overlap is not None:
                    points,area=overlap
                    error=float(np.max(np.abs((points-triangles[ti,0])@normals[ti])))
                    # This is sqrt(original11E800 epsilon=.0001), not a fitted
                    # geometric tolerance. The unchanged source predicate
                    # remains the final per-contact triangle test.
                    eligible=error<.01
                    if eligible:
                        overlap_stats['qualified_pairs']+=1
                        overlap_stats['maximum_local_render_plane_error_wu']=max(overlap_stats['maximum_local_render_plane_error_wu'],error)
                        whole=float(np.max(np.abs(triangles[ti]@planes[pi][:3]-planes[pi][3]))/np.linalg.norm(planes[pi][:3]))
                        overlap_stats['maximum_whole_triangle_collision_plane_error_wu']=max(overlap_stats['maximum_whole_triangle_collision_plane_error_wu'],whole)
                        prior=overlap_stats['minimum_common_projected_area_wu2']
                        overlap_stats['minimum_common_projected_area_wu2']=area if prior is None else min(prior,area)
                if not eligible:overlap_stats['rejected_pairs']+=1
                overlap_memo[key]=eligible
            return overlap_memo[key]
        old_refs=data['refs'];new_refs=[];additions=[];missing_planes=0;referenced_original=set()
        for li,leaf in enumerate(data['leaves']):
            cluster,count,first=map(int,leaf)
            assert first>=0 and first+count<=len(old_refs)
            original=[list(r) for r in old_refs[first:first+count]]
            referenced_original.update(range(first,first+count));owned={tuple(r) for r in original}
            by_plane=defaultdict(list)
            for ni in paths[li]:by_plane[int(nodes[ni][0])].append(ni)
            collision_leaf=data['collision_leaves'][li]
            rc=int(collision_leaf[1])&0xff;rf=int(collision_leaf[2])&0xffff
            assert not rc or rf+rc<=len(data['refs2']),(data['name'],li,rc,rf,len(data['refs2']))
            if not rc:
                leaf[:]=[cluster,len(original),len(new_refs)];new_refs.extend(original);continue
            added=[]
            for pi,node2 in data['refs2'][rf:rf+rc]:
                pi=int(pi)&0x7fff
                if pi not in by_plane:missing_planes+=1;continue
                assert pi<len(planes)
                real_surfaces=[s for s in surfaces(node2) if (int(data['surfaces'][s][0])&0x7fff)==pi]
                for surface in sorted(real_surfaces):
                    points=polygon(surface);lo=points.min(0);hi=points.max(0)
                    ids=candidates(pi)
                    band=.01 if common_area else max_plane_distance
                    ids=ids[np.all(bounds[ids,1]>=lo-band,axis=1)&
                            np.all(bounds[ids,0]<=hi+band,axis=1)]
                    for ti in ids:
                        if common_area and not common_owner(pi,surface,ti):continue
                        row=(int(ti)*3,int(ti),min(by_plane[pi]))
                        if row in owned:continue
                        owned.add(row);added.append(list(row))
            assert len(original)+len(added)<65536,'Original leaf count field overflow'
            leaf[:]=[cluster,len(original)+len(added),len(new_refs)]
            new_refs.extend(original);new_refs.extend(sorted(added))
            if added:additions.append(dict(leaf=li,prior_count=count,added_count=len(added)))
        # Retain any unreferenced source records as metadata history. They are
        # deliberately not assigned to an invented leaf.
        unused=[r for ri,r in enumerate(old_refs) if ri not in referenced_original]
        new_refs.extend(copy.deepcopy(unused));data['refs']=new_refs
        reports.append(dict(name=data['name'],original_references=len(old_refs),
            authored_references=len(new_refs),added_references=sum(r['added_count'] for r in additions),
            changed_leaves=additions,source_unreferenced_rows_retained=len(unused),
            collision_leaf_planes_without_ancestor=missing_planes,
            maximum_plane_distance_wu=max_plane_distance,
            common_area_mode=common_area,common_area_stats=overlap_stats if common_area else None,
            source_strip_lightmap_triangle_identity_proved_on_all_original_rows=True,
            source_planes_nodes_surfaces_geometry_strips_and_lighting_exact=True,
            rule=('Real convex collision polygon/render triangle projected intersection of positive area; every render corner lies strictly inside the source-defined .01WU collision-plane band and every collision-plane intersection vertex lies strictly within original11E800 .01WU render-plane bound; real leaf-ancestor node; actual unchanged strip/lightmap-triangle identity. No expanded polygon, fitted normal threshold, or nearest triangle.' if common_area else 'Real collision polygon bounds and plane; real ancestor node; actual coplanar render triangle. Conservative broad-phase candidates are resolved by the unchanged original barycentric routine.'),
            native_contact_qualification_required=True))
    return result,reports


_NATIVE_CONTACT_DRIVER = r'''
#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/math.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <set>
using namespace halo2;using J=nlohmann::json;
Vec3 v3(const J& a){return {a[0].get<float>(),a[1].get<float>(),a[2].get<float>()};}
int main(int argc,char** argv){try{
 if(argc<2||argc>5)throw std::runtime_error("expected canonical saved-BSP fixture [grid step [down/combined [queries]]]");const float step=argc>=3?std::stof(argv[2]):.05f;const bool down_only=argc>=4&&std::string(argv[3])=="down";std::ifstream f(argv[1]);J input;f>>input;BspSurfaceLighting bank;bank.bsp=0;J output=J::array();
 for(const auto& d:input){
  BspSurfaceLighting::Geometry g;for(const auto& a:d["position"])g.positions.push_back(v3(a));
  for(const auto& a:d["indices"])g.indices.push_back(a.get<std::uint16_t>());
  for(const auto& a:d["parts"])g.parts.push_back({a[0].get<std::uint16_t>(),a[1].get<std::uint16_t>(),a[2].get<std::uint16_t>(),a[3].get<std::uint16_t>()});
  BspSurfaceLighting::CollisionBank c;
  for(const auto& a:d["nodes"]){c.nodes.push_back({std::uint16_t(a[0].get<int>()),std::uint32_t(a[1].get<int>())&0xffffffu,std::uint32_t(a[2].get<int>())&0xffffffu});c.node_planes.push_back(std::uint16_t(a[0].get<int>()));}
  for(const auto& a:d["planes"])c.planes.push_back({a[0].get<float>(),a[1].get<float>(),a[2].get<float>(),a[3].get<float>()});
  for(const auto& a:d["collision_leaves"])c.collision_leaves.push_back({std::uint8_t(a[0].get<int>()),std::uint8_t(a[1].get<int>()),std::uint16_t(a[2].get<int>())});
  for(const auto& a:d["refs2"])c.references2.push_back({std::uint16_t(a[0].get<int>()),std::uint16_t(a[1].get<int>())});
  for(const auto& a:d["nodes2"])c.nodes2.push_back({{a[0].get<float>(),a[1].get<float>()},a[2].get<float>(),{std::uint16_t(a[3].get<int>()),std::uint16_t(a[4].get<int>())}});
  for(const auto& a:d["surfaces"]){c.surfaces.push_back({std::uint16_t(a[0].get<int>()),std::uint16_t(a[1].get<int>()),std::uint8_t(a[2].get<int>()),std::uint8_t(a[3].get<int>()),std::int16_t(a[4].get<int>())});c.surface_planes.push_back(std::uint16_t(a[0].get<int>()));}
  for(const auto& a:d["edges"])c.edges.push_back({{std::uint16_t(a[0].get<int>()),std::uint16_t(a[1].get<int>())},{std::uint16_t(a[2].get<int>()),std::uint16_t(a[3].get<int>())},{std::int16_t(a[4].get<int>()),std::int16_t(a[5].get<int>())}});
  for(const auto& a:d["vertices"])c.vertices.push_back(v3(a));
  for(const auto& a:d["leaves"])c.leaves.push_back({std::int16_t(a[0].get<int>()),std::uint16_t(a[1].get<int>()),a[2].get<std::uint32_t>()});
  for(const auto& a:d["refs"])c.references.push_back({std::uint16_t(a[0].get<int>()),std::uint16_t(a[1].get<int>()),a[2].get<int>()});
  c.surface_leaves.resize(c.surfaces.size());const auto index=bank.definitions.size();bank.definitions.push_back(std::move(g));bank.definition_collision.push_back(std::move(c));BspSurfaceLighting::Instance i;i.definition=std::uint16_t(index);i.axes={Vec3{1,0,0},Vec3{0,1,0},Vec3{0,0,1}};bank.instances.push_back(i);
 }
 if(argc>=4&&std::string(argv[3])=="combined"){
  Vec3 lo{1e6f,1e6f,1e6f},hi{-1e6f,-1e6f,-1e6f};for(const auto& c:bank.definition_collision)for(auto p:c.vertices){lo=minimum(lo,p);hi=maximum(hi,p);}unsigned rays=0,hits=0,attributed=0,unattributed=0,ambiguous=0,ties=0;J failures=J::array();const int nx=std::max(2,int(std::ceil((hi.x-lo.x)/step))),ny=std::max(2,int(std::ceil((hi.y-lo.y)/step)));
  J queries=J::array(),groups=J::object();if(argc==5){std::ifstream qf(argv[4]);qf>>queries;}else for(int x=0;x<=nx;++x)for(int y=0;y<=ny;++y)queries.push_back({{"origin",{lo.x+(hi.x-lo.x)*float(x+.37f)/float(nx+1),lo.y+(hi.y-lo.y)*float(y+.61f)/float(ny+1),hi.z+.75f}},{"delta",{0,0,-(hi.z-lo.z+1.5f)}},{"group","global_grid"}});
  for(const auto& query:queries){Vec3 origin=v3(query["origin"]),ray=v3(query["delta"]);const auto group=query.value("group",std::string("ungrouped"));if(!groups.contains(group))groups[group]={{"rays",0},{"hits",0},{"attributed",0},{"unattributed",0},{"ambiguous",0}};auto& gr=groups[group];gr["rays"]=gr["rays"].get<unsigned>()+1;++rays;float closest=2;struct Hit{unsigned definition;BspVectorContact contact;Vec3 point;bool attributed;};std::vector<Hit> candidates;
   for(unsigned di=0;di<bank.definition_collision.size();++di){const auto q=original_bsp_vector_query(bank.definition_collision[di],origin,ray,1,9);if(!q.contact)continue;const auto& h=*q.contact;if(h.fraction>closest)continue;if(h.fraction<closest){closest=h.fraction;candidates.clear();}const auto p=origin+ray*h.fraction;CollisionSurfaceIdentity id{0,int(di),int(h.surface),int(di),h.oriented_plane};candidates.push_back({di,h,p,bool(bank.attribute(id,p,h.leaf))});}
   if(candidates.empty())continue;++hits;gr["hits"]=gr["hits"].get<unsigned>()+1;if(candidates.size()>1)++ties;bool all=true,any=false;for(const auto& h:candidates){all&=h.attributed;any|=h.attributed;}if(all){++attributed;gr["attributed"]=gr["attributed"].get<unsigned>()+1;}else if(any){++ambiguous;gr["ambiguous"]=gr["ambiguous"].get<unsigned>()+1;}else{++unattributed;gr["unattributed"]=gr["unattributed"].get<unsigned>()+1;}
   if(!all&&failures.size()<160){J owners=J::array();for(const auto& h:candidates)owners.push_back({{"definition",h.definition},{"surface",h.contact.surface},{"leaf",h.contact.leaf},{"point",{h.point.x,h.point.y,h.point.z}},{"attributed",h.attributed},{"leaf_reference_count",bank.definition_collision[h.definition].leaves.at(h.contact.leaf).count}});failures.push_back({{"group",group},{"origin",{origin.x,origin.y,origin.z}},{"owners",owners}});}
  }
  std::cout<<J({{"rays",rays},{"hits",hits},{"all_closest_owners_attributed",attributed},{"unattributed",unattributed},{"ambiguous_attribution_ties",ambiguous},{"geometric_ties",ties},{"groups",groups},{"failure_examples",failures}}).dump(2)<<'\n';return 0;
 }
 for(std::size_t di=0;di<input.size();++di){const auto& c=bank.definition_collision[di];Vec3 lo{1e6f,1e6f,1e6f},hi{-1e6f,-1e6f,-1e6f};for(auto p:c.vertices){lo=minimum(lo,p);hi=maximum(hi,p);}const auto get=[](Vec3 v,int k){return k==0?v.x:k==1?v.y:v.z;};const auto set=[](Vec3& v,int k,float x){if(k==0)v.x=x;else if(k==1)v.y=x;else v.z=x;};J axes=J::array(),failures=J::array();std::set<unsigned> reached,failed_surfaces;
  for(int axis=0;axis<3;++axis)for(int sign:{-1,1}){if(down_only&&(axis!=2||sign!=-1))continue;unsigned rays=0,hits=0,attributed=0;const int a=(axis+1)%3,b=(axis+2)%3;const int na=std::max(2,int(std::ceil((get(hi,a)-get(lo,a))/step))),nb=std::max(2,int(std::ceil((get(hi,b)-get(lo,b))/step)));
   for(int x=0;x<=na;++x)for(int y=0;y<=nb;++y){Vec3 origin{},ray{};set(origin,a,get(lo,a)+(get(hi,a)-get(lo,a))*float(x+.37f)/float(na+1));set(origin,b,get(lo,b)+(get(hi,b)-get(lo,b))*float(y+.61f)/float(nb+1));set(origin,axis,sign>0?get(lo,axis)-.75f:get(hi,axis)+.75f);set(ray,axis,float(sign)*(get(hi,axis)-get(lo,axis)+1.5f));++rays;auto q=original_bsp_vector_query(c,origin,ray,1,9);if(!q.contact)continue;++hits;const auto& h=*q.contact;reached.insert(h.surface);const auto p=origin+ray*h.fraction;CollisionSurfaceIdentity id{0,int(di),int(h.surface),int(di),h.oriented_plane};auto attr=bank.attribute(id,p,h.leaf);if(attr){++attributed;}else{failed_surfaces.insert(h.surface);if(failures.size()<96)failures.push_back({{"axis",axis},{"sign",sign},{"point",{p.x,p.y,p.z}},{"surface",h.surface},{"leaf",h.leaf},{"flags",h.flags},{"leaf_reference_count",c.leaves.at(h.leaf).count}});}}
   axes.push_back({{"axis",axis},{"sign",sign},{"rays",rays},{"hits",hits},{"attributed",attributed},{"failures",hits-attributed}});
  }
  output.push_back({{"definition",di},{"name",input[di]["name"]},{"axes",axes},{"reached_surfaces",reached.size()},{"unreached_surface_count",c.surfaces.size()-reached.size()},{"failed_surface_count",failed_surfaces.size()},{"failure_examples",failures}});
 }
 std::cout<<output.dump(2)<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
'''
