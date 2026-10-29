#!/usr/bin/env python3
"""Physical unchanged 275770 actual cancellation admission with native campaign dependencies versus typed semantic native owners.

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
from validate_native_campaign_ai_cancel_tracks_tu15 import corpus as tracks_corpus
BASE,SIZE,FRAMES,CLOCK,TIMERS,ACTORS,SECONDARY,FRAME_BITS,FRAME_ROWS,ACTOR_ROWS,SECONDARY_ROWS,QUERY=0x1000000,0x18000,0x100a100,0x100a200,0x100a400,0x100a300,0x100a500,0x100a800,0x100b000,0x100c000,0x1012000,0x1013000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4ecbd0,4),(0x50eef4,4),(0x50eef0,4),(0x50eef8,4),(0x5020a8,4),(0x4f3218,4),(0x5020c4,4),(0x4f54f4,4),(0x51df54,4),(0x5020a4,4),(0x5020c8,4),(0x50eeec,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    originals=tracks_corpus(image);seed=next(row for row in originals if row['name']=='conflict-match');rows=[];configs=[]
    for row in originals:
        if row['entry']==0x275770:continue
        configs.append(dict(label='fallback-'+row['name'],source=row,kind='fallback'))
    for kind in ('disabled1','disabled255','object4','object255','object2','group-none','team-none','team-negative','team-limit','scan-empty','timer-no-match','age29','age30','age31','future','wrap','signed-age','negative-rate','minimum-rate','multiple-timers','members-none','member0','member1','member2','member3','member-negative','member-self','member-later-active','members-inactive','C4-only','A4-only','mode0-team1','mode3-team1','mode0-team2','salt-high','actual-caller'):
        configs.append(dict(label=kind,source=seed,kind=kind))
    for options in configs:
        row=deepcopy(options['source']);old_entry=row['entry'];main=bytearray.fromhex(row['banks'][0]['before']);main+=bytes(SIZE-len(main))
        def put(address,value,width=4):main[address-BASE:address-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
        def relation(offset,value,on=True):
            at=BASE+0x600+offset+(value*16+1)//32*4;word=int.from_bytes(main[at-BASE:at-BASE+4],'little');mask=1<<((value*16+1)&31);put(at,(word|mask)if on else(word&~mask))
        prop=row['property'];actor=row['excluded'];group=row['group'];kind=options['kind'];property_row=BASE+0x2000+(prop&65535)*0xc4;group_row=BASE+0x6000+(group&65535)*0x50
        put(property_row+0x34,0,1)
        if old_entry==0x217310:put(group_row+0x12,row['team'],2)
        for index in range(8):put(BASE+0x6000+index*0x50+0x18,0xffffffff)
        put(FRAMES+0x20,4);put(FRAMES+0x24,0x10);put(FRAMES+0x38,2);put(FRAMES+0x44,FRAME_ROWS);put(FRAMES+0x48,FRAME_BITS);put(FRAME_BITS,0)
        put(CLOCK+2,30,2);put(CLOCK+8,1000);put(TIMERS,0xffffffff);put(TIMERS+0x1c,0xffffffff);put(ACTORS+0x44,ACTOR_ROWS);put(SECONDARY+0x44,SECONDARY_ROWS)
        for index in range(8):
            put(FRAME_ROWS+index*0x10,0x7777,2);put(ACTOR_ROWS+index*0x888+0x80,0xffffffff);put(ACTOR_ROWS+index*0x888+0x338,0xffffffff)
        if kind!='fallback':
            put(BASE+0x500+8,1);put(group_row+0x12,2,2);relation(0xc4,2,True);relation(0xa4,2,False)
            if kind.startswith('disabled'):put(property_row+0x34,1 if kind=='disabled1'else 255,1)
            elif kind.startswith('object'):put(BASE+0x9000+0x10a,int(kind[6:]),1)
            elif kind=='group-none':group=0xffffffff
            elif kind=='team-none':put(group_row+0x12,0xffff,2)
            elif kind=='team-negative':put(group_row+0x12,0xfffe,2)
            elif kind=='team-limit':put(group_row+0x12,16,2)
            elif kind in ('timer-no-match','age29','age30','age31','future','wrap','signed-age','negative-rate','minimum-rate','multiple-timers','salt-high'):
                put(FRAME_BITS,3 if kind=='multiple-timers'else 1)
                salt=0xffff if kind=='salt-high'else 0x7777;put(FRAME_ROWS,salt,2)
                if kind!='timer-no-match':put(TIMERS,salt<<16)
                previous=1000-{'age29':29,'age30':30,'age31':31}.get(kind,31)
                if kind=='future':previous=1001
                elif kind=='wrap':put(CLOCK+8,5);previous=0xfffffff0
                elif kind=='signed-age':put(CLOCK+8,0x80000010);previous=0
                elif kind=='negative-rate':put(CLOCK+2,0xffff,2);previous=1002
                elif kind=='minimum-rate':put(CLOCK+2,0x8000,2);previous=1000
                elif kind=='multiple-timers':previous=999;put(TIMERS+0x1c,0x77770001);put(TIMERS+0x1c+0xc,900)
                put(TIMERS+0xc,previous)
            elif kind.startswith('member'):
                statuses={'members-none':[None,None],'member0':[0],'member1':[1],'member2':[2],'member3':[3],'member-negative':[-1],'member-self':[1],'member-later-active':[3,1],'members-inactive':[None,-1,3]}[kind]
                indices=[actor&65535]if kind=='member-self'else list(range(len(statuses)))
                datums=[actor]if kind=='member-self'else[0x88880000|index for index in indices]
                put(group_row+0x18,datums[0])
                for n,(index,status)in enumerate(zip(indices,statuses)):
                    at=ACTOR_ROWS+index*0x888;put(at+0x80,datums[n+1]if n+1<len(datums)else 0xffffffff)
                    if status is not None:put(at+0x338,0x99990000|n);put(BASE+0x4000+n*0x3c+0x24,status,2)
            elif kind=='A4-only':relation(0xc4,2,False);relation(0xa4,2,True)
            elif kind in ('mode0-team1','mode3-team1','mode0-team2'):
                put(BASE+0x500+8,3 if kind=='mode3-team1'else 0);put(group_row+0x12,2 if kind=='mode0-team2'else 1,2)
            elif kind=='actual-caller':
                actor=0x88880001;put(ACTOR_ROWS+0x888+0x338,0x99990000);put(ACTOR_ROWS+0x888+0x7c,group)
                put(BASE+0x4000+8,prop);put(BASE+0x4000+0x14,0xaaaa0000);put(BASE+0x4000+0x27,1,1);put(QUERY+0x8c,0,1);put(SECONDARY_ROWS+0xd8,0,1)
        row.update(name=options['label'],entry=0x275770,actor=actor,property=prop,group=group,arguments=[actor,group],stop=STOP)
        row.pop('capture_entry',None);row.pop('caller',None)
        row['gp']=[prop,0x23456789,0x3456789a,0x456789ab,SP,0x56789abc,0x6789abcd,0x789abcde]
        number=len(rows);depth=number%4;row['before_x87']=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        row['before_fp']=[(0x7f,0x27f,0x37f)[number%3]|((number%4)<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|((number%4)<<13)|(0,0x40,0x8000,0x8040)[number%4]]
        row['banks'][0]['before']=bytes(main).hex();row['banks'] +=[dict(address=p,before=struct.pack('<I',value).hex())for p,value in((0x4f54f4,FRAMES),(0x51df54,CLOCK),(0x5020a4,TIMERS),(0x5020c8,ACTORS),(0x50eeec,SECONDARY))]
        if kind=='actual-caller':row.update(entry=0x1bd8f0,arguments=[actor,BASE+0x13400,QUERY],stop=0x1bd978,capture_entry=0x275770)
        rows.append(row)
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_cancel_admission_reference_tu15.cpp'),'-o',str(observer)])
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
            actual_actor,actual_group=struct.unpack_from('<2I',captured,position);position+=8;count,=struct.unpack_from('<I',captured,position);position+=4
            prefix_sites=list(struct.unpack_from('<'+'I'*count,captured,position));position+=count*4
            count,=struct.unpack_from('<I',captured,position);position+=4;prefix_stores=[]
            for _ in range(count):
                site,destination,length=struct.unpack_from('<III',captured,position);position+=12;prefix_stores.append(dict(instruction=site,address=destination,before=captured[position:position+length].hex(),written=captured[position+4:position+4+length].hex()));position+=8
            if position!=len(captured)or 0x1bd973 not in prefix_sites:raise ValueError('Actual admission caller receipt differs')
            row['caller']=dict(entry=0x1bd8f0,call_site=0x1bd973,callee=row['capture_entry'],return_site=0x1bd978,prefix_steps=prefix_steps,prefix_sites=prefix_sites,prefix_engine_stores=prefix_stores,supplied_returns=[])
            row.update(entry=row['capture_entry'],arguments=[actual_actor,actual_group],stack=row['gp'][4],property=row['gp'][0],actor=actual_actor,group=actual_group)
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
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_cancel_admission_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x275770:157,0x275610:103,0x2755b0:28,0x274f40:31,0x217310:34,0xc1e90:11,0x6ca40:33,0x16ea60:30,0x1d0430:24,0x275ab0:9}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Heap closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Heap original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Heap original instruction inventory count differs')
        excluded={int(p,16)for p in routine["unqualified_mode2_sites"]}
        if excluded!=({0x217353,0x217355,0x217356,0x21735b,0x21735d,0x21735f}if entry==0x217310 else {0x275846,0x275848,0x275849,0x27584e}if entry==0x275770 else set()):raise ValueError("Policy admission exclusion differs")
        missing=(expected-excluded)-reached
        if missing:raise ValueError('Heap reachable original sites missing: '+','.join(f'{p:08x}'for p in sorted(missing)))
        coverage[f'{entry:08x}']=dict(required_sites=len(expected-excluded),reached_sites=len((expected-excluded)&reached),unqualified_mode2_sites=sorted(excluded),missing_sites=[])
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
    sources=[ROOT/'include/halo2/native_campaign_ai_cancel_admission_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_cancel_admission_tu15.cpp',ROOT/'tests/native_campaign_ai_cancel_admission_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_cancel_admission_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_cancel_admission_tu15_sites.json',ROOT/'src/core/native_tu15_data_array_iteration.cpp',ROOT/'src/campaign/ai/native_campaign_ai_cancel_tracks_tu15.cpp',ROOT/'src/campaign/ai/native_campaign_ai_cancel_support_tu15.cpp',ROOT/'include/halo2/native_tu15_data_array_iteration.hpp',ROOT/'include/halo2/native_campaign_ai_cancel_tracks_tu15.hpp',ROOT/'include/halo2/native_campaign_ai_cancel_support_tu15.hpp',ROOT/'tools/validate_native_campaign_ai_cancel_tracks_tu15.py',ROOT/'cmake/CampaignNativeAiCancelAdmission.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-cancel-admission.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(actual_original_caller_receipts=[dict(name=row['name'],**row['caller'])for row in fixture['cases']if 'caller'in row],caller_contract='actual full275770 with native private iterator and captured ticks; realnonmode2 profile; actual1BD8F0 caller prefix stops before speech',original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'disabled':('if(read(memory,row+0x34u,1))','if(false)'),
            'object_flag':('body+0x10au,1)&4u','body+0x10au,1)&8u'),
            'relation_table':('policy+0xc4u','policy+0xa4u'),
            'time_tie':('tick-previous)<std::int32_t(rate)','tick-previous)<=std::int32_t(rate)'),
            'time_unsigned':('std::bit_cast<std::int32_t>(tick-previous)<std::int32_t(rate)','(tick-previous)<std::uint32_t(std::int32_t(rate))'),
            'early_quiet_exit':('if(std::bit_cast<std::int32_t>(tick-previous)<std::int32_t(rate))quiet=false;','if(std::bit_cast<std::int32_t>(tick-previous)<std::int32_t(rate)){quiet=false;break;}'),
            'status':('status<=2','status<=3'),
            'link_movement':('const auto next=read(memory,actor_row+0x80u),track=read(memory,actor_row+0x338u);','const auto track=read(memory,actor_row+0x338u),next=read(memory,actor_row+0x80u);')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=30)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered admission store','ordered admission input','admission result differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected admission mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_admission LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Admission mutation location changed: '+name)
                core=project/(name+'.cpp');core.write_text(text.replace(*mutation))
            target='campaign_admission_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}" "{sources[6]}" "{sources[7]}" "{sources[8]}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_admission_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['ignore property disabled input','wrong object suppression bit','use A4insteadC4 relation','make fresh time comparison inclusive','compare elapsed unsigned','break player scan on first fresh timer','admit active status3','move actor next read after track input'],
            excluded=['actualmode2 virtual160A60 and ten original sites','inherited inactive-to-active undefined iterator','full1BD8F0 cancellation/constant34speech','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
