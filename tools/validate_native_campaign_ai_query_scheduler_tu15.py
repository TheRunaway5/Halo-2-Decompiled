#!/usr/bin/env python3
"""Physical unchanged 278930/278720/278A50/2787F0/278B50/2786D0 query scheduler closure versus typed semantic native owners.

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

BASE,SIZE,ACTORS,QUERIES,CLOCK,ACTOR_ROWS,QUERY_ROWS,REQUEST=0x1000000,0x12000,0x1000100,0x1000200,0x1000300,0x1001000,0x1007000,0x100a000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x5020c8,4),(0x50eefc,4),(0x51df54,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x278720);rows=[];configs=[]
    for index in range(4):
        configs += [dict(label=f'find-{index}',entry=0x2786d0,find=index),dict(label=f'select-free-{index}',entry=0x278720,free=index),dict(label=f'select-expired-{index}',entry=0x278720,expired=index),dict(label=f'select-evict-{index}',entry=0x278720,best=index,priority=7)]
    configs += [dict(label='find-none',entry=0x2786d0,find=-1),dict(label='select-reject-equal',entry=0x278720,best=2,priority=2),dict(label='select-none-low-enough',entry=0x278720,all_high=True,priority=32767),dict(label='select-first-tie',entry=0x278720,all_equal=True,priority=3),dict(label='select-signed-expiration',entry=0x278720,expired=0,expiration=0x80000000),dict(label='select-signed-priority',entry=0x278720,best=3,best_priority=-32768,priority=-32767)]
    for index in (-1,0,3):configs.append(dict(label=f'release-{index}',entry=0x278a50,slot=index))
    configs.append(dict(label='release-index-publication-alias',entry=0x278a50,alias='release'))
    for member in range(11):configs.append(dict(label=f'reserve-member-{member}',entry=0x2787f0,free=0,member=member,expiration_bits=(0x7f800001,0x80000000,0x7fc01234)[member%3]))
    configs += [dict(label='reserve-select-fails',entry=0x2787f0,all_high=True,priority=1),dict(label='reserve-expired-member',entry=0x2787f0,expired=1,member=5)]
    for mode in ('empty','one','many','excluded','expired','reject','alias','caller-layout'):
        configs.append(dict(label='broadcast-'+mode,entry=0x278930,broadcast=mode))
    for count in (0,1,2,65535):configs.append(dict(label=f'retire-count-{count}',entry=0x278b50,count=count))
    for index in range(4):configs.append(dict(label=f'retire-find-{index}',entry=0x278b50,count=1,find=index,retire_actor=True))
    for invalid in ('flags','extent','zero-salt','wrong-salt','no-slot','other-query','different-stride'):
        configs.append(dict(label='retire-actor-'+invalid,entry=0x278b50,count=1,retire_actor=True,invalid=invalid))
    for poison,stride,index in ((0,188,0),(8,188,0),(8,187,0),(0,188,3),(8,188,3)):
        configs.append(dict(label=f'retire-pool-{poison}-{stride}-{index}',entry=0x278b50,count=1,poison=poison,stride=stride,query_index=index,extent=index+1))
    configs += [dict(label='retire-member-and-query-publication-alias',entry=0x278b50,count=1,alias='retire'),dict(label='retire-live-count-wrap',entry=0x278b50,count=1,live_count=0)]
    for options in configs:
        main=bytearray(rng.randbytes(SIZE))
        def put(address,value,width=4):main[address-BASE:address-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
        actor=0x12340002;query=0x56780000|options.get('query_index',3);slot=options.get('slot',0);priority=options.get('priority',7);expiration_bits=options.get('expiration_bits',0x7fc01234)
        for pool,bank,stride in ((ACTORS,ACTOR_ROWS,0x888),(QUERIES,QUERY_ROWS,options.get('stride',188))):
            put(pool+0x24,stride);put(pool+0x2a,options.get('poison',0)if pool==QUERIES else 0,1);put(pool+0x34,64);put(pool+0x38,options.get('extent',8)if pool==QUERIES else 8);put(pool+0x3c,options.get('live_count',8));put(pool+0x44,bank);put(pool+0x48,BASE+0x800)
        put(BASE+0x800,0xffffffff);put(CLOCK+8,0)
        for n in range(8):
            row=ACTOR_ROWS+n*0x888;put(row,0x1234,2)
            for index in range(-1,4):
                at=row+0x194+index*12;put(at,0x1b,2);put(at+2,index%4,2);put(at+4,query);put(at+8,100)
        for n in range(8):
            row=QUERY_ROWS+n*188;put(row,0x5678 if n in (0,query&65535)else 0,2)
            for member in range(-1,11):
                at=row+member*12;put(at+4,0xffffffff);put(at+8,1,2);put(at+10,options.get('best_priority',2)if member==options.get('best',-2)else 8 if options.get('all_high')else 2 if options.get('all_equal')else 6,2);put(at+12,100)
            put(row+0x7c,options.get('count',1),2)
        row=ACTOR_ROWS+(actor&65535)*0x888;query_row=QUERY_ROWS+(query&65535)*188
        if 'free'in options:put(row+0x194+options['free']*12,0xffff,2)
        if 'expired'in options:put(row+0x194+options['expired']*12+8,options.get('expiration',0xffffffff))
        if 'find'in options:
            for index in range(4):put(row+0x194+index*12+4,query if index==options['find']else query^1)
        put(REQUEST,0x1b,2);put(REQUEST+4,query);put(REQUEST+8,expiration_bits);put(REQUEST+0x10,query);put(REQUEST+0x18,0,2)
        if options['entry']==0x278a50:
            at=row+0x194+slot*12;put(at+2,0,2);put(at+4,query)
        if options['entry']==0x2787f0:
            first=options.get('member',0)
            for member in range(min(first,10)):put(query_row+member*12+4,actor+1)
            if first>=10:put(query_row+0x7c,0xffffffff)
        if 'broadcast'in options:
            mode=options['broadcast']
            for n in range(8):
                for index in range(4):put(ACTOR_ROWS+n*0x888+0x194+index*12,0xffff if mode!='reject'else 0x1b,2)
            if mode in ('one','excluded','expired','reject','caller-layout'):put(query_row+4,actor)
            if mode=='many':
                for member in range(10):put(query_row+member*12+4,0x12340000|(member%8))
            if mode=='expired':
                put(row+0x194,0x1b,2);put(row+0x194+8,0xffffffff)
            if mode=='reject':
                for member in range(4):put(query_row+member*12+10,8,2)
            if mode=='alias':
                request=query_row;actor=0x56780000|(query&65535);row=ACTOR_ROWS+(actor&65535)*0x888
                put(query_row+4,query);put(query_row+0xa,2,2);put(row+0x194,0x1b,2);put(row+0x194+2,0,2);put(row+0x194+4,query);put(row+0x194+8,0xffffffff)
        if options.get('retire_actor'):
            put(query_row+16,actor);put(query_row+20,0 if options.get('invalid')!='flags'else 1,2)
            put(row+0x194+options.get('find',0)*12+4,query)
            mode=options.get('invalid')
            if mode=='extent':put(ACTORS+0x38,2)
            if mode=='zero-salt':put(row,0,2)
            if mode=='wrong-salt':put(row,0x1235,2)
            if mode in ('no-slot','other-query'):
                for index in range(4):put(row+0x194+index*12,0xffff if mode=='no-slot'else 0x1b,2);put(row+0x194+index*12+4,query^1)
            if mode=='different-stride':put(ACTORS+0x24,0x880);put(ACTOR_ROWS+(actor&65535)*0x880,0x1234,2)
        request=locals().get('request',REQUEST)
        if options.get('alias')=='release':
            at=row+0x194;put(at+4,0);put(at+2,0,2);put(QUERIES+0x44,at-2)
        if options.get('alias')=='retire':
            request=query_row-0x14;put(request+0x10,query);put(request+0x18,0,2)
        gp=[actor,actor,0x3456789a,request,SP,0x56789abc,query,0x67890000|(priority&65535)]
        arguments=[]
        if options['entry']==0x278720:arguments=[actor,priority&65535]
        elif options['entry']==0x278a50:arguments=[slot&65535]
        elif options['entry']==0x2787f0:gp[0]=query;arguments=[actor,expiration_bits]
        elif options['entry']==0x278930:arguments=[actor if options['broadcast']=='excluded'else 0xffffffff]
        xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)];number=len(rows);depth=number%4
        before_x87=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        rows.append(dict(name=options['label'],entry=options['entry'],actor=actor,query=query,slot=slot,priority=priority,expiration_bits=expiration_bits,request=request,gp=gp,xmm=xmm,stack=SP,stop=STOP,arguments=arguments,before_x87=before_x87,
            before_fp=[(0x7f,0x27f,0x37f)[number%3]|((number%4)<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|((number%4)<<13)|(0,0x40,0x8000,0x8040)[number%4]],
            banks=[dict(address=BASE,before=bytes(main).hex())]+[dict(address=p,before=struct.pack('<I',value).hex())for p,value in ((0x5020c8,ACTORS),(0x50eefc,QUERIES),(0x51df54,CLOCK))]))
        if options.get('broadcast')and options['broadcast']!='alias':
            rows[-1]['typed_request']=dict(kind=struct.unpack_from('<H',main,request-BASE)[0],query=struct.unpack_from('<I',main,request-BASE+4)[0],expiration_bits=struct.unpack_from('<I',main,request-BASE+8)[0])
        if options.get('broadcast')=='caller-layout':
            rows[-1]['request']=SP+0x28;rows[-1]['gp'][3]=SP+0x28;rows[-1]['stack_request']=bytes(main[request-BASE:request-BASE+12]).hex()
        if 'request'in locals():del request
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_query_scheduler_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        sections=[f'.original 0x{address:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        for index,bank in enumerate(row['banks']):
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']));assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"');sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,SP-STACK,STOP)
        for index,value in enumerate(row['arguments']):struct.pack_into('<I',stack,SP-STACK+4+index*4,value)
        if 'stack_request'in row:stack[SP-STACK+0x28:SP-STACK+0x34]=bytes.fromhex(row['stack_request'])
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
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_query_scheduler_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x278930:48,0x278720:61,0x278a50:25,0x2787f0:48,0x278b50:75,0x2786d0:24,0x16e8a0:62}
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
    profile=site_reference['native_request_caller_storage_profile']
    for line in profile['listing']:
        site,encoding,*_=line.split();encoding=bytes.fromhex(encoding)
        if original_bytes(image,int(site,16),len(encoding))!=encoding:raise ValueError('Actual caller request storage bytes differ')
    if profile['callee_request_offset']!=0x28 or profile['entry']!='00278460':raise ValueError('Actual caller request ownership differs')
    return dict(native_request_caller_storage_profile=profile,image_sha256=PIN,backend='unchanged-tu15-physical-i386-interleaved-inputs-stores-fp',cases=rows,instructions=total,coverage=coverage,
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
    sources=[ROOT/'include/halo2/native_campaign_ai_query_scheduler_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_query_scheduler_tu15.cpp',ROOT/'tests/native_campaign_ai_query_scheduler_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_query_scheduler_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_query_scheduler_tu15_sites.json',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'src/core/native_tu15_data_array_delete.cpp',ROOT/'include/halo2/native_tu15_data_array_delete.hpp',ROOT/'cmake/CampaignNativeAiQueryScheduler.cmake',ROOT/'cmake/NativeTu15DataArrayDelete.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-query-scheduler.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(native_request_caller_storage_profile=fixture['native_request_caller_storage_profile'],disjoint_native_request_profiles=sum('typed_request'in row for row in fixture['cases']),caller_contract='typed slot/boolean results and actual data; original internal calls/taildelete; scratch registers not synthesized',original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'expiration_signed':('signed_dword(expiration)<signed_dword(read(memory,clock+8u))','expiration<read(memory,clock+8u)'),
            'priority_tie':('if(candidate<best_priority)','if(candidate<=best_priority)'),
            'release_reread':('const auto after_publication=signed_word(read(memory,slot+2u,2));','const auto after_publication=member;'),
            'reservation_tail_read':('if(index>=10)return false;\n            ++index;','if(index>=9)return false;\n            ++index;'),
            'broadcast_reread':('const auto current_query=request.query();','const auto current_query=query;'),
            'retire_reread':('const auto current_query=read(memory,state+0x10u),current_pool=read(memory,0x50eefcu);','const auto current_query=query,current_pool=read(memory,0x50eefcu);')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered scheduler store','ordered scheduler input','scheduler result differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected scheduler mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_scheduler LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Scheduler mutation location changed: '+name)
                core=project/(name+'.cpp');core.write_text(text.replace(*mutation))
            target='campaign_scheduler_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}" "{ROOT}/src/core/native_tu15_data_array_delete.cpp")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_scheduler_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['unsigned expiration comparison','replace strict priority tie','skip member-index reread','omit full reservation tail read','skip broadcast query reread','skip retirement query reread'],
            excluded=['278310 actualbehavior1B cancellation callback1BD8F0 and dependencies','278460/whole campaign','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
