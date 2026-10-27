#!/usr/bin/env python3
"""Actual raw renderer programs: C++ AOT arithmetic versus offline GPU oracle."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import struct

from check_native_fragment_semantics import ROOT,run,runner


def full_key(row):
    return row['program_sha256']+'-c'+str(row['class_index'])+''.join('-'+str(value) for value in row['semantics'])


def directed_rows():
    def token(mac,ilu=0,mm=0,im=0,om=15,output=9,reg=0,a_file=2,a_reg=0,c_reg=0,last=True):
        return [0,(mac<<21)|(ilu<<25)|0x1b,
            (a_file<<26)|(a_reg<<28)|(3<<11)|(1<<21)|(2<<19)|(3<<17)|
            (c_reg>>2)|(1<<6)|(2<<4)|(3<<2),
            (1<<28)|((c_reg&3)<<30)|(mm<<24)|(reg<<20)|(im<<16)|(om<<12)|2048|(output<<3)|int(last)]
    rows=[]
    cases=[('distance_mask', [token(8)],6),('set_less_than',[token(11)],15)]
    alias=[token(1,mm=15,om=0,last=False),
           token(1,1,mm=15,im=15,om=15,output=0,reg=12,a_file=1,c_reg=12,last=False),
           token(1,a_file=1,a_reg=1)]
    cases.append(('paired_position_snapshot',alias,15))
    for label,instructions,expected_mask in cases:
        raw=struct.pack('<2H',0x2078,len(instructions))+b''.join(struct.pack('<4I',*value) for value in instructions)
        rows.append(dict(program_sha256=hashlib.sha256(raw).hexdigest(),vertex_program_hex=raw.hex(),
            class_index=0,semantics=[0],basic_raw_semantics=True,relative_constants=False,
            vertex_instructions=instructions,synthetic_case=label,expected_input0_components=expected_mask))
    return rows


def data_for(row,count):
    words=[value for instruction in row['vertex_instructions'] for value in instruction]
    raw=struct.pack('<2H',0x2078,len(words)//4)+struct.pack('<'+'I'*len(words),*words)
    if raw.hex()!=row['vertex_program_hex'] or hashlib.sha256(raw).hexdigest()!=row['program_sha256']:
        raise ValueError('Actual renderer probe instructions differ from their complete source identity')
    instructions=words+[0]*(544-len(words));out=bytearray()
    rng=random.Random(int(hashlib.sha256(full_key(row).encode()).hexdigest()[:16],16))
    for case in range(count):
        # Immediate registers are explicitly supplied diagnostic input data;
        # this does not grant a live owner for absent material attributes.
        attributes=[[rng.randrange(257)/256 for _ in range(4)] for _ in range(16)]
        for index,semantic in enumerate(row['semantics']):
            if semantic==0:attributes[index]=[rng.randrange(-256,257)/256 for _ in range(3)]+[1.]
            elif semantic==1:attributes[index]=[rng.randrange(40)/255 for _ in range(4)]
            elif semantic==2:
                influences=row['class_index']-2
                cuts=[0,*sorted(rng.randrange(256) for _ in range(influences-1)),255]
                attributes[index]=[(cuts[i+1]-cuts[i])/255 for i in range(influences)]+[0.]*(4-influences)
            elif semantic==3:attributes[index]=[rng.randrange(257)/256,rng.randrange(257)/256,0.,1.]
            else:attributes[index]=[rng.randrange(-128,129)/128 for _ in range(3)]+[1.]
        if case==0:attributes[0]=[0.,0.,0.,1.]
        constants=[[rng.randrange(-32,33)/128 for _ in range(4)] for _ in range(192)]
        constants[:12]=[[1.,0.,0.,0.],[0.,1.,0.,0.],[0.,0.,1.,0.],[0.,0.,0.,1.],
            [1.,0.,0.,1.],[0.,-1.,0.,.5],[0.,0.,-1.,2.],[0.,0.,1.,767.8125],
            [640.,0.,0.,640.5],[0.,-360.,0.,360.5],[640.,-360.,16777215.,0.],[640.5,360.5,0.,0.]]
        constants[172]=[1.,1.,0.,0.]
        # All forty source palette entries exist. Bone indices above are
        # genuine UNORM bytes0..39 and every addressed c50..169 row is owned.
        for bone in range(40):
            constants[50+bone*3:53+bone*3]=[[1.,0.,0.,(bone%7)/32],
                [0.,1.,0.,(bone%5)/32],[0.,0.,1.,(bone%3)/32]]
        values=[v for vector in (*attributes,*constants) for v in vector]
        out+=struct.pack('<544I832f4I',*instructions,*values,0,0,0,len(words)//4)
    return bytes(out)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--matrix',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--samples',type=int,default=64)
    args=parser.parse_args()
    if not 1<=args.samples<=1024:parser.error('Bounded sample count required')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)/'renderer-vertex-semantics'
    scratch.mkdir();exporter=scratch/'exporter'
    tools=[shutil.which(name) for name in ('c++','glslangValidator','spirv-opt','spirv-val')]
    if not all(tools):raise RuntimeError('Native C++/Vulkan toolchain required')
    cxx,compiler,optimizer,validator=tools
    run([cxx,'-std=c++20','-O1','-I'+str(ROOT/'include'),ROOT/'tools/export_renderer_vertex_probes.cpp',
         ROOT/'src/ui/menu_world_vertex_source.cpp','-o',exporter])
    document=json.loads(args.matrix.read_bytes())
    extension=document.get('raw_renderer_extension',False)
    actual=[row for row in document['rows'] if row.get('basic_raw_semantics') or row.get('raw_renderer_extension')]
    expected_count=120 if extension else 13
    if len(actual)!=expected_count:raise ValueError('Actual complete arithmetic corridor was incomplete')
    requested=actual+directed_rows()
    matrix=scratch/'matrix.json';matrix.write_text(json.dumps(dict(rows=requested)))
    sources=scratch/'native';run([exporter,matrix,sources])
    exported=json.loads((sources/'sources.json').read_bytes())
    rows={full_key(row):row for row in requested}
    if set(rows)!={row['full_key'] for row in exported['entries']}:
        raise ValueError('Requested complete arithmetic corridor was incomplete')
    transport=scratch/'transport.cpp'
    definition='struct Fixture {std::array<std::array<unsigned,4>,136> vertexInstructions;std::array<Vector,16> vertexAttributes;std::array<Vector,192> constants;std::array<unsigned,4> flags;};'
    runner(transport,definition,5520,32);executable=scratch/'transport'
    run([cxx,'-std=c++20','-O1','-I'+str(ROOT/'include'),transport,'-lvulkan','-o',executable])
    validation=ROOT/'.tools/validation/usr'
    if not (validation/'share/vulkan/explicit_layer.d').is_dir():
        raise RuntimeError('Required Khronos Vulkan validation layer is unavailable')
    gpu_env={**os.environ,'VK_INSTANCE_LAYERS':'VK_LAYER_KHRONOS_validation',
        'VK_LAYER_PATH':str(validation/'share/vulkan/explicit_layer.d'),
        'LD_LIBRARY_PATH':str(validation/'lib')+(':'+os.environ['LD_LIBRARY_PATH'] if os.environ.get('LD_LIBRARY_PATH') else '')}
    report=dict(schema='HALO2_RAW_RENDERER_VERTEX_ARITHMETIC_V1',
        scope='Actual full-program C++ AOT arithmetic against unchanged repository vertex_program.glsl executed only in disposable hardware compute oracle; fetch/epilogue and original Xbox hardware pixels are separate gates',
        samples=args.samples,actual_programs=expected_count,directed_programs=3,programs=len(rows),completed=False,passed=True,
        model_input_domain='Bone-index UNORM bytes0..39, byte weights summing255 for2..4 influences, explicitly supplied40-entry finite c50..169 palette; arithmetic proof only',
        vulkan_application_api='1.3',validation_layer='VK_LAYER_KHRONOS_validation',validation_messages_fail=True,
        transport_sha256=hashlib.sha256(transport.read_bytes()).hexdigest(),
        source_sha256={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
            (ROOT/'src/ui/menu_world_vertex_source.cpp',ROOT/'tools/export_renderer_vertex_probes.cpp',
             ROOT/'tools/check_native_renderer_vertex_semantics.py',ROOT/'shaders/vertex_program.glsl',ROOT/'tests/reference/vertex_program_reference.cpp')},
        matrix_sha256=hashlib.sha256(args.matrix.read_bytes()).hexdigest(),entries=[])
    header='''#version 450
#extension GL_GOOGLE_include_directive : require
layout(local_size_x=1)in;
struct Fixture {uvec4 vertexInstructions[136];vec4 vertexAttributes[16];vec4 constants[192];uvec4 flags;};
layout(set=0,binding=0,std430)readonly buffer Inputs{Fixture fixtures[];};
layout(set=0,binding=1,std430)writeonly buffer Outputs{vec4 results[];};
Fixture material;
#define H2_EXTENDED_VERTEX_BANK
#define H2_VERTEX_REFERENCE_INPUTS
#define H2_VERTEX_REFERENCE_CONSTANTS
vec4 vpConstant(uint index){return material.constants[index];}
#include "vertex_program.glsl"
'''
    main='''void main(){uint index=gl_GlobalInvocationID.x;material=fixtures[index];
runVertexProgram();for(uint slot=0u;slot<16u;++slot)results[index*32u+slot]=vpOut[slot];
runNativeVertex();for(uint slot=0u;slot<16u;++slot)results[index*32u+16u+slot]=vpOut[slot];}
'''
    for source in exported['entries']:
        identity=source['program_sha256'];row=rows[source['full_key']]
        if 'expected_input0_components' in row and source['consumed_input_components'][0]!=row['expected_input0_components']:
            raise ValueError('Independent directed input-lane expectation differs from compiler metadata')
        text=header+(sources/source['source']).read_text()+main
        path=scratch/'probe.comp';path.write_text(text);module=scratch/'probe.spv';raw=scratch/'raw.spv'
        run([compiler,'-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),path,'-o',raw])
        run([optimizer,'--preserve-interface','-O',raw,'-o',module])
        run([validator,'--target-env','vulkan1.3',module])
        data=data_for(row,args.samples);input_path=scratch/'input.bin';input_path.write_bytes(data)
        output=scratch/'output.bin';device=run([executable,input_path,module,output],env=gpu_env,reject_validation_errors=True).strip()
        words=struct.unpack('<'+str(output.stat().st_size//4)+'I',output.read_bytes())
        different=0;examples=[]
        for case in range(args.samples):
            for lane in range(64):
                reference,native=words[case*128+lane],words[case*128+64+lane]
                if reference!=native:
                    different+=1
                    if len(examples)<12:examples.append(dict(case=case,output=lane//4,lane=lane%4,reference=reference,native=native))
        entry=dict(source,synthetic_case=row.get('synthetic_case'),device=device,compared_words=args.samples*64,differing_words=different,differences=examples,
            module_sha256=hashlib.sha256(module.read_bytes()).hexdigest(),source_sha256=hashlib.sha256(text.encode()).hexdigest(),
            input_sha256=hashlib.sha256(data).hexdigest(),passed=different==0 and (extension or source['old_corridor_rejected']))
        if row.get('synthetic_case')=='paired_position_snapshot':
            wrong=text.replace('h2RendererL1=h2RendererC1;','h2RendererL1=h2RendererM1;')
            if wrong==text:raise ValueError('Paired snapshot negative control did not change its actual source statement')
            path.write_text(wrong)
            run([compiler,'-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),path,'-o',raw])
            run([optimizer,'--preserve-interface','-O',raw,'-o',module]);run([validator,'--target-env','vulkan1.3',module])
            run([executable,input_path,module,output],env=gpu_env,reject_validation_errors=True)
            bad=struct.unpack('<'+str(output.stat().st_size//4)+'I',output.read_bytes())
            changed=sum(bad[case*128+lane]!=bad[case*128+64+lane] for case in range(args.samples) for lane in range(64))
            entry['late_ilu_read_negative_control_differing_words']=changed
            entry['passed'] &= changed>0
        report['entries'].append(entry);report['passed'] &= entry['passed']
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(identity[:12],entry['compared_words'],different,flush=True)
    report['completed']=True;args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
