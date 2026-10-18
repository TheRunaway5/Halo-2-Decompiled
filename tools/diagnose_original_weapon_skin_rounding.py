#!/usr/bin/env python3
"""Isolate replay skin precision with fixed actual inputs, without production edits.

Compare single-variable arithmetic hypotheses, then their combinations. A match
describes this captured host replay only; it does not define physical NV2A rules.
Run under tools/run_test.py. Retain only the small arithmetic diagnostic report.
"""
import argparse
import ctypes
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from original_weapon_gpu_evidence import load_json


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def f32(value):
    return struct.unpack('<f',struct.pack('<f',value))[0]


def bits(value):
    return struct.unpack('<I',struct.pack('<f',value))[0]


library = ctypes.CDLL('libm.so.6')
fma = library.fmaf; fma.argtypes = [ctypes.c_float]*3; fma.restype = ctypes.c_float


def evaluate(palette, vertex, normalization='div', blending='muladd', dot='left', weights='div'):
    active, bones, stored, source = vertex['active'],vertex['bones'],vertex['stored'],vertex['values']
    w = [1.,0.,0.,0.]
    for i in range(1,active):
        w[i] = f32(stored[i]/255.) if weights=='div' else f32(stored[i]*f32(1./255.))
    for i in range(active-1,0,-1): w[0] = f32(w[0]-w[i])
    matrix = [f32(w[active-1]*value) for value in palette[bones[active-1]]]
    for i in range(active-2,-1,-1):
        matrix = [f32(a+f32(w[i]*b)) if blending=='muladd' else fma(w[i],b,a) for a,b in zip(matrix,palette[bones[i]])]
    inputs = list(source[3:])
    if normalization=='mul':
        for i,value in enumerate(inputs):
            denominator = 511 if i%3==2 else 1023
            integer = round(value*denominator)
            assert bits(f32(integer/denominator)) == bits(value)
            inputs[i] = f32(integer*f32(1./denominator))
    output = []
    for attribute in range(3):
        b = inputs[attribute*3:attribute*3+3]
        for axis in range(3):
            a = matrix[axis*4:axis*4+3]; p = [f32(x*y) for x,y in zip(a,b)]
            if dot=='left': value=f32(f32(p[0]+p[1])+p[2])
            elif dot=='right': value=f32(p[0]+f32(p[1]+p[2]))
            elif dot=='fma_left': value=fma(a[2],b[2],fma(a[1],b[1],p[0]))
            elif dot=='fma_right': value=fma(a[0],b[0],fma(a[1],b[1],p[2]))
            elif dot=='fma_xy_z': value=f32(fma(a[0],b[0],p[1])+p[2])
            elif dot=='fma_yz_x': value=f32(fma(a[1],b[1],p[2])+p[0])
            else: raise AssertionError(dot)
            output.append(bits(value))
    return output


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--comparison',type=Path,required=True)
    p.add_argument('--postvs',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args(); assert 'HALO2_TEST_SCRATCH' in os.environ
    baseline=load_json(args.comparison/'report.json'); assert not baseline['passed']
    original=load_json(args.postvs/'report.json'); assert original['passed']
    data=(args.comparison/'input.bin').read_bytes(); magic,count=struct.unpack_from('<II',data);assert magic==0x534b4e31
    offset=8; cases=[]
    for _ in range(count):
        event,nodes,vertices=struct.unpack_from('<III',data,offset);offset+=12
        palette=[struct.unpack_from('<12f',data,offset+i*48) for i in range(nodes)];offset+=nodes*48
        draw=next(d for d in original['draws'] if d['event']==event)
        raw=(args.postvs/draw['vertices']['file']).read_bytes();assert hashlib.sha256(raw).hexdigest()==draw['vertices']['sha256']
        positions={};at=0
        for signature in draw['outputs']:
            positions[signature['varName']]=at;at+=signature['compCount']*4
        assert at==draw['mesh']['vertexByteStride']
        samples=[]
        for _ in range(vertices):
            row=struct.unpack_from('<6I4B12f',data,offset);offset+=76
            index,active,*rest=row; bones=rest[:4];stored=rest[4:8];values=rest[8:]
            gpu=[struct.unpack_from('<4f',raw,index*at+positions[f'v_vtxT{i}'])for i in (1,2,3)]
            expected=[bits(row[channel])for channel in (2,1,0)for row in gpu]
            samples.append(dict(vertex=index,active=active,bones=bones,stored=stored,values=values,expected=expected))
        cases.append(dict(event=event,palette=palette,samples=samples))
    assert offset==len(data)
    defaults=dict(normalization='div',blending='muladd',dot='left',weights='div')
    choices=dict(normalization=('div','mul'),blending=('muladd','fma'),
                 dot=('left','right','fma_left','fma_right','fma_xy_z','fma_yz_x'),weights=('div','mul'))
    first=cases[0]; sample=first['samples'][0]

    def score(settings,case_samples):
        mismatches=0; compared=0
        for case,samples in case_samples:
            for vertex in samples:
                got=evaluate(case['palette'],vertex,**settings)
                mismatches+=sum(a!=b for a,b in zip(got,vertex['expected']));compared+=len(got)
        return dict(settings=settings,mismatched_words=mismatches,compared_words=compared)

    single=[score(defaults,[(first,[sample])])]
    for name,options in choices.items():
        for choice in options:
            if choice!=defaults[name]: single.append(score(dict(defaults,**{name:choice}),[(first,[sample])]))
    combinations=[]
    for values in itertools.product(*choices.values()):
        settings=dict(zip(choices,values));combinations.append(score(settings,[(first,[sample])]))
    candidates=[row['settings'] for row in combinations if row['mismatched_words']==0]
    full=[score(settings,[(case,case['samples'])for case in cases])for settings in candidates]
    result=dict(passed=True,scope=__doc__,minimal_event=first['event'],minimal_vertex=sample['vertex'],
        baseline_minimal_basis_mismatches=single[0]['mismatched_words'],single_variable_hypotheses=single,
        combination_count=len(combinations),zero_mismatch_minimal_candidates=candidates,
        candidate_full_corpus_results=full,complete_host_replay_matches=[r for r in full if r['mismatched_words']==0],
        failed_native_comparison_sha256=sha(args.comparison/'report.json'),input_sha256=sha(args.comparison/'input.bin'),
        original_postvs_sha256=sha(args.postvs/'report.json'),tool_sha256=sha(Path(__file__)),
        production_changed=False,physical_xbox_arithmetic_inferred=False,full_weapon_parity=False,
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'])
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items()if k not in ('single_variable_hypotheses','zero_mismatch_minimal_candidates','candidate_full_corpus_results')}),flush=True)


if __name__=='__main__':main()
