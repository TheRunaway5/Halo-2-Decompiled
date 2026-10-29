#!/usr/bin/env python3
"""Physical unchanged 129500/33B590/12E9A0 versus semantic native ducking.

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

BASE,SIZE,GAME,CLOCK=0x1000000,0x3000,0x1001000,0x1002400
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x41b774,4),(0x4f2c50,4),(0x4f3218,4),(0x4f9c6d,1),
         (0x556dfe,9),(0x4f2d40,4),(0x4f2d70,4),(0x46a180,4),(0x46a1dc,4),(0x46a0d0,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x129500);rows=[]
    configurations=[dict(label='ordinary')]
    for name,old,now in (('same',1000,1000),('one',1000,1001),('slow',1000,1501),('wrap',0xfffffff0,0x10),
                         ('negative',200,100),('signed-min',0,0x80000000),('signed-max',0,0x7fffffff)):
        configurations.append(dict(label='clock-'+name,last=old,now=now))
    for gain in (0,0x3e800000,0x3f800000,0x3fc00000,0xc0000000,0x7fc12345,0x7f812345):
        configurations.append(dict(label=f'paused-gain-{gain:x}',paused=1,gain=gain,delay=0))
        configurations.append(dict(label=f'release-gain-{gain:x}',gain=gain))
    for delay in (0xffffffff,0,2,200,0x80000000):
        configurations.append(dict(label=f'delay-{delay:x}',paused=1,delay=delay))
    for seconds in (0,0xbf800000,0x7fc12345,0x7f812345):
        configurations.append(dict(label=f'delay-init-{seconds:x}',paused=1,delay=0xffffffff,delay_seconds=seconds))
    for duration in (0,1,0xbf800000,0x3a83126f,0x7f800000,0x7fc12345,0x7f812345):
        configurations.append(dict(label=f'fadeout-{duration:x}',paused=1,delay=0,fade_out=duration,gain=0xffc54321))
        configurations.append(dict(label=f'fadein-{duration:x}',fade_in=duration,gain=0xffc54321))
    for gate in range(8):configurations.append(dict(label=f'activity-gate-{gate}',gate=gate))
    for counter in range(2):
        for value in (-1,0,1):configurations.append(dict(label=f'activity-counter-{counter}-{value}',counter=counter,counter_value=value))
    configurations += [dict(label='no-game',game=False),dict(label='inactive',active=0),dict(label='noncampaign',kind=2),
                       dict(label='negative-paused-step',last=200,now=100,paused=1,delay=0,gain=0xc0000000),
                       dict(label='paired-accumulators',accumulated=0x7fc12345,effect=0xffc54321)]
    for rounding in range(4):
        for denorm in (0,0x40,0x8000,0x8040):
            for options in configurations:
                main=bytearray(rng.randbytes(SIZE));put=lambda offset,value:struct.pack_into('<I',main,offset,value&0xffffffff)
                put(0x80,options.get('last',1000));put(0x84,0xfffffff0)
                for offset,value in ((0x1e0,0),(0x1e4,options.get('gain',0x3e800000)),(0x1e8,options.get('delay',0xffffffff)),
                    (0x1ec,options.get('delay_seconds',0x3d4ccccd)),(0x1f0,options.get('fade_out',0x3dcccccd)),
                    (0x1f4,options.get('fade_in',0x3e4ccccd)),(0x1fc,options.get('accumulated',0x3f000000)),(0x204,options.get('effect',0x3f400000))):put(offset,value)
                put(GAME-BASE+8,options.get('kind',1));main[GAME-BASE+0x1120]=options.get('active',1);main[GAME-BASE+0x1121]=options.get('paused',0)
                put(CLOCK-BASE,options.get('now',1033));activity=bytearray(9)
                gates=(0x4f9c6d,0x556e03,0x556dfe,0x556dff,0x556e02,0x556e00,0x556e01,0x556e06)
                primary=0
                if 'gate'in options:
                    gate=gates[options['gate']]
                    if gate==0x4f9c6d:primary=1
                    else:activity[gate-0x556dfe]=1
                counters=[0,0]
                if 'counter'in options:counters[options['counter']]=options['counter_value']
                word=lambda value:struct.pack('<I',value&0xffffffff)
                values=(main,word(CLOCK),word(BASE),word(GAME if options.get('game',True)else 0),bytes([primary]),activity,
                        word(counters[0]),word(counters[1]),original_bytes(image,0x46a180,4),original_bytes(image,0x46a1dc,4),original_bytes(image,0x46a0d0,4))
                gp=[0x12345678,0x23456789,0x3456789a,0x456789ab,SP,0x56789abc,0x6789abcd,0x789abcde]
                xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)]
                index=len(rows);rows.append(dict(name=f'r{rounding}-d{denorm:x}-'+options['label'],entry=0x129500,gp=gp,xmm=xmm,stack=SP,stop=STOP,
                    before_fp=[(0x7f,0x27f,0x37f)[index%3],((index%8)<<11)|((index%2)<<14)|(index%64),0x1f80|(rounding<<13)|denorm],
                    banks=[dict(address=p,before=bytes(value).hex())for(p,_),value in zip(REGIONS,values)]))
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ducking_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        sections=[f'.original 0x{address:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        for index,bank in enumerate(row['banks']):
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']));assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"');sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,SP-STACK,STOP);(owned/'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=row['before_fp'];environment=bytearray(28);struct.pack_into('<H',environment,0,cw);struct.pack_into('<H',environment,4,sw);struct.pack_into('<H',environment,8,0xffff);(owned/'environment.bin').write_bytes(environment)
        assembly.append('.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n fldenv environment\n ldmxcsr sse_control')
        for index in range(8):assembly.append(f' movdqu vector{index},%xmm{index}')
        for reg,value in zip(('eax','ecx','edx','ebx','esp','ebp','esi','edi'),row['gp']):assembly.append(f' mov ${value},%{reg}')
        assembly.append(f' int3\n jmp *entry\n.balign 4\nentry: .long {row["entry"]}\nsse_control: .long {mxcsr}\nenvironment: .incbin "{owned/"environment.bin"}"')
        for index,vector in enumerate(row['xmm']):assembly.append(f'vector{index}: .long '+','.join(map(str,vector)))
        sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly)+'\n');(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        checked(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')]);checked(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')])
        try:checked([str(observer),str(owned/'reference'),hex(STOP),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')])
        except RuntimeError as error:raise RuntimeError(row['name']+': '+str(error))from error
        data=(owned/'output.bin').read_bytes();cw,sw,mxcsr,instructions=struct.unpack_from('<4I',data);offset=16;row.update(after_fp=[cw,sw,mxcsr],instructions=instructions);total+=instructions
        for bank,(_,length)in zip(row['banks'],REGIONS):bank['after']=data[offset:offset+length].hex();offset+=length
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['reads']=[]
        for _ in range(count):
            destination,length,stores_before=struct.unpack_from('<III',data,offset);offset+=12;row['reads'].append(dict(address=destination,stores_before=stores_before,value=data[offset:offset+length].hex()));offset+=4
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['writes']=[]
        for _ in range(count):
            site,destination=struct.unpack_from('<II',data,offset);offset+=8;row['writes'].append(dict(instruction=site,address=destination,before=data[offset:offset+4].hex(),written=data[offset+4:offset+8].hex()));offset+=8
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['sites']=list(struct.unpack_from('<'+'I'*count,data,offset));offset+=count*4
        if offset!=len(data):raise ValueError('Ducking observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    for entry,end,file in ((0x129500,0x1296d2,'src/campaign/source_campaign_audio_tu15_generated.cpp'),
                           (0x12e9a0,0x12ea0b,'src/campaign/source_campaign_audio_tu15_generated.cpp'),
                           (0x33b590,0x33b598,'src/input/source_input_startup_tu15_generated.cpp')):
        source=(ROOT/file).read_text();body=re.search(r'static void run_'+f'{entry:x}'+r'\(WorldState &s\) \{\n(.*?)\n\}',source,re.S).group(1)
        expected={int(site,16)for site in re.findall(r'instruction\(0x([0-9a-f]+)u\)',body)};missing=expected-reached
        if missing:raise ValueError('Ducking original sites missing: '+','.join(hex(p)for p in sorted(missing)))
        coverage[f'{entry:08x}']=dict(required_sites=len(expected),reached_sites=len(expected&reached),missing_sites=[],original_body_bytes_sha256=hashlib.sha256(text[entry-address:end-address]).hexdigest())
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
    sources=[ROOT/'include/halo2/native_campaign_audio_ducking.hpp',ROOT/'src/audio/native_campaign_audio_ducking_adapter.hpp',ROOT/'tests/native_campaign_audio_ducking_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ducking_reference_tu15.cpp',Path(__file__)]
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-ducking.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[0].read_text();variants={'native':None,'clock':('const auto elapsed=now-previous;','const auto elapsed=std::uint32_t{};'),'activity':('if(native_campaign_audio_activity(state))return;','if(false)return;'),
            'publication':('state.elapsed_milliseconds(ticks+elapsed);\n    const bool present=state.game_present();','const bool present=state.game_present();\n    state.elapsed_milliseconds(ticks+elapsed);')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered ducking store differs','ordered ducking input count differs','ordered ducking input differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected ducking mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_ducking LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            includes=ROOT/'include'
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Ducking mutation location changed')
                includes=project/name;(includes/'halo2').mkdir(parents=True);(includes/'halo2/native_campaign_audio_ducking.hpp').write_text(text.replace(*mutation))
            target='campaign_ducking_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}")\ntarget_include_directories({target} BEFORE PRIVATE "{includes}" "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_ducking_(native|clock|activity|publication)$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['zero signed/wrapped clock delta','skip activity input query','move game-owner read across tick publication'],
            excluded=['whole campaign audio closure','live playback','immutable literal reads replaced by pinned values','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
