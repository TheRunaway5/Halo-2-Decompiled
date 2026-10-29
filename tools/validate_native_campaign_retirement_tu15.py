#!/usr/bin/env python3
"""Physical unchanged 129EA0 retirement closure versus typed semantic native owners.

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

BASE,SIZE,SOURCES,CONTROLLERS,GENERIC,SOURCE_ROWS,CONTROLLER_ROWS,GENERIC_ROWS=0x1000000,0x9000,0x1000100,0x1000200,0x1000300,0x1001000,0x1004000,0x1006000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4f2c4c,4),(0x52d668,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x129ea0);rows=[];configs=[]
    configs.append(dict(label='source-no-controller',entry=0x129ea0,controller=False))
    for category in (0,255):
        for owner in (0xffffffff,0x12340001):
            for references in (0,1,2,127,128,129,130,255):
                configs.append(dict(label=f'source-{category}-{owner:x}-{references}',entry=0x129ea0,category=category,owner=owner,references=references))
    for side in (0,1,127,128,255):configs.append(dict(label=f'side-{side}',entry=0x129ea0,category=255,side=side,references=2))
    for poison in (0,8):
        for index in (0,1,3,33):
            configs.append(dict(label=f'source-poison-{poison}-{index}',entry=0x129ea0,poison=poison,index=index,controller_index=index,references=1))
    configs.append(dict(label='controller-none',entry=0x220690,none=True))
    for references in (0,1,2,127,128,129,130,255):
        for poison in (0,8):configs.append(dict(label=f'controller-{references}-{poison}',entry=0x220690,references=references,poison=poison))
    for stride in (0,1,2,3,4,5,28,188):
        for poison in (0,8):
            for index in (0,3,33):configs.append(dict(label=f'pool-{stride}-{poison}-{index}',entry=0x16e8a0,stride=stride,poison=poison,index=index))
    configs += [dict(label='cursor-unchanged',entry=0x16e8a0,cursor=0,index=3),dict(label='nontrailing',entry=0x16e8a0,index=1,extent=4),
                dict(label='trailing-all-empty',entry=0x16e8a0,index=3,all_empty=True),dict(label='count-wrap',entry=0x16e8a0,index=0,count=0),
                dict(label='side-publication-aliases-source-controller',entry=0x129ea0,category=255,alias=True)]
    for options in configs:
        main=bytearray(rng.randbytes(SIZE));put=lambda address,value,size=4:main.__setitem__(slice(address-BASE,address-BASE+size),(value&((1<<(8*size))-1)).to_bytes(size,'little'))
        index=options.get('index',3);control_index=options.get('controller_index',1)
        for pool,bank,stride,slot,bitmap in ((SOURCES,SOURCE_ROWS,188,index,BASE+0x800),(CONTROLLERS,CONTROLLER_ROWS,28,control_index,BASE+0x900),(GENERIC,GENERIC_ROWS,options.get('stride',28),index,BASE+0xa00)):
            put(pool+0x24,stride);put(pool+0x2a,options.get('poison',0),1);put(pool+0x34,options.get('cursor',64));put(pool+0x38,options.get('extent',slot+1));put(pool+0x3c,options.get('count',2));put(pool+0x44,bank);put(pool+0x48,bitmap)
            put(bitmap,0xffffffff);put(bitmap+4,0xffffffff)
            for n in range(40):put(bank+n*stride,0 if options.get('all_empty')or n not in (0,slot)else 0x1234,2)
        source=SOURCE_ROWS+index*188;controller=CONTROLLER_ROWS+control_index*28
        put(source+3,options.get('category',255),1);put(source+0xa8,(0x56780000|control_index)if options.get('controller',True)else 0xffffffff)
        put(controller+2,options.get('references',1),1);put(controller+3,options.get('side',1),1);put(controller+4,options.get('owner',0))
        if options.get('alias'):
            address=source+0xa5-28;put(CONTROLLERS+0x44,address);put(address+2,2,1);put(address+3,7,1);put(address+4,0)
            put(address+28+2,3,1);put(address+28+3,1,1);put(address+28+4,0);put(source+0xa8,1)
        word=lambda value:struct.pack('<I',value&0xffffffff);values=[main,word(SOURCES),word(CONTROLLERS)]
        gp=[0x12340000|index,0x23456789,GENERIC,0x456789ab,SP,0x56789abc,0xffffffff if options.get('none')else 0x56780000|control_index,0x12340000|index]
        xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)];number=len(rows);depth=number%4
        before_x87=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        rows.append(dict(name=options['label'],entry=options['entry'],gp=gp,xmm=xmm,stack=SP,stop=STOP,arguments=[],before_x87=before_x87,
            before_fp=[(0x7f,0x27f,0x37f)[number%3]|((number%4)<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|((number%4)<<13)|(0,0x40,0x8000,0x8040)[number%4]],
            banks=[dict(address=p,before=bytes(value).hex())for(p,_),value in zip(REGIONS,values)]))
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_retirement_reference_tu15.cpp'),'-o',str(observer)])
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
        if offset!=len(data):raise ValueError('Retirement observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_audio_retirement_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Listener instruction inventory image pin differs')
    counts={0x129ea0:30,0x220690:22,0x16e8a0:62}
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
    sources=[ROOT/'include/halo2/native_campaign_audio_retirement.hpp',ROOT/'src/audio/native_campaign_audio_retirement_adapter.hpp',ROOT/'tests/native_campaign_audio_retirement_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_retirement_reference_tu15.cpp',Path(__file__),ROOT/'src/core/native_tu15_data_array_delete.cpp',ROOT/'src/audio/native_tu15_sound_kernel.hpp',ROOT/'include/halo2/native_tu15_data_array_delete.hpp',ROOT/'tests/fixtures/native_campaign_audio_retirement_tu15_sites.json']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-retirement.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[0].read_text();variants={'native':None,'flag':('std::uint8_t((previous&0x80u)|((previous-1u)&0x7fu))','std::uint8_t((previous-1u)&0x7fu)'),
            'reread':('const auto after_publication=source.controller();','const auto after_publication=reference;'),
            'movement':('auto controllers=state.controllers();auto controller=controllers.row(reference);\n        if(source.category()==','const auto category=source.category();auto controllers=state.controllers();auto controller=controllers.row(reference);\n        if(category==')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered retirement store differs','ordered retirement input count differs','ordered retirement input differs','retirement result differs','retirement result differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected retirement mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_retirement LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            includes=ROOT/'include'
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Retirement mutation location changed')
                includes=project/name;(includes/'halo2').mkdir(parents=True);(includes/'halo2/native_campaign_audio_retirement.hpp').write_text(text.replace(*mutation))
            target='campaign_retirement_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{ROOT}/src/core/native_tu15_data_array_delete.cpp")\ntarget_include_directories({target} BEFORE PRIVATE "{includes}" "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_retirement_(native|flag|reread|movement)$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['drop controller upper reference flag','skip source controller reread after publication','move category read before controller pool lookup'],
            excluded=['whole campaign audio closure','live playback start/retirement','immutable literal reads replaced by pinned values','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
