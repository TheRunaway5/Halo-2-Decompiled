#!/usr/bin/env python3
"""Retain authored Xbox shadow programs and selected pass metadata, read-only."""
import hashlib, json, struct, sys
from pathlib import Path
from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer-shading/original-xemu/shadow-gpu-audit'
MAPS=('ascension','beavercreek','burial_mounds','coagulation','colossus',
      'cyclotron','foundation','headlong','lockout','midship','waterworks',
      'zanzibar','containment','warlock','deltatap','turf','backwash','dune',
      'elongation','gemini','triplicate')

def main():
    OUT.mkdir(exist_ok=True)
    reports=[]
    for ordinal,name in enumerate(MAPS):
        c=Cache(ROOT/('assets/disc/maps' if ordinal<12 else 'assets/map-pack/maps')/(name+'.map'))
        programs=[];passes=[]
        for datum,t in c.tags.items():
            if t['group']=='vrtx' and t['name'] in (
                    'rasterizer\\vertex_shaders_xbox\\shadow_buffer_generation',
                    'rasterizer\\vertex_shaders_xbox\\shadow_buffer_application_lightmap'):
                _,root=c.tag(datum,'vrtx')
                variants=[]
                for class_index,variant in enumerate(c.block(root+4,28)):
                    count=c.u32(variant+12)
                    if not count:continue
                    at=c.pointer(c.u32(variant+16),c.magic,count)
                    code=c.data[at:at+count]
                    classification=c.u32(variant)
                    leaf=t['name'].split('\\')[-1]
                    file=f'{leaf}-class{classification}-{hashlib.sha256(code).hexdigest()[:16]}.vp'
                    (OUT/file).write_bytes(code)
                    variants.append(dict(class_index=class_index,classification_field=classification,bytes=count,
                        sha256=hashlib.sha256(code).hexdigest(),file=file))
                programs.append(dict(datum=f'{datum:08X}',name=t['name'],variants=variants))
            if t['group']=='spas' and t['name'] in (
                    'shaders\\shader_passes\\shadow\\shadow_generate',
                    'shaders\\shader_passes\\shadow\\shadow_apply_combined'):
                key=c.shader_pass(datum)
                passes.append(dict(datum=f'{datum:08X}',**c.passes[key]))
        reports.append(dict(map=name,programs=programs,passes=passes))
        c.close()
    result=dict(maps=reports,scope='Authored class/pass payload provenance only; active original scheduling is established separately by the XBE and retained RDC.')
    (OUT/'authored-shadow-assets-21.json').write_text(json.dumps(result,indent=2)+'\n')
    unique={v['sha256'] for r in reports for p in r['programs']for v in p['variants']}
    print(json.dumps(dict(maps=len(reports),unique_compiled_programs=len(unique),
                         program_records=sum(len(r['programs'])for r in reports))))

if __name__=='__main__':main()
