#!/usr/bin/env python3
"""Physical unchanged 16EA60/6CA40/C1E90/1D0430/275AB0 actual cancel prerequisites versus typed semantic native owners.

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

BASE,SIZE,POOL,ITERATOR,BITMAP,WORDS,ROWS,TIMERS,GROUPS,GROUP_ROWS=0x1000000,0x10000,0x1000100,0x1000200,0x1001000,0x1001100,0x1006000,0x1000800,0x1000400,0x1009000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x5020a4,4),(0x50eef8,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x16ea60);rows=[];configs=[]
    for first in (-2147483648,-1,0,1,31,32,33,95,96,2147483647):
        for hit in (False,True):configs.append(dict(label=f'next-{first}-{hit}',entry=0x16ea60,first=first,hit=hit))
    configs += [dict(label='next-negative-limit',entry=0x16ea60,first=0,limit=0xffffffff),dict(label='next-cross-word',entry=0x16ea60,first=1,found=33),dict(label='next-sparse-end',entry=0x16ea60,first=0,found=95)]
    for cursor in (0xffffffff,0xfffffffe,0,30,31,32,94,95):
        for hit in (False,True):configs.append(dict(label=f'iterator-{cursor:x}-{hit}',entry=0x6ca40,cursor=cursor,hit=hit))
    for salt in (0,1,32767,32768,65535):configs.append(dict(label=f'iterator-salt-{salt}',entry=0x6ca40,cursor=0xffffffff,found=0,salt=salt))
    configs += [dict(label='iterator-full-index',entry=0x6ca40,cursor=65535,found=65536,limit=65537,stride=0),dict(label='iterator-cursor-publication-alias-salt',entry=0x6ca40,cursor=0xffffffff,found=0,alias=True)]
    for index in (-65,-64,-33,-32,-1,0,1,31,32,33,63,64,95):
        for value in (0,1,0x80000000,0xffffffff):configs.append(dict(label=f'bit-{index}-{value:x}',entry=0xc1e90,index=index,value=value))
    for match in (-1,0,1,2):configs.append(dict(label=f'timer-{match}',entry=0x1d0430,match=match))
    for group in (0x12340000,0x56780003,0xffff0007):
        for head in (0xffffffff,0x12340002):configs.append(dict(label=f'group-{group:x}-{head:x}',entry=0x275ab0,group=group,head=head))
    for options in configs:
        main=bytearray(rng.randbytes(SIZE));main[BITMAP-BASE:BITMAP-BASE+0x3000]=bytes(0x3000)
        def put(address,value,width=4):main[address-BASE:address-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
        first=options.get('first',0);cursor=options.get('cursor',0xffffffff);limit=options.get('limit',96);capacity=options.get('capacity',97);stride=options.get('stride',4);iterator=ROWS-0xc if options.get('alias')else ITERATOR
        put(POOL+0x20,capacity);put(POOL+0x24,stride);put(POOL+0x38,limit);put(POOL+0x44,ROWS);put(POOL+0x48,BITMAP)
        put(iterator+4,POOL);put(iterator+0xc,cursor)
        for n in range(100):put(ROWS+n*stride,options.get('salt',0x1234),2)
        if options.get('alias'):put(iterator+0xc,cursor)
        start=first if options['entry']==0x16ea60 else ((cursor+1)&0xffffffff)
        found=options.get('found',start if options.get('hit')and 0<=start<96 else None)
        if found is not None:
            at=BITMAP+(found>>5)*4;word=int.from_bytes(main[at-BASE:at-BASE+4],'little');put(at,word|(1<<(found&31)))
        actor=0x12340002;put(TIMERS,actor if options.get('match')in (0,2)else actor^1);put(TIMERS+0x1c,actor if options.get('match')in (1,2)else actor^1)
        group=options.get('group',0x56780003);put(GROUPS+0x44,GROUP_ROWS);put(GROUP_ROWS+(group&65535)*0x50+0x18,options.get('head',0xffffffff))
        index=options.get('index',0)
        if options['entry']==0xc1e90:put(WORDS+(index//32)*4,options['value'])
        gp=[group,POOL,first&0xffffffff if options['entry']==0x16ea60 else index&0xffffffff,actor,SP,0x56789abc,iterator,0x789abcde]
        arguments=[WORDS]if options['entry']==0xc1e90 else[iterator]if options['entry']==0x275ab0 else[]
        number=len(rows);depth=number%4;before_x87=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        row=dict(name=options['label'],entry=options['entry'],pool=POOL,iterator=iterator,first=first,words=WORDS,index=index,actor=actor,group=group,gp=gp,xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)],stack=SP,stop=STOP,arguments=arguments,before_x87=before_x87,
            before_fp=[(0x7f,0x27f,0x37f)[number%3]|((number%4)<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|((number%4)<<13)|(0,0x40,0x8000,0x8040)[number%4]],
            banks=[dict(address=BASE,before=bytes(main).hex()),dict(address=0x5020a4,before=struct.pack('<I',TIMERS).hex()),dict(address=0x50eef8,before=struct.pack('<I',GROUPS).hex())])
        if options['entry']==0x6ca40 and not options.get('alias'):row['native_local_iterator']=list(struct.unpack_from('<4I',main,iterator-BASE))
        rows.append(row)
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_cancel_support_reference_tu15.cpp'),'-o',str(observer)])
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
        try:checked([str(observer),str(owned/'reference'),hex(row['stop']),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')]+([hex(row['capture_entry'])]if 'capture_entry'in row else []))
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
        if offset!=len(data):raise ValueError('Path heap observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_cancel_support_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x16ea60:30,0x6ca40:33,0xc1e90:11,0x1d0430:24,0x275ab0:9}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Heap closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Heap original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Heap original instruction inventory count differs')
        missing=expected-reached
        if missing:raise ValueError('Heap reachable original sites missing: '+','.join(f'{p:08x}'for p in sorted(missing)))
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
    sources=[ROOT/'include/halo2/native_tu15_data_array_iteration.hpp',ROOT/'src/core/native_tu15_data_array_iteration.cpp',ROOT/'tests/native_campaign_ai_cancel_support_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_cancel_support_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_cancel_support_tu15_sites.json',ROOT/'src/campaign/ai/native_campaign_ai_cancel_support_tu15.cpp',ROOT/'include/halo2/native_campaign_ai_cancel_support_tu15.hpp',ROOT/'cmake/NativeTu15DataArrayIteration.cmake',ROOT/'cmake/CampaignNativeAiCancelSupport.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-cancel-support.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(caller_contract='actual primitive arguments/results and real6CA40->16EA60 call; native private iterator avoids source stack',native_local_iterator_profiles=sum('native_local_iterator'in row for row in fixture['cases']),original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();texts={1:sources[1].read_text(),6:sources[6].read_text()};variants={'native':None,
            'bit_signed':(1,'std::uint32_t(index>>5)','std::uint32_t(index/32)',1),
            'salt_movement':(1,'state.cursor(index,true);\n        const auto salt=read(memory,row,2);','const auto salt=read(memory,row,2);\n        state.cursor(index,true);',1),
            'datum_index':(1,'(salt<<16u)|index','(salt<<16u)|(index&0xffffu)',1),
            'failure_cursor':(1,'state.cursor(capacity,false);','state.cursor(0xffffffffu,false);',1),
            'next_limit':(1,'if(first>=limit)','if(first>limit)',1),
            'timer_stride':(6,'records+index*0x1cu','records+index*0x18u',2),
            'group_stride':(6,'(group&0xffffu)*0x50u','(group&0xffffu)*0x40u',1)}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=30)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered cancel store','ordered cancel input','cancel result differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected cancel mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_cancel LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            implementations={1:sources[1],6:sources[6]}
            if mutation:
                index,needle,replacement,count=mutation;text=texts[index]
                if text.count(needle)!=count:raise ValueError('Cancel mutation location changed: '+name)
                changed=project/(name+'.cpp');changed.write_text(text.replace(needle,replacement));implementations[index]=changed
            target='campaign_cancel_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{implementations[1]}" "{implementations[6]}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_cancel_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['truncate negative bit-word division','move salt read before cursor publication','mask full returned index','wrong failed cursor','scan limit input order','wrong timer stride','wrong group stride'],
            excluded=['full275770/1BD8F0 cancellation','real160A60virtual policy/speech/object dependencies','unmapped storage','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
