#!/usr/bin/env python3
"""Qualify native async insertion and its genuine ordering dependency."""
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
from native_async_insert_replacements import authenticate,PINS
from native_async_order_replacements import authenticate as authenticate_compare
from validate_native_async_order_tu15 import compare

REGIONS=((0x4ef000,4096),(0x61000000,16384),(0x62000000,8192))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--generation',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    from validate_native_tu15_prt_helpers import PIN,physical_original
    if not args.xbe.is_file():print('SKIP missing TU1.5 image');return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong English TU1.5 image')
    source=root/'src/physics/physics_game_arena_runtime_generated.cpp'
    _,body=authenticate(source.read_text(),'122020');_,child=authenticate_compare(source.read_text())
    shadow=root/'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp';_,shadow_body=authenticate(shadow.read_text(),'1232c0')
    from native_selected_operand_binding import require_selected_identity
    require_selected_identity(shadow.read_text(),shadow_body,{PINS['1232c0'][1]})
    def relocated(text,first,last,delta):
        return re.sub(r'0x([0-9a-f]+)u',lambda m:f'0x{int(m[1],16)+delta:x}u' if first<=int(m[1],16)<=last else m[0],text)
    preamble='\nusing namespace halo2::physics_world_runtime_detail;\n'
    preamble+=('std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){'
        'if(image!=SourcePhysicsContactImage::TitleUpdate15)throw std::runtime_error("Unexpected async image");'
        'if(p==0x4e9ebcu)return 0x4ef1bcu;if(p==0x12209eu||p==0x12333eu)return 0x12333eu;'
        'if(p==0x1a47d0u)return 0x1aaa80u;throw std::runtime_error("Unexpected async selected operand");}\n')
    def reference(namespace,parent,leaf):
        return 'namespace '+namespace+'{'+preamble+leaf.replace('run_1a5080(', 'retained_compare(',1)+(
            '\nstatic void run_1a5080(WorldState&s){::record_async_child(s);retained_compare(s);}\n')+parent+'\n}\n'
    actual_parent=relocated(body,0x122020,0x1220d7,0x12a0)
    actual_child=relocated(child,0x1a5080,0x1a5109,0x62b0)
    shadow_preamble='\nusing namespace halo2::physics_world_runtime_detail;\nstd::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}\n'
    shadow_reference='namespace insert_shadow_reference{'+shadow_preamble+actual_child.replace('selected(0x1a47d0u,s.image)','0x1aaa80u').replace('run_1a5080(', 'retained_compare(',1)
    shadow_reference+=('\nstatic void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){'
        'if(target!=0x1ab330u||site!=0x123339u||tail)throw std::runtime_error("Unexpected shadow insert child");'
        '::record_async_child(s);retained_compare(s);}\n')+shadow_body+'\n}\n'
    (scratch/'async_insert_reference.inc').write_text(reference('insert_reference',actual_parent,actual_child)+reference('insert_canonical',body,child)+shadow_reference)
    binary=scratch/'native-async-insert'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(root/'include'),'-I'+str(root/'src'),'-I'+str(scratch),str(root/'tests/native_tu15_async_insert_abi_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    rng=random.Random(0x1232c0);rows=[];receipts=[];inputs=scratch/'inputs';expected=scratch/'expected'
    with inputs.open('wb') as out:
        for ordinal in range(512):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            def put(p,v,n=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=p<=base+size-n:bank[p-base:p-base+n]=(v&((1<<(n*8))-1)).to_bytes(n,'little');return
                raise ValueError('Insertion fixture escaped banks')
            stack=0x62001000+ordinal%16;incoming=0x61000100+ordinal%4;alias=(ordinal//16)%4
            if alias==1:incoming=stack-64
            elif alias==2:incoming=stack-80
            elif alias==3:incoming=0x4ef180
            length=(0,1,2,3,7)[(ordinal//64)%5];kind=(ordinal//4)%8
            nodes=[0x61001000+n*128+ordinal%4 for n in range(length)]
            for n,node in enumerate(nodes):
                priority=100 if kind in (2,3,4,5) else 100-n*30
                callback=0x1aaa80 if kind&1 else (0x1aa500 if n%2 else 0x1aaed0)
                put(node,priority);put(node+8,(0xfffffff0+n*3)&0xffffffff);put(node+20,1000-n*10)
                put(node+0x34,callback);put(node+0x3c,nodes[n+1] if n+1<length else 0)
            priority=(200,-200,100,100,100,100,75,0x80000000)[kind]
            put(incoming,priority);put(incoming+8,0xfffffff4);put(incoming+20,990)
            put(incoming+0x34,0x1aaa80 if kind&1 else 0x1aa500);put(incoming+0x3c,0x61003000)
            put(0x4ef1bc,nodes[0] if nodes else 0)
            gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10);status=(ordinal%4)*0x200
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            source_word=rng.getrandbits(32)
            for i,v in enumerate((0x63000000,incoming,source_word)):put(stack+4*i,v)
            out.write(struct.pack('<12I',*gp,flags,mxcsr,cw,status))
            for lane in xmm:out.write(struct.pack('<4I',*lane))
            out.write(b''.join(banks))
            rows.append(dict(id=ordinal,name=f'insert-{ordinal}-alias-{alias}',entry=0x1232c0,gp=gp,eflags=flags,
                matrix=incoming,source=source_word,max_steps=2000,xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=[],
                banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)]))
            receipts.append(dict(alias=alias,length=length,kind=kind))
    sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x1232c0,0x1ab330),regions=REGIONS,
        trace_calls=[dict(entry=0x1ab330,arguments=3,gp=True,xmm=True)])
    def emit_abi(out,gp,flags,fp,xmm):
        out.write(struct.pack('<12I',*gp,flags,fp[2],fp[1],fp[0]))
        for lane in xmm:out.write(struct.pack('<4I',*lane))
    with expected.open('wb') as out:
        for receipt,row in zip(receipts,physical['cases'],strict=True):
            if not row['completed'] or not row['live_x87_unchanged']:raise ValueError('Original insertion did not return')
            receipt['offset']=out.tell();receipt['child_calls']=len(row['calls']);out.write(struct.pack('<I',0))
            emit_abi(out,row['boundary_gp'],row['boundary_eflags'],row['after_fp'],row['boundary_xmm'])
            out.write(b''.join(bytes.fromhex(bank['after']) for bank in row['banks']))
            out.write(struct.pack('<I',len(row['writes'])))
            for event in row['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                out.write(struct.pack('<III',event['instruction'],event['address'],len(written))+before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            out.write(struct.pack('<I',len(row['calls'])))
            for call in row['calls']:emit_abi(out,call['gp'],call['eflags'],call['fp'],call['xmm'])
            receipt['bytes']=out.tell()-receipt['offset']
    env=dict(os.environ);env.pop('HALO2_REFERENCE_ASYNC_QUEUE',None)
    if args.sanitizers:env['ASAN_OPTIONS']='detect_leaks=0'
    access_receipts=[]
    for protocol in ([],['--canonical-sites']):
        outputs=[];accesses=[]
        for mode in ([],['--reference']):
            with inputs.open('rb') as inp:result=subprocess.run([str(binary),*protocol,*mode],stdin=inp,capture_output=True,env=env,check=True)
            outputs.append(result.stdout)
            with inputs.open('rb') as inp:result=subprocess.run([str(binary),*protocol,*mode,'--accesses'],stdin=inp,capture_output=True,env=env,check=True)
            accesses.append(result.stdout)
        compare(outputs[0],outputs[1],receipts,'Insertion canonical')
        if not protocol:compare(outputs[0],expected.read_bytes(),receipts,'Insertion physical')
        if accesses[0]!=accesses[1]:raise ValueError('Insertion source access/cursor stream differs')
        access_receipts.append(dict(namespace='canonical' if protocol else 'actual',bytes=len(accesses[0]),sha256=hashlib.sha256(accesses[0]).hexdigest()))
    with inputs.open('rb') as inp:shadow_result=subprocess.run([str(binary),'--shadow-reference'],stdin=inp,capture_output=True,env=env,check=True)
    compare(shadow_result.stdout,expected.read_bytes(),receipts,'Shadow insertion physical')
    with inputs.open('rb') as inp:shadow_accesses=subprocess.run([str(binary),'--shadow-reference','--accesses'],stdin=inp,capture_output=True,env=env,check=True)
    if hashlib.sha256(shadow_accesses.stdout).hexdigest()!=access_receipts[0]['sha256']:raise ValueError('Shadow insertion source access/cursor stream differs')
    sites=sorted({site for row in physical['cases'] for site in row['executed_sites']})
    generation=[]
    if args.generation:
        correspondence=root/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/correspondence-v1/complete-report.json'
        arena_output=scratch/'fresh-arena.cpp';arena_manifest=scratch/'fresh-arena.json'
        subprocess.run([sys.executable,'-B',str(root/'tools/generate_physics_game_arena_tu15_20261012.py'),
            '--correspondence',str(correspondence),'--output',str(arena_output),'--manifest',str(arena_manifest)],check=True)
        catalog=sorted({int(entry,16) for entry in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{',shadow.read_text(),re.M)})
        shadow_output=scratch/'fresh-shadow.cpp';shadow_directory=scratch/'shadow-generation'
        command=[sys.executable,'-B',str(root/'tools/generate_shadow_frame_tu15.py'),
            '--directory',str(shadow_directory),'--output',str(shadow_output)]
        for entry in catalog:command+=['--additional-entry',hex(entry)]
        subprocess.run(command,check=True)
        for original,generated,manifest in ((source,arena_output,arena_manifest),(shadow,shadow_output,shadow_directory/'generation.json')):
            if original.read_bytes()!=generated.read_bytes():raise ValueError('Fresh native async owner source differs: '+str(original))
            subprocess.run(['c++','-std=c++20','-fsyntax-only','-I'+str(root/'include'),'-I'+str(root/'src'),str(generated)],check=True)
            generated_receipt=json.loads(manifest.read_text())
            generation.append(dict(source=str(original.relative_to(root)),passed=True,
                generated_sha256=hashlib.sha256(generated.read_bytes()).hexdigest(),manifest_sha256=hashlib.sha256(manifest.read_bytes()).hexdigest(),
                complete_bodies=len(generated_receipt['complete_native_entries']),instructions=len(generated_receipt['lowered_instructions']),syntax_passed=True))
    report=dict(passed=True,cases=len(rows),physical_original=proof,callee_entry_abi_packets=sum(r['child_calls'] for r in receipts),
        compared_bytes=expected.stat().st_size,comparison_sha256=hashlib.sha256(expected.read_bytes()).hexdigest(),
        canonical_ordered_access_cursor_comparisons=access_receipts,shadow_actual_copy_compared=True,original_executed_sites=[f'{site:x}' for site in sites],
        parent_executed_sites=sum(0x1232c0<=site<0x123380 for site in sites),child_executed_sites=sum(0x1ab330<=site<0x1ab3bc for site in sites),
        address_undefined_sanitizers=args.sanitizers,exceptional_partial_abi_qualified=False,physical_implicit_read_stream_claimed=False,
        legacy_map_pack_image_qualified=False,whole_async_queue_qualified=False,retained_large_artifacts=[],
        generation=generation,
        source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (
            source,shadow,Path(__file__).resolve(),root/'tools/native_async_insert_replacements.py',
            root/'src/platform/native_tu15_async_order.hpp',root/'tests/native_tu15_async_insert_abi_tests.cpp',
            root/'tools/generate_physics_game_arena_tu15_20261012.py',root/'tools/generate_physics_live_support_tu15.py',
            root/'tools/generate_shadow_frame_tu15.py',root/'CMakeLists.txt')})
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','cases','callee_entry_abi_packets','parent_executed_sites','child_executed_sites')}));return 0


if __name__=='__main__':sys.exit(main())
