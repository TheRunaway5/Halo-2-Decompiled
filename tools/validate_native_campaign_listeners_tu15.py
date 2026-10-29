#!/usr/bin/env python3
"""Physical unchanged 12A810 listener closure versus typed semantic native queries.

Real original i386 instructions, ordered mutable input reads/stores, retained
banks, every original instruction site and actual FP. No original results are
supplied. ELF references, vectors, mutation sources and builds stay in scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
sys.dont_write_bytecode=True
from validate_native_campaign_audio_leaves_tu15 import ROOT,ORIGINAL,PIN,text_section,sha,save,checked

BASE,SIZE,SOURCE,LISTENERS,PLAYERS,STRUCTURE,SCALE,BYTES,CLUSTER_BYTES,OUTPUT=0x1000000,0x4000,0x1000100,0x1000400,0x1000800,0x1001000,0x1001800,0x1002000,0x1002400,0x1003000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4f2c50,4),(0x4f54f0,4),(0x4ecc18,4),(0x45695c,4),(0x46a148,4),(0x46a26c,4),(0x46a2d4,8))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x12a810);rows=[];configurations=[]
    for mode in (0,2,15):
        for player in (0,1,2,3,4):configurations.append(dict(label=f'nonpos-{mode}-{player}',entry=0x12a810,mode=mode,player=player))
    for active in (0,1,3,15):
        for flags in (0,0x100,0x800):
            for limit in (0,0x41200000,0xc1200000,0x7fc12345):
                configurations.append(dict(label=f'select-{active}-{flags:x}-{limit:x}',entry=0x12a810,active=active,flags=flags,limit=limit))
    for mode in (0,2,8,15):
        for value in (0x3f800000,1,0x7f800000,0x7fc12345,0x7f812345):
            configurations.append(dict(label=f'absolute-{mode}-{value:x}',entry=0x12a930,mode=mode,x=value,y=0xffc54321 if value==0x7fc12345 else 0x40000000))
    for column in (0,1):
        for z in (0xbf800000,0,0x3f000000,0x3f800000,0x40000000,0x7fc12345,0x7f812345):
            configurations.append(dict(label=f'column-{column}-{z:x}',entry=0x12a930,column=column,listener_z=z))
    for height in (0xbf800000,0x7fc12345,0x7f812345):
        configurations.append(dict(label=f'height-{height:x}',entry=0x12a930,column=1,listener_z=0x3f000000,height=height))
    for options in (dict(listener=0xffffffff),dict(flags=1),dict(mode=0),dict(source_cluster=-1),dict(listener_cluster=-1),dict(source_location=2),dict(source_cluster=0)):
        configurations.append(dict(label='cutoff-special-'+str(options),entry=0x2287b0,**options))
    for cell in (0,1,127,128,254,255):
        for scaled in (0,1,-1):
            configurations.append(dict(label=f'scale-{cell}-{scaled}',entry=0x253070,cell=cell,scaled=scaled))
            configurations.append(dict(label=f'cutoff-{cell}-{scaled}',entry=0x2287b0,cell=cell,scaled=scaled,ratio=0x40000000))
    for ratio in (0,1,0xbf800000,0x3f800000,0x7f800000,0x7fc12345,0x7f812345):
        configurations.append(dict(label=f'ratio-{ratio:x}',entry=0x2287b0,ratio=ratio))
    for lower,upper in ((0x7fc12345,0xffc54321),(0x7f812345,0x43800000),(0x7f800000,0xff800000),(0xc0000000,0x40000000)):
        configurations.append(dict(label=f'range-{lower:x}-{upper:x}',entry=0x253070,lower=lower,upper=upper))
    for first,second in ((0,0),(0,7),(7,0),(2,3),(3,2)):
        for entry in (0x253070,0x2531e0):configurations.append(dict(label=f'pair-{entry:x}-{first}-{second}',entry=entry,first=first,second=second))
    configurations += [dict(label='null-output',entry=0x12a810,output=0),dict(label='source-output-alias',entry=0x12a810,output=SOURCE+0xc),
                       dict(label='listener-output-alias',entry=0x12a810,output=LISTENERS+0x88+0x30),
                       dict(label='no-player-fallback',entry=0x12a810,active=0,flags=0x100,player=4)]
    for rounding in range(4):
        for options in configurations:
            main=bytearray(rng.randbytes(SIZE));put=lambda address,value:struct.pack_into('<I',main,address-BASE,value&0xffffffff)
            put(SOURCE,options.get('flags',0));main[SOURCE-BASE+2]=options.get('column',0);main[SOURCE-BASE+3]=options.get('mode',1)
            for address,value in ((SOURCE+0xc,options.get('x',0)),(SOURCE+0x10,options.get('y',0)),(SOURCE+0x14,options.get('z',0)),
                (SOURCE+0x28,options.get('source_location',1)),(SOURCE+0x30,options.get('height',0x3f800000))):put(address,value)
            struct.pack_into('<h',main,SOURCE-BASE+0x2c,options.get('source_cluster',1))
            for index in range(4):
                row=LISTENERS+0x88+index*0x48;put(row,2);struct.pack_into('<h',main,row-BASE+4,options.get('listener_cluster',0));main[row-BASE+6]=bool(options.get('active',15)&(1<<index))
                for address,value in ((row+0x30,(0x40400000,0x3f800000,0x40000000,0x40800000)[index]),(row+0x34,0x40800000 if index==0 else 0),(row+0x38,options.get('listener_z',0))):put(address,value)
                put(PLAYERS+0xc+index*4,0x12340000 if index==options.get('player',0)else 0xffffffff)
            put(STRUCTURE+0x9c,8);put(STRUCTURE+0xe8,CLUSTER_BYTES);put(STRUCTURE+0x224,options.get('scaled',1));put(STRUCTURE+0x228,SCALE)
            put(SCALE+4,options.get('lower',0));put(SCALE+8,options.get('upper',0x43800000));put(SCALE+0x28,BYTES)
            main[BYTES-BASE:BYTES-BASE+64]=bytes([options.get('cell',127)])*64;main[CLUSTER_BYTES-BASE:CLUSTER_BYTES-BASE+64]=bytes([options.get('cell',127)])*64
            word=lambda value:struct.pack('<I',value&0xffffffff)
            values=[main,word(LISTENERS),word(PLAYERS),word(STRUCTURE)]+[original_bytes(image,p,n)for p,n in REGIONS[4:]]
            gp=[0x12345678,0x23456789,0x3456789a,0x456789ab,SP,0x56789abc,0x6789abcd,0x789abcde];entry=options['entry'];arguments=[]
            if entry==0x12a810:arguments=[SOURCE,options.get('limit',0x41200000),options.get('output',OUTPUT)]
            elif entry==0x12a930:gp[1]=SOURCE;gp[2]=options.get('listener',0)
            elif entry==0x2287b0:gp[1]=options.get('listener',0);gp[2]=SOURCE;arguments=[options.get('ratio',0x3f800000)]
            else:
                gp[0]=options.get('first',0);gp[1]=options.get('second',1)
                if entry==0x253070:gp[2]=STRUCTURE
                else:gp[7]=STRUCTURE
            xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)]
            index=len(rows);denorm=(0,0x40,0x8000,0x8040)[index%4]
            depth=index%4;before_x87=[struct.pack('<QH',(value<<(63-(value.bit_length()-1))),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
            rows.append(dict(before_x87=before_x87,name=f'r{rounding}-'+options['label'],entry=entry,gp=gp,xmm=xmm,stack=SP,stop=STOP,arguments=arguments,
                before_fp=[(0x7f,0x27f,0x37f)[index%3]|(rounding<<10),((index%8)<<11)|((index%2)<<14)|(index%64),0x1f80|(rounding<<13)|denorm],
                banks=[dict(address=p,before=bytes(value).hex())for(p,_),value in zip(REGIONS,values)]))
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_listeners_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        sections=[f'.original 0x{address:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        for index,bank in enumerate(row['banks']):
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']));assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"');sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,SP-STACK,STOP)
        for index,value in enumerate(row['arguments']):struct.pack_into('<I',stack,SP-STACK+4+index*4,value)
        (owned/'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=row['before_fp'];depth=len(row['before_x87']);sw=(sw&~0x3800)|(((((sw>>11)+depth)&7))<<11);environment=bytearray(28);struct.pack_into('<H',environment,0,cw);struct.pack_into('<H',environment,4,sw);struct.pack_into('<H',environment,8,0xffff);(owned/'environment.bin').write_bytes(environment)
        assembly.append('.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n fldenv environment\n ldmxcsr sse_control')
        for index in range(depth-1,-1,-1):assembly.append(f' fldt extended{index}')
        for index in range(8):assembly.append(f' movdqu vector{index},%xmm{index}')
        for reg,value in zip(('eax','ecx','edx','ebx','esp','ebp','esi','edi'),row['gp']):assembly.append(f' mov ${value},%{reg}')
        assembly.append(f' int3\n jmp *entry\n.balign 4\nentry: .long {row["entry"]}\nsse_control: .long {mxcsr}\nenvironment: .incbin "{owned/"environment.bin"}"')
        for index,vector in enumerate(row['xmm']):assembly.append(f'vector{index}: .long '+','.join(map(str,vector)))
        for index,value in enumerate(row['before_x87']):assembly.append(f'extended{index}: .byte '+','.join(str(byte)for byte in bytes.fromhex(value)))
        sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly)+'\n');(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        checked(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')]);checked(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')])
        try:checked([str(observer),str(owned/'reference'),hex(STOP),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')])
        except RuntimeError as error:raise RuntimeError(row['name']+': '+str(error))from error
        data=(owned/'output.bin').read_bytes();cw,sw,mxcsr,instructions=struct.unpack_from('<4I',data);offset=16;row['after_gp']=list(struct.unpack_from('<8I',data,offset));offset+=32;row['after_xmm']=[list(struct.unpack_from('<4I',data,offset+i*16))for i in range(8)];row['after_xmm0']=row['after_xmm'][0];offset+=128;row['after_x87']=[data[offset+i*16:offset+i*16+10].hex()for i in range(len(row['before_x87']))];offset+=128;row.update(after_fp=[cw,sw,mxcsr],instructions=instructions);total+=instructions
        for bank,(_,length)in zip(row['banks'],REGIONS):bank['after']=data[offset:offset+length].hex();offset+=length
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['reads']=[]
        for _ in range(count):
            destination,length,stores_before=struct.unpack_from('<III',data,offset);offset+=12;row['reads'].append(dict(address=destination,stores_before=stores_before,value=data[offset:offset+length].hex()));offset+=4
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['writes']=[]
        for _ in range(count):
            site,destination,length=struct.unpack_from('<III',data,offset);offset+=12;row['writes'].append(dict(instruction=site,address=destination,before=data[offset:offset+length].hex(),written=data[offset+4:offset+4+length].hex()));offset+=8
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['sites']=list(struct.unpack_from('<'+'I'*count,data,offset));offset+=count*4
        if offset!=len(data):raise ValueError('Listeners observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_audio_listeners_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Listener instruction inventory image pin differs')
    counts={0x12a810:98,0x12a930:77,0x2287b0:45,0x253070:66,0x2531e0:26}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Listener closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Listener original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Listener original instruction inventory count differs')
        missing=expected-reached
        if missing:raise ValueError('Listener reachable original sites missing: '+','.join(f'{p:08x}'for p in sorted(missing)))
        coverage[f'{entry:08x}']=dict(required_sites=len(expected),reached_sites=len(expected&reached),missing_sites=[])
    return dict(image_sha256=PIN,backend='unchanged-tu15-physical-i386-interleaved-inputs-stores-fp',cases=rows,instructions=total,coverage=coverage,
                original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[])

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--native',type=Path)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not ORIGINAL.is_file():
        args.output.mkdir(parents=True,exist_ok=True)
        save(args.output/'report.json',dict(passed=False,status='skipped',validation_not_run=True,
            reason='Required owned TU1.5 offline reference is unavailable',required_reference=str(ORIGINAL),retained_large_artifacts=[]))
        print('SKIP: required owned TU1.5 offline reference is unavailable')
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();args.output.mkdir(parents=True,exist_ok=True);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[]);save(args.output/'report.json',report)
    sources=[ROOT/'include/halo2/native_campaign_audio_listeners.hpp',ROOT/'src/audio/native_campaign_audio_listeners_adapter.hpp',ROOT/'tests/native_campaign_audio_listeners_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_listeners_reference_tu15.cpp',Path(__file__),ROOT/'src/audio/native_tu15_sound_kernels_listener.cpp',ROOT/'src/audio/native_tu15_sound_kernel.hpp',ROOT/'include/halo2/native_campaign_audio_ducking.hpp',ROOT/'include/halo2/native_campaign_audio_leaves.hpp',ROOT/'tests/fixtures/native_campaign_audio_listeners_tu15_sites.json']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-listeners.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[0].read_text();variants={'native':None,'nearest':('if(above(best,measure)){chosen=index;best=measure;}','if(above(measure,best)){chosen=index;best=measure;}'),
            'column':('if(!equal(clamped,dz))return maximum_distance;','if(false)return maximum_distance;'),
            'range_reads':('const auto lower=scalar(scale.lower());const auto upper=scalar(scale.upper());','const auto upper=scalar(scale.upper());const auto lower=scalar(scale.lower());')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered listener store differs','ordered listener input count differs','ordered listener input differs','selected listener differs','listener distance/scale differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected listeners mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_listeners LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            includes=ROOT/'include'
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Listeners mutation location changed')
                includes=project/name;(includes/'halo2').mkdir(parents=True);(includes/'halo2/native_campaign_audio_listeners.hpp').write_text(text.replace(*mutation))
            target='campaign_listeners_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{ROOT}/src/audio/native_tu15_sound_kernels_listener.cpp")\ntarget_include_directories({target} BEFORE PRIVATE "{includes}" "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_listeners_(native|nearest|column|range_reads)$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['choose farthest listener','discard vertical column bounds','move upper range read before lower read'],
            excluded=['whole campaign audio closure','live playback start/retirement','immutable literal reads replaced by pinned values','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
