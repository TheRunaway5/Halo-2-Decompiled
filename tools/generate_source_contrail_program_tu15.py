#!/usr/bin/env python3
"""Compile the actual rasterizer-global index4/class0 contrail effect vertex program."""
import argparse,hashlib,json
from pathlib import Path
from audit_rendering import Cache
from generate_particle_graphics_program import HELPERS
from generate_particle_vertex_probe import translate
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cache',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();c=Cache(a.cache)
g=next(t for t in c.tags.values()if t['group']=='matg');_,root=c.tag(g['datum'],'matg');r=c.block(root+0x108,0x1c8);assert len(r)==1;s=c.block(r[0]+0x58,8);datum=c.u32(s[4]+4);_,v=c.tag(datum,'vrtx');cls=c.block(v+4,28)[0];assert [c.u16(p)for p in c.block(cls+4,2)]==[0,3,9,14];n=c.u32(cls+12);at=c.pointer(c.u32(cls+16),c.magic,n);code=bytes(c.data[at:at+n]);source=HELPERS+translate(code,'contrailProgram',attribute_expression='contrailAttributes[i]',corner_expression=None);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(source)
h=14695981039346656037
for b in code:h=((h^b)*1099511628211)&0xffffffffffffffff
a.output.with_suffix('.hpp').write_text('#pragma once\n#include <cstdint>\nnamespace halo2 {inline constexpr std::uint64_t source_contrail_program_hash='+hex(h)+'ull;}\n');a.output.with_suffix('.json').write_text(json.dumps(dict(vertex_global=4,classification=0,datum=f'{datum:08x}',original_sha256=hashlib.sha256(code).hexdigest(),source_sha256=hashlib.sha256(source.encode()).hexdigest(),expected_vertices_embedded=False),indent=2)+'\n')
