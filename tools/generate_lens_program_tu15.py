#!/usr/bin/env python3
"""Recover the literal final-flare pixel program from its authenticated parent."""
import argparse,json,hashlib
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_trace
from generate_lens_query_tu15 import QueryCode
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from audit_rendering import Cache

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN;c=QueryCode(x)
    for owner,site,target in ((0x2bcb0,0x2c302,0x13850),(0x13850,0x13b5e,0x2dee0)):
        assert site in _trace(c,owner,set()).code
        instruction=c.at(site);assert instruction.mnemonic=='call' and instruction.operands[0].imm==target
    assert c.at(0x2e14a).bytes.hex()=='a1a0224900'
    assert c.at(0x2e14f).bytes.hex()=='8b485c' and c.at(0x2e152).bytes.hex()=='8b4154'
    expected={0x2e0f9:(54,1),0x2e0ff:(53,0x11002),0x2e109:(34,0x08081524),
        0x2e113:(45,0xc5),0x2e11d:(35,0x0c0c0408),0x2e127:(46,0xd4),
        0x2e131:(8,0x050f0004),0x2e13b:(9,0x0c0d1400)}
    assert c.at(0x2e0e6).op_str=='eax, eax' and c.at(0x2e0e8).op_str=='ecx, 0x3c'
    assert c.at(0x2e0ed).op_str=='edi, 0x491788' and c.at(0x2e0f2).mnemonic=='rep stosd'
    assert c.at(0x2df70).op_str=='ebp, 1'
    words=[0]*60
    for pc,(index,value)in expected.items():
        i=c.at(pc);assert i.mnemonic=='mov' and i.operands[0].mem.disp==0x491788+index*4
        assert (i.operands[1].type==2 and i.operands[1].imm==value)or(pc==0x2e0f9 and i.op_str.endswith(', ebp'))
        words[index]=value
    cache=Cache('assets/map-pack/maps/elongation.map')
    globals=next(d for d,t in cache.tags.items()if t['group']=='matg');_,root=cache.tag(globals,'matg')
    assert cache.u32(root+0x108)==1;raster=cache.pointer(cache.u32(root+0x10c),cache.magic,0x68)
    shaders=cache.block(raster+0x58,8);datum=cache.u32(shaders[10]+4)
    assert cache.u32(shaders[10])==0x76727478
    tag,at=cache.tag(datum,'vrtx');assert tag['name']=='rasterizer\\vertex_shaders_xbox\\lens_flare'
    row=cache.block(at+4,28);assert len(row)==1 and cache.u32(row[0])==0
    size=cache.u32(row[0]+12);code_at=cache.pointer(cache.u32(row[0]+16),cache.magic,size)
    vertex=bytes(cache.data[code_at:code_at+size]);assert len(vertex)==68
    cache.close()
    header='#pragma once\n#include "halo2/render_program.hpp"\nnamespace halo2 {\n'
    header+='// Generated from original2DEE0 literal stores; authored matg rasterizer vertex slot10.\n'
    header+='inline constexpr std::array<std::uint32_t,60> source_lens_pixel_words{'+','.join(hex(w)+'u'for w in words)+'};\n'
    header+='inline constexpr std::array<std::uint8_t,68> source_lens_vertex_bytes{'+','.join(hex(w)for w in vertex)+'};\n}\n'
    Path('include/halo2/source_lens_program_tu15.hpp').write_text(header)
    Path('shaders/source_lens_program.glsl').write_text('// Generated from original2DEE0, not a simplified flare formula.\nconst uint lensProgram[60]=uint[60]('+','.join(hex(w)+'u'for w in words)+');\n')
    a.report.write_text(json.dumps(dict(original_sha256=NEW_PIN,pixel_words=words,
        vertex_hex=vertex.hex(),vertex_sha256=hashlib.sha256(vertex).hexdigest(),
        stores={hex(pc):dict(word=i,value=v,bytes=bytes(c.at(pc).bytes).hex())for pc,(i,v)in expected.items()},
        parent_calls={'0002c302':'00013850','00013b5e':'0002dee0'},
        selector='2E14A..2E152: matg rasterizer+5C, vertex reference+54 (slot10)',full_goal_complete=False),indent=2)+'\n')
    print('Recovered original final flare program: 60 pixel words and four authored vertex MOVs')
if __name__=='__main__':main()
