#!/usr/bin/env python3
"""Physical unchanged 129500/33B590/12E9A0 versus semantic native fades.

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

BASE,SIZE,TABLE,ROWS,CLOCK=0x1000000,0x3000,0x1000000,0x1000100,0x1002000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4f2c4c,4),(0x4f2c50,4),(0x44ad40,8),(0x455b0c,12),(0x46a0d0,4),(0x46a0e8,8),(0x46a118,4),(0x46a12c,4),(0x46a1dc,4))

def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')

def corpus(image):
    rng=random.Random(0x12d320);rows=[];configurations=[]
    for active in (0,1):
        for curve in (-1,0,1,2):
            for start,end,now in ((0,1000,250),(1000,0,750),(-1,1000,500),(1000,-1,500),(0,0,0),(-1,-1,0),
                                  (100,200,0),(100,200,300),(0x7fffffff,0x80000000,0xffffffff)):
                configurations.append(dict(label=f'sample-{active}-{curve}-{start}-{end}-{now}',entry=0x12d320,active=active,curve=curve,start=start,end=end,now=now))
    for gain in (0xc2800000,0xc2000000,0xc0000000,0,0x3f800000,0xff800000,0x7f800000,1,0x7fc12345,0xffc54321,0x7f812345):
        for curve in (0,1):configurations.append(dict(label=f'gain-{gain:x}-{curve}',entry=0x12d320,curve=curve,gain=gain))
    for policy in (0,1,2,255):
        for first,second,current in ((0,0,0),(0x3f800000,0x3f800000,0),(0x3f800000,0x3f800000,0x40000000),
            (0,0x3f800000,0x3e800000),(0,0x3f800000,0xbf800000),(0,0x3f800000,0x40000000),
            (0,0x38d1b716,0x38d1b715),(0,0x38d1b717,0x38d1b718),(0,0x38d1b718,0x38d1b719),
            (0x7fc12345,0x7fc54321,0xffc12345),(0x7f812345,0,0),(0,0,1),(1,2,3)):
            configurations.append(dict(label=f'fraction-{policy}-{first:x}-{second:x}-{current:x}',entry=0x12db00,policy=policy,arguments=[first,second,current]))
    for source,target in ((0xffffffff,0xffffffff),(0x12340000,0xffffffff),(0xffffffff,0xabcd0001),(0x12340000,0xabcd0001),(0x12340000,0xabcd0000)):
        for active in (0,1):
            for seconds in (0x3f000000,0,0xbf800000,0x7f800000,0x7fc12345,0x7f812345):
                configurations.append(dict(label=f'request-{source:x}-{target:x}-{active}-{seconds:x}',entry=0x129900,source=source,target=target,active=active,arguments=[seconds]))
    for amplitude in (0,0x80000000,1,0xbf800000,0xff800000,0x7f800000,0x3f800001,0x3f800000,0x3a25b7e0,0x3a25b7e1,0x7fc12345,0x7f812345):
        configurations.append(dict(label=f'amplitude-{amplitude:x}',entry=0x220200,amplitude=amplitude))
    for field in ('start','end'):
        configurations.append(dict(label='clock-alias-'+field,entry=0x12d320,start=-1,end=-1,clock_alias=field))
        configurations.append(dict(label='request-clock-alias-'+field,entry=0x129900,source=0x12340000,target=0xabcd0000,start=-1,end=1000,clock_alias=field,arguments=[0x3f000000]))
    for rounding in range(4):
        for options in configurations:
            main=bytearray(rng.randbytes(SIZE));put=lambda offset,value:struct.pack_into('<I',main,offset,value&0xffffffff)
            put(0x44,ROWS);put(CLOCK-BASE+0x84,options.get('now',250))
            for slot in range(2):
                offset=ROWS-BASE+slot*0xbc;main[offset+4]=0x80 if options.get('active',1) else 0
                struct.pack_into('<h',main,offset+0xae,options.get('curve',0))
                for field,value in ((0xb0,options.get('gain',0xc2000000)),(0xb4,options.get('start',0)),(0xb8,options.get('end',1000))):put(offset+field,value)
            word=lambda value:struct.pack('<I',value&0xffffffff)
            clock_owner=ROWS+({'start':0xb4,'end':0xb8}[options['clock_alias']])-0x84 if 'clock_alias'in options else CLOCK
            values=[main,word(TABLE),word(clock_owner)]+[original_bytes(image,p,n)for p,n in REGIONS[3:]]
            gp=[options.get('source',0x12340000),options.get('policy',0),0x3456789a,options.get('request_curve',options.get('curve',0))&0xffff,SP,0x56789abc,0x6789abcd,options.get('target',0xabcd0001)]
            xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)]
            if 'amplitude'in options:xmm[0][0]=options['amplitude']
            index=len(rows);denorm=(0,0x40,0x8000,0x8040)[index%4]
            rows.append(dict(name=f'r{rounding}-'+options['label'],entry=options['entry'],gp=gp,xmm=xmm,stack=SP,stop=STOP,arguments=options.get('arguments',[]),
                before_fp=[(0x7f,0x27f,0x37f)[index%3]|(rounding<<10),((index%8)<<11)|((index%2)<<14)|(index%64),0x1f80|(rounding<<13)|denorm],
                banks=[dict(address=p,before=bytes(value).hex())for(p,_),value in zip(REGIONS,values)]))
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_fades_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        sections=[f'.original 0x{address:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        for index,bank in enumerate(row['banks']):
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']));assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"');sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,SP-STACK,STOP)
        for index,value in enumerate(row['arguments']):struct.pack_into('<I',stack,SP-STACK+4+index*4,value)
        (owned/'stack.bin').write_bytes(stack)
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
        data=(owned/'output.bin').read_bytes();cw,sw,mxcsr,instructions=struct.unpack_from('<4I',data);offset=16;row['after_gp']=list(struct.unpack_from('<8I',data,offset));offset+=32;row['after_xmm0']=list(struct.unpack_from('<4I',data,offset));offset+=16;row.update(after_fp=[cw,sw,mxcsr],instructions=instructions);total+=instructions
        for bank,(_,length)in zip(row['banks'],REGIONS):bank['after']=data[offset:offset+length].hex();offset+=length
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['reads']=[]
        for _ in range(count):
            destination,length,stores_before=struct.unpack_from('<III',data,offset);offset+=12;row['reads'].append(dict(address=destination,stores_before=stores_before,value=data[offset:offset+length].hex()));offset+=4
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['writes']=[]
        for _ in range(count):
            site,destination,length=struct.unpack_from('<III',data,offset);offset+=12;row['writes'].append(dict(instruction=site,address=destination,before=data[offset:offset+length].hex(),written=data[offset+4:offset+4+length].hex()));offset+=8
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['sites']=list(struct.unpack_from('<'+'I'*count,data,offset));offset+=count*4
        if offset!=len(data):raise ValueError('Fades observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_audio_fades_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Fade instruction inventory image pin differs')
    counts={0x129900:45,0x12d320:130,0x12db00:34,0x220000:27,0x220200:44}
    dominated={0x12d320:{0x12d4b9,0x12d4ba,0x12d4bb,0x12d4c0,0x12d4c1,0x12d4c4,0x12d4ce,0x12d4cf,0x12d4d0,0x12d4d2,0x12d4d3,0x12d4d6},
               0x220200:{0x22025c,0x22025f,0x22026e,0x220273,0x220274,0x22027a,0x22027c,0x22027d}}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Fade closed owner graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Fade original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Fade original instruction inventory count differs')
        missing=expected-reached
        if missing!=dominated.get(entry,set()):raise ValueError('Fade reachable original sites missing or dominated assumption changed')
        coverage[f'{entry:08x}']=dict(required_sites=len(expected),reached_sites=len(expected&reached),missing_sites=[f'{p:08x}'for p in sorted(missing)],
            unexecuted_reason='Ordered results already bounded by the closed amplitude/log conversion; see numerical argument in parity document'if missing else None)
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
    sources=[ROOT/'include/halo2/native_campaign_audio_fades.hpp',ROOT/'src/audio/native_campaign_audio_fades_adapter.hpp',ROOT/'tests/native_campaign_audio_fades_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_fades_reference_tu15.cpp',Path(__file__),ROOT/'include/halo2/native_campaign_audio_ducking.hpp',ROOT/'include/halo2/native_campaign_audio_leaves.hpp',ROOT/'tests/fixtures/native_campaign_audio_fades_tu15_sites.json']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-fades.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),
            coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[0].read_text();variants={'native':None,'fraction':('unsigned(above(scalar(first),scalar(current)))==policy?0u:one','unsigned(above(scalar(first),scalar(current)))==policy?one:0u'),'endpoint':('const auto from=increasing?sound.gain():silence;','const auto from=increasing?0u:silence;'),'publication':('const auto later=clock.milliseconds();const auto old_end=sound.end();sound.end(old_end+later+1u);','const auto old_end=sound.end();const auto later=clock.milliseconds();sound.end(old_end+later+1u);')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered fade store differs','ordered fade input count differs','ordered fade input differs','sampled fade gain differs','fade fraction differs')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected fades mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_fades LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            includes=ROOT/'include'
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Fades mutation location changed')
                includes=project/name;(includes/'halo2').mkdir(parents=True);(includes/'halo2/native_campaign_audio_fades.hpp').write_text(text.replace(*mutation))
            target='campaign_fades_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}")\ntarget_include_directories({target} BEFORE PRIVATE "{includes}" "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_fades_(native|fraction|endpoint|publication)$','--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['reverse degenerate interval step','drop source gain endpoint','move old-end read across clock read'],
            excluded=['whole campaign audio closure','live playback start/retirement','immutable literal reads replaced by pinned values','arbitrary scratch registers/dead stack','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
