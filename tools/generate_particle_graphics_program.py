#!/usr/bin/env python3
"""Compile an actual resident particle disc program for graphics.

Inputs and constants belong to the caller's source draw state. No captured
particle state, camera constants or expected vertices enter the generated code.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from compile_menu_world_vertex_catalog import digest, local
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
 inputs=p.add_mutually_exclusive_group(required=True)
 inputs.add_argument('--cache',type=Path,help='offline original reference cache')
 inputs.add_argument('--preserved-sources',type=Path,help='existing port GLSL source manifest; no game files required')
 p.add_argument('--classification',type=int,choices=[1,7,8,9],default=7)
 p.add_argument('--variant',choices=['standard','distortion'],default='standard')
 p.add_argument('--vertex-datum',type=lambda value:int(value,0),help='explicit original vrtx tag for offline translation')
 a=p.parse_args()
 if a.variant=='distortion' and a.classification!=7:raise ValueError('distortion requires original classification 7')
 if a.vertex_datum is not None and not a.cache:raise ValueError('vertex datum requires an original cache')
 if a.preserved_sources:
  manifest=json.loads(a.preserved_sources.read_bytes())
  if manifest.get('schema')!='H2_NATIVE_PARTICLE_SOURCES_V1':raise ValueError('unknown native particle source schema')
  matches=[entry for entry in manifest['entries'] if entry['classification']==a.classification and entry.get('variant','standard')==a.variant]
  if len(matches)!=1:raise ValueError('native particle classification is missing or ambiguous')
  entry=matches[0];source=local(a.preserved_sources.parent,entry['source']).read_text()
  if hashlib.sha256(source.encode()).hexdigest()!=digest(entry['source_sha256']):raise ValueError('preserved particle GLSL identity differs')
  digest(entry['original_program_sha256'])
  fnv=int(entry['program_fnv64'],16)
  if not 0<=fnv<=0xffffffffffffffff:raise ValueError('native particle program identity is invalid')
  if entry['expected_vertices_embedded'] is not False:raise ValueError('particle source embeds expected outputs')
  metadata={key:entry[key] for key in ('vertex_datum','classification','instructions','original_program_sha256','source_sha256','expected_vertices_embedded')}
 else:
  from audit_rendering import Cache
  from generate_particle_vertex_probe import translate
  if a.variant!='standard' and a.vertex_datum is None:raise ValueError('nonstandard program requires an explicit original vertex datum')
  datum=a.vertex_datum if a.vertex_datum is not None else (0xe1ec0077 if a.classification==1 else 0xe20e0099)
  c=Cache(a.cache);_,root=c.tag(datum,'vrtx');at=c.block(root+4,28)[a.classification]
  size=c.u32(at+12);start=c.pointer(c.u32(at+16),c.magic,size);code=bytes(c.data[start:start+size])
  source=HELPERS+translate(code,'particleProgram',attribute_expression='particleAttributes[i]',corner_expression=None if a.classification==1 else 'particleCorner')+'\n'
  fnv=14695981039346656037
  for byte in code:fnv=((fnv^byte)*1099511628211)&0xffffffffffffffff
  metadata={'vertex_datum':f'{datum:08x}','classification':a.classification,
   'instructions':struct.unpack_from('<H',code,2)[0],'original_program_sha256':hashlib.sha256(code).hexdigest(),
   'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'expected_vertices_embedded':False}
 metadata['variant']=a.variant
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(source)
 symbol='particle_distortion_program_hash' if a.variant=='distortion' else {1:'particle_model_program_hash',7:'particle_program_hash',8:'particle_directional_program_hash',9:'particle_perpendicular_program_hash'}[a.classification]
 a.output.with_suffix('.hpp').write_text('#pragma once\n#include <cstdint>\nnamespace halo2 {\ninline constexpr std::uint64_t '+symbol+'='+hex(fnv)+'ull;\n}\n')
 a.output.with_suffix('.json').write_text(json.dumps(metadata,indent=2)+'\n')
if __name__=='__main__':main()
