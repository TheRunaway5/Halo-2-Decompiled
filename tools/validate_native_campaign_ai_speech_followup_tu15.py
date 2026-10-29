#!/usr/bin/env python3
"""Unchanged physical TU1.5 speech followup versus semantic native owners."""
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
OBJECTS,HEADERS,BODY,ACTORS,ACTOR_ROWS,CLOCK,DEFINITION,SPEECH=(BASE+v for v in (0,0x1000,0x8000,0x4000,0x5000,0x12000,0x13000,0x14000))
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4ecbd0,4),(0x5020c8,4),(0x51df54,4),(0x46a0d0,4),(0x46a270,4))
def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')
def corpus(image):
    rng=random.Random(0x1110d0);rows=[];configs=[]
    durations=[0,0x80000000,0xbf800000,1,0x007fffff,0x00800000,0x3f000000,0x3f800000,0x7f7fffff,0x7f800000,0xff800000,0x7fc01234,0x7f800001]
    targets=[0,0x80000000,0xbf800000,1,0x3f000000,0x3f800000,0x40000000,0x7f7fffff,0x7f800000,0xff800000,0x7fc01234,0x7f800001]
    for channel in (-32768,-15,-14,-6,-4,-2,-1,0,1,12,13,32767):
        for duration in (0,0x3f000000):configs.append(dict(kind='unit',label=f'channel-{channel}-{duration:x}',channel=channel,duration=duration))
    for duration in durations:
        for rate in (-32768,-1,0,1,30,32767):configs.append(dict(kind='unit',label=f'blend-{duration:x}-{rate}',duration=duration,rate=rate))
    for target in targets:
        for rounding in range(4):configs.append(dict(kind='unit',label=f'immediate-{target:x}-{rounding}',target=target,duration=0,rounding=rounding))
    configs += [dict(kind='unit',label=label,**options)for label,options in [
        ('invalid-stamp-no-fp',dict(stamp=0xffff1234,target=0x7f800001,duration=0x7f800001)),
        ('negative-state-offset',dict(offset=-0x100,duration=0)),('target-alias-channel-byte',dict(channel=-6,target=0x3f000000,duration=0)),
        ('state-copy-lowbyte-clamp',dict(channel=3,target=0x3f7f0000,duration=0)),('zero-stamp-valid',dict(stamp=0,duration=0))]]
    for channel in (-32768,-1,0,1,12,13,32767):
        for timer in (0,1,0xffff):configs.append(dict(kind='actor',label=f'actor-{channel}-{timer}',channel=channel,timer=timer))
    configs += [dict(kind='actor',label=label,**options)for label,options in [
        ('actor-invalid-unit-stamp',dict(stamp=0xffff0000)),('actor-timer-alias-inverse',dict(timer_alias=True)),
        ('actor-negative-unit-state',dict(offset=-0x100)),('actor-negative-rate',dict(rate=-30)),('actor-zero-rate',dict(rate=0))]]
    for seconds in durations+[0x3f4ccccd,0x3fc00000,0x4f000000,0xcf000000]:
        for rate in (-32768,-30,0,1,30,32767):configs.append(dict(kind='ticks',label=f'ticks-{seconds:x}-{rate}',seconds=seconds,rate=rate))
    for rounding in range(4):
        for precision in range(3):configs.append(dict(kind='ticks',label=f'tick-round-{rounding}-{precision}',seconds=0x3fc00000,rate=1,rounding=rounding,precision=precision))
    configs += [dict(kind='actor',label='actual215840-followup-'+str(channel),channel=channel,caller=True,timer=0xfffe,rate=30)for channel in (0,12,13)]
    for options in configs:
        main=bytearray(rng.randbytes(SIZE))
        def put(p,v,n=4):main[p-BASE:p-BASE+n]=int(v&((1<<(n*8))-1)).to_bytes(n,'little')
        actor_record=ACTOR_ROWS+0x888
        offset=actor_record+0x6ce-0x60-BODY if options.get('timer_alias')else options.get('offset',0x400)
        state=BODY+offset;channel=options.get('channel',3);target=options.get('target',0x3f800000);duration=options.get('duration',0x3f000000);seconds=options.get('seconds',0x3f000000);rate=options.get('rate',30);timer=options.get('timer',0x1234)
        for p,v,n in [(OBJECTS+0x44,HEADERS,4),(HEADERS+20,BODY,4),(BODY+0x33e,offset,2),(ACTORS+0x44,ACTOR_ROWS,4),(actor_record+0x18,0x10000001,4),(state+0x5c,options.get('stamp',0x10000001),4),(CLOCK+2,rate,2),(DEFINITION+0x42,channel+1,2),(SPEECH+0x1c,timer,2),(BODY+0x12c,0x20000001,4)]:put(p,v,n)
        number=len(rows);depth=number%8 if options['kind']=='ticks'else number%9;precision=options.get('precision',number%3);rounding=options.get('rounding',(number//3)%4)
        before_x87=[struct.pack('<QH',v<<(63-(v.bit_length()-1)),16383+v.bit_length()-1).hex()for v in range(1,depth+1)]
        gp=[0x20000001 if options['kind']=='actor'else 0x10000001,channel&0xffff,0x3456789a,0x456789ab,SP,0x56789abc,channel&0xffff,0x789abcde]
        entry={'unit':0x1110d0,'actor':0x215b40,'ticks':0x1494f0}[options['kind']];arguments=[timer]if options['kind']=='actor'else[seconds]if options['kind']=='ticks'else[duration]
        xmm=[[target if i==0 else 0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)]
        values=[bytes(main)]+[struct.pack('<I',v)for v in (OBJECTS,ACTORS,CLOCK)]+[original_bytes(image,p,4)for p in (0x46a0d0,0x46a270)]
        row=dict(name=options['label'],kind=options['kind'],entry=entry,unit=0x10000001,actor=0x20000001,channel=channel,target=target,duration=duration,seconds=seconds,timer=timer,capacity=8-depth,gp=gp,xmm=xmm,stack=SP,stop=STOP,arguments=arguments,before_x87=before_x87,before_fp=[(0x7f,0x27f,0x37f)[precision]|rounding<<10,((number%8)<<11)|(((number%2)<<9)if depth==0 else 0)|(number%64),0x1f80|(((number//12)%4)<<13)|(0,0x40,0x8000,0x8040)[(number//48)%4]],banks=[dict(address=p,before=v.hex())for(p,_),v in zip(REGIONS,values)])
        if options.get('caller'):
            row.update(entry=0x215b06,capture_entry=0x215b40,stop=0x215b28,arguments=[]);row['gp'][0]=DEFINITION;row['gp'][5]=CLOCK;row['gp'][6]=SPEECH;row['gp'][7]=BODY
        rows.append(row)
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_speech_followup_reference_tu15.cpp'),'-o',str(observer)])
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
        try:checked([str(observer),str(owned/'reference'),hex(row['stop']),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')]+([hex(row['capture_entry'])]if 'capture_entry'in row else []))
        except RuntimeError as error:raise RuntimeError(row['name']+': '+str(error))from error
        if 'capture_entry'in row:
            captured=(owned/'output.bin.capture').read_bytes();cw,sw,mxcsr,prefix_steps=struct.unpack_from('<4I',captured);position=16
            row['gp']=list(struct.unpack_from('<8I',captured,position));position+=32;row['xmm']=[list(struct.unpack_from('<4I',captured,position+i*16))for i in range(8)];position+=128
            row['before_x87']=[captured[position+i*16:position+i*16+10].hex()for i in range(len(row['before_x87']))];position+=128;row['before_fp']=[cw,sw,mxcsr]
            for bank,(_,length)in zip(row['banks'],regions):bank['before']=captured[position:position+length].hex();position+=length
            actual_timer,=struct.unpack_from('<I',captured,position);position+=4;count,=struct.unpack_from('<I',captured,position);position+=4
            prefix_sites=list(struct.unpack_from('<'+'I'*count,captured,position));position+=count*4
            count,=struct.unpack_from('<I',captured,position);position+=4;prefix_stores=[]
            for _ in range(count):
                site,destination,length=struct.unpack_from('<III',captured,position);position+=12;prefix_stores.append(dict(instruction=site,address=destination,before=captured[position:position+length].hex(),written=captured[position+4:position+4+length].hex()));position+=8
            if position!=len(captured)or 0x215b23 not in prefix_sites:raise ValueError('Actual admission caller receipt differs')
            row['caller']=dict(entry=0x215b06,call_site=0x215b23,callee=row['capture_entry'],return_site=0x215b28,prefix_steps=prefix_steps,prefix_sites=prefix_sites,prefix_engine_stores=prefix_stores,supplied_returns=[])
            row.update(entry=row['capture_entry'],arguments=[actual_timer],stack=row['gp'][4],timer=actual_timer&65535,actor=row['gp'][0],channel=row['gp'][1]&65535)
            if row['channel']>=32768:row['channel']-=65536
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
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_speech_followup_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x1110d0:64,0x215b40:21,0x1494f0:11}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Speech followup closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Heap original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Heap original instruction inventory count differs')
        unreachable={int(p,16)for p in routine.get("unreachable_sites",[])}
        if unreachable!=({0x111181}if entry==0x1110d0 else set()):raise ValueError("Speech followup unreachable branch changed")
        missing=(expected-unreachable)-reached
        if missing:raise ValueError('Speech queue reachable original sites missing: '+','.join(f'{p:08x}'for p in sorted(missing)))
        coverage[f'{entry:08x}']=dict(required_sites=len(expected-unreachable),reached_sites=len((expected-unreachable)&reached),unreachable_sites=sorted(unreachable),missing_sites=sorted(missing))
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
    sources=[ROOT/'include/halo2/native_campaign_ai_speech_followup_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_speech_followup_tu15.cpp',ROOT/'tests/native_campaign_ai_speech_followup_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_speech_followup_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_speech_followup_tu15_sites.json',ROOT/'include/halo2/native_campaign_audio_ducking.hpp',ROOT/'include/halo2/native_campaign_audio_leaves.hpp',ROOT/'cmake/CampaignNativeAiSpeechFollowup.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-speech-followup.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(actual_original_caller_receipts=[dict(name=row['name'],**row['caller'])for row in fixture['cases']if 'caller'in row],caller_contract='actual unit/actor and by-value source channel/duration; full private caller fields, one extended temporary for1494F0 only',original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'actor_index13':('if(channel<0||channel>=13)return {};','if(channel<0||channel>13)return {};'),
            'negative_channel':('if(channel!=-1&&channel<13)','if(channel>=0&&channel<13)'),
            'last_channel':('n<13;++n','n<12;++n'),
            'negative_clamp':('return result<=255u?result:255u;','return result<=255u?result:0u;'),
            'timer_word':('write(m,record+0x6ceu,timer,2);','write(m,record+0x6ceu,timer+1u,2);'),
            'tick_rounding':('fistpl %0','fisttpl %0'),
            'target_read_order':('const auto current=scalar(read(m,state+0x68u));','const auto current=early_current;')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=60)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered speech store','ordered speech input','speech result differs','speech FP differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected speech mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_speech_followup LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Speech mutation location changed: '+name)
                core=project/(name+'.cpp');changed=text.replace(*mutation)
                if name=='target_read_order':changed=changed.replace('    for(unsigned n=0;n<13;++n)','    const auto early_current=scalar(read(m,state+0x68u));\n    for(unsigned n=0;n<13;++n)')
                core.write_text(changed)
            target='campaign_speech_followup_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_speech_followup_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['accept actor channel13','reject source negative channel indices','omit last byte clear','clamp negative/invalid byte to0','change actor WORD timer','truncate instead ofFISTP rounding','read target before thirteen preceding stores'],
            excluded=['whole215840 retirement and203D80 speech dispatch','whole117240/116D60 submission','unmapped input faults','unmasked FP traps','no extended temporary for1494F0'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
