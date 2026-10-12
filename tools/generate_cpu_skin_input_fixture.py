#!/usr/bin/env python3
"""Freeze original part-class selection and compiled FP variant payloads.

The class result comes from unchanged 1CAA0/1CB20 execution in
 audit_first_person_vertex_routes.py, not from the native selector.
No current visibility, animation phase or GPU hardware precision is inferred.
"""
from pathlib import Path
import hashlib
import json
import struct
from audit_rendering import Cache
ROOT=Path(__file__).resolve().parents[1]
FAMILIES=('lightmap_dynamic','lightmap_dynamic_env','lightmap_dynamic_tex_detail',
          'stencil_emissive','stencil_env_bumped_tangentspace','stencil_texture',
          'transparent_generic_reflection')

def main():
    report=json.loads((ROOT/'analysis/multiplayer-shading/first-person-vertex-routes-original.json').read_text())
    out=['# Original default.xbe '+report['executable_sha256'],
         '# C map model section part mode opaque transparent type original_class',
         '# P leaf class semantic_count semantics... original_compiled_bytes']
    for model in report['models_data']:
        for s in model['sections']:
            for p in s['parts']:
                out.append('C '+ ' '.join(map(str,[model['map'],model['model'],s['section'],p['part'],
                    s['classification'],s['opaque_influences'],s['transparent_influences'],p['type'],p['selected_class']])))
    c=Cache(ROOT/'assets/disc/maps/cyclotron.map')
    manifest=[]
    for family in FAMILIES:
        t=next(t for t in c.tags.values() if t['group']=='vrtx' and t['name'].endswith('\\'+family))
        _,root=c.tag(t['datum'],'vrtx')
        for classification,at in enumerate(c.block(root+4,28)):
            if classification>6:break
            semantics=[c.u16(s) for s in c.block(at+4,2)]
            size,pointer=c.u32(at+12),c.u32(at+16)
            if not size:raise RuntimeError('required compiled variant missing')
            code=bytes(c.data[pointer-c.magic:pointer-c.magic+size])
            out.append('P '+' '.join(map(str,[family,classification,len(semantics),*semantics,code.hex()])))
            manifest.append(dict(family=family,classification=classification,sha256=hashlib.sha256(code).hexdigest(),semantics=semantics))
    c.close()
    target=ROOT/'tests/fixtures/retail_cpu_skin_inputs.tsv'
    target.write_text('\n'.join(out)+'\n')
    proof=dict(untouched_functions=report['untouched_functions'],executable_sha256=report['executable_sha256'],
        classes=sum(x.startswith('C ') for x in out),programs=manifest,fixture_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
        selected_reflection_classes=[x for x in report['class_records'] if x['name'].endswith('\\transparent_generic_reflection')],
        boundary=report['boundary'])
    (ROOT/'analysis/multiplayer-shading/cpu-skin-input-fixture-proof.json').write_text(json.dumps(proof,indent=2)+'\n')
    print(target,proof['classes'],len(manifest),proof['fixture_sha256'])
if __name__=='__main__':main()
