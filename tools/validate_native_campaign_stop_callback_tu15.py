#!/usr/bin/env python3
"""Physical unchanged 18F6B0 stop callback closure versus typed semantic native owners.

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

BASE,SIZE,SOURCES,OBJECTS,GAME,SOURCE_ROWS,OBJECT_ROWS,BODY=0x1000000,0x19000,0x1000100,0x1000200,0x1000300,0x1002000,0x1004000,0x1010000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4f2c4c,4),(0x4ecbd0,4),(0x4f9b5c,4),(0x44ebf4,32),(0x44ec14,32))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x18f6b0);rows=[];configs=[]
    for active in (0,1):
        for kind in (0,1,2,31,32,33,255):
            for matches in (False,True):configs.append(dict(label=f'stop-{active}-{kind}-{matches}',entry=0x18f6b0,active=active,kind=kind,matches=matches))
    for entry in (0x18f6b0,0xbcdf0):
        for option in (dict(none=True),dict(extent=0),dict(extent=0x80000000),dict(extent=1),dict(salt=0),dict(salt=0x1235),dict(body=0)):
            configs.append(dict(label=f'lookup-{entry:x}-{option}',entry=entry,**option))
        for salt in (0x7fff,0x8000,0xffff):configs.append(dict(label=f'signed-salt-{entry:x}-{salt:x}',entry=entry,salt=salt,handle_salt=salt))
    for mask in (0,1,2,3,0x80000000,0xffffffff):
        for kind in (0,1,31,32,33,255):configs.append(dict(label=f'mask-{mask:x}-{kind}',entry=0xbcdf0,mask=mask,kind=kind))
    for entry in (0x18f6b0,0xd3300):
        for offset in (0,16,-16,-32768,32767):
            for matches in (False,True):configs.append(dict(label=f'association-{entry:x}-{offset}-{matches}',entry=entry,offset=offset,matches=matches))
    configs += [dict(label='inactive-does-not-query-pools',entry=0x18f6b0,active=0,unmapped_sources=True,unmapped_objects=True),
        dict(label='none-does-not-query-pools',entry=0x18f6b0,none=True,unmapped_sources=True,unmapped_objects=True),
        dict(label='body-zero-does-not-query-sources',entry=0x18f6b0,body=0,unmapped_sources=True),
        dict(label='fresh-fixed-stride-body',entry=0x18f6b0,stride=16,matches=True),
        dict(label='association-aliases-source-tag',entry=0x18f6b0,alias=True,matches=True)]
    for descriptor in (0x44ebf4,0x44ec14):
        for settings in (dict(active=0),dict(active=1,matches=True),dict(active=1,matches=False),dict(none=True),dict(kind=2)):
            configs.append(dict(label=f'real-caller-{descriptor:x}-{settings}',entry=0x129e30,caller_descriptor=descriptor,**settings))
    for options in configs:
        main=bytearray(rng.randbytes(SIZE));put=lambda address,value,size=4:main.__setitem__(slice(address-BASE,address-BASE+size),(value&((1<<(8*size))-1)).to_bytes(size,'little'))
        index=1;source_index=3;stride=options.get('stride',12);body=options.get('body',BODY);sound=0xabcd4321
        put(GAME+0x29,options.get('active',1),1);put(OBJECTS+0x38,options.get('extent',64));put(OBJECTS+0x24,stride);put(OBJECTS+0x44,OBJECT_ROWS);put(SOURCES+0x44,SOURCE_ROWS)
        header=OBJECT_ROWS+index*stride;put(header,options.get('salt',0x1234),2);put(header+3,options.get('kind',0),1);put(header+8,body)
        if options.get('stride'):
            put(OBJECT_ROWS+index*12+8,BODY+0x4000);put(BODY+0x4000+0x33e,0,2);put(BODY+0x4000+0x38,sound);put(BODY+0x38,0x55556666)
        if body:
            if options.get('alias'):body=BASE+0x3000;put(header+8,body)
            offset=options.get('offset',0)
            if options.get('alias'):offset=SOURCE_ROWS+source_index*188+0xc-body-0x38
            put(body+0x33e,offset,2);put(body+offset+0x38,sound if options.get('matches',True)else 0x55556666)
        put(SOURCE_ROWS+source_index*188,0x9876,2);put(SOURCES+0x24,188);put(SOURCES+0x38,4)
        put(SOURCE_ROWS+source_index*188+0xc,sound)
        if 'caller_descriptor'in options:
            put(SOURCE_ROWS+source_index*188+4,0,1);put(SOURCE_ROWS+source_index*188+0xac,0xffff,2)
            put(SOURCE_ROWS+source_index*188+0x14,options['caller_descriptor'])
        datum=0xffffffff if options.get('none')else (options.get('handle_salt',0x1234)<<16)|index
        gp=[datum,0x23456789,0x3456789a,0x456789ab,SP,0x56789abc,0x6789abcd,0x789abcde];entry=options['entry']
        arguments=[datum,0x98760000|source_index,0xfedcba98]if entry==0x18f6b0 else [options.get('mask',3)]if entry==0xbcdf0 else [sound]
        if 'caller_descriptor'in options:
            put(SOURCE_ROWS+source_index*188+0x10,datum);gp[0]=0x98760000|source_index;arguments=[0xfedcba98]
        xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)];number=len(rows);depth=number%4
        before_x87=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        word=lambda value:struct.pack('<I',value&0xffffffff)
        values=[main,word(0xdead0000 if options.get('unmapped_sources')else SOURCES),word(0xdead1000 if options.get('unmapped_objects')else OBJECTS),word(GAME)]+[original_bytes(image,0x44ebf4,32),original_bytes(image,0x44ec14,32)]
        rows.append(dict(name=options['label'],entry=entry,capture_entry=0x18f6b0 if 'caller_descriptor'in options else 0,gp=gp,xmm=xmm,stack=SP,stop=0x129e8a if 'caller_descriptor'in options else STOP,arguments=arguments,before_x87=before_x87,
            before_fp=[(0x7f,0x27f,0x37f)[number%3]|((number%4)<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|((number%4)<<13)|(0,0x40,0x8000,0x8040)[number%4]],
            banks=[dict(address=p,before=bytes(value).hex())for(p,_),value in zip(REGIONS,values)]))
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_stop_callback_reference_tu15.cpp'),'-o',str(observer)])
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
        try:checked([str(observer),str(owned/'reference'),hex(row['stop']),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')]+([hex(row['capture_entry'])]if row['capture_entry']else []))
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
        captured,=struct.unpack_from('<I',data,offset);offset+=4
        if captured:
            launch_gp=list(row['gp']);prefix_steps,=struct.unpack_from('<I',data,offset);offset+=4
            actual_gp=list(struct.unpack_from('<8I',data,offset));offset+=32;actual_fp=list(struct.unpack_from('<3I',data,offset));offset+=12
            actual_xmm=[list(struct.unpack_from('<4I',data,offset+i*16))for i in range(8)];offset+=128
            actual_x87=[data[offset+i*16:offset+i*16+10].hex()for i in range(len(row['before_x87']))];offset+=128
            frame=list(struct.unpack_from('<4I',data,offset));offset+=16
            for bank,(_,length)in zip(row['banks'],REGIONS):
                snapshot=data[offset:offset+length].hex();offset+=length
                if snapshot!=bank['before']:raise ValueError('Actual caller preparation changed retained banks')
                bank['before']=snapshot
            count,=struct.unpack_from('<I',data,offset);offset+=4;prefix_sites=list(struct.unpack_from('<'+'I'*count,data,offset));offset+=count*4
            if frame[0]!=0x129e8a or actual_gp[0]!=0x18f6b0 or frame[2]!=launch_gp[0]:raise ValueError('Actual indirect caller frame differs')
            descriptor=struct.unpack_from('<I',bytes.fromhex(row['banks'][0]['before']),SOURCE_ROWS-BASE+3*188+0x14)[0]
            if descriptor not in (0x44ebf4,0x44ec14)or struct.unpack('<I',original_bytes(image,descriptor+0x10,4))[0]!=0x18f6b0:raise ValueError('Actual caller descriptor differs')
            total+=prefix_steps;row['instructions']+=prefix_steps
            row.update(caller=dict(entry=0x129e30,return_site=frame[0],captured_callee=0x18f6b0,prefix_instructions=prefix_steps,prefix_sites=prefix_sites,actual_frame=frame,
                descriptor=descriptor,descriptor_sha256=hashlib.sha256(original_bytes(image,descriptor,32)).hexdigest(),return_contract='void; EAX/ECX/EDX are dead at actual continuation',
                live_gp_before={name:actual_gp[i]for name,i in [('ebx',3),('esp',4),('ebp',5),('esi',6),('edi',7)]},live_gp_after={name:row['after_gp'][i]for name,i in [('ebx',3),('esp',4),('ebp',5),('esi',6),('edi',7)]},
                fp_before=actual_fp,fp_after=row['after_fp'],caller_preparation_retained_banks_unchanged=True),
                entry=0x18f6b0,gp=actual_gp,stack=actual_gp[4],arguments=frame[1:],before_fp=actual_fp,xmm=actual_xmm,before_x87=actual_x87)
        if offset!=len(data):raise ValueError('Stop callback observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_audio_stop_callback_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Listener instruction inventory image pin differs')
    counts={0x18f6b0:22,0xbcdf0:35,0xd3300:12}
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
    sources=[ROOT/'include/halo2/native_campaign_audio_stop_callback.hpp',ROOT/'src/audio/native_campaign_audio_stop_callback_adapter.hpp',ROOT/'tests/native_campaign_audio_stop_callback_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_stop_callback_reference_tu15.cpp',Path(__file__),ROOT/'src/audio/native_tu15_sound_kernel.hpp',ROOT/'src/audio/native_tu15_sound_manager.cpp',ROOT/'tests/native_campaign_audio_stop_manager_groups.cpp',ROOT/'tests/fixtures/native_campaign_audio_stop_callback_tu15_sites.json']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-stop_callback.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],actual_original_caller_cases=sum('caller'in row for row in fixture['cases']),actual_caller_receipts=[dict(name=row['name'],**row['caller'])for row in fixture['cases']if 'caller'in row],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[0].read_text();variants={'native':None,'mask':('if(!(mask&(1u<<(header.type()&31u))))return 0u;','if(false)return 0u;'),
            'association':('if(current==sound)association.clear();','if(current!=sound)association.clear();'),
            'stride':('auto header=pool.direct(object&0xffffu);','auto header=pool.indexed(object&0xffffu);')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered stop callback store differs','ordered stop callback store count differs','ordered stop callback input count differs','ordered stop callback input differs','actual salted object lookup result differs','actual salted object lookup result differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected stop_callback mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_stop_callback LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            includes=ROOT/'include'
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Stop callback mutation location changed')
                includes=project/name;(includes/'halo2').mkdir(parents=True);(includes/'halo2/native_campaign_audio_stop_callback.hpp').write_text(text.replace(*mutation))
            target='campaign_stop_callback_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{ROOT}/src/audio/native_tu15_sound_manager.cpp" "{ROOT}/tests/native_campaign_audio_stop_manager_groups.cpp")\ntarget_include_directories({target} BEFORE PRIVATE "{includes}" "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_stop_callback_(native|mask|association|stride)$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['skip actual object type mask','invert sound-association match','replace fresh fixed-stride lookup with generic stride'],
            excluded=['whole campaign audio closure','live playback start/stop_callback','immutable literal reads replaced by pinned values','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
