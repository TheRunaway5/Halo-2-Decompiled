#!/usr/bin/env python3
"""Test captured position arithmetic after independently identifying basis rounding.

This is a host-replay arithmetic diagnosis, not a production correction or a
physical Xbox precision claim. Run via tools/run_test.py; retain only a report.
"""
import argparse
import itertools
import json
import os
from pathlib import Path
import shutil
import struct
from diagnose_original_weapon_skin_rounding import f32,bits,fma,sha
from original_weapon_gpu_evidence import load_json


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--comparison',type=Path,required=True)
    p.add_argument('--basis-diagnostic',type=Path,required=True)
    p.add_argument('--inspection',type=Path,required=True)
    p.add_argument('--postvs',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();assert 'HALO2_TEST_SCRATCH' in os.environ
    diagnosis=load_json(args.basis_diagnostic)
    identified=diagnosis['complete_host_replay_matches'];assert len(identified)==1
    assert identified[0]['settings']==dict(normalization='div',blending='muladd',dot='right',weights='mul')
    raw=(args.comparison/'input.bin').read_bytes();magic,count=struct.unpack_from('<II',raw);assert magic==0x534b4e31
    post=load_json(args.postvs/'report.json');offset=8;samples=[]
    for _ in range(count):
        event,nodes,vertices=struct.unpack_from('<III',raw,offset);offset+=12
        palette=[struct.unpack_from('<12f',raw,offset+i*48)for i in range(nodes)];offset+=nodes*48
        draw=next(d for d in post['draws']if d['event']==event)
        shader=load_json(args.inspection/f'event-{event}.json')
        constants=next(v for b in shader['stages']['0']['blocks']for v in b['values']if v['name']=='c')['members']
        source=(args.inspection/shader['stages']['0']['source']['file']).read_text()
        assert 'return vec4(dot(vec4(src0.xyz, 1.0), src1));' in source
        pos=next(a for a in shader['vertex_inputs']if a['name']=='v0'and a['used'])
        binding=next(b for b in shader['vertex_data']if b['slot']==pos['vertexBuffer'])
        input_path=args.inspection/binding['data']['file'];assert sha(input_path)==binding['data']['sha256'];source_bytes=input_path.read_bytes()
        output_path=args.postvs/draw['vertices']['file'];assert sha(output_path)==draw['vertices']['sha256'];gpu=output_path.read_bytes()
        signature_offsets={};at=0
        for signature in draw['outputs']:
            signature_offsets[signature['varName']]=at;at+=signature['compCount']*4
        assert at==draw['mesh']['vertexByteStride']
        for _ in range(vertices):
            row=struct.unpack_from('<6I4B12f',raw,offset);offset+=76
            index,active,*rest=row;bones=rest[:4];stored=rest[4:8]
            weights=[1.,0.,0.,0.]
            for i in range(1,active):weights[i]=f32(stored[i]*f32(1./255.))
            for i in range(active-1,0,-1):weights[0]=f32(weights[0]-weights[i])
            matrix=[f32(weights[active-1]*v)for v in palette[bones[active-1]]]
            for i in range(active-2,-1,-1):matrix=[f32(a+f32(weights[i]*b))for a,b in zip(matrix,palette[bones[i]])]
            packed=struct.unpack_from('<3h',source_bytes,index*binding['binding']['byteStride']+pos['byteOffset'])
            expected=[struct.unpack_from('<I',gpu,index*at+signature_offsets[f'v_vtxT{i}']+12)[0]for i in (1,2,3)]
            samples.append(dict(event=event,vertex=index,raw_position=packed,matrix=matrix,expected=expected,
                scale=constants[170]['f32'][:3],translation=constants[171]['f32'][:3],eye=constants[7]['f32'][:3]))
    assert offset==len(raw)

    def score(normalization,decompression,summation,subset):
        mismatch=0;compared=0;examples=[]
        for sample in subset:
            position=[]
            for value,scale,translation in zip(sample['raw_position'],sample['scale'],sample['translation']):
                norm=max(-1.,f32(value/32767.)if normalization=='div'else f32(value*f32(1./32767.)))
                position.append(f32(f32(norm*scale)+translation)if decompression=='muladd'else fma(norm,scale,translation))
            result=[]
            for axis in range(3):
                m=sample['matrix'][axis*4:axis*4+4];products=[f32(a*b)for a,b in zip(m,position+[1.])]
                if summation=='left4':v=f32(f32(f32(products[0]+products[1])+products[2])+products[3])
                elif summation=='right4':v=f32(products[0]+f32(products[1]+f32(products[2]+products[3])))
                elif summation=='pairs4':v=f32(f32(products[0]+products[1])+f32(products[2]+products[3]))
                elif summation=='right3_add':v=f32(f32(products[0]+f32(products[1]+products[2]))+products[3])
                elif summation=='fma_right':v=fma(m[0],position[0],fma(m[1],position[1],fma(m[2],position[2],m[3])))
                else:raise AssertionError(summation)
                result.append(bits(f32(sample['eye'][axis]-v)))
            differences=sum(a!=b for a,b in zip(result,sample['expected']));mismatch+=differences;compared+=3
            if differences and len(examples)<2:examples.append(dict(event=sample['event'],vertex=sample['vertex'],actual=result,expected=sample['expected']))
        return dict(settings=dict(normalization=normalization,decompression=decompression,summation=summation),mismatched_words=mismatch,compared_words=compared,examples=examples)

    settings=list(itertools.product(('div','mul'),('muladd','fma'),('left4','right4','pairs4','right3_add','fma_right')))
    minimal=[score(*s,samples[:1])for s in settings]
    candidates=[row['settings']for row in minimal if row['mismatched_words']==0]
    complete=[score(**s,subset=samples)for s in candidates]
    sources=args.output.with_name(args.output.stem+'-sources');sources.mkdir(exist_ok=False)
    for name in ('diagnose_original_weapon_skin_position.py','diagnose_original_weapon_skin_rounding.py','original_weapon_gpu_evidence.py'):
        shutil.copyfile(Path(__file__).with_name(name),sources/name)
    result=dict(passed=True,scope=__doc__,samples=len(samples),minimal_combinations=minimal,
        candidate_full_corpus_results=complete,complete_host_replay_matches=[r for r in complete if r['mismatched_words']==0],
        basis_diagnostic_sha256=sha(args.basis_diagnostic),failed_native_comparison_sha256=sha(args.comparison/'report.json'),
        source_files_sha256={str(p):sha(p)for p in sources.iterdir()},production_changed=False,
        physical_xbox_arithmetic_inferred=False,full_weapon_parity=False,runner_scratch=os.environ['HALO2_TEST_SCRATCH'])
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items()if k not in ('minimal_combinations','candidate_full_corpus_results','source_files_sha256')}),flush=True)


if __name__=='__main__':main()
