#!/usr/bin/env python3
"""Instrument actual compiled world sources for full GPU output comparison."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'tools'))
from compile_menu_world_vertex_catalog import triangle_vertex_source

CAPTURE='''
layout(set=2,binding=0,std430) buffer WorldVertexCapture {uint words[];} worldCapture;
void worldCaptureVector(uint at,vec4 value) {
    uvec4 bits=floatBitsToUint(value);
    for(uint lane=0u;lane<4u;++lane)worldCapture.words[at+lane]=bits[lane];
}
void worldCaptureVertex() {
    uint at=uint(gl_VertexIndex)*51u;
    worldCaptureVector(at,gl_Position);worldCapture.words[at+4u]=floatBitsToUint(gl_PointSize);
    worldCaptureVector(at+5u,v_vtxD0);worldCaptureVector(at+9u,v_vtxD1);
    worldCaptureVector(at+13u,v_vtxB0);worldCaptureVector(at+17u,v_vtxB1);
    worldCapture.words[at+21u]=floatBitsToUint(v_vtxFog);
    worldCaptureVector(at+22u,v_vtxT0);worldCaptureVector(at+26u,v_vtxT1);
    worldCaptureVector(at+30u,v_vtxT2);worldCaptureVector(at+34u,v_vtxT3);
    worldCaptureVector(at+38u,v_vtxPos0);worldCaptureVector(at+42u,v_vtxPos1);
    worldCaptureVector(at+46u,v_vtxPos2);worldCapture.words[at+50u]=floatBitsToUint(v_triMZ);
}
'''

def run(command):
    result=subprocess.run([str(x) for x in command],capture_output=True,text=True)
    if result.returncode:raise RuntimeError('Shader validation failed: '+repr(command)+'\n'+result.stdout+result.stderr)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',required=True)
    parser.add_argument('--validator',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    entries=[];seen=set()
    for manifest in [ROOT/'shaders/native-programs/menu/sources.json',ROOT/'shaders/pstencil/native-programs/sources.json']:
        data=json.loads(manifest.read_text())
        for entry in data['entries']:
            source=(manifest.parent/entry['source']).read_bytes()
            if hashlib.sha256(source).hexdigest()!=entry['source_sha256']:raise ValueError('Preserved source identity differs')
            key=(entry['class_index'],entry['program_sha256'],tuple(entry['semantics']))
            if key in seen:raise ValueError('Duplicate admitted source identity')
            seen.add(key)
            prefix=f'entry-{len(entries)}'
            triangle=triangle_vertex_source(source.decode(),entry['semantics'],entry['consumed_inputs'],ROOT/'shaders')
            pos=triangle.rfind('void main() {\n')
            instrumented=triangle[:pos]+CAPTURE+triangle[pos:]
            instrumented=instrumented.rstrip()[:-1]+'    worldCaptureVertex();\n}\n'
            native=source.decode().replace('void main() {\n',CAPTURE+'void main() {\n')
            native=native.rstrip()[:-1]+'    worldCaptureVertex();\n}\n'
            for name,text in [('original',source.decode()),('triangle',instrumented),('native',native)]:
                path=args.output/(prefix+'-'+name+'.vert');path.write_text(text)
                spv=path.with_suffix('.spv')
                run([args.compiler,'-V','--target-env','vulkan1.3','-S','vert',path,'-o',spv]);run([args.validator,'--target-env','vulkan1.3',spv])
            entries.append(entry|{'abi':data['abi'],'original':prefix+'-original.spv','triangle':prefix+'-triangle.spv','native':prefix+'-native.spv','instrumented_triangle_sha256':hashlib.sha256(instrumented.encode()).hexdigest()})
    assembly=Path(__file__).with_name('assemble.comp')
    run([args.compiler,'-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),assembly,'-o',args.output/'assemble.comp.spv'])
    run([args.validator,'--target-env','vulkan1.3',args.output/'assemble.comp.spv'])
    run([args.compiler,'-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),ROOT/'tests/menu_world_vertex_catalog_capture.geom','-o',args.output/'capture.geom.spv'])
    run([args.validator,'--target-env','vulkan1.3',args.output/'capture.geom.spv'])
    (args.output/'manifest.json').write_text(json.dumps({'schema':'H2_WORLD_TRIANGLE_SHADER_CAPTURE_V1','entries':entries},indent=2)+'\n')

if __name__=='__main__':main()
