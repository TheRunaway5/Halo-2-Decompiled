#!/usr/bin/env python3
"""Physical unchanged 27BB60/27BC00/27BCE0 priority-heap closure versus typed semantic native owners.

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

BASE,SIZE,WORKSPACE=0x1000000,0x24000,0x1010000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x46a16c,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x27bb60);rows=[];configs=[]
    for slot in (0,1,2,3,7,31):
        for priority in (-32768,-1,0,32767):configs.append(dict(label=f'up-{slot}-{priority}',entry=0x27bb60,slot=slot,priority=priority))
    configs += [dict(label='up-parent-tie',entry=0x27bb60,slot=7,priority=3),dict(label='up-promote-then-tie',entry=0x27bb60,slot=31,priority=7)]
    for tail in (-32768,-2,-1,0,1,2,3,31,1023,1024,32767):
        configs.append(dict(label=f'insert-{tail}',entry=0x27bce0,count=tail,priority=-32768,node=-3 if tail&1 else 1024))
    for count in (0,1,2,3,4,5,8,16,31):
        for shape in ('ascending','descending','ties'):
            configs.append(dict(label=f'down-{count}-{shape}',entry=0x27bc00,count=count,shape=shape,priority=32767))
    configs += [dict(label='down-signed-wrap-stop',entry=0x27bc00,count=-32768,slot=1),
        dict(label='down-count-publication-alias',entry=0x27bc00,count=2,slot=-1,alias='count'),
        dict(label='down-backpointer-aliases-future-priority',entry=0x27bc00,count=17,alias='priority'),
        dict(label='up-backpointer-aliases-priority',entry=0x27bb60,slot=31,priority=-100,node=1024)]
    configs += [dict(label=f'actual-search-pop-{count}',entry=0x27b340,count=count,caller=True)for count in (3,4,8,17)]
    for options in configs:
        main=bytearray(rng.randbytes(SIZE))
        def put(address,value):struct.pack_into('<H',main,address-BASE,value&65535)
        def heap(slot,node,priority):put(WORKSPACE+0x110b2+slot*4,node);put(WORKSPACE+0x110b4+slot*4,priority)
        for slot in range(-8,1025):
            shape=options.get('shape','ascending');priority=slot if shape=='ascending'else 1000-slot if shape=='descending'else 7
            heap(slot,slot%63,priority)
        slot=options.get('slot',1);count=options.get('count',32);priority=options.get('priority',1000)
        put(WORKSPACE+0x110b0,count)
        if options['entry']!=0x27bce0:heap(slot,options.get('node',-3),priority)
        if options.get('alias')=='count':
            put(WORKSPACE+0x110b0,2);heap(-2,3,-5);heap(-4,4,-10)
        if options.get('alias')=='priority':
            heap(1,1,1000);heap(2,2,100);heap(3,1024,-3);heap(6,6,100);heap(7,7,-2)
            heap(14,14,100);heap(15,15,-10)
        if options.get('caller'):
            struct.pack_into('<I',main,WORKSPACE-BASE,0x3f800000);struct.pack_into('<I',main,WORKSPACE-BASE+0x4c,0x3f800000)
        gp=[WORKSPACE,(0x23450000|(slot&65535)),(0x34560000|(slot&65535)),0x456789ab,SP,0x56789abc,0x6789abcd,0x789abcde]
        arguments=[0x98760000|(options.get('node',5)&65535),0x87650000|(priority&65535)]if options['entry']==0x27bce0 else []
        if options.get('caller'):arguments=[WORKSPACE]
        xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)];number=len(rows);depth=number%4
        before_x87=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        rows.append(dict(name=options['label'],entry=options['entry'],workspace=WORKSPACE,slot=slot,gp=gp,xmm=xmm,stack=SP,stop=STOP,arguments=arguments,before_x87=before_x87,
            before_fp=[(0x7f,0x27f,0x37f)[number%3]|((number%4)<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|((number%4)<<13)|(0,0x40,0x8000,0x8040)[number%4]],
            banks=[dict(address=BASE,before=bytes(main).hex()),dict(address=0x46a16c,before=original_bytes(image,0x46a16c,4).hex())]))
        if options.get('caller'):rows[-1].update(stop=0x27b3db,capture_entry=0x27bc00)
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_path_heap_reference_tu15.cpp'),'-o',str(observer)])
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
        if 'capture_entry'in row:
            captured=(owned/'output.bin.capture').read_bytes();cw,sw,mxcsr,prefix_steps=struct.unpack_from('<4I',captured);position=16
            row['gp']=list(struct.unpack_from('<8I',captured,position));position+=32;row['xmm']=[list(struct.unpack_from('<4I',captured,position+i*16))for i in range(8)];position+=128
            row['before_x87']=[captured[position+i*16:position+i*16+10].hex()for i in range(len(row['before_x87']))];position+=128;row['before_fp']=[cw,sw,mxcsr]
            for bank,(_,length)in zip(row['banks'],REGIONS):bank['before']=captured[position:position+length].hex();position+=length
            count,=struct.unpack_from('<I',captured,position);position+=4;prefix_sites=list(struct.unpack_from('<'+'I'*count,captured,position));position+=count*4
            if position!=len(captured)or 0x27b3d6 not in prefix_sites:raise ValueError('Actual path-search caller receipt differs')
            row['caller']=dict(entry=0x27b340,call_site=0x27b3d6,callee=row['capture_entry'],return_site=0x27b3db,prefix_steps=prefix_steps,prefix_sites=prefix_sites,supplied_returns=[])
            row.update(entry=row['capture_entry'],arguments=[],stack=row['gp'][4],workspace=row['gp'][0],slot=struct.unpack('<h',struct.pack('<H',row['gp'][1]&65535))[0])
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
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_path_heap_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x27bb60:41,0x27bc00:57,0x27bce0:15}
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
    sources=[ROOT/'include/halo2/native_campaign_ai_path_heap_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_path_heap_tu15.cpp',ROOT/'tests/native_campaign_ai_path_heap_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_path_heap_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_path_heap_tu15_sites.json',ROOT/'cmake/CampaignNativeAiPathHeap.cmake',ROOT/'include/halo2/physics_contact_record.hpp',ROOT/'tools/validate_native_campaign_audio_leaves_tu15.py']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-path-heap.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(caller_contract='actual heap data and void result; preserved EAX/nonvolatile GPRs, SIMD, x87; ECX/EDX scratch not synthesized',actual_original_caller_receipts=[dict(name=row['name'],**row['caller'])for row in fixture['cases']if 'caller'in row],original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'ties':('if(priority>=parent_priority)break;','if(priority>parent_priority)break;'),
            'signed':('if(child_priority<best_priority)','if(std::uint16_t(child_priority)<std::uint16_t(best_priority))'),
            'read_order':('const auto parent_priority=heap.priority(parent),parent_node=heap.node(parent);','const auto parent_node=heap.node(parent),parent_priority=heap.priority(parent);'),
            'store_order':('heap.priority(slot,priority,0x27bcb7);\n    heap.node(slot,node,0x27bcbf);','heap.node(slot,node,0x27bcbf);\n    heap.priority(slot,priority,0x27bcb7);'),
            'count_reload':('for(;;) {\n        auto selected=slot','const auto cached_count=heap.count();\n    for(;;) {\n        auto selected=slot')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered heap store','ordered heap input','heap result differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected heap mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_heap LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Heap mutation location changed: '+name)
                core=project/(name+'.cpp');changed=text.replace(*mutation)
                if name=='count_reload':changed=changed.replace('if(child>=heap.count())break;','if(child>=cached_count)break;')
                core.write_text(changed)
            target='campaign_heap_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_heap_(native|ties|signed|read_order|store_order|count_reload)$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['promote equal priority','compare priority unsigned','move parent node input before priority','swap final sift-down stores','snapshot mutable heap count'],
            excluded=['27B340/200D30/full1C9F10 path search','upstream floating cost quantization','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
