#!/usr/bin/env python3
"""Offline hardware differential of fixed combiner lowering and source evaluator.

The evaluator exists only in this disposable diagnostic compute module. It is
excluded from production shader inventories. No framebuffer parity is inferred.
"""
import argparse
import hashlib
import json
import lzma
import math
import os
from pathlib import Path
import random
import re
import shutil
import struct
import subprocess
import sys

from native_world_fragment_kernel import lower_combiner_program
from compile_native_fragment_kernels import controls, key
from native_spirv_precision import fixed_combiner_precision

ROOT = Path(__file__).resolve().parents[1]


def run(command, *, env=None, reject_validation_errors=False):
    result = subprocess.run([str(x) for x in command], capture_output=True, text=True,env=env)
    if result.returncode:
        raise RuntimeError('Combiner semantic probe failed: '+(result.stdout+result.stderr)[-6000:])
    if reject_validation_errors and re.search(r'Validation Error|VUID-',result.stdout+result.stderr):
        raise RuntimeError('Vulkan validation rejected the semantic probe: '+(result.stdout+result.stderr)[-6000:])
    return result.stdout


def validation_environment():
    validation=ROOT/'.tools/validation/usr'
    if not (validation/'share/vulkan/explicit_layer.d').is_dir():
        raise RuntimeError('Required Khronos Vulkan validation layer is unavailable')
    return {**os.environ,'VK_INSTANCE_LAYERS':'VK_LAYER_KHRONOS_validation',
        'VK_LAYER_PATH':str(validation/'share/vulkan/explicit_layer.d'),
        'LD_LIBRARY_PATH':str(validation/'lib')+(':'+os.environ['LD_LIBRARY_PATH'] if os.environ.get('LD_LIBRARY_PATH') else '')}


def unproved_directed():
    result = []
    for mapping in range(8):
        words = [0]*60
        words[53] = 1|0x1100
        words[54] = 1
        words[34] = ((mapping*32+8)<<24)|(0xc9<<16)|(0x2a<<8)|0x8b
        words[0] = ((mapping*32+24)<<24)|(0xd9<<16)|(0x3a<<8)|0x9b
        words[45] = (mapping<<15)|0x00000c00|((mapping&3)<<12)|((mapping&1)<<14)
        words[26] = (mapping<<15)|0x00000c00|((mapping&1)<<14)
        words[8],words[9] = 0x0c200000,0x00001c00
        result.append(words)
    rng = random.Random(0x15c0b1)
    for index in range(16):
        words = [0]*60
        count = 1 if index&1 else 2+(index%3)
        words[53] = count|0x11000|(0x100 if count>1 or index&2 else 0)
        words[54] = index&1
        for stage in range(count):
            for offset in (0,34):
                operands = [rng.randrange(256) for _ in range(4)]
                words[offset+stage] = sum(value<<(24-i*8) for i,value in enumerate(operands))
            for offset in (26,45):
                # AB/CD/SUM target collisions expose commit ordering. RGB
                # blue-to-alpha writes must precede the alpha snapshot commit.
                words[offset+stage] = rng.randrange(1<<4)|0xc0|0xc00|rng.randrange(1<<8)<<12
        words[8],words[9] = rng.getrandbits(32),rng.getrandbits(32)
        result.append(words)
    return result


def directed():
    """Defined register/mapping encodings, with adversarial valid collisions."""
    result = []
    output_modes=(0,1,2,3,4,6)
    for mapping in range(8):
        words=unproved_directed()[mapping]
        words[45]=(output_modes[mapping%6]<<15)|(words[45]&0x7fff)
        words[26]=(output_modes[mapping%6]<<15)|(words[26]&0x7fff)
        result.append(words)
    rng=random.Random(0x15c0b1)
    readable=(0,1,2,3,4,5,8,9,10,11,12,13)
    writable=(0,4,5,8,9,10,11,12,13)
    for index in range(16):
        words=[0]*60;count=1 if index&1 else 2+(index%3)
        words[53]=count|0x11000|(0x100 if count>1 or index&2 else 0)
        words[54]=index&1
        for stage in range(count):
            for offset in (0,34):
                operands=[rng.choice(readable)|(rng.randrange(2)<<4)|(rng.randrange(8)<<5) for _ in range(4)]
                words[offset+stage]=sum(value<<(24-i*8) for i,value in enumerate(operands))
            for offset in (26,45):
                operation=rng.choice(writable)|(rng.choice(writable)<<4)|(12<<8)|(rng.choice(output_modes)<<15)|(rng.randrange(2)<<14)
                if offset==45:operation|=(rng.randrange(4)<<12)|(rng.randrange(4)<<18)
                words[offset+stage]=operation
        operands=[]
        for variable in range(7):
            available=readable+((15,) if variable==0 else (14,15) if variable<4 else ())
            register=rng.choice(available)
            operands.append(register|((rng.randrange(2)<<4) if register<14 else 0)|(rng.randrange(2)<<5))
        words[8]=sum(value<<(24-i*8) for i,value in enumerate(operands[:4]))
        words[9]=(operands[4]<<24)|(operands[5]<<16)|(operands[6]<<8)|(rng.randrange(8)<<5)
        result.append(words)
    return result


def programs(paths, include_directed=True, unsupported_directed=False):
    rows = (unproved_directed() if unsupported_directed else directed()) if include_directed else []
    for path in ([paths] if isinstance(paths,Path) else paths or []):
        document = json.loads(path.read_bytes())
        for field in ('entries','fragment_programs'):
            for row in document.get(field,[]):
                value = row.get('controls',row.get('program'))
                if isinstance(value,list) and len(value)==60:
                    rows.append(value)
    unique = {key(controls(row)):controls(row) for row in rows}
    return sorted(unique.items())


def shader(rows, float_bank, dynamic_reference, preserve_source_operations=False):
    source = (ROOT/'shaders/combiner.glsl').read_text()
    header = '''#version 450
#extension GL_EXT_control_flow_attributes : require
layout(local_size_x=1)in;
struct Fixture {uvec4 program[15];vec4 samples[4];vec4 color0;vec4 color1;vec4 fog;vec4 constants[18];};
layout(set=0,binding=0,std430)readonly buffer Inputs{Fixture fixtures[];};
layout(set=0,binding=1,std430)writeonly buffer Outputs{vec4 results[];};
Fixture material;
#define H2_COMBINER_FOG() material.fog
#define H2_FIXED_STAGE_UNROLL [[unroll]]
'''
    if float_bank:
        header += '''vec4 floatConstant(uint index){return material.constants[index==43u?16u:index==44u?17u:index>=18u?2u*(index-18u)+1u:2u*(index-10u)];}
#define H2_COMBINER_CONSTANT(index) floatConstant(index)
'''
    body = header
    for index,(_,words) in enumerate(rows if not dynamic_reference else rows[:1]):
        reference = source
        if not dynamic_reference:
            # The old renderer specializes every control word before pipeline
            # execution. Keep the source evaluator's arithmetic untouched,
            # with only its word accessor bound to that identical fixed key.
            accessor = 'uint word(uint index){const uint controls[60]=uint[60]('+','.join(str(x)+'u' for x in words)+');\n'
            accessor += 'if((index>=10u&&index<26u)||index==43u||index==44u)return material.program[index/4u][index%4u];return controls[index];}'
            reference = re.sub(r'uint word\(uint index\)\s*\{.*?^\}',accessor,
                               reference,count=1,flags=re.DOTALL|re.MULTILINE)
        for old,new in {'registers':'referenceRegisters','runCombiners':'runReference',
                        'argb':'referenceArgb','word':'referenceWord',
                        'inputValue':'referenceInput','outputValue':'referenceOutput',
                        'storeOutput':'referenceStore','combine':'referenceCombine',
                        'commit':'referenceCommit'}.items():
            reference = re.sub(r'\b'+old+r'\b',new+str(index),reference)
        body += reference+'\n'
        if not float_bank:
            body += '#undef H2_COMBINER_CONSTANT\n'
    for index,(_,words) in enumerate(rows):
        color = (lambda i:f'material.constants[{16 if i==43 else 17 if i==44 else 2*(i-18)+1 if i>=18 else 2*(i-10)}]') if float_bank else None
        lowered = lower_combiner_program(words,color_expression=color,
            preserve_source_operations=preserve_source_operations)
        for old in ('registers','runCombiners','argb'):
            lowered = re.sub(r'\b'+old+r'\b',old+str(index),lowered)
        body += lowered+'\n'
    if dynamic_reference:
        body += 'vec4 referenceSelected(uint id){return runReference0(material.samples,material.color0,material.color1);}\n'
    else:
        body += 'vec4 referenceSelected(uint id){switch(id){\n'
        for index in range(len(rows)):
            body += f'case {index}u:return runReference{index}(material.samples,material.color0,material.color1);\n'
        body += 'default:return vec4(-99);}}\n'
    body += 'vec4 selected(uint id){switch(id){\n'
    for index in range(len(rows)):
        body += f'case {index}u:return runCombiners{index}(material.samples,material.color0,material.color1);\n'
    body += 'default:return vec4(-99);}}\n'
    body += '''void main(){uint index=gl_GlobalInvocationID.x;material=fixtures[index];
uint id=index/SAMPLES;
results[index*3u]=referenceSelected(id);
results[index*3u+1u]=selected(id);
results[index*3u+2u]=selected((id+1u)%PROGRAMS);
}
'''
    return body


def fixtures(rows, count, unorm_inputs, byte_float_bank=False):
    raw = bytearray()
    for identity,control in rows:
        rng = random.Random(0x1500c0de^int(identity[:16],16))
        for case in range(count):
            words = control.copy()
            for index in (*range(10,26),43,44):
                words[index] = rng.getrandbits(32)
            values = []
            for _ in range(4):
                values += [rng.randrange(256)/255 if unorm_inputs else rng.randrange(-128,256)/128 for _ in range(3)]+[rng.randrange(256)/255]
            for _ in range(3):
                values += [rng.randrange(256)/255 if unorm_inputs else rng.randrange(-128,256)/128 for _ in range(3)]+[rng.randrange(256)/255]
            # The explicit float bank deliberately retains values beyond byte
            # precision. Repacking it would be a real family-wrapper mismatch.
            for slot in range(18):
                if byte_float_bank:
                    color=words[10+slot//2+(slot%2)*8 if slot<16 else 43+slot-16]
                    values += [((color>>shift)&255)/255 for shift in (16,8,0,24)]
                else:
                    values += [rng.randrange(1<<20)/(1<<20) for _ in range(4)]
            raw += struct.pack('<60I100f',*words,*values)
    return raw


def no_contraction(path, native_only=False):
    """Diagnostic-only SPIR-V decoration of floating arithmetic results."""
    words=list(struct.unpack('<'+str(path.stat().st_size//4)+'I',path.read_bytes()))
    position=5;insertion=None;results=[];existing=set();names={};function=None
    records=[]
    while position<len(words):
        size,opcode=words[position]>>16,words[position]&65535
        if not size:raise ValueError('Malformed compute module')
        if insertion is None and 19<=opcode<=39:insertion=position
        if opcode==5:
            names[words[position+1]]=struct.pack('<'+'I'*(size-2),*words[position+2:position+size]).split(b'\0',1)[0].decode()
        if opcode==54:function=words[position+2]
        if opcode in (129,131,133,136,142,148):records.append((function,words[position+2]))
        if opcode==56:function=None
        if opcode==71 and words[position+2]==42:existing.add(words[position+1])
        position+=size
    results=[result for function,result in records if not native_only or
             re.match(r'^(runCombiners|argb)\d+\(',names.get(function,''))]
    if native_only and not results:raise ValueError('Raw module lacks independently named native arithmetic bodies')
    if insertion is None:raise ValueError('Compute module has no type section')
    annotations=[]
    for result in sorted(set(results)-existing):annotations += [(3<<16)|71,result,42]
    words[insertion:insertion]=annotations
    path.write_bytes(struct.pack('<'+str(len(words))+'I',*words))
    return len(results)


def runner(path, fixture_definition=None, fixture_size=640, output_vectors=3):
    # Reuse the project's small, dependency-free Vulkan compute transport;
    # only its input/output ABI changes. No Cache or Renderer build is needed.
    source = (ROOT/'tests/texture_program_tests.cpp').read_text().split('int main() {',1)[0]
    definition=fixture_definition or 'struct Fixture {std::array<unsigned,60> program;std::array<Vector,4> samples;Vector color0,color1,fog;std::array<Vector,18> constants;};'
    source = re.sub(r'struct Fixture \{.*?static_assert\(sizeof\(Fixture\)==160\);',
        definition+f'static_assert(sizeof(Fixture)=={fixture_size});',
        source, count=1, flags=re.DOTALL)
    source = source.replace('fixtures.size()*4*sizeof(Vector)',f'fixtures.size()*{output_vectors}*sizeof(Vector)')
    source = source.replace('result(fixtures.size()*4)',f'result(fixtures.size()*{output_vectors})')
    source = source.replace('app.apiVersion=VK_API_VERSION_1_1','app.apiVersion=VK_API_VERSION_1_3')
    source = source.replace('for(auto candidate:devices) {','for(auto candidate:devices) { VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(candidate,&props);if(props.deviceType==VK_PHYSICAL_DEVICE_TYPE_CPU)continue;')
    source += '''int main(int argc,char**argv)try{
if(argc!=4)throw std::runtime_error("runner INPUT SHADER OUTPUT");
std::ifstream input(argv[1],std::ios::binary|std::ios::ate);if(!input)throw std::runtime_error("input unavailable");
auto size=std::size_t(input.tellg());if(!size||size%sizeof(Fixture))throw std::runtime_error("input ABI");
std::vector<Fixture> fixtures(size/sizeof(Fixture));input.seekg(0);input.read(reinterpret_cast<char*>(fixtures.data()),size);
Compute computer;auto values=computer.run(fixtures);std::ofstream output(argv[3],std::ios::binary);output.write(reinterpret_cast<const char*>(values.data()),values.size()*sizeof(Vector));
VkPhysicalDeviceProperties props;vkGetPhysicalDeviceProperties(computer.physical,&props);std::cout<<props.deviceName<<"\\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\\n";return 1;}
'''
    source = source.replace('std::ifstream file(HALO2_TEXTURE_TEST_SHADER,','std::ifstream file(shader_path,')
    source = source.replace('std::vector<Vector> run(const std::vector<Fixture> &fixtures) {',
        'std::vector<Vector> run(const std::vector<Fixture> &fixtures,const char* shader_path) {')
    source = source.replace('computer.run(fixtures)','computer.run(fixtures,argv[2])')
    path.write_text(source)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--registry',type=Path,action='append',default=[])
    parser.add_argument('--only-registry',action='store_true')
    parser.add_argument('--unsupported-directed',action='store_true',help='Reproduce the initial unproved/reserved synthetic encodings; not source acceptance')
    parser.add_argument('--program-key',action='append',default=[])
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--samples',type=int,default=64)
    parser.add_argument('--batch',type=int,default=12)
    parser.add_argument('--dynamic-reference',action='store_true',help='Diagnostic generic oracle without the old pipeline control specialization')
    parser.add_argument('--unorm-inputs',action='store_true',help='Restrict textures and primary/secondary/fog inputs to finite byte colors in 0..1')
    parser.add_argument('--byte-float-bank',action='store_true',help='Use the actual menu/patchy CPU byte/255 constant expansion instead of arbitrary float-bank API stress')
    parser.add_argument('--validation-layer',action='store_true')
    parser.add_argument('--ir-prefix',type=Path,help='Explicitly retain bounded lossless SPIR-V text diagnostics under this report prefix')
    parser.add_argument('--no-contraction',action='store_true',help='Diagnostic SPIR-V NoContraction on both source and native arithmetic')
    parser.add_argument('--native-no-contraction',action='store_true',help='Keep generic source unchanged; decorate native arithmetic before optimization only')
    parser.add_argument('--preserve-source-operations',action='store_true',help='Diagnostic native GLSL precise AB/CD/SUM boundaries; original generic source remains unchanged')
    args = parser.parse_args()
    if not 1<=args.samples<=1024 or not 2<=args.batch<=64:
        parser.error('Bounded sample/batch counts required')
    if args.no_contraction and args.native_no_contraction:parser.error('Choose one contraction diagnostic')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)/'fragment-semantics'
    scratch.mkdir()
    compiler,optimizer,validator,cxx = [shutil.which(name) for name in ('glslangValidator','spirv-opt','spirv-val','c++')]
    if not all((compiler,optimizer,validator,cxx)):
        raise RuntimeError('Complete native shader/C++ toolchain required')
    cpp = scratch/'runner.cpp';runner(cpp);executable=scratch/'runner'
    run([cxx,'-std=c++20','-O1','-I'+str(ROOT/'include'),cpp,'-lvulkan','-o',executable])
    rows = programs(args.registry,not args.only_registry,args.unsupported_directed)
    if args.program_key:
        wanted=set(args.program_key)
        if wanted-{identity for identity,_ in rows}:raise ValueError('Requested combiner key missing from complete inputs')
        rows=[row for row in rows if row[0] in wanted]
    if not rows:raise ValueError('No combiner controls requested')
    report = dict(schema='HALO2_NATIVE_FRAGMENT_SEMANTICS_V1',
        scope='Hardware compute comparison against unchanged repository combiner source; no Xbox framebuffer or texture sampling precision claim',
        registries={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in args.registry},programs=len(rows),samples=args.samples,
        directed_scope='unsupported encodings diagnostic' if args.unsupported_directed else 'defined register and mapping encodings',
        reference_control_specialization=not args.dynamic_reference,
        input_domain='finite_byte_unorm' if args.unorm_inputs else 'finite_signed_rgb_unorm_alpha',
        float_bank_domain='completed_cpu_byte_expansion' if args.byte_float_bank else 'arbitrary_finite_float_api_stress',
        no_contraction=args.no_contraction,
        native_no_contraction=args.native_no_contraction,
        preserve_source_operations=args.preserve_source_operations,
        vulkan_application_api='1.3',vulkan_validation_layer=args.validation_layer,
        source_sha256={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in
            (ROOT/'shaders/combiner.glsl',ROOT/'tools/native_world_fragment_kernel.py',ROOT/'tools/compile_native_fragment_kernels.py',Path(__file__).resolve())},
        expected_batches=2*math.ceil(len(rows)/args.batch),completed=False,batches=[],passed=True)
    for first in range(0,len(rows),args.batch):
        batch=rows[first:first+args.batch]
        data=fixtures(batch,args.samples,args.unorm_inputs,args.byte_float_bank);input_path=scratch/'input.bin';input_path.write_bytes(data)
        for float_bank in (False,True):
            path=scratch/'probe.comp';path.write_text(shader(batch,float_bank,args.dynamic_reference,args.preserve_source_operations))
            raw=scratch/'raw.spv';module=scratch/'probe.spv';output=scratch/'output.bin'
            run([compiler,'-V','--target-env','vulkan1.3',f'-DSAMPLES={args.samples}u',f'-DPROGRAMS={len(batch)}u',path,'-o',raw])
            if args.native_no_contraction:
                precision_raw,precision=fixed_combiner_precision(raw.read_bytes())
                raw.write_bytes(precision_raw)
                before_decorations=precision['float32_arithmetic_results']
            else:
                before_decorations=no_contraction(raw) if args.no_contraction else 0
            run([validator,'--target-env','vulkan1.3',raw])
            run([optimizer,'-O',raw,'-o',module])
            decorations=no_contraction(module) if args.no_contraction else before_decorations
            run([validator,'--target-env','vulkan1.3',module])
            device=run([executable,input_path,module,output],env=validation_environment() if args.validation_layer else None,
                       reject_validation_errors=args.validation_layer).strip()
            words=struct.unpack('<'+str(output.stat().st_size//4)+'I',output.read_bytes())
            actual=[];negative=0;maximum=0.0;different=0;byte_different=0
            per_program={identity:dict(compared_words=args.samples*4,differing_words=0,
                maximum_absolute_error=0.0,derived_rgba8_differences=0,examples=0)
                for identity,_ in batch}
            for index in range(len(batch)*args.samples):
                reference=words[index*12:index*12+4];native=words[index*12+4:index*12+8];wrong=words[index*12+8:index*12+12]
                negative+=sum(a!=b for a,b in zip(reference,wrong))
                for lane,(a,b) in enumerate(zip(reference,native)):
                    if a!=b:
                        different+=1
                        identity=batch[index//args.samples][0];program_result=per_program[identity]
                        program_result['differing_words']+=1
                        af,bf=struct.unpack('<2f',struct.pack('<2I',a,b))
                        delta=abs(af-bf)
                        maximum=max(maximum,delta if math.isfinite(delta) else 1e30)
                        program_result['maximum_absolute_error']=max(program_result['maximum_absolute_error'],delta if math.isfinite(delta) else 1e30)
                        if math.isfinite(af) and math.isfinite(bf):
                            changed_byte=round(min(max(af,0),1)*255)!=round(min(max(bf,0),1)*255)
                            byte_different+=changed_byte;program_result['derived_rgba8_differences']+=changed_byte
                        if program_result['examples']<4:
                            program_result['examples']+=1
                            source_words=struct.unpack_from('<60I100f',data,index*640)
                            actual.append(dict(program=batch[index//args.samples][0],sample=index%args.samples,lane=lane,reference=a,native=b,
                                source_program=list(source_words[:60]),input_samples=list(source_words[60:76]),input_color0=list(source_words[76:80]),
                                input_color1=list(source_words[80:84]),input_fog=list(source_words[84:88]),
                                float_constants=list(source_words[88:]) if float_bank else None))
            entry=dict(first=first,count=len(batch),float_bank=float_bank,device=device,
                compared_words=len(batch)*args.samples*4,differing_words=different,differences=actual,maximum_absolute_error=maximum,
                shader_source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),module_sha256=hashlib.sha256(module.read_bytes()).hexdigest(),input_sha256=hashlib.sha256(data).hexdigest(),
                derived_rgba8_differences=byte_different,derived_rgba8_method='CPU nearest-even round(clamp(float,0,1)*255); real attachment bytes require renderer comparison',
                no_contraction_decorations=decorations,wrong_native_module_different_words=negative,
                program_results=per_program,passed=different==0)
            if args.ir_prefix:
                entry['retained_ir']=[]
                for label,source in (('before-optimizer',raw),('after-optimizer',module)):
                    text=run([shutil.which('spirv-dis'),source]).encode()
                    packed=lzma.compress(text)
                    if len(packed)>1024*1024:raise ValueError('SPIR-V text diagnostic exceeds its bounded retention')
                    destination=args.ir_prefix.with_name(args.ir_prefix.name+f'-{first}-'+('float' if float_bank else 'packed')+'-'+label+'.spvasm.xz')
                    destination.parent.mkdir(parents=True,exist_ok=True);destination.write_bytes(packed)
                    if lzma.decompress(destination.read_bytes())!=text:raise ValueError('SPIR-V diagnostic retention was not lossless')
                    entry['retained_ir'].append(dict(path=str(destination),sha256=hashlib.sha256(packed).hexdigest(),bytes=len(packed),decoded_sha256=hashlib.sha256(text).hexdigest(),decoded_bytes=len(text),verified_lossless=True))
            report['batches'].append(entry);report['passed'] &= not actual
            args.report.parent.mkdir(parents=True,exist_ok=True)
            args.report.write_text(json.dumps(report,indent=2)+'\n')
            print(json.dumps({k:v for k,v in entry.items() if k!='differences'}),flush=True)
    if not any(row['wrong_native_module_different_words'] for row in report['batches']):
        report['passed']=False;report['negative_control_failed']=True
    report['completed']=True
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
