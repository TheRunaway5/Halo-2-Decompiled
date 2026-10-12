#!/usr/bin/env python3
"""Compile an actual resident particle disc program for graphics.

Inputs and constants belong to the caller's source draw state. No captured
particle state, camera constants or expected vertices enter the generated code.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from audit_rendering import Cache
from generate_particle_vertex_probe import translate
ROOT=Path(__file__).resolve().parents[1]
HELPERS='''vec4 r[13],v[16],outv[16];int address;
vec4 constant(int index){return particleConstants[index];}
vec4 multiply(vec4 a,vec4 b) {
 vec4 az=mix(a,vec4(1),isnan(a)),bz=mix(b,vec4(1),isnan(b));
 vec4 zeros=sign(az)*sign(bz),ret=a*b;
 if(zeros.x==0)ret.x=0;if(zeros.y==0)ret.y=0;
 if(zeros.z==0)ret.z=0;if(zeros.w==0)ret.w=0;
 return ret;
}
float clampAwayZeroInf(float t) {
 return t>0 || floatBitsToUint(t)==0u?
 clamp(t,uintBitsToFloat(528482304u),uintBitsToFloat(1602224128u)):
 clamp(t,uintBitsToFloat(3749707776u),uintBitsToFloat(2675965952u));
}
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
 p.add_argument('--classification',type=int,choices=[7,8],default=7);a=p.parse_args()
 c=Cache(ROOT/'assets/disc/maps/mainmenu.map');_,root=c.tag(0xe20e0099,'vrtx');at=c.block(root+4,28)[a.classification]
 size=c.u32(at+12);start=c.pointer(c.u32(at+16),c.magic,size);code=bytes(c.data[start:start+size])
 source=HELPERS+translate(code,'particleProgram',attribute_expression='particleAttributes[i]',corner_expression='particleCorner')+'\n'
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(source)
 fnv=14695981039346656037
 for byte in code:fnv=((fnv^byte)*1099511628211)&0xffffffffffffffff
 symbol='particle_program_hash' if a.classification==7 else 'particle_directional_program_hash'
 a.output.with_suffix('.hpp').write_text('#pragma once\n#include <cstdint>\nnamespace halo2 {\ninline constexpr std::uint64_t '+symbol+'='+hex(fnv)+'ull;\n}\n')
 a.output.with_suffix('.json').write_text(json.dumps({'vertex_datum':'e20e0099','classification':a.classification,
  'instructions':struct.unpack_from('<H',code,2)[0],'original_program_sha256':hashlib.sha256(code).hexdigest(),
  'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'expected_vertices_embedded':False},indent=2)+'\n')
if __name__=='__main__':main()
