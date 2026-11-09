#!/usr/bin/env python3
"""Compare the full native resource controller and genuine owner boundaries."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import struct
import subprocess
import sys
from native_geometry_request_replacements import canonical_body,PINS,PUBLISH_PINS

REGIONS=((0x475000,4096),(0x4f0000,4096),(0x4f2000,8192),(0x1000000,65536))
OWNERS=(0x130eb0,0x1236a0,0x2dbf74,0x128870,0x22a680)


def hardware(xbe,binary,scratch,env):
    from validate_native_tu15_prt_helpers import PIN,physical_original
    if hashlib.sha256(xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong English TU1.5 original image')
    root=Path(__file__).resolve().parents[1]
    sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    rng=random.Random(0x130980beef);rows=[];receipts=[]
    inputs=scratch/'request-physical.in';expected=scratch/'request-physical.expected';actual=scratch/'request-physical.actual'
    with inputs.open('wb') as stream:
        for ordinal in range(256):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            def put(p,value,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=p<=base+size-width:bank[p-base:p-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Physical request fixture escaped input banks')
            kind=ordinal%10;flags=(ordinal//10)%16
            # Ready/priority/wait first boundaries are explicit positive controls.
            if kind==3:flags&=~2
            if kind==4:flags|=2
            if kind==6:flags|=1
            header=0x1000100;physical=0x1004000;pool=0x1004100;records=0x1005000;rows_at=0x1006100
            stack=0x100e000+ordinal%16
            put(0x4f0414,0x1002100);put(header+0x18,-1 if ordinal%3==0 else 2,2)
            group=(0x6c746d70,0x73627370,0x6d6f6465)[ordinal%3]
            put(0x1002100-16,group);put(0x1002100+32,group)
            put(header+8,0 if kind==1 else 1);put(header+12,-1 if kind==2 else 0)
            put(header+0x1e,1 if kind==0 else 0,1);put(header+0x20,-1 if kind in (3,4) else 2)
            put(0x4f2d6c,physical);put(physical+0x64,pool);put(pool+0x44,records)
            put(physical+0x38,1 if kind==8 else 0x12345678);put(physical+0x34,ordinal%32)
            put(0x4f2d5c,0x1006000);put(0x1006044,rows_at);put(0x4f2d64,0x1008000)
            put(rows_at+26,1 if kind in (7,8) else 0,1);put(rows_at+27,0 if kind==5 else 255,1)
            put(rows_at+28,0x12345678);put(records+56,3)
            put(0x4751ac,1 if kind==9 else 0,1);put(0x4f2d70,(ordinal//7)%4);put(0x4f2d74,(ordinal//11)%3)
            put(0x4f2d78,0xffffffff)
            if kind==8:put(pool+0x44,header+0x1e-68)
            if kind==0 and ordinal%2==0:put(pool+0x44,stack-16-68)
            gp=[rng.getrandbits(32) for _ in range(8)];gp[0]=(gp[0]&0xffffff00)|flags;gp[1]=header;gp[4]=stack
            eflags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10)
            status=(ordinal%4)*0x200;mxcsr=0x1f80|((ordinal%4)<<13)
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            for i,value in enumerate((0x63000000,1,2)):put(stack+i*4,value)
            stream.write(struct.pack('<12I',*gp,eflags,mxcsr,cw,status))
            for lane in xmm:stream.write(struct.pack('<4I',*lane))
            stream.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'request-{kind}-{flags}',entry=0x130980,gp=gp,eflags=eflags,
                matrix=1,source=2,max_steps=10000,xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=[],
                stop_before_entries=list(OWNERS),banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(kind=kind,flags=flags))
    original,proof=physical_original(Xbe(xbe),rows,scratch,entries=(0x130980,),regions=REGIONS)
    with expected.open('wb') as stream:
        for receipt,row in zip(receipts,original['cases'],strict=True):
            if not row['completed'] or not row['live_x87_unchanged']:raise ValueError('Original request did not complete its declared boundary')
            stopped=row['stopped_at'] if row['stopped_at'] in OWNERS else 0
            receipt.update(stopped_owner=f'{stopped:x}' if stopped else None,offset=stream.tell())
            stream.write(struct.pack('<13I',stopped,*row['boundary_gp'],row['boundary_eflags'],row['after_fp'][2],row['after_fp'][1],row['after_fp'][0]))
            for lane in row['boundary_xmm']:stream.write(struct.pack('<4I',*lane))
            stream.write(b''.join(bytes.fromhex(bank['after']) for bank in row['banks']))
            stream.write(struct.pack('<I',len(row['writes'])))
            for event in row['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                stream.write(struct.pack('<III',event['instruction'],event['address'],len(written))+before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            receipt['bytes']=stream.tell()-receipt['offset']
    with inputs.open('rb') as source,actual.open('wb') as out:subprocess.run([str(binary),'--physical'],stdin=source,stdout=out,env=env,check=True)
    left,right=actual.read_bytes(),expected.read_bytes()
    if left!=right:
        index=next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        case=next((r for r in receipts if r['offset']<=index<r['offset']+r['bytes']),{})
        raise ValueError(f'Physical request mismatch at{index} local{index-case.get("offset",0)} {case}: {left[index:index+16].hex()} != {right[index:index+16].hex()}')
    return dict(proof=proof,cases=receipts,compared_bytes=len(left),comparison_sha256=hashlib.sha256(left).hexdigest(),
        actual_child_outcomes_supplied=False,actual_child_instructions_executed=False,
        boundary_only_owners={f'{target:x}':sum(row['stopped_owner']==f'{target:x}' for row in receipts) for target in OWNERS})


def hardware_publish(xbe,binary,scratch,env):
    from validate_native_tu15_prt_helpers import physical_original
    root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    folder=scratch/'publication';folder.mkdir();rng=random.Random(0x22a680);rows=[];receipts=[]
    inputs=folder/'inputs';expected=folder/'expected';actual=folder/'actual'
    with inputs.open('wb') as stream:
        for ordinal in range(384):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            def put(p,value,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=p<=base+size-width:bank[p-base:p-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Publication fixture escaped input banks')
            header=0x1000100;allocation=0x1008000;data=allocation+8;descriptors=0x1001000;stack=0x100e000+ordinal%16
            alias=ordinal%16;count=(-1,0,1,2,3,4)[(ordinal//16)%6]
            if alias in (4,5,6):count=1
            if alias in (1,2,3):count=3
            put(header+0x1c,(-32,0,32,96)[(ordinal//7)%4],2)
            put(header+8,256);put(header+0x10,count);put(header+0x14,descriptors)
            for j in range(max(0,count)):
                descriptor=descriptors+j*16;kind=(0,1,2,3,255)[(ordinal+j)%5]
                locator=16+j*128;selector=65535 if ordinal%3 else j
                offset=0xffffffff if ordinal%11==0 else 0x300+j*64
                size=(0xfffffff1,0x80000000,103,511)[(ordinal+j)%4]
                stride=(1,7,255)[(ordinal+j)%3]
                if j==0 and alias in (1,2,3,4,5,6):
                    kind=1;target={1:stack-4,2:stack-8,3:stack+4,4:stack-12,5:stack-16,6:stack-20}[alias]
                    locator=target-data;offset=0x300;size=8
                    if alias==1:offset=(0x63000000-256-allocation-8)&0xffffffff
                    if alias==2:size=32
                    if alias==3:size=0x1009000
                put(descriptor,kind,1);put(descriptor+4,locator,2);put(descriptor+6,selector if kind>1 else stride,2)
                put(descriptor+8,size);put(descriptor+12,offset)
                if kind>1:
                    buffer=data+locator if selector==65535 else 0x100a000+j*256+selector*32
                    if selector!=65535:put(data+locator+4,0x100a000+j*256)
                    put(buffer+1,stride,1);put(buffer+4,0xe1234567);put(buffer+8,7)
            gp=[rng.getrandbits(32) for _ in range(8)];gp[7]=header;gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10)
            status=(ordinal%4)*0x200;mxcsr=0x1f80|((ordinal%4)<<13)
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            for i,value in enumerate((0x63000000,allocation,2)):put(stack+i*4,value)
            stream.write(struct.pack('<12I',*gp,flags,mxcsr,cw,status))
            for lane in xmm:stream.write(struct.pack('<4I',*lane))
            stream.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'publication-{ordinal}',entry=0x22a680,gp=gp,eflags=flags,
                matrix=allocation,source=2,max_steps=10000,xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=[],
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(alias=alias,count=count))
    original,proof=physical_original(Xbe(xbe),rows,folder,entries=(0x22a680,),regions=REGIONS)
    with expected.open('wb') as stream:
        for receipt,row in zip(receipts,original['cases'],strict=True):
            if not row['completed'] or not row['live_x87_unchanged']:raise ValueError('Original publication did not return')
            receipt['offset']=stream.tell()
            stream.write(struct.pack('<13I',0,*row['boundary_gp'],row['boundary_eflags'],row['after_fp'][2],row['after_fp'][1],row['after_fp'][0]))
            for lane in row['boundary_xmm']:stream.write(struct.pack('<4I',*lane))
            stream.write(b''.join(bytes.fromhex(bank['after']) for bank in row['banks']))
            stream.write(struct.pack('<I',len(row['writes'])))
            for event in row['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                stream.write(struct.pack('<III',event['instruction'],event['address'],len(written))+before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            receipt['bytes']=stream.tell()-receipt['offset']
    with inputs.open('rb') as source,actual.open('wb') as out:subprocess.run([str(binary),'--publish-physical'],stdin=source,stdout=out,env=env,check=True)
    left,right=actual.read_bytes(),expected.read_bytes()
    if left!=right:
        index=next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
        case=next((r for r in receipts if r['offset']<=index<r['offset']+r['bytes']),{})
        raise ValueError(f'Physical publication mismatch at{index} local{index-case.get("offset",0)} {case}: {left[index:index+16].hex()} != {right[index:index+16].hex()}')
    access_results=[]
    for mode in ('--publish-accesses','--publish-reference-accesses'):
        with inputs.open('rb') as source:
            result=subprocess.run([str(binary),mode],stdin=source,capture_output=True,env=env,check=True)
        access_results.append(result.stdout)
    if access_results[0]!=access_results[1]:raise ValueError('Native publication retained access order differs')
    return dict(proof=proof,cases=receipts,compared_bytes=len(left),comparison_sha256=hashlib.sha256(left).hexdigest(),
        ordered_access_bytes=len(access_results[0]),ordered_access_sha256=hashlib.sha256(access_results[0]).hexdigest(),
        shared_data_owner=True,exceptional_partial_abi_qualified=False)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--xbe',type=Path)
    parser.add_argument('--skip-hardware',action='store_true')
    parser.add_argument('--publication',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if not args.skip_hardware:
        if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
            print('SKIP physical request requires Linux x86 ptrace');return 77
        if not args.xbe or not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    source=root/'src/physics/physics_live_support_tu15_generated.cpp'
    body=canonical_body(re.search(r'static void run_130980\(WorldState &s\) \{.*?\n\}',source.read_text(),re.S)[0])
    publication_body=canonical_body(re.search(r'static void run_22a680\(WorldState &s\) \{.*?\n\}',source.read_text(),re.S)[0])
    digest=lambda data:hashlib.sha256(data).hexdigest()
    if digest(body.replace('WorldState &s','STATE &s').encode()) not in PINS:
        raise ValueError('Canonical full resource controller changed')
    if digest(publication_body.replace('WorldState &s','STATE &s').encode()) not in PUBLISH_PINS:
        raise ValueError('Canonical full resource publication changed')
    stubs='\n'.join('static void run_'+entry+'(WorldState&s){::declared_service(s,0x'+entry+'u);}'
        for entry in ('130eb0','1236a0','2dbf74','128870','22a680'))
    (scratch/'geometry_request_reference.inc').write_text(
        'namespace geometry_reference {\nusing namespace halo2::physics_world_runtime_detail;\n'
        'static std::uint32_t sign_extend(std::uint32_t v,unsigned n){return n==1?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)))):std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}\n'
        +stubs+'\n'+body+'\n'+publication_body.replace('run_22a680(', 'publish(',1)+'\n}\n')
    binary=scratch/'native-geometry-request'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),
        str(root/'tests/native_tu15_geometry_request_abi_tests.cpp'),str(root/'src/rendering/source_geometry_resource.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    env=dict(os.environ);env.pop('HALO2_REFERENCE_GEOMETRY_RESOURCE',None)
    if args.sanitizers:env['ASAN_OPTIONS']='detect_leaks=0'
    result=subprocess.run([str(binary)],env=env,text=True,capture_output=True)
    if result.returncode:raise ValueError(result.stderr[-5000:])
    physical=None if args.skip_hardware else hardware(args.xbe,binary,scratch,env)
    publication=hardware_publish(args.xbe,binary,scratch,env) if args.publication and not args.skip_hardware else None
    report=dict(passed=not args.skip_hardware,canonical_body_sha256=digest(body.encode()),cases=json.loads(result.stdout),physical_original=physical,publication=publication,
        address_undefined_sanitizers=args.sanitizers,leak_detection=False,
        actual_production_child_outcomes_qualified=False,exceptional_memory_fault_abi_qualified=False,
        normal_providers='Independent declared admission/promotion/completion contracts; genuine/native22A680 publication',
        native_child_composition=['22a680'],residual_children=['130eb0','1236a0','2dbf74','128870'],
        actual_owner_boundaries='Exact GP/flags/SIMD/FP and complete preceding bank/read/store prefix at each reached service',
        source_sha256={str(path.relative_to(root)):digest(path.read_bytes()) for path in (
            source,Path(__file__).resolve(),root/'src/rendering/native_tu15_geometry_request_abi.hpp',
            root/'tests/native_tu15_geometry_request_abi_tests.cpp',root/'tools/native_geometry_request_replacements.py',
            root/'src/rendering/native_tu15_geometry_publish_abi.hpp',root/'src/rendering/source_geometry_resource.cpp',
            root/'include/halo2/source_geometry_resource.hpp',root/'CMakeLists.txt',root/'cmake/PhysicsLiveWorldTu15.cmake')},
        retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['cases']))


if __name__=='__main__':main()
