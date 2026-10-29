#!/usr/bin/env python3
"""Physical unchanged 217370 actual weighted speech choice with native SSE/extended math versus typed semantic native owners.

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

BASE,SIZE,CANDIDATES,RANDOM=0x1000000,0x4000,0x1001000,0x1003000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4f3cd8,4),(0x456910,4),(0x46a160,4),(0x46a0c8,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x217370);rows=[];configs=[]
    inverse=pow(1664525,-1,1<<32)
    seeds=[0,1,0xffffffff,0x12345678,((0-1013904223)*inverse)&0xffffffff,((0xffff0000-1013904223)*inverse)&0xffffffff]
    patterns=[('equal',[0x3f800000,0x3f800000]),('threshold',[0x3f800000,0x3f4ccccd]),('below',[0x3f800000,0x3f4ccccc]),('above',[0x3f800000,0x3f4cccce]),('zero',[0,0]),('negative',[0xbf800000,0xc0000000]),('signed-zero',[0x80000000,0]),('subnormal',[1,2]),('normal-edge',[0x007fffff,0x00800000]),('infinity',[0x3f800000,0x7f800000]),('two-infinities',[0x7f800000,0x7f800000]),('negative-infinity',[0xff800000,0xbf800000]),('qnan',[0x7fc01234,0x3f800000]),('snan',[0x7f800001,0x3f800000]),('nans',[0xffc01234,0x7f800001]),('overflow',[0x7f7fffff,0x7f7fffff]),('three',[0x3f800000,0x3f000000,0x3e800000]),('sum-order',[0x3f666666,0x3f800000,0x3f666666])]
    for label,weights in patterns:
        for seed in seeds:configs.append(dict(label=f'{label}-{seed:x}',weights=weights,seed=seed))
    for label,weights in [patterns[n]for n in (1,6,7,9,12,13,15)]:
        for precision in range(3):
            for rounding in range(4):configs.append(dict(label=f'fp-{label}-{precision}-{rounding}',weights=weights,seed=seeds[(precision+rounding)%len(seeds)],precision=precision,rounding=rounding))
    for count in (-32768,-2,0,1):
        for seed in seeds:configs.append(dict(label=f'count-{count}-{seed:x}',weights=[0x7f800001]if count==1 else[],count=count,seed=seed))
    configs += [dict(label='count-one-null-data-full-x87',weights=[0x7f800001],count=1,seed=0,array=0,full=True),dict(label='hundred-last',weights=[0x3f800000]*100,seed=seeds[-1]),dict(label='hundred-first',weights=[0x3f800000]*100,seed=seeds[-2]),dict(label='RNG-aliases-first-score',weights=[0x3f800000,0x3f800000],alias=True),dict(label='real-caller-private-layout',weights=[0x3f800000,0x3f4ccccd,0x3f4ccccc],seed=0x12345678,private=True)]
    for options in configs:
        main=bytearray(rng.randbytes(SIZE));weights=options['weights'];count=options.get('count',len(weights));address=SP+0x1c if options.get('private')else options.get('array',CANDIDATES);owner=CANDIDATES+0x10 if options.get('alias')else RANDOM
        record=bytearray(rng.randbytes(len(weights)*20))
        for index,weight in enumerate(weights):struct.pack_into('<I',record,index*20+16,weight)
        if not options.get('private')and address:main[address-BASE:address-BASE+len(record)]=record
        if not options.get('alias'):struct.pack_into('<I',main,RANDOM-BASE,options.get('seed',0))
        gp=[0x12345678,0x23456789,0x3456789a,0x456789ab,SP,0x56789abc,0x67890000|(count&65535),address]
        number=len(rows);depth=8 if options.get('full')else number%8
        before_x87=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        precision=options.get('precision',number%3);rounding=options.get('rounding',(number//3)%4)
        values=[bytes(main),struct.pack('<I',0 if options.get('full')else owner)]+[original_bytes(image,p,4)for p in (0x456910,0x46a160,0x46a0c8)]
        row=dict(name=options['label'],entry=0x217370,candidates=address,count=count,capacity=8-depth,gp=gp,xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)],stack=SP,stop=STOP,arguments=[],before_x87=before_x87,
            before_fp=[(0x7f,0x27f,0x37f)[precision]|(rounding<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|(((number//12)%4)<<13)|(0,0x40,0x8000,0x8040)[(number//48)%4]],banks=[dict(address=p,before=value.hex())for(p,_),value in zip(REGIONS,values)])
        if options.get('private'):
            row['banks'].append(dict(address=address,before=bytes(record).hex()));row['private_candidates']=True
        if not options.get('alias')and address:row['native_local_candidates']=True
        rows.append(row)
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_speech_choice_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        regions=[(bank['address'],len(bank['before'])//2)for bank in row['banks']];(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in regions))
        sections=[f'.original 0x{address:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        for index,bank in enumerate(row['banks']):
            if STACK<=bank['address']<STACK+0x10000:continue
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']));assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"');sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,SP-STACK,STOP)
        for index,value in enumerate(row['arguments']):struct.pack_into('<I',stack,SP-STACK+4+index*4,value)
        for bank in row['banks']:
            if STACK<=bank['address']<STACK+0x10000:
                values=bytes.fromhex(bank['before']);stack[bank['address']-STACK:bank['address']-STACK+len(values)]=values
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
        try:checked([str(observer),str(owned/'reference'),hex(row['stop']),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')]+([hex(row['capture_entry'])]if 'capture_entry'in row else []))
        except RuntimeError as error:raise RuntimeError(row['name']+': '+str(error))from error
        data=(owned/'output.bin').read_bytes();cw,sw,mxcsr,instructions=struct.unpack_from('<4I',data);offset=16;row['after_gp']=list(struct.unpack_from('<8I',data,offset));offset+=32;row['after_xmm']=[list(struct.unpack_from('<4I',data,offset+i*16))for i in range(8)];row['after_xmm0']=row['after_xmm'][0];offset+=128;row['after_x87']=[data[offset+i*16:offset+i*16+10].hex()for i in range(len(row['before_x87']))];offset+=128;row.update(after_fp=[cw,sw,mxcsr],instructions=instructions);total+=instructions
        for bank,(_,length)in zip(row['banks'],regions):bank['after']=data[offset:offset+length].hex();offset+=length
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['reads']=[]
        for _ in range(count):
            destination,length,stores_before=struct.unpack_from('<III',data,offset);offset+=12;row['reads'].append(dict(address=destination,stores_before=stores_before,value=data[offset:offset+length].hex()));offset+=4
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['writes']=[]
        for _ in range(count):
            site,destination,length=struct.unpack_from('<III',data,offset);offset+=12;row['writes'].append(dict(instruction=site,address=destination,before=data[offset:offset+length].hex(),written=data[offset+4:offset+4+length].hex()));offset+=8
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['sites']=list(struct.unpack_from('<'+'I'*count,data,offset));offset+=count*4
        if offset!=len(data):raise ValueError('Path heap observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_speech_choice_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x217370:77}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Heap closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Heap original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Heap original instruction inventory count differs')
        unreachable={int(p,16)for p in routine["unreachable_sites"]}
        if unreachable!={0x21740b}:raise ValueError("Speech choice unreachable branch differs")
        missing=(expected-unreachable)-reached
        if missing:raise ValueError('Heap reachable original sites missing: '+','.join(f'{p:08x}'for p in sorted(missing)))
        coverage[f'{entry:08x}']=dict(required_sites=len(expected-unreachable),reached_sites=len((expected-unreachable)&reached),unreachable_sites=sorted(unreachable),missing_sites=[])
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
    sources=[ROOT/'include/halo2/native_campaign_ai_speech_choice_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_speech_choice_tu15.cpp',ROOT/'tests/native_campaign_ai_speech_choice_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_speech_choice_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_speech_choice_tu15_sites.json',ROOT/'include/halo2/native_campaign_audio_ducking.hpp',ROOT/'include/halo2/native_campaign_audio_leaves.hpp',ROOT/'cmake/CampaignNativeAiSpeechChoice.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-speech-choice.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(caller_contract='actual signed count/candidate array, real shared RNG owner, one available extended temporary except count1; native private candidates match actual caller layout',native_local_profiles=sum('native_local_candidates'in row for row in fixture['cases']),original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'threshold_equal':('if(at_least(weight,threshold))','if(above(weight,threshold))'),
            'draw_divisor':('const auto normalization=read(memory,0x46a160u);','const auto normalization=0x37800000u;'),
            'cumulative_equal':('if(at_least(cumulative,scalar(target)))','if(above(cumulative,scalar(target)))'),
            'LCG':('previous*1664525u+1013904223u','previous*1664525u+1013904224u'),
            'single_rng':('if(count==1)return {0,false,false,0,0,0};','if(false)return {0,false,false,0,0,0};'),
            'rng_movement':('const auto owner=read(memory,0x4f3cd8u),previous=read(memory,owner);','const auto owner=early_owner,previous=read(memory,owner);')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=60)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered speech store','ordered speech input','speech result differs','speech FP differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected speech mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_speech LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Speech mutation location changed: '+name)
                core=project/(name+'.cpp');changed=text.replace(*mutation)
                if name=='rng_movement':changed=changed.replace('    if(count>0) {','    const auto early_owner=read(memory,0x4f3cd8u);\n    if(count>0) {')
                core.write_text(changed)
            target='campaign_speech_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_speech_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['reject score equal to threshold','replace exact1/65535 by1/65536','reject cumulative equality','changeLCG increment','advance RNG for single candidate','move RNG owner capture before score publications'],
            excluded=['full213D30/1BD8F0/constant34speech dispatch','candidate collection214EA0/publication215390','unmapped input faults','no extended scratch except count1','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
