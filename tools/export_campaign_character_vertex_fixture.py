#!/usr/bin/env python3
"""Retain original compiled ordinary campaign character vertex programs.

The existing unchanged1CAA0/1CB20 part-class corpus remains an independent
control. New program bytes come directly from every campaign cache. The offset
shield family has its separate local-position adapter/graphics comparison.
This fixture does not infer current actor geometry or owner admission.
"""
import argparse
import hashlib
import json
from pathlib import Path
from audit_campaign import MAPS
from audit_rendering import Cache, file_sha

ROOT=Path(__file__).resolve().parents[1]
FAMILIES=('lightmap_dynamic','lightmap_dynamic_env','lightmap_dynamic_tex_detail',
          'stencil_emissive','stencil_env_bumped_tangentspace','stencil_texture',
          'transparent_generic_reflection','lightmap_dynamic_tex_change_color_env',
          'lightmap_dynamic_tex_env','transparent_generic_default',
          'lightmap_dynamic_tex_change_color','lightmap_dynamic_tex')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    base=ROOT/'tests/fixtures/retail_cpu_skin_inputs.tsv'
    rows=[line for line in base.read_text().splitlines() if line.startswith('#') or line.startswith('C ')]
    programs={}
    maps=[]
    for name in MAPS:
        c=Cache(ROOT/'assets/disc/maps'/(name+'.map'))
        present=[]
        for family in FAMILIES:
            tag=next((t for t in c.tags.values() if t['group']=='vrtx' and t['name'].endswith('\\'+family)),None)
            if tag is None:
                continue
            present.append(family)
            _,root=c.tag(tag['datum'],'vrtx')
            for classification,at in enumerate(c.block(root+4,28)[:7]):
                semantics=[c.u16(x)for x in c.block(at+4,2)]
                size,pointer=c.u32(at+12),c.u32(at+16)
                code=bytes(c.data[pointer-c.magic:pointer-c.magic+size])
                assert len(code)==size and size>0
                row='P '+' '.join(map(str,[family,classification,len(semantics),*semantics,code.hex()]))
                key=(family,classification)
                assert key not in programs or programs[key]==row,(name,key)
                programs[key]=row
        maps.append(dict(map=name,sha256=file_sha(c.path),present_families=present))
        c.close()
    assert len(programs)==84
    rows+=list(programs.values())
    a.output.write_text('\n'.join(rows)+'\n')
    a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(scope=__doc__,maps=maps,
        base_class_fixture_sha256=file_sha(base),programs=len(programs),families=FAMILIES,
        tool_sha256=file_sha(Path(__file__)),fixture_sha256=file_sha(a.output)),indent=2)+'\n')
    print('PASS 84 original program variants; identical in every campaign cache containing each family')


if __name__=='__main__':main()
