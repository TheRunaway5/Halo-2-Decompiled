#!/usr/bin/env python3
"""Physical unchanged 1BD9D0 actual group broadcast through native2787F0 versus typed semantic native owners.

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

from copy import deepcopy
from validate_native_campaign_ai_query_scheduler_tu15 import corpus as scheduler_corpus
BASE,SIZE,ACTORS,QUERIES,GROUPS,ACTOR_ROWS,QUERY_ROWS,GROUP_ROWS,EXTRA=0x1000000,0x12000,0x1000100,0x1000200,0x1000400,0x1001000,0x1007000,0x1009000,0x3f800000
EXTRA_SIZE=0x6000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x5020c8,4),(0x50eefc,4),(0x51df54,4),(0x50eef8,4),(EXTRA,EXTRA_SIZE))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    template=scheduler_corpus(image)[0];rows=[];configs=[]
    for mode in ('no-group','empty-group','self','other-salt-self','one','many','reject','equal-priority','full-query','one-space-two-actors','next-link-alias','rows-alias','rows-alias-self'):
        configs.append(dict(label=mode,mode=mode))
    for kind in ('free','expired','evict'):
        for index in range(4):configs.append(dict(label=f'{kind}-{index}',mode=kind,index=index))
    for member in range(10):configs.append(dict(label=f'reserve-member-{member}',mode='member',member=member))
    for options in configs:
        row=deepcopy(template);main=bytearray.fromhex(row['banks'][0]['before']);extra=bytearray(EXTRA_SIZE)
        def put(address,value,width=4):
            bank,base=(extra,EXTRA)if EXTRA<=address<EXTRA+EXTRA_SIZE else(main,BASE)
            bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
        actor=0x12340002;query=0x56780003;query_row=QUERY_ROWS+3*188;mode=options['mode'];self_row=ACTOR_ROWS+2*0x888
        put(GROUPS+0x44,GROUP_ROWS);put(self_row+0x7c,0x99990001)
        for n in range(10):
            for base in (ACTOR_ROWS,EXTRA):
                at=base+n*0x888;put(at+0x80,0xffffffff)
                for index in range(4):
                    slot=at+0x194+index*12;put(slot,0xffff if mode not in ('free','expired','evict','reject','equal-priority')else 0x1b,2);put(slot+2,index,2);put(slot+4,query);put(slot+8,100)
                    if mode=='free'and index==options['index']:put(slot,0xffff,2)
                    if mode=='expired'and index==options['index']:put(slot+8,0xffffffff)
        for member in range(10):
            put(query_row+member*12+4,0xffffffff);put(query_row+member*12+10,1 if mode=='evict'and member==options['index']else 3 if mode=='equal-priority'else 6,2)
        chain=[0x12340001]
        if mode=='no-group':put(self_row+0x7c,0xffffffff);chain=[]
        elif mode=='empty-group':chain=[]
        elif mode=='self':chain=[actor]
        elif mode=='other-salt-self':chain=[0xffff0002]
        elif mode=='many':chain=[0x12340000|n for n in (0,1,2,3,4,5,6,7)]
        elif mode=='one-space-two-actors':
            chain=[0x12340001,0x12340003]
            for member in range(9):put(query_row+member*12+4,0x12340005)
        elif mode=='full-query':
            for member in range(10):put(query_row+member*12+4,0x12340005)
        elif mode=='member':
            for member in range(options['member']):put(query_row+member*12+4,0x12340005)
        elif mode in ('next-link-alias','rows-alias','rows-alias-self'):
            chain=[0x12340001,0x12340002 if mode=='rows-alias-self'else 0x12340003]
        for index,member in enumerate(chain):put(ACTOR_ROWS+(member&65535)*0x888+0x80,chain[index+1]if index+1<len(chain)else 0xffffffff)
        put(GROUP_ROWS+0x50+0x18,chain[0]if chain else 0xffffffff)
        if mode=='next-link-alias':
            query_row=ACTOR_ROWS+0x888+0x74;put(QUERIES+0x44,query_row-3*188);put(query_row+4,0xffffffff);put(query_row+16,0xffffffff)
        elif mode in ('rows-alias','rows-alias-self'):
            query_row=ACTORS+0x38;put(QUERIES+0x44,query_row-3*188);put(query_row+4,0xffffffff);put(query_row+16,0xffffffff)
        row.update(name=options['label'],entry=0x1bd9d0,actor=actor,query=query,arguments=[actor,query,0x31415926,0x27182818]);row.pop('typed_request',None)
        row['gp']=[0x12345678,0x23456789,0x3456789a,0x456789ab,SP,0x56789abc,0x6789abcd,0x789abcde]
        number=len(rows);depth=number%4;row['before_x87']=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        row['before_fp']=[(0x7f,0x27f,0x37f)[number%3]|((number%4)<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|((number%4)<<13)|(0,0x40,0x8000,0x8040)[number%4]]
        row['banks'][0]['before']=bytes(main).hex();row['banks'] += [dict(address=0x50eef8,before=struct.pack('<I',GROUPS).hex()),dict(address=EXTRA,before=bytes(extra).hex())];rows.append(row)
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_query_start_reference_tu15.cpp'),'-o',str(observer)])
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
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_query_start_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x1bd9d0:52,0x2787f0:48,0x278720:61,0x278a50:25}
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
    sources=[ROOT/'include/halo2/native_campaign_ai_query_start_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_query_start_tu15.cpp',ROOT/'tests/native_campaign_ai_query_start_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_query_start_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_query_start_tu15_sites.json',ROOT/'src/campaign/ai/native_campaign_ai_query_scheduler_tu15.cpp',ROOT/'include/halo2/native_campaign_ai_query_scheduler_tu15.hpp',ROOT/'tools/validate_native_campaign_ai_query_scheduler_tu15.py',ROOT/'src/core/native_tu15_data_array_delete.cpp',ROOT/'cmake/CampaignNativeAiQueryStart.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-query-start.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(caller_contract='actual start entry with four arguments and real nested2787F0 call; low16 count and did_reserve SIMD fact; no synthesized scratch',original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'skip_datum':('if(row!=self&&','if(member!=actor&&'),
            'rows_snapshot':('const auto actor_rows=read(memory,pool+0x44u),row=','const auto actor_rows=initial_rows,row='),
            'next_movement':('const auto next=read(memory,row+0x80u);','std::uint32_t next{};'),
            'wrong_priority':('query,member,3,0x3f800000u','query,member,2,0x3f800000u'),
            'wrong_expiration':('query,member,3,0x3f800000u','query,member,3,0u')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=30)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered start store','ordered start input','start result differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected start mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_start LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Start mutation location changed: '+name)
                core=project/(name+'.cpp');changed=text.replace(*mutation)
                if name=='next_movement':changed=changed.replace('        member=next;','        next=read(memory,row+0x80u);member=next;')
                core.write_text(changed)
            target='campaign_start_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}" "{sources[6]}" "{ROOT}/src/core/native_tu15_data_array_delete.cpp")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_start_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['skip self by datum instead of row address','snapshot mutable rows pointer','move next link read after reservation','change real priority3','change real expiration bits1.0'],
            excluded=['full278460 caller entry','1BD8F0 actual cancellation','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
