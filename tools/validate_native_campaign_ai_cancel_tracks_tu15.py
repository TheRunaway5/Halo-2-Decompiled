#!/usr/bin/env python3
"""Physical unchanged 275610 actual track-conflict with real campaign policy versus typed semantic native owners.

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

BASE,SIZE,PROPS,TRACKS,GROUPS,AI,GAME,POLICY,BITMAP,PROP_ROWS,TRACK_ROWS,GROUP_ROWS=0x1000000,0xa000,0x1000100,0x1000200,0x1000300,0x1000400,0x1000500,0x1000600,0x1001000,0x1002000,0x1004000,0x1006000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4ecbd0,4),(0x50eef4,4),(0x50eef0,4),(0x50eef8,4),(0x5020a8,4),(0x4f3218,4),(0x5020c4,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x275610);rows=[];configs=[]
    for value in (-32768,-2,-1,0,1,2,15,16,32767):
        for bit in (0,1):configs.append(dict(label=f'policy-{value}-{bit}',entry=0x217310,team=value,bit=bit))
    for mode in (0,3):
        for value in (-2,0,1,2,15,16):configs.append(dict(label=f'policy-mode-{mode}-{value}',entry=0x217310,team=value,mode=mode))
    for status in (-32768,-1,0,1,2,3,32767):
        for same in (False,True):configs.append(dict(label=f'active-{status}-{same}',entry=0x2755b0,status=status,same=same))
    configs += [dict(label='active-empty',entry=0x2755b0,empty=True),dict(label='active-third',entry=0x2755b0,chain=True),dict(label='active-NONE-owner-excluded',entry=0x2755b0,status=2,owner_none=True)]
    for mode in ('empty','first','second','none','raw-datum'):
        configs.append(dict(label='find-'+mode,entry=0x274f40,find=mode))
    for mode in ('match','inactive','no-groups','self-only','wrong-salt-self','policy-mismatch','policy-bit-mismatch','policy-bit-match','no-prop','other-object','inactive-track','NONE-owner','skip-then-match','initial-active','initial-same-excluded','sparse','all-missing','mode0','mode3','different-stride'):
        configs.append(dict(label='conflict-'+mode,entry=0x275610,conflict=mode))
    configs.append(dict(label='actual275770-caller',entry=0x275770,conflict='actual-caller'))
    for options in configs:
        main=bytearray(rng.randbytes(SIZE));main[BITMAP-BASE:BITMAP-BASE+0x100]=bytes(0x100);main[POLICY-BASE:POLICY-BASE+0x100]=bytes(0x100)
        def put(address,value,width=4):main[address-BASE:address-BASE+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
        def bit(value):
            address=POLICY+0xa4+(value*16+1)//32*4;word=int.from_bytes(main[address-BASE:address-BASE+4],'little');put(address,word|(1<<((value*16+1)&31)))
        prop=0x12340001;excluded=0x23450002;group=0x34560001;object_datum=0x45670005
        put(PROPS+0x44,PROP_ROWS);put(TRACKS+0x44,TRACK_ROWS);put(GROUPS+0x44,GROUP_ROWS);put(GROUPS+0x24,0x50);put(GROUPS+0x38,4);put(GROUPS+0x20,8);put(GROUPS+0x48,BITMAP);put(BITMAP,15)
        put(AI+1,1,1);put(GAME+8,options.get('mode',1));put(BASE+0x800+0x44,BASE+0x8000);put(BASE+0x8000+5*12+8,BASE+0x9000);put(BASE+0x9000+0x10a,0,1)
        for index in range(8):
            row=PROP_ROWS+index*0xc4;put(row+8,object_datum if index in (1,2,4)else object_datum^1);put(row+0x14,0xffffffff);put(row+0x18,0xffffffff)
            row=TRACK_ROWS+index*0x3c;put(row+4,excluded+7);put(row+0x24,1,2);put(row+0x34,0xffffffff)
            row=GROUP_ROWS+index*0x50;put(row,0x3456,2);put(row+0x12,2,2);put(row+0x14,0xffffffff)
        put(GROUP_ROWS+2*0x50+0x14,0x12340002);put(PROP_ROWS+2*0xc4+0x18,0x56780001)
        if options['entry']==0x217310:
            value=options['team']
            if options.get('bit')and 0<=value<16:bit(value)
        if options['entry']==0x2755b0:
            if not options.get('empty'):put(PROP_ROWS+0xc4+0x18,0x56780000)
            put(TRACK_ROWS+0x24,options.get('status',0),2);put(TRACK_ROWS+4,excluded if options.get('same')else 0xffffffff if options.get('owner_none')else excluded+1)
            if options.get('owner_none'):excluded=0xffffffff
            if options.get('chain'):
                put(TRACK_ROWS+0x34,0x56780001);put(TRACK_ROWS+0x3c+0x24,3,2);put(TRACK_ROWS+0x3c+0x34,0x56780002);put(TRACK_ROWS+2*0x3c+0x24,2,2)
        if options['entry']==0x274f40:
            mode=options['find'];base=GROUP_ROWS+(group&65535)*0x50
            if mode!='empty':put(base+0x14,0x12340002 if mode=='first'else 0xabcd0003 if mode=='raw-datum'else 0x12340003)
            if mode in ('second','none'):put(PROP_ROWS+3*0xc4+0x14,0x12340004)
            if mode in ('none','raw-datum'):object_datum^=2 if mode=='none'else 1
        if 'conflict'in options:
            mode=options['conflict']
            if mode=='actual-caller':put(PROP_ROWS+0xc4+0x34,0,1);put(GROUP_ROWS+0x50+0x12,0xffff,2)
            elif mode=='inactive':put(AI+1,0,1)
            elif mode=='no-groups':put(BITMAP,0)
            elif mode=='self-only':put(BITMAP,2)
            elif mode=='wrong-salt-self':
                group=0xffff0001;put(BITMAP,2);put(GROUP_ROWS+0x50+0x14,0x12340002)
            elif mode=='policy-mismatch':put(GROUP_ROWS+2*0x50+0x12,1,2)
            elif mode=='policy-bit-mismatch':put(GROUP_ROWS+2*0x50+0x12,3,2);bit(3)
            elif mode=='policy-bit-match':bit(2)
            elif mode=='no-prop':put(GROUP_ROWS+2*0x50+0x14,0xffffffff)
            elif mode=='other-object':put(PROP_ROWS+2*0xc4+8,object_datum^1)
            elif mode=='inactive-track':put(TRACK_ROWS+0x3c+0x24,3,2)
            elif mode=='NONE-owner':put(TRACK_ROWS+0x3c+4,0xffffffff)
            elif mode=='skip-then-match':put(GROUP_ROWS+0x14,0x12340004);put(GROUP_ROWS+0x12,1,2)
            elif mode in ('initial-active','initial-same-excluded'):
                put(PROP_ROWS+0xc4+0x18,0x56780000);put(TRACK_ROWS+4,excluded if mode=='initial-same-excluded'else excluded+1)
                if mode=='initial-same-excluded':put(BITMAP,0)
            elif mode=='sparse':put(BITMAP,8);put(GROUP_ROWS+3*0x50+0x14,0x12340002)
            elif mode=='all-missing':put(GROUP_ROWS+2*0x50+0x14,0xffffffff)
            elif mode in ('mode0','mode3'):put(GAME+8,0 if mode=='mode0'else 3)
            elif mode=='different-stride':
                put(GROUPS+0x24,0x54);put(BITMAP,8);put(GROUP_ROWS+3*0x54,0x3456,2);put(GROUP_ROWS+3*0x54+0x12,2,2);put(GROUP_ROWS+3*0x50+0x14,0x12340002)
        gp=[prop if options['entry']in (0x275610,0x275770)else options.get('team',0)&65535 if options['entry']==0x217310 else 0xabcdef12,prop if options['entry']==0x2755b0 else group if options['entry']==0x274f40 else excluded,0x3456789a,excluded,SP,0x56789abc,0x6789abcd,object_datum]
        arguments=[excluded,group]if options['entry']==0x275770 else[group]if options['entry']==0x275610 else[];number=len(rows);depth=number%4
        before_x87=[struct.pack('<QH',value<<(63-(value.bit_length()-1)),16383+value.bit_length()-1).hex()for value in range(1,depth+1)]
        bindings=((0x50eef4,PROPS),(0x50eef0,TRACKS),(0x50eef8,GROUPS),(0x5020a8,AI),(0x4f3218,GAME),(0x5020c4,POLICY))
        rows.append(dict(name=options['label'],entry=options['entry'],property=prop,excluded=excluded,group=group,object=object_datum,team=options.get('team',0),gp=gp,xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)],stack=SP,stop=STOP,arguments=arguments,before_x87=before_x87,
            before_fp=[(0x7f,0x27f,0x37f)[number%3]|((number%4)<<10),((number%8)<<11)|((number%2)<<14)|(number%64),0x1f80|((number%4)<<13)|(0,0x40,0x8000,0x8040)[number%4]],
            banks=[dict(address=BASE,before=bytes(main).hex()),dict(address=0x4ecbd0,before=struct.pack('<I',BASE+0x800).hex())]+[dict(address=p,before=struct.pack('<I',v).hex())for p,v in bindings]))
        if options['entry']==0x275770:rows[-1].update(stop=0x27594e,capture_entry=0x275610)
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_cancel_tracks_reference_tu15.cpp'),'-o',str(observer)])
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
            actual_group,=struct.unpack_from('<I',captured,position);position+=4;count,=struct.unpack_from('<I',captured,position);position+=4
            prefix_sites=list(struct.unpack_from('<'+'I'*count,captured,position));position+=count*4
            if position!=len(captured)or 0x275949 not in prefix_sites:raise ValueError('Actual track-conflict caller receipt differs')
            row['caller']=dict(entry=0x275770,call_site=0x275949,callee=row['capture_entry'],return_site=0x27594e,prefix_steps=prefix_steps,prefix_sites=prefix_sites,supplied_returns=[])
            row.update(entry=row['capture_entry'],arguments=[actual_group],stack=row['gp'][4],property=row['gp'][0],excluded=row['gp'][1],group=actual_group)
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
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_cancel_tracks_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x275610:103,0x2755b0:28,0x274f40:31,0x217310:34,0x16ea60:30,0xc1e90:11}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Heap closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Heap original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Heap original instruction inventory count differs')
        excluded={int(p,16)for p in routine["unqualified_mode2_sites"]}
        if excluded!=({0x217353,0x217355,0x217356,0x21735b,0x21735d,0x21735f}if entry==0x217310 else set()):raise ValueError("Policy admission exclusion differs")
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
    sources=[ROOT/'include/halo2/native_campaign_ai_cancel_tracks_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_cancel_tracks_tu15.cpp',ROOT/'tests/native_campaign_ai_cancel_tracks_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_cancel_tracks_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_cancel_tracks_tu15_sites.json',ROOT/'src/core/native_tu15_data_array_iteration.cpp',ROOT/'include/halo2/native_tu15_data_array_iteration.hpp',ROOT/'cmake/NativeTu15DataArrayIteration.cmake',ROOT/'cmake/CampaignNativeAiCancelTracks.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-cancel-tracks.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(actual_original_caller_receipts=[dict(name=row['name'],**row['caller'])for row in fixture['cases']if 'caller'in row],caller_contract='actual full275610 plus complete2755B0/274F40; realnonmode2 policy profile only; no inactive-to-active undefined iterator',original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'status':('status<=2','status<=3'),
            'excluded_actor':('read(memory,row+4u)!=excluded_actor','read(memory,row+4u)==excluded_actor'),
            'policy_bit':('std::int32_t(value)*16+1)?0:1','std::int32_t(value)*16+1)?1:0'),
            'policy_bound':('if(value<0||value>=16)','if(value<0||value>=15)'),
            'skip_index':('if(candidate==group)','if((candidate&0xffffu)==(group&0xffffu))'),
            'policy_filter':('if(candidate_category!=category)continue;','if(false)continue;'),
            'final_exclusion':('memory,matched,0xffffffffu','memory,matched,excluded_actor'),
            'capacity_read':('(void)read(memory,groups+0x20u);','')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=30)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered tracks store','ordered tracks input','tracks result differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected tracks mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_tracks LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Tracks mutation location changed: '+name)
                core=project/(name+'.cpp');core.write_text(text.replace(*mutation))
            target='campaign_tracks_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}" "{sources[6]}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_tracks_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['admit inactive status3','invert excluded actor','invert actual policy bit','wrong team bound','skip group by lowindex','omit actual policyclass filter','reuse original excluded actor for candidate','omit actual capacity read'],
            excluded=['actualmode2 virtual160A60 and six policy instruction sites','inactive-to-active undefined private iterator','full1BD8F0 cancellation/speech','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
