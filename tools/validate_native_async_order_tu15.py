#!/usr/bin/env python3
"""Compare native async task ordering with canonical and unchanged i386 code."""
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
from native_async_order_replacements import authenticate,BODY_PIN,SELECTED_PIN

REGIONS=((0x61000000,16384),(0x62000000,8192))


def compare(left,right,receipts,label):
    if left==right:return
    index=next((i for i,(a,b) in enumerate(zip(left,right)) if a!=b),min(len(left),len(right)))
    case=next((r for r in receipts if r['offset']<=index<r['offset']+r['bytes']),{})
    raise ValueError(f'{label} mismatch at {index} local {index-case.get("offset",0)} {case}: '
                     f'{left[index:index+16].hex()} != {right[index:index+16].hex()}')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--generation',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP physical async comparison requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing TU1.5 original');return 77
    from validate_native_tu15_prt_helpers import PIN,physical_original
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong English TU1.5 image')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    source=root/'src/physics/physics_game_arena_runtime_generated.cpp'
    _,body=authenticate(source.read_text())
    # The production Arena intentionally publishes the older observation PCs.
    # Physical comparison uses the authenticated TU1.5 address delta for this
    # unchanged 0x8c-byte leaf; this does not change any executed operation.
    actual=re.sub(r'0x([0-9a-f]+)u',lambda m:f'0x{int(m[1],16)+0x62b0:x}u'
        if 0x1a5080<=int(m[1],16)<=0x1a5109 else m[0],body)
    reference_preamble='\nusing namespace halo2::physics_world_runtime_detail;\n'
    reference_preamble+=(
        'std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){'
        'if(image!=SourcePhysicsContactImage::TitleUpdate15||p!=0x1a47d0u)throw std::runtime_error("Unexpected async operand");return 0x1aaa80u;}\n')
    (scratch/'async_order_reference.inc').write_text(
        'namespace async_reference {'+reference_preamble+actual+'\n}\n'
        'namespace async_canonical {'+reference_preamble+body+'\n}\n')
    binary=scratch/'native-async-order'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),
        str(root/'tests/native_tu15_async_order_abi_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    rng=random.Random(0x1ab330);rows=[];receipts=[]
    inputs=scratch/'inputs';expected=scratch/'expected';actual_output=scratch/'actual'
    with inputs.open('wb') as out:
        for ordinal in range(512):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            def put(p,v,n=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=p<=base+size-n:bank[p-base:p-base+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little');return
                raise ValueError('Async fixture escaped banks')
            stack=0x62001000+ordinal%16;incoming=0x61000100+ordinal%4;other=0x61000200+ordinal%4;history=0x61000300+ordinal%4
            alias=ordinal%16;kind=(ordinal//16)%8
            if alias==1:history=other+12
            elif alias==2:history=incoming+12
            elif alias==3:incoming=other
            elif alias==4:history=other-4
            elif alias==5:history=other+8
            elif alias in (6,7,8):history=stack-(20,16,12)[alias-6]
            elif alias in (9,10,11):history=stack+(4,8,12)[alias-9]
            elif alias==12:incoming=stack-8
            elif alias==13:other=stack-8
            elif alias==14:other=stack-4
            elif alias==15:history=other+13
            base_file=rng.getrandbits(32);base_offset=rng.getrandbits(32)
            file_delta=(0,1,0x7fffffff,0x80000000,0xffffffff)[(ordinal//7)%5]
            other_delta=file_delta if (ordinal//3)%3==0 else (file_delta+(1 if kind%2 else -1))&0xffffffff
            put(history,base_file);put(history+4,base_offset);put(history+8,(0,1,255)[(ordinal//5)%3],1)
            put(incoming,base_file+file_delta);put(other,base_file+other_delta)
            put(incoming+12,base_offset+(ordinal%7));put(other+12,base_offset+((ordinal+3)%7))
            incoming_cb=0x1aaa80 if kind&1 else 0x1aa500
            other_cb=0x1aaa80 if kind&2 else 0x1aaed0
            priorities=((0,0),(1,0),(0,1),(0x80000000,0x7fffffff),(0x7fffffff,0x80000000),(0xffffffff,0xffffffff),(0x80000000,0x80000000),(17,17))
            incoming_priority,other_priority=priorities[(ordinal//64)%8]
            gp=[rng.getrandbits(32) for _ in range(8)];gp[1]=other_priority;gp[2]=incoming_priority;gp[3]=other;gp[4]=stack;gp[7]=incoming
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10);status=(ordinal%4)*0x200
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            for i,v in enumerate((0x63000000,history,incoming_cb,other_cb)):put(stack+4*i,v)
            out.write(struct.pack('<12I',*gp,flags,mxcsr,cw,status))
            for lane in xmm:out.write(struct.pack('<4I',*lane))
            out.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'async-{ordinal}-alias-{alias}',entry=0x1ab330,gp=gp,eflags=flags,
                matrix=history,source=incoming_cb,max_steps=1000,xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=[],
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(alias=alias,kind=kind,incoming_priority=incoming_priority,other_priority=other_priority))
    sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x1ab330,),regions=REGIONS)
    with expected.open('wb') as out:
        for receipt,row in zip(receipts,physical['cases'],strict=True):
            if not row['completed'] or not row['live_x87_unchanged']:raise ValueError('Original async comparison did not return unchanged FP')
            receipt['offset']=out.tell()
            out.write(struct.pack('<13I',0,*row['boundary_gp'],row['boundary_eflags'],row['after_fp'][2],row['after_fp'][1],row['after_fp'][0]))
            for lane in row['boundary_xmm']:out.write(struct.pack('<4I',*lane))
            out.write(b''.join(bytes.fromhex(bank['after']) for bank in row['banks']))
            out.write(struct.pack('<I',len(row['writes'])))
            for event in row['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                out.write(struct.pack('<III',event['instruction'],event['address'],len(written))+before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            receipt['bytes']=out.tell()-receipt['offset']
    env=dict(os.environ);env.pop('HALO2_REFERENCE_ASYNC_QUEUE',None)
    if args.sanitizers:env['ASAN_OPTIONS']='detect_leaks=0'
    for mode in ([],['--reference']):
        with inputs.open('rb') as inp,actual_output.open('wb') as out:subprocess.run([str(binary),*mode],stdin=inp,stdout=out,env=env,check=True)
        compare(actual_output.read_bytes(),expected.read_bytes(),receipts,'Native' if not mode else 'Canonical')
    access_receipts=[]
    for protocol in ([],['--canonical-sites']):
        access_streams=[];outputs=[]
        for mode in ([],['--reference']):
            with inputs.open('rb') as inp:
                result=subprocess.run([str(binary),*protocol,*mode,'--cursor','--accesses'],stdin=inp,capture_output=True,env=env,check=True)
            access_streams.append(result.stdout)
            with inputs.open('rb') as inp:
                result=subprocess.run([str(binary),*protocol,*mode,'--cursor'],stdin=inp,capture_output=True,env=env,check=True)
            outputs.append(result.stdout)
        if access_streams[0]!=access_streams[1]:raise ValueError('Async ordered source access/cursor stream differs')
        compare(outputs[0],outputs[1],receipts,'Canonical namespace' if protocol else 'Actual namespace')
        if not protocol:compare(outputs[0],expected.read_bytes(),receipts,'Cursor physical')
        access_receipts.append(dict(namespace='canonical1A5080' if protocol else 'actual1AB330',
            bytes=len(access_streams[0]),sha256=hashlib.sha256(access_streams[0]).hexdigest()))
    guards=subprocess.run([str(binary),'--guard-tests'],env=env,text=True,capture_output=True,check=True)
    sites=sorted({site for row in physical['cases'] for site in row['executed_sites']})
    generation=None
    if args.generation:
        generated=scratch/'arena-generated.cpp';manifest=scratch/'arena-generation.json'
        correspondence=root/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/correspondence-v1/complete-report.json'
        command=[sys.executable,'-B',str(root/'tools/generate_physics_game_arena_tu15_20261012.py'),
            '--correspondence',str(correspondence),'--output',str(generated),'--manifest',str(manifest)]
        subprocess.run(command,check=True)
        if generated.read_bytes()!=source.read_bytes():raise ValueError('Fresh complete Arena generation differs from checked-in native-bound source')
        subprocess.run(['c++','-std=c++20','-fsyntax-only','-I'+str(root/'include'),'-I'+str(root/'src'),str(generated)],check=True)
        receipt=json.loads(manifest.read_text())
        generation=dict(passed=True,generated_cpp_sha256=hashlib.sha256(generated.read_bytes()).hexdigest(),
            manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest(),
            complete_bodies=len(receipt['complete_native_entries']),original_instructions=len(receipt['lowered_instructions']),
            correspondence_sha256=hashlib.sha256(correspondence.read_bytes()).hexdigest(),syntax_passed=True)
    report=dict(passed=True,canonical_body_sha256=BODY_PIN,selected_resolver_sha256=SELECTED_PIN,physical_original=proof,
        cases=len(rows),aliases={str(k):sum(r['alias']==k for r in receipts) for k in range(16)},
        compared_bytes=expected.stat().st_size,comparison_sha256=hashlib.sha256(expected.read_bytes()).hexdigest(),
        canonical_ordered_access_cursor_comparisons=access_receipts,diagnostic_admission=json.loads(guards.stdout),
        original_executed_sites=[f'{site:x}' for site in sites],original_executed_site_count=len(sites),
        ordered_source_reads_include_implicit_pop_ret=True,physical_read_stream_claimed=False,
        generation=generation,
        address_undefined_sanitizers=args.sanitizers,exceptional_memory_fault_abi_qualified=False,
        legacy_map_pack_route_qualified=False,whole_async_queue_qualified=False,retained_large_artifacts=[],
        source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (
            source,Path(__file__).resolve(),root/'tools/native_async_order_replacements.py',
            root/'src/platform/native_tu15_async_order.hpp',root/'tests/native_tu15_async_order_abi_tests.cpp',
            root/'tools/generate_physics_game_arena_tu15_20261012.py',root/'CMakeLists.txt')})
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','cases','compared_bytes','original_executed_site_count')}))
    return 0


if __name__=='__main__':sys.exit(main())
