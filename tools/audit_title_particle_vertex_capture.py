#!/usr/bin/env python3
"""Measure each resident authored particle VP against original GPU vertices.

This identifies a program from measured outputs, not its filename. Full live
declaration selection, particle simulation and rasterization remain separate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-frontend/particle_vertex_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-particle-vertex-candidates.json')
    p.add_argument('--gpu-shader',type=Path,help='explicit generated GPU program for a separate arithmetic comparison')
    p.add_argument('--gpu-sources',type=Path,nargs='+',help='explicit source inputs for a generated GPU program')
    p.add_argument('--classifications',type=int,nargs='+',help='explicit subset of resident original programs')
    args=p.parse_args()
    capture=ROOT/'analysis/frontend/xemu-capture'
    source=json.loads((capture/'title-billboard-capture.json').read_text())
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map')
    _,root=cache.tag(0xe20e0099,'vrtx')
    classes=[i for i,at in enumerate(cache.block(root+4,28))if cache.u32(at+12)]
    if args.classifications:
        if not set(args.classifications)<=set(classes):p.error('requested original classification is missing')
        classes=[i for i in classes if i in args.classifications]
    fixtures=[];inputs=bytearray();programs=[]
    for classification in classes:
        at=cache.block(root+4,28)[classification];size=cache.u32(at+12)
        start=cache.pointer(cache.u32(at+16),cache.magic,size);code=bytes(cache.data[start:start+size])
        programs.append({'classification':classification,'bytes':size,'instructions':struct.unpack_from('<H',code,2)[0],
            'sha256':hashlib.sha256(code).hexdigest()})
        for draw in source['draws']:
            event=draw['event']
            constants=(capture/f'title-billboard-{event}-stage-0-uniform-0.bin').read_bytes()[:3072]
            generic=(capture/f'title-billboard-{event}-stage-0-uniform-1.bin').read_bytes()
            assert len(generic)==240
            original_input=(capture/f'title-billboard-{event}-input-0.bin').read_bytes()
            for vertex,reference in enumerate(draw['postvs']):
                raw=original_input[vertex*8:vertex*8+8]
                attributes=struct.pack('<4f',*struct.unpack('<4h',raw))+generic
                inputs+=struct.pack('<II',0xe20e0099,classification)+constants+attributes
                fixtures.append({'classification':classification,'event':event,'vertex':vertex,
                    'original_clip':reference[:4],
                    'outputs':{0:reference[44:48],3:reference[8:12],4:reference[12:16],
                        7:reference[16:20],8:reference[20:24],9:reference[28:32],10:reference[32:36],
                        11:reference[36:40],12:reference[40:44]}})
    inputs=struct.pack('<I',len(fixtures))+inputs
    args.output.parent.mkdir(parents=True,exist_ok=True)
    input_path=args.output.with_suffix('.inputs.bin');input_path.write_bytes(inputs)
    producer_outputs={};native_gpu=None
    for mode in ('cpu','gpu'):
        output=args.output.with_suffix(f'.{mode}.bin')
        command=[str(args.binary.resolve()),str(cache.path.resolve()),str(input_path),str(output),'--'+mode]
        if mode=='gpu' and args.gpu_shader:command.append(str(args.gpu_shader.resolve()))
        process=subprocess.run(command,check=True,capture_output=True,text=True)
        print(process.stdout,end='')
        if process.stderr:print(process.stderr,end='')
        for line in process.stdout.splitlines():
            if line.startswith('Particle vertex GPU: '):native_gpu=line.removeprefix('Particle vertex GPU: ')
        raw=output.read_bytes();assert len(raw)==len(fixtures)*256
        producer_outputs[mode]=[list(struct.iter_unpack('<4f',raw[i:i+256]))for i in range(0,len(raw),256)]
    rows=[]
    f32=lambda v:struct.unpack('<f',struct.pack('<f',v))[0]
    for program in programs:
        row={**program,'checks':{}}
        for mode,results in producer_outputs.items():
            max_error={};not_exact={};first=[]
            for fixture,actual in zip(fixtures,results):
                if fixture['classification']!=program['classification']:continue
                for output,expected in fixture['outputs'].items():
                    values=list(actual[output])
                    if output==0:values[:2]=[f32(int(f32(v*16.))/16.)for v in values[:2]]
                    if output in (3,4,7,8):values=[min(1.,max(0.,v))for v in values]
                    for component,(a,b)in enumerate(zip(values,expected)):
                        key=f'o{output}.{component}'
                        error=abs(a-b);max_error[key]=max(max_error.get(key,0.),error)
                        if struct.pack('<f',a)!=struct.pack('<f',b):
                            not_exact[key]=not_exact.get(key,0)+1
                            if len(first)<8:first.append({'event':fixture['event'],'vertex':fixture['vertex'],
                                'component':key,'native':a,'original':b,'error':error})
                # Snapped screen position is a separate primary reference.
                # Clip arithmetic still needs actual native graphics-stage
                # capture before claiming identical post-VS geometry.
            row['checks'][mode]={'max_error_by_component':max_error,
                'different_words_by_component':not_exact,'first_mismatches':first,
                'all_compared_outputs_byte_exact':not not_exact}
        rows.append(row)
    report={'capture_sha256':'077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431',
        'native_gpu_device':native_gpu,
        'native_binary_sha256':hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        'gpu_shader_path':str((args.gpu_shader or args.binary.parent/'shaders/particle_vertex.comp.spv').resolve()),
        'native_shader_sha256':hashlib.sha256((args.gpu_shader or args.binary.parent/'shaders/particle_vertex.comp.spv').read_bytes()).hexdigest(),
        'native_shader_source_sha256':{str(path.resolve()):hashlib.sha256(path.read_bytes()).hexdigest()
            for path in (args.gpu_sources or ([]if args.gpu_shader else
                [ROOT/'shaders/particle_vertex.comp',ROOT/'shaders/vertex_program.glsl',ROOT/'shaders/render_view.glsl']))},
        'input_sha256':hashlib.sha256(inputs).hexdigest(),'events':[d['event']for d in source['draws']],
        'vertices_per_candidate':len(fixtures)//len(classes),'programs':rows,
        'runtime_declaration_selection_verified':False,'native_graphics_stage_verified':False,
        'whole_frame_parity_verified':False}
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    for row in rows:
        print(row['classification'],row['instructions'],{mode:sum(value['different_words_by_component'].values())
            for mode,value in row['checks'].items()})


if __name__=='__main__':main()
