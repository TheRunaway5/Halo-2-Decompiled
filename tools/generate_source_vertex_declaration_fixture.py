#!/usr/bin/env python3
"""Freeze original world-class semantic lists and unchanged1C330 outputs.

This uses authored cache bytes and the independent declaration audit's
unchanged-executable results. Ordinary asset availability is not live admission.
The transparent_generic_offset record is a labeled latent-family control.
"""
import hashlib
import json
from pathlib import Path
import struct
from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]
SUPPORTED={0,3,4,5,6,8,9,14,19,20}

def main():
    audit_path=ROOT/'analysis/multiplayer-shading/world-vertex-declarations.json'
    audit=json.loads(audit_path.read_text())
    programs={(p['map'],p['datum']):p for p in audit['programs'] if p['classification']==0}
    selected=set()
    for link in audit['links']:
        if link['dynamic_role_material'] or link['bsp_or_selected_model_material']:
            selected.add((link['map'],link['vrtx']))
    layout={tuple(row['semantics']):row['declaration_words'] for row in audit['original_declaration_layouts']}
    unique={}
    for key,p in programs.items():
        semantics=tuple(p['semantics'])
        latent=p['name'].endswith('\\transparent_generic_offset')
        if semantics and set(semantics)<=SUPPORTED and (key in selected or latent):
            unique.setdefault((semantics,p['sha256']),dict(p,latent=latent))
    caches={}
    rows=['# Original vrtx world-class0: D map datum leaf latent count semantics... codehex 64 original1C330 declaration words.']
    for p in sorted(unique.values(),key=lambda p:(p['name'],p['sha256'])):
        name=p['map']
        if name not in caches:
            path=ROOT/('assets/map-pack/maps' if (ROOT/'assets/map-pack/maps'/f'{name}.map').is_file() else 'assets/disc/maps')/f'{name}.map'
            caches[name]=Cache(path)
        c=caches[name]
        _,root=c.tag(p['datum'],'vrtx')
        cl=c.block(root+4,28)[0]
        n=c.u32(cl+4)
        semantics=struct.unpack_from('<'+'H'*n,c.data,c.u32(cl+8)-c.magic)
        code=bytes(c.data[c.u32(cl+16)-c.magic:c.u32(cl+16)-c.magic+c.u32(cl+12)])
        assert list(semantics)==p['semantics'] and hashlib.sha256(code).hexdigest()==p['sha256']
        fields=['D',name,f"{p['datum']:08x}",p['name'].split('\\')[-1],str(int(p['latent'])),str(n)]
        fields+=list(map(str,semantics))+[code.hex()]+[f'{w:08x}' for w in layout[semantics]]
        rows.append(' '.join(fields))
    for c in caches.values():c.close()
    destination=ROOT/'tests/fixtures/retail_world_vertex_declarations.tsv'
    destination.write_text('\n'.join(rows)+'\n')
    proof=dict(authority=audit['original_authority'],source_audit_sha256=hashlib.sha256(audit_path.read_bytes()).hexdigest(),
        records=len(unique),layouts=len({k[0] for k in unique}),latent_records=sum(p['latent'] for p in unique.values()),
        fixture_sha256=hashlib.sha256(destination.read_bytes()).hexdigest(),
        boundary='Class0 authored availability and unchanged1C330 stream declaration. Native input/output controls use explicitly supplied decoded world streams; no original live GPU class selection or physical NV2A precision is inferred.')
    (ROOT/'analysis/multiplayer-shading/world-vertex-declaration-fixture-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(json.dumps(proof,indent=2))

if __name__=='__main__':main()
