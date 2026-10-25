#!/usr/bin/env python3
"""Audit placed Dire Valley render materials and their native texture owners.

Use run_test.py. This checks the real compiled BSP parts, placed scenery,
item/vehicle models, original-program bitmap remaps and actual MCC partitions.
Pixel composition, alpha behavior and lighting still require native captures.
"""
import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import struct
import numpy as np

from mcc_content import MccCache, texture_resource
from port_dire_valley import save, sha
from polish_dire_valley import SHIELD, OFFSET
from prepare_dust2_h2ek import source_meshes


class Bank:
    def __init__(self,path):
        self.path=path;self.library=json.loads((path/'library.json').read_text())
        self.rows={(r['group'],r['name']):r for r in self.library['shader_tags']};self.loaded={}
    def get(self,group,name):
        key=group,name
        if key not in self.loaded:
            r=self.rows[key];b=(self.path/'payloads'/r['payload']).read_bytes()
            if len(b)!=r['bytes'] or hashlib.sha256(b).hexdigest()+'.bin'!=r['payload']:
                raise ValueError('Native payload identity mismatch: '+name)
            self.loaded[key]=(b,dict(r['pointers']),dict(r['references']))
        return self.loaded[key]
    def block(self,row,at,stride):
        b,p,_=row;count=struct.unpack_from('<I',b,at)[0]
        if not count:return []
        base=p[at+4]
        if base+count*stride>len(b):raise ValueError('Native block outside payload')
        return [base+i*stride for i in range(count)]
    def bitmaps(self,name):
        row=self.get('shad',name);b,_,refs=row;pp=self.block(row,32,124)[0]
        stem=self.get(*refs[pp]);tp=self.block(stem,88,40)[0]
        mappings=self.block(stem,tp+32,4);slots=set()
        for impl in self.block(stem,tp+24,6):
            packed=struct.unpack_from('<H',stem[0],impl)[0];first,count=packed&511,packed>>9
            for i in mappings[first:first+count]:slots.add(struct.unpack_from('<H',stem[0],i)[0])
        images=self.block(row,pp+4,12);result=[]
        for slot in sorted(slots):
            if slot==65535:continue
            if slot>=len(images):raise ValueError('Native remap outside material bitmap array')
            at=images[slot];ref=refs.get(at);datum=struct.unpack_from('<I',b,at)[0]
            if ref is None and datum==0xffffffff:continue
            if ref is None:raise ValueError('Bitmap lacks an authenticated reference')
            result.append(dict(slot=slot,name=ref[1],frame=struct.unpack_from('<I',b,at+4)[0]))
        return result


def validate(package,report):
    c=MccCache(package/'content/halo2/Mods/dire_valley/dire_valley.map')
    bank=Bank(package/'content/native-shaders/maps/dire_valley');by_name={(t['group'],t['name']):t for t in c.tags.values()}
    bsp=next(t for t in c.tags.values() if t['group']=='sbsp');materials=c.block(bsp['offset']+164,32)
    roots=collections.defaultdict(set);part_count=0
    for offset,stride in [(156,176),(312,200)]:
        for section in c.block(bsp['offset']+offset,stride):
            if not c.u16(section):continue
            raw=c.u32(section+40)+c.u32(section+48)+8;resources=c.block(section+56,16)
            first=resources[0];base=raw+c.u32(first+12);size=c.u32(first+8)
            if size%72:raise ValueError('BSP part stride changed')
            for at in range(base,base+size,72):
                if c.u16(at) in (0,1,5) or c.u16(at+8)==0:continue
                index=c.u16(at+4)
                if index>=len(materials):raise ValueError('Visible part has no material')
                t=c.tags[c.u32(materials[index]+12)];roots[t['name']].add('world/static-props');part_count+=1
    scenario=c.tags[c.u32(c.index+12)];s=scenario['offset'];objects=[]
    palette=c.block(s+0x58,40)
    for at in c.block(s+0x50,0x5c):objects.append(c.tags[c.u32(palette[c.u16(at)]+4)])
    equipment=c.block(s+0x120,0x90)
    for at in equipment:
        t=c.tags[c.u32(at+0x5c)]
        for item in c.block(t['offset'],16):objects.append(c.tags[c.u32(item+8)])
    models=[];pending=list({o['datum']:o for o in objects}.values());visited=set()
    while pending:
        obj=pending.pop()
        if obj['datum'] in visited:continue
        visited.add(obj['datum'])
        if len(visited)>256:raise ValueError('Placed model attachment graph exceeds audit bound')
        model=c.tags[c.u32(obj['offset']+0x38)];render=c.tags[c.u32(model['offset']+4)];models.append(dict(object=obj['name'],render_model=render['name']))
        # Include authored vehicle/model attachments, including alternate
        # turret variants, instead of auditing only the parent vehicle body.
        for variant in c.block(model['offset']+0x50,0x38):
            for at in c.block(variant+0x1c,0x10):
                child=c.tags.get(c.u32(at+12))
                if child is None or child['offset'] is None or child['size']<0x3c:continue
                linked=c.tags.get(c.u32(child['offset']+0x38))
                if linked is not None and linked['group']=='hlmt':pending.append(child)
        for at in c.block(render['offset']+0x60,32):
            t=c.tags.get(c.u32(at+12))
            if t:roots[t['name']].add(obj['name'])
    errors=[];rows=[];textures={}
    marker_coverage=None
    polish_path=package/'authoring/material_polish.json'
    if not polish_path.exists():
        errors.append('Source marker UVs have not been fitted to native atlas cells')
    else:
        polish=json.loads(polish_path.read_text())
        ass=package/'authoring/data/scenarios/multi/dire_valley/structure/dire_valley.ass'
        if sha(ass)!=polish['ass_sha256']:errors.append('Polished ASS fingerprint changed')
        stream=source_meshes(ass);next(stream);records,_=next(stream)
        expected=[]
        for record in records[-polish['marker_vertices']:]:
            lines=record.splitlines();uv=np.fromstring(lines[4].decode(),sep=' ');uv[1]=1.-uv[1]
            expected.append((np.fromstring(lines[0].decode(),sep=' ')/100.,uv))
        points=[];coordinates=[]
        for section in c.block(bsp['offset']+156,176):
            count=c.u16(section)
            if not count:continue
            base=c.u32(section+40)+c.u32(section+48)+8
            resources=c.block(section+56,16)
            pos=next(at for at in resources if c.u16(at+4)==56 and c.u16(at+6)==0)
            uv=next(at for at in resources if c.u16(at+4)==56 and c.u16(at+6)==1)
            stride=c.u32(pos+8)//count
            points.extend(np.ndarray((count,3),'<f4',c.data,base+c.u32(pos+12),strides=(stride,4)))
            coordinates.extend(np.ndarray((count,2),'<f4',c.data,base+c.u32(uv+12),strides=(8,4)))
        points=np.asarray(points);coordinates=np.asarray(coordinates);matched=0
        for position,uv in expected:
            candidates=np.max(abs(points-position),axis=1)<=.02
            if candidates.any() and (np.max(abs(coordinates[candidates]-uv),axis=1)<=2e-4).any():matched+=1
        marker_coverage=dict(authored_vertices=len(expected),compiled_matches=matched,uv_tolerance=2e-4,position_welding_tolerance=.02)
        if matched!=len(expected):errors.append('Native marker UVs missing from compiled world: '+str(marker_coverage))
    # Check every rendered vertex of every remaining static prop prototype,
    # after the real compiler applied scale, reordering and ASS V conversion.
    authoring=package/'authoring'
    scenery_source=json.loads((authoring/'scenery_geometry.json').read_text())
    lighting=json.loads((authoring/'lighting_authoring.json').read_text())
    native=lighting['native_tree_scenery']+lighting.get('native_prop_scenery',[])
    stream=source_meshes(authoring/'data/scenarios/multi/dire_valley/structure/dire_valley.ass')
    ass_materials=next(stream);prototypes=list(stream)
    def base_name(name):return name.split(' lm:')[0].rstrip('!)%@')
    paths={base_name(k):v.replace('/','\\') for k,v in scenery_source['material_mapping'].items()}
    compiled={}
    for section in c.block(bsp['offset']+312,200):
        count=c.u16(section)
        if not count:continue
        base=c.u32(section+40)+c.u32(section+48)+8;res=c.block(section+56,16)
        pos=next(at for at in res if c.u16(at+4)==56 and c.u16(at+6)==0)
        uv=next(at for at in res if c.u16(at+4)==56 and c.u16(at+6)==1)
        indices=next(at for at in res if c.u16(at+4)==32)
        points=np.ndarray((count,3),'<f4',c.data,base+c.u32(pos+12),strides=(c.u32(pos+8)//count,4))
        coords=np.ndarray((count,2),'<f4',c.data,base+c.u32(uv+12),strides=(8,4))
        ids=np.frombuffer(c.data,dtype='<u2',count=c.u32(indices+8)//2,offset=base+c.u32(indices+12))
        parts=res[0];start=base+c.u32(parts+12)
        for at in range(start,start+c.u32(parts+8),72):
            if c.u16(at) in (0,1,5) or not c.u16(at+8):continue
            shader=c.tags[c.u32(materials[c.u16(at+4)]+12)]['name']
            used=np.unique(ids[c.u16(at+6):c.u16(at+6)+c.u16(at+8)])
            compiled.setdefault(shader,[]).extend(np.c_[points[used],coords[used]])
    compiled={k:np.asarray(v) for k,v in compiled.items()}
    static_prototypes={(row['mesh'],row['scale']) for row in scenery_source['placement_transforms'] if row not in native}
    expected_count=0;matched_count=0;prototype_rows=[]
    for mesh,scale in sorted(static_prototypes):
        start_expected,start_matched=expected_count,matched_count;misses=[];uv_distances=[]
        records,faces=prototypes[mesh];used=set()
        for face in faces:
            material=json.loads(ass_materials[int(face[0])][0])
            if '!' not in material:continue
            shader=paths[base_name(material)]
            used.update((shader,int(i)) for i in face[1:])
        for shader,i in used:
            lines=records[i].splitlines();xyz=np.fromstring(lines[0].decode(),sep=' ')*scale/100.
            uv=np.fromstring(lines[4].decode(),sep=' ');uv[1]=1.-uv[1];expected_count+=1
            actual=compiled.get(shader)
            if actual is None:continue
            matching_uv=np.max(abs(actual[:,3:]-uv),axis=1)<=2e-4
            uv_distances.append(float(np.min(np.max(abs(actual[matching_uv,:3]-xyz),axis=1)))
                if matching_uv.any() else None)
            # The stock compiler welds these precise prop vertices by up to
            # 0.000389 world units (retained per-prototype measurements).
            # Keep UVs strict while allowing the measured compiler snap.
            near=np.max(abs(actual[:,:3]-xyz),axis=1)<=5e-4
            if near.any() and (np.max(abs(actual[near,3:]-uv),axis=1)<=2e-4).any():matched_count+=1
            elif len(misses)<3:
                nearest=int(np.argmin(np.max(abs(actual[:,:3]-xyz),axis=1)))
                misses.append(dict(shader=shader,expected_xyz=xyz.tolist(),expected_uv=uv.tolist(),
                    nearest_xyz=actual[nearest,:3].tolist(),nearest_uv=actual[nearest,3:].tolist()))
        prototype_rows.append(dict(mesh=mesh,scale=scale,expected=expected_count-start_expected,
            matched=matched_count-start_matched,misses=misses,
            vertices_without_exact_uv=sum(v is None for v in uv_distances),
            max_compiler_position_delta_for_exact_uv=max((v for v in uv_distances if v is not None),default=None)))
    prop_coverage=dict(prototypes=len(static_prototypes),authored_render_vertices=expected_count,
        compiled_matches=matched_count,position_welding_tolerance=5e-4,uv_tolerance=2e-4,details=prototype_rows)
    if matched_count!=expected_count:errors.append('Static prop compiled UV mismatch: '
        +str({k:v for k,v in prop_coverage.items() if k!='details'}))
    for name,owners in sorted(roots.items()):
        if name=='shaders\\invalid':errors.append('Visible placeholder shader: '+name)
        try:images=bank.bitmaps(name)
        except (KeyError,ValueError,IndexError) as ex:
            errors.append('Native material unresolved: '+name+' '+str(ex));continue
        for image in images:
            key=image['name'],image['frame']
            if key in textures:continue
            if image['name'] in [r'rasterizer\invalid',r'objects\bitmaps\default\grid_256x256']:
                errors.append('Active diagnostic texture: '+name+' -> '+image['name'])
            t=by_name.get(('bitm',image['name']))
            if t is None or t['offset'] is None:
                errors.append('Missing bitmap owner: '+name+' -> '+image['name']);continue
            frames=c.block(t['offset']+68,168)
            if image['frame']>=len(frames):errors.append('Bitmap frame outside owner');continue
            at=frames[image['frame']];pointer,length=c.u32(at+56),c.u32(at+80)
            bank_path=package/'content/halo2/Mods/dire_valley/dire_valley.map' if pointer>>30==0 else package/'content/halo2/h2_maps_win64_dx11/textures.dat'
            try:
                with bank_path.open('rb') as stream:raw=texture_resource(stream,pointer,length)
                textures[key]=dict(name=image['name'],frame=image['frame'],dimensions=[c.u16(at+4),c.u16(at+6)],format=c.u16(at+12),decoded_bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest())
            except ValueError as ex:errors.append('Bitmap partition invalid: '+image['name']+' '+str(ex))
        rows.append(dict(shader=name,owners=sorted(owners),native_template=bank.get('shad',name)[2][4][1],active_bitmaps=images))
    shield=next(row for row in rows if row['shader']==SHIELD)
    if not any(i['slot']==2 and i['name']==OFFSET for i in shield['active_bitmaps']):errors.append('Portable shield authored offset binding missing')
    result=dict(schema='H2_DIRE_VALLEY_MATERIAL_AUDIT_V1',passed=not errors,errors=errors,
        cache_sha256=sha(c.path),bank_sha256=sha(bank.path/'library.json'),visible_bsp_parts=part_count,
        placed_scenery=len(c.block(s+0x50,0x5c)),equipment_and_vehicle_placements=len(equipment),
        placed_object_models=models,material_roots=len(roots),native_materials=rows,texture_owners=list(textures.values()),
        compiled_marker_uv_coverage=marker_coverage,
        compiled_prop_uv_coverage=prop_coverage,
        scope='Compiled asset ownership, active native bitmap remaps and compressed resource integrity; visual acceptance is separate.')
    save(report,result);return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--package',required=True,type=Path);p.add_argument('--report',required=True,type=Path)
    a=p.parse_args();r=validate(a.package,a.report)
    summary={k:v for k,v in r.items() if k not in ['native_materials','texture_owners','placed_object_models']}
    summary['compiled_prop_uv_coverage']={k:v for k,v in r['compiled_prop_uv_coverage'].items() if k!='details'}
    print(json.dumps(summary));raise SystemExit(0 if r['passed'] else 1)
