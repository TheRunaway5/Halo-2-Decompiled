#!/usr/bin/env python3
"""Physical unchanged TU1.5 unit speech reservation versus semantic native owners."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
sys.dont_write_bytecode=True
from validate_native_campaign_audio_leaves_tu15 import ROOT,ORIGINAL,PIN,text_section,sha,save,checked

BASE,SIZE=0x1000000,0x20000
OBJECTS,HEADERS,BODY,TAGS=BASE,BASE+0x1000,BASE+0x2000,BASE+0x4000
DIALOGUE,SOUND,GLOBALS,REFERENCE,CLASSES,CLASS_ROWS=BASE+0x5000,BASE+0x6000,BASE+0x7000,BASE+0x7400,BASE+0x8000,BASE+0x8400
GESTALT,RANGES,ACTIONS,FLAGS,OUTPUT,PRIOR=BASE+0x9000,BASE+0xa000,BASE+0xb000,BASE+0x12000,BASE+0x18000,BASE+0x18010
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4ecbd0,4),(0x4f0414,4),(0x4ecc1c,4),(0x52d664,4),(0x52be90,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x116c00);rows=[];configs=[]
    for count in (0,1,2,3,4,5,7,8,15,16,31,32,0x7fffffff,0x80000000,0xffffffff):configs.append(dict(kind='width',label=f'width-{count}',count=count))
    configs += [dict(kind='prefix',label=label,**options)for label,options in [
        ('class-enabled',dict()),('class-disabled',dict(class_flags=0)),('prefix-none',dict(prefix=0xffff)),
        ('prefix-zero',dict(prefix=0)),('prefix-65534',dict(prefix=65534)),('negative-class',dict(class_index=-1))]]
    for count in (-32768,-1,0,1,2,3,7,16,31,32,32767):
        for bit in (-1,0,4):configs.append(dict(kind='blocked',label=f'blocked-{count}-{bit}',permutations=count,blocked=bit))
    configs += [dict(kind='blocked',label=label,**options)for label,options in [
        ('sound-none',dict(sound=0xffffffff)),('multiple-ranges',dict(ranges=2)),('prefix-disabled',dict(class_flags=0)),
        ('prefix-absent',dict(prefix=0xffff)),('range-negative',dict(range_index=-1,blocked=2)),
        ('class-negative-eligibility',dict(class_index=-1,blocked=3)),('word-crossing',dict(prefix=25,blocked=4))]]
    for action in (-32768,-2,-1,0,1,2,3,32767):
        for value in (0,0xffffffff,123):configs.append(dict(kind='source',label=f'source-{action}-{value}',action=action,output_value=value))
    configs += [dict(kind='source',label='source-'+label,**options)for label,options in [
        ('no-dialogue',dict(dialogue=0xffffffff)),('sound-none',dict(sound=0xffffffff)),('blocked',dict(blocked=0)),
        ('last-bit',dict(blocked=4)),('negative-count',dict(action_count=-1)),('unit-negative-offset',dict(unit_offset=-0x100)),
        ('output-alias-tag',dict(output_alias='tag')),('output-alias-action',dict(output_alias='action'))]]
    for current in (-32768,-1,0,1,9,10,11,12,13,14,15,32767):
        for priority in (-1,0,10,12,13,14,15):configs.append(dict(kind='reserve',label=f'reserve-{current}-{priority}',current=current,priority=priority))
    configs += [dict(kind='reserve',label='reserve-'+label,**options)for label,options in [
        ('preselected',dict(output_value=123)),('zero-selected',dict(output_value=0)),('no-output-owner',dict(output_address=0)),
        ('no-action',dict(action=-1)),('missing-action',dict(action=4)),('no-dialogue',dict(dialogue=0xffffffff)),
        ('dead-priority',dict(unit_flags=4,priority=14)),('dead-15',dict(unit_flags=4,priority=15)),
        ('special-12-active',dict(priority=12,current=11,active=1)),('special-12-inactive',dict(priority=12,current=11,active=0)),
        ('special-12-high',dict(priority=12,current=13,active=1)),('special-12-low',dict(priority=12,current=9,active=0)),
        ('null-prior',dict(prior_address=0)),('outputs-alias',dict(prior_address=OUTPUT)),
        ('output-alias-current',dict(output_alias='current')),('output-alias-prior',dict(output_alias='prior',prior_address=0)),
        ('output-prior-share-state',dict(output_alias='prior',prior_alias='prior')),('prior-alias-current',dict(prior_alias='current')),
        ('unit-negative-state',dict(unit_offset=-0x100)),('ignored-eligibility',dict(blocked=0)),
        ('negative-source-action',dict(action=-2)),('source-range-none',dict(sound=0xffffffff))]]
    configs += [dict(kind='reserve',label='actual117240-caller-'+str(priority),priority=priority,current=10,caller=True)for priority in (0,12,15)]
    configs.append(dict(kind='prefix',label='uninitialized-globals-original-fault',fault=True))
    for options in configs:
        main=bytearray(rng.randbytes(SIZE))
        def put(p,v,n=4):main[p-BASE:p-BASE+n]=int(v&((1<<(n*8))-1)).to_bytes(n,'little')
        def width(v):
            v&=0xffffffff
            return max(1,(v-1).bit_length())if v>1 else 1
        unit_offset=options.get('unit_offset',0x400);state=BODY+unit_offset;range_index=options.get('range_index',0);range_row=RANGES+range_index*12
        prefix=options.get('prefix',8);permutations=options.get('permutations',3);class_index=options.get('class_index',0)
        out={'tag':state,'current':state+0xc,'prior':state+0x44,'action':ACTIONS+0xc}.get(options.get('output_alias'),options.get('output_address',OUTPUT))
        prior={'current':state+0xc,'prior':state+0x44}.get(options.get('prior_alias'),options.get('prior_address',PRIOR))
        sound=options.get('sound',0x20000002)
        for p,v,n in [(OBJECTS+0x44,HEADERS,4),(HEADERS+20,BODY,4),(BODY+0x342,unit_offset,2),(BODY+0x10a,options.get('unit_flags',0),1),(state,options.get('dialogue',0x10000001),4),(state+0xc,options.get('current',0),2),(state+0x44,0x12345678,4),(state+0x49,options.get('active',0),1),(TAGS+24,DIALOGUE,4),(TAGS+40,SOUND,4),(TAGS+56,CLASSES,4),(DIALOGUE+0xc,options.get('action_count',3),4),(DIALOGUE+0x10,ACTIONS,4),(SOUND+2,class_index,1),(SOUND+8,range_index,2),(SOUND+0xa,options.get('ranges',1),1),(GLOBALS+0xc0,0 if options.get('fault')else 1,4),(GLOBALS+0xc4,REFERENCE,4),(REFERENCE+4,0x30000003,4),(CLASSES+4,CLASS_ROWS,4),(CLASS_ROWS+class_index*0x5c+0xa,options.get('class_flags',0x20),1),(GESTALT+0x24,RANGES,4),(range_row+6,prefix,2),(range_row+0xa,permutations,2)]:put(p,v,n)
        for i in range(3):put(ACTIONS+i*16+0xc,sound)
        main[FLAGS-BASE-0x1100:FLAGS-BASE+0x2100]=bytes(0x3200)
        if options.get('blocked',-1)>=0:
            index=(prefix+(permutations&0xffffffff)+width(permutations)+options['blocked'])&0xffffffff
            signed_index=index if index<0x80000000 else index-0x100000000
            p=FLAGS+(signed_index>>5)*4;put(p,1<<(index&31))
        if out:put(out,options.get('output_value',0xffffffff))
        if prior and prior!=out and not options.get('prior_alias'):put(prior,0x87654321)
        number=len(rows);depth=number%9;action=options.get('action',0);priority=options.get('priority',12)
        gp=[options.get('count',3)if options['kind']=='width'else sound if options['kind']=='blocked'else 0x10000001,0x23456789,0x34560000|(action&0xffff),0x456789ab,SP,0x56789abc,0x6789abcd,out]
        arguments=[range_row,SOUND]if options['kind']=='prefix'else[priority&0xffff,prior,out]if options['kind']=='reserve'else[]
        entry={'width':0x140ea0,'prefix':0x21fe40,'blocked':0x213890,'source':0x116b80,'reserve':0x116c00}[options['kind']]
        before_x87=[struct.pack('<QH',v<<(63-(v.bit_length()-1)),16383+v.bit_length()-1).hex()for v in range(1,depth+1)]
        values=[bytes(main)]+[struct.pack('<I',v)for v in (OBJECTS,TAGS,GLOBALS,GESTALT,FLAGS)]
        rows.append(dict(name=options['label'],kind=options['kind'],entry=entry,unit=0x10000001,sound=sound,action=action,priority=priority,count=options.get('count',3),range=range_row,definition=SOUND,output=out,prior=prior,gp=gp,xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)],stack=SP,stop=STOP,arguments=arguments,before_x87=before_x87,before_fp=[(0x7f,0x27f,0x37f)[number%3]|((number//3)%4)<<10,((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|(((number//12)%4)<<13)],banks=[dict(address=p,before=v.hex())for(p,_),v in zip(REGIONS,values)]))
        if options.get('fault'):rows[-1].update(fault=True,stop=0x21fe59)
        if out==OUTPUT and prior in (PRIOR,0)and options['kind']in ('reserve','source'):rows[-1]['native_local_output']=True
        if options.get('caller'):
            row=rows[-1];row.update(entry=0x117240,capture_entry=0x116c00,stop=0x11727b,arguments=[row['unit'],priority,0]);row['gp'][0]=action
            row['banks'].append(dict(address=SP+8,before='ffffffff'));row['native_local_output']=True
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_speech_reservation_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        regions=[(bank['address'],len(bank['before'])//2)for bank in row['banks']];(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in regions))
        sections=[f'.original 0x{address:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        for index,bank in enumerate(row['banks']):
            if STACK<=bank['address']<STACK+0x10000:continue
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']));assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"');sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,row['stack']-STACK,STOP)
        for index,value in enumerate(row['arguments']):struct.pack_into('<I',stack,SP-STACK+4+index*4,value)
        for at,value in row.get('stack_locals',[]):struct.pack_into('<I',stack,at-STACK,value)
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
        try:checked([str(observer),str(owned/'reference'),hex(row['stop']),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')]+([hex(row['capture_entry'])]if 'capture_entry'in row else ['fault']if row.get('fault')else []))
        except RuntimeError as error:raise RuntimeError(row['name']+': '+str(error))from error
        if 'capture_entry'in row:
            captured=(owned/'output.bin.capture').read_bytes();cw,sw,mxcsr,prefix_steps=struct.unpack_from('<4I',captured);position=16
            row['gp']=list(struct.unpack_from('<8I',captured,position));position+=32;row['xmm']=[list(struct.unpack_from('<4I',captured,position+i*16))for i in range(8)];position+=128
            row['before_x87']=[captured[position+i*16:position+i*16+10].hex()for i in range(len(row['before_x87']))];position+=128;row['before_fp']=[cw,sw,mxcsr]
            for bank,(_,length)in zip(row['banks'],regions):bank['before']=captured[position:position+length].hex();position+=length
            actual_priority,actual_prior,actual_output=struct.unpack_from('<3I',captured,position);position+=12;count,=struct.unpack_from('<I',captured,position);position+=4
            prefix_sites=list(struct.unpack_from('<'+'I'*count,captured,position));position+=count*4
            count,=struct.unpack_from('<I',captured,position);position+=4;prefix_stores=[]
            for _ in range(count):
                site,destination,length=struct.unpack_from('<III',captured,position);position+=12;prefix_stores.append(dict(instruction=site,address=destination,before=captured[position:position+length].hex(),written=captured[position+4:position+4+length].hex()));position+=8
            if position!=len(captured)or 0x117276 not in prefix_sites:raise ValueError('Actual admission caller receipt differs')
            row['caller']=dict(entry=0x117240,call_site=0x117276,callee=row['capture_entry'],return_site=0x11727b,prefix_steps=prefix_steps,prefix_sites=prefix_sites,prefix_engine_stores=prefix_stores,supplied_returns=[])
            row.update(entry=row['capture_entry'],arguments=[actual_priority,actual_prior,actual_output],stack=row['gp'][4],priority=actual_priority,prior=actual_prior,output=actual_output,unit=row['gp'][0],action=row['gp'][2]&65535)
            if row['action']>=32768:row['action']-=65536
        if row.get('fault'):
            fault,=struct.unpack('<I',(owned/'output.bin.fault').read_bytes());assert fault==4;row['fault_address']=fault
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
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_speech_reservation_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x116c00:76,0x116b80:40,0x213890:61,0x21fe40:30,0x140ea0:13}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Speech reservation closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Heap original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Heap original instruction inventory count differs')
        unreachable={int(p,16)for p in routine["excluded_unmapped_fault_sites"]}
        missing=(expected-unreachable)-reached
        if missing:raise ValueError('Speech queue reachable original sites missing: '+','.join(f'{p:08x}'for p in sorted(missing)))
        coverage[f'{entry:08x}']=dict(required_sites=len(expected-unreachable),reached_sites=len((expected-unreachable)&reached),excluded_unmapped_fault_sites=sorted(unreachable),missing_sites=sorted(missing))
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
    sources=[ROOT/'include/halo2/native_campaign_ai_speech_reservation_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_speech_reservation_tu15.cpp',ROOT/'tests/native_campaign_ai_speech_reservation_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_speech_reservation_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_speech_reservation_tu15_sites.json',ROOT/'cmake/CampaignNativeAiSpeechReservation.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-speech-reservation.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(actual_original_caller_receipts=[dict(name=row['name'],**row['caller'])for row in fixture['cases']if 'caller'in row],caller_contract='actual unit/action/priority and optional aliased output owners; full occupied extended stack remains untouched',original_cases=len(fixture['cases']),original_fault_prefixes=sum(bool(row.get('fault'))for row in fixture['cases']),native_local_profiles=sum('native_local_output'in row for row in fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'zero_output':('if(output.load()!=0)output.store(sound);','output.store(sound);'),
            'priority_equal':('else admitted=std::int32_t(priority)>current;','else admitted=std::int32_t(priority)>=current;'),
            'special_priority':('priority==13||priority==14','priority==12||priority==13||priority==14'),
            'eligibility_last':('offset<5','offset<4'),
            'output_write_order':('if(output.present())output.store(sound);\n    if(prior.present())prior.store(read(m,state+0x44u));','if(prior.present())prior.store(read(m,state+0x44u));\n    if(output.present())output.store(sound);'),
            'class_history':('&0x20u))return none;','&0x10u))return none;')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=60)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered speech store','ordered speech input','speech result differs','speech FP differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected speech mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_speech_reservation LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Speech mutation location changed: '+name)
                core=project/(name+'.cpp');changed=text.replace(*mutation)
                core.write_text(changed)
            target='campaign_speech_reservation_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_speech_reservation_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['write zero-valued output field','admit ordinary equal priority','treat12 as13/14 equal-priority policy','omit fifth eligibility bit','reverse final output publication order','read wrong sound-class history flag'],
            excluded=['whole117240/116D60 speech submission','other unmapped input faults','full213D30/1BD8F0/constant34speech dispatch'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
