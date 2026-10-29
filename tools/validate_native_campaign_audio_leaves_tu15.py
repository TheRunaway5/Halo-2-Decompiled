#!/usr/bin/env python3
"""Independent unchanged TU1.5 class fades/voice routing, native C++ and mutants.

Original i386 instructions run only in disposable offline reference processes.
All binaries, raw banks and generated vectors stay in run_test.py scratch.
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

ROOT=Path(__file__).resolve().parents[1]
ORIGINAL=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
BASE,SIZE,VOICE,OUTPUT,FADES=0x1000000,0xa000,0x1000000,0x1004000,0x1008000
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x50ebf0,4),(0x52d674,4))

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()
def save(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n')
def checked(command):
    result=subprocess.run(command,text=True,capture_output=True)
    if result.returncode:raise RuntimeError(f'{command[0]} failed ({result.returncode}): {result.stderr[-4096:]}')
    return result
def text_section(image):
    word=lambda offset:struct.unpack_from('<I',image,offset)[0]
    base,count,table=word(0x104),word(0x11c),word(0x120)
    for index in range(count):
        row=table-base+index*56
        address,size,offset,length,name=struct.unpack_from('<5I',image,row+4)
        name=image[name-base:].split(b'\0',1)[0].decode()
        if name=='.text':return address,size,image[offset:offset+length]
    raise ValueError('Original executable text absent')

def corpus():
    rng=random.Random(0x228310)
    specials=(0,0x80000000,1,0x80000001,0x3c23d70a,0x3d088889,0x3f000000,
              0x3f800000,0x40000000,0xbf800000,0x7f800000,0xff800000,
              0x7fc12345,0xffc54321,0x7f812345,0xff854321)
    elapsed=(0xbf800000,0,0x80000000,1,0x80000001,0x3c23d70a,0x3d088889,
             0x3f000000,0x40000000,0x7f800000,0x7fc12345,0x7f812345)
    rows=[]
    def make(name,entry,cw=0x27f,mxcsr=0x1f80,seconds=0x3d088889,count=0,capacity=64,pattern=0):
        bank=bytearray(rng.randbytes(SIZE))
        put=lambda offset,*values:struct.pack_into('<'+'I'*len(values),bank,offset,*values)
        for index in range(54):
            target=specials[(index+pattern)%len(specials)]
            current=specials[(index*3+pattern+1)%len(specials)]
            remaining=specials[(index*5+pattern+2)%len(specials)]
            put(FADES-BASE+index*16,target,current,remaining,0xa1b20000+index)
        struct.pack_into('<hh',bank,4,count,64)
        struct.pack_into('<h',bank,8,capacity)
        for voice in range(max(0,count)):
            bank[12+voice*0x34]=(255,0,31,32,63,127,128,254)[(voice+pattern)%8]
            bank[13+voice*0x34]=(voice+pattern)%8
        for index,value in enumerate((-32768,-1,0,31,64,32767)):
            struct.pack_into('<h',bank,0x1a0c+index*2,value)
        gp=[0x12345678,0x23456789,0x3456789a,0x456789ab,SP,0x56789abc,0x6789abcd,0x789abcde]
        xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)]
        xmm[1][0]=seconds
        rows.append(dict(name=name,entry=entry,gp=gp,xmm=xmm,stack=SP,stop=STOP,output=OUTPUT,
            before_fp=[cw,((len(rows)%8)<<11)|((len(rows)%2)<<14)|(len(rows)%64),mxcsr],
            banks=[dict(address=BASE,before=bank.hex()),dict(address=0x50ebf0,before=struct.pack('<I',FADES).hex()),
                   dict(address=0x52d674,before=struct.pack('<I',VOICE).hex())]))
    # Every SSE rounding/DAZ/FTZ context, the actual nonpositive/unordered gate,
    # completion/interpolation/NaN remaining times and distinct NaN operands.
    for rounding in range(4):
        for denorm in (0,0x40,0x8000,0x8040):
            for pattern,seconds in enumerate(elapsed):
                make(f'fades-r{rounding}-d{denorm:x}-dt{seconds:x}',0x228310,
                     cw=(0x7f,0x27f,0x37f)[pattern%3],mxcsr=0x1f80|(rounding<<13)|denorm,
                     seconds=seconds,pattern=pattern)
    for pattern,count in enumerate((-32768,-1,0,1,2,31,32,33,63,64)):
        for capacity in (-31,-1,0,1,32,33,64):
            make(f'routes-count{count}-capacity{capacity}',0x225d70,count=count,capacity=capacity,pattern=pattern)
    return rows

def physical(rows,scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original English TU1.5 identity differs')
    image=ORIGINAL.read_bytes();address,size,text=text_section(image)
    owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    (owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS))
    observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_audio_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        sections=[f'.original 0x{address:x} : {{ *(.original) }}']
        for index,bank in enumerate(row['banks']):
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']))
            assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"')
            sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<II',stack,SP-STACK,STOP,OUTPUT)
        (owned/'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"')
        sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=row['before_fp'];environment=bytearray(28)
        struct.pack_into('<H',environment,0,cw);struct.pack_into('<H',environment,4,sw);struct.pack_into('<H',environment,8,0xffff)
        (owned/'environment.bin').write_bytes(environment)
        assembly.append(f'''.code32
.section .bridge,"ax"
.globl _start
_start:
 fninit
 fldenv environment
 ldmxcsr sse_control
''')
        for index in range(8):
            assembly.append(f' movdqu vector{index},%xmm{index}')
        for register,value in zip(('eax','ecx','edx','ebx','esp','ebp','esi','edi'),row['gp']):
            assembly.append(f' mov ${value},%{register}')
        assembly.append(f' int3\n jmp *entry\n.balign 4\nentry: .long {row["entry"]}\nsse_control: .long {mxcsr}\nenvironment: .incbin "{owned/"environment.bin"}"')
        for index,vector in enumerate(row['xmm']):assembly.append(f'vector{index}: .long '+','.join(map(str,vector)))
        sections.append('.bridge 0x70000000 : { *(.bridge) }')
        (owned/'reference.s').write_text('\n'.join(assembly)+'\n')
        (owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        checked(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')])
        checked(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')])
        try:checked([str(observer),str(owned/'reference'),hex(STOP),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')])
        except RuntimeError as error:raise RuntimeError(row['name']+': '+str(error))from error
        result=(owned/'output.bin').read_bytes();cw,sw,mxcsr,instructions=struct.unpack_from('<4I',result);offset=16
        row['after_fp']=[cw,sw,mxcsr];row['instructions']=instructions;total+=instructions
        for bank,(_,length)in zip(row['banks'],REGIONS):
            bank['after']=result[offset:offset+length].hex();offset+=length
        count,=struct.unpack_from('<I',result,offset);offset+=4;row['writes']=[]
        for _ in range(count):
            site,destination=struct.unpack_from('<II',result,offset);offset+=8
            row['writes'].append(dict(instruction=site,address=destination,before=result[offset:offset+4].hex(),written=result[offset+4:offset+8].hex()));offset+=8
        count,=struct.unpack_from('<I',result,offset);offset+=4
        row['sites']=list(struct.unpack_from('<'+'I'*count,result,offset));offset+=count*4
        if offset!=len(result):raise ValueError('Original audio output length differs')
    coverage={};source=(ROOT/'src/campaign/source_campaign_audio_tu15_generated.cpp').read_text()
    for entry in (0x228310,0x225d70):
        body=re.search(r'static void run_'+f'{entry:x}'+r'\(WorldState &s\) \{\n(.*?)\n\}',source,re.S).group(1)
        expected={int(site,16)for site in re.findall(r'instruction\(0x([0-9a-f]+)u\)',body)}
        reached=set().union(*(set(row['sites'])for row in rows if row['entry']==entry))
        missing=expected-reached
        if missing:raise ValueError('Original audio instruction coverage missing '+','.join(hex(p)for p in sorted(missing)))
        coverage[f'{entry:08x}']={'required_sites':len(expected),'reached_sites':len(reached),'missing_sites':[]}
    # Pin the entire unchanged executable above; ranges additionally identify
    # the two exact contiguous original bodies independent of generated C++.
    for entry,end in ((0x228310,0x228388),(0x225d70,0x225e3e)):
        coverage[f'{entry:08x}']['original_body_bytes_sha256']=hashlib.sha256(text[entry-address:end-address]).hexdigest()
    return dict(image_sha256=PIN,backend='unchanged-tu15-physical-i386-stores-banks-fp',cases=rows,instructions=total,
                original_instruction_bytes_changed=False,supplied_dependency_returns=[],coverage=coverage)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--native',type=Path)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not ORIGINAL.is_file():
        args.output.mkdir(parents=True,exist_ok=True)
        save(args.output/'report.json',dict(passed=False,status='skipped',validation_not_run=True,
            reason='Required owned TU1.5 offline reference is unavailable',required_reference=str(ORIGINAL),retained_large_artifacts=[]))
        print('SKIP: required owned TU1.5 offline reference is unavailable')
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();args.output.mkdir(parents=True,exist_ok=True)
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[])
    save(args.output/'report.json',report)
    sources=[ROOT/'src/audio/native_tu15_sound_kernels_campaign_channels.cpp',ROOT/'include/halo2/native_campaign_audio_leaves.hpp',
             ROOT/'tests/native_campaign_audio_leaves_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_audio_reference_tu15.cpp',Path(__file__)]
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(corpus(),scratch);path=scratch/'original-audio.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],
            ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),original_image_sha256=PIN,
            fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_instruction_bytes_changed=False,supplied_dependency_returns=[],coverage=fixture['coverage'])
        save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();source=sources[0].read_text()
        mutations={'fade':'const auto contribution=mul(difference,fraction);','route':'bank.word(word,bank.word(word)|(1u<<(voice&31)),NativeCampaignRouteWrite::category);'}
        replacements={'fade':'const auto contribution=_mm_setzero_ps();','route':'bank.word(word,bank.word(word),NativeCampaignRouteWrite::category);'}
        for name,needle in mutations.items():
            if source.count(needle)!=1:raise ValueError('Audio mutation location changed')
            (project/f'{name}.cpp').write_text(source.replace(needle,replacements[name]))
        negative=project/'negative.py';negative.write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],text=True,capture_output=True)
assert r.returncode!=0 and 'ordered audio store differs' in r.stderr,(r.returncode,r.stdout,r.stderr)
print('Independent original rejected native audio mutation:',r.stderr.strip())
''')
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_audio LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n'
        targets=[]
        for name,unit in [('native',sources[0]),('fade',project/'fade.cpp'),('route',project/'route.cpp')]:
            target='native_campaign_audio_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{unit}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include" "{ROOT}/src")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if name!='native':command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME campaign_audio_{name} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake)
        command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),
                 '--scratch-root',str(scratch/'builds'),'--filter','^campaign_audio_(native|fade|route)$','--jobs','2','--test-jobs','1',
                 '--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True)
            report['provided_native']={'path':str(args.native),'exit_code':actual.returncode,'stdout':actual.stdout[-4096:],'stderr':actual.stderr[-4096:]}
        report.update(passed=result.returncode==0 and (not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            mutation_controls=['zero fade contribution','omit voice category bit'],
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),
            excluded=['whole campaign audio dependency closure','live audio registration','Xbox audible waveform parity','non-clear compiled direction flag','unmasked FP traps'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({key:report.get(key)for key in ('passed','original_cases','ordered_original_stores','original_instructions')}))
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
