#!/usr/bin/env python3
"""Qualify1236A0 under actual native mutex policy and segmented original i386."""
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
from native_async_promote_replacements import authenticate
from native_async_insert_replacements import authenticate as authenticate_insert
from native_async_order_replacements import authenticate as authenticate_compare
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x4ec000,20480),(0x61000000,16384),(0x62000000,8192))
ABI_BYTES=176


def decode_abi(data):
    words=struct.unpack('<44I',data)
    return dict(gp=list(words[:8]),eflags=words[8],fp=[words[11],words[10],words[9]],
        xmm=[list(words[12+i*4:16+i*4]) for i in range(8)])


class Reader:
    def __init__(self,data):self.data=data;self.offset=0
    def take(self,count):
        value=self.data[self.offset:self.offset+count];self.offset+=count
        if len(value)!=count:raise ValueError('Truncated promotion result')
        return value
    def word(self):return struct.unpack('<I',self.take(4))[0]
    def abi(self):return decode_abi(self.take(ABI_BYTES))
    def banks(self):return [self.take(size) for _,size in REGIONS]
    def stores(self):
        result=[]
        for _ in range(self.word()):
            pc,address,size=struct.unpack('<III',self.take(12));before=self.take(4);after=self.take(4)
            result.append(dict(instruction=pc,address=address,before=before[:size].hex(),written=after[:size].hex()))
        return result
    def case(self):
        result=dict(abi=self.abi(),banks=self.banks(),stores=self.stores(),calls=[])
        for _ in range(self.word()):result['calls'].append(dict(entry=self.word(),abi=self.abi()))
        result['phases']=[]
        for _ in range(self.word()):
            result['phases'].append(dict(target=self.word(),before=self.abi(),after=self.abi(),banks=self.banks(),stores=self.stores()))
        return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true')
    parser.add_argument('--cases',type=int,default=512);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    if sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686'):
        print('SKIP segmented original async promotion requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 executable');return 77
    if digest(args.xbe)!=PIN:raise ValueError('Wrong English TU1.5 executable')
    primary=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';_,parent=authenticate(primary.read_text())
    arena=ROOT/'src/physics/physics_game_arena_runtime_generated.cpp';_,compare=authenticate_compare(arena.read_text())
    shadow=ROOT/'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp';_,insert=authenticate_insert(shadow.read_text(),'1232c0')
    def selected(body,source):
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        mapping={int(a,16):int(b,16) for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
        return re.sub(r'selected\(0x([0-9a-f]+)u,s.image\)',lambda m:f'0x{mapping[int(m[1],16)]:x}u',body)
    compare=selected(compare,arena.read_text())
    compare=re.sub(r'0x([0-9a-f]+)u',lambda m:f'0x{int(m[1],16)+0x62b0:x}u' if 0x1a5080<=int(m[1],16)<=0x1a5109 else m[0],compare)
    compare=compare.replace('run_1a5080(', 'retained_compare(',1);insert=selected(insert,shadow.read_text())
    reference='namespace promote_reference {using namespace halo2::physics_world_runtime_detail;\n'
    reference+='static void dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);\n'+compare+'\n'+insert+'\n'+parent+'\n'
    reference+='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){'
    reference+='if(target==0x1232c0u){::record_promote_child(target,s);run_1232c0(s);}'
    reference+='else if(target==0x1ab330u){::record_promote_child(target,s);retained_compare(s);}'
    reference+='else s.required(site,target,tail);}\n}\n'
    (scratch/'async_promote_reference.inc').write_text(reference)
    binary=scratch/'native-async-promote'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-pthread',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_async_promote_abi_tests.cpp'),
        str(ROOT/'src/platform/native_synchronization.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    rng=random.Random(0x1236a0);initial=[];receipts=[];inputs=scratch/'promotion.in'
    with inputs.open('wb') as out:
        for ordinal in range(args.cases):
            banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
            def put(p,value,n=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=p<=base+size-n:bank[p-base:p-base+n]=(value&((1<<(8*n))-1)).to_bytes(n,'little');return
                raise ValueError(f'Promotion fixture escaped0x{p:x}')
            index=(0,1,17,127,149,150,151,255)[ordinal%8]
            alias=('ordinary','priority-slot','saved-bx-slot','return-slot')[(ordinal//64)%4]
            stack=0x62001000+ordinal%16
            if index==150 and alias!='ordinary':stack=0x62001000+{'priority-slot':0xce,'saved-bx-slot':0xd6,'return-slot':0xd2}[alias]
            slot=0x4ecc38+index*64;valid=(ordinal//8)%4!=0 or alias=='return-slot'
            prefix=(0,1,3,7)[(ordinal//32)%4]
            if alias!='ordinary' or index==150:prefix=max(1,prefix)
            predecessors=[0x61000100+n*128+ordinal%4 for n in range(prefix)]
            if index==150:predecessors[0]=0x61000096 if alias=='ordinary' else stack-{'priority-slot':0x38,'saved-bx-slot':0x40,'return-slot':0x3c}[alias]
            elif alias!='ordinary':predecessors[0]=stack-{'priority-slot':0x38,'saved-bx-slot':0x40,'return-slot':0x3c}[alias]
            # Keep the frame-aliasing predecessor immediately before the task.
            if alias!='ordinary':predecessors=predecessors[1:]+predecessors[:1]
            if index==150 and alias!='ordinary' and len(predecessors)>1:predecessors[0]=0x61000096
            successors=[0x61002000+n*128+ordinal%4 for n in range((0,1,3)[(ordinal//4)%3])]
            if alias=='return-slot':predecessors=predecessors[-1:];successors=[]
            handle=predecessors[0] if index==150 else ((0x12340000+ordinal*256)&0xffffff00)|index
            if index==150 and not valid:handle^=0x100
            priority=(0x80000000,0x7fffffff,0xffffffff,0,75,100,200)[(ordinal//2)%7]
            if alias=='return-slot':priority=200
            chain=predecessors+[slot]+successors
            for n,node in enumerate(chain):
                put(node,10 if alias=='return-slot' else 100-n*10);put(node+8,(0xfffffff0+n*3)&0xffffffff)
                put(node+20,1000-n*10);put(node+0x34,0x1aaa80 if ordinal&1 else 0x1aa500)
                put(node+0x3c,chain[n+1] if n+1<len(chain) else (0x63000000 if alias=='return-slot' else 0))
            put(slot+4,handle if valid else handle^0x100);put(0x4ef1bc,chain[0])
            gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=stack;gp[6]=handle
            if alias=='saved-bx-slot':gp[3]=slot
            return_word=slot if alias=='return-slot' else 0x63000000
            put(stack,return_word);put(stack+4,slot if alias=='priority-slot' else priority);put(stack+8,rng.getrandbits(32))
            put(0x4ecc2c,0xf3000001+ordinal%2)
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10)
            status=((ordinal%8)<<11)|((ordinal%8)<<8)|(0x4000 if ordinal&8 else 0)|((ordinal>>4)&63)
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            raw=struct.pack('<12I',*gp,flags,mxcsr,cw,status)+b''.join(struct.pack('<4I',*lane) for lane in xmm)
            out.write(raw+b''.join(banks));entry_abi=decode_abi(raw);entry_abi['fp']=[cw,status,mxcsr]
            initial.append(dict(abi=entry_abi,banks=[bytes(b) for b in banks]))
            receipts.append(dict(index=index,alias=alias,valid=valid,prefix=len(predecessors),successors=len(successors),priority=priority))
    env=dict(os.environ);env.pop('HALO2_REFERENCE_ASYNC_QUEUE',None)
    if args.sanitizers:env['ASAN_OPTIONS']='detect_leaks=0'
    streams=[];access=[]
    for mode in ([],['--reference']):
        with inputs.open('rb') as inp:run=subprocess.run([str(binary),*mode],stdin=inp,capture_output=True,env=env)
        if run.returncode:raise ValueError('Promotion failed '+str(mode)+': '+run.stderr.decode()[-3000:])
        streams.append(run.stdout)
        with inputs.open('rb') as inp:run=subprocess.run([str(binary),*mode,'--accesses'],stdin=inp,capture_output=True,env=env)
        if run.returncode:raise ValueError('Promotion access replay failed: '+run.stderr.decode()[-3000:])
        access.append(run.stdout)
    if streams[0]!=streams[1]:
        first=next((i for i,(a,b) in enumerate(zip(*streams)) if a!=b),min(map(len,streams)))
        raise ValueError(f'Native/canonical promotion differs at byte{first}: {streams[0][first:first+20].hex()} vs{streams[1][first:first+20].hex()}')
    if access[0]!=access[1]:raise ValueError('Native/canonical promotion ordered access/cursor stream differs')
    reader=Reader(streams[0]);outputs=[reader.case() for _ in receipts]
    if reader.offset!=len(reader.data):raise ValueError('Unconsumed promotion output')
    def row(abi,banks,entry,stop,name):
        def word(p):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=p<=base+size-4:return struct.unpack_from('<I',bank,p-base)[0]
            raise ValueError('Continuation stack escaped banks')
        sp=abi['gp'][4]
        result=dict(name=name,entry=entry,gp=abi['gp'],eflags=abi['eflags'],xmm=abi['xmm'],before_fp=abi['fp'],live_x87=[],
            return_pc=word(sp),matrix=word(sp+4),source=word(sp+8),max_steps=4000,
            banks=[dict(address=base,before=bank.hex()) for (base,_),bank in zip(REGIONS,banks)])
        if stop:result['stop_before_entries']=[stop]
        return result
    rows=[];expected=[]
    for n,(before,result,receipt) in enumerate(zip(initial,outputs,receipts,strict=True)):
        wait,release=result['phases']
        if (wait['target'],release['target'])!=(0x2e0478,0x2e0342):raise ValueError('Promotion mutex boundary order changed')
        rows.append(row(before['abi'],before['banks'],0x1236a0,0x2e0478,f'{n}-before-native-wait'))
        expected.append(dict(abi=wait['before'],banks=wait['banks'],stores=wait['stores'],calls=[]))
        rows.append(row(wait['after'],wait['banks'],0x1236b0,0x2e0342,f'{n}-declared-post-wait-continuation'))
        expected.append(dict(abi=release['before'],banks=release['banks'],stores=release['stores'][len(wait['stores']):],calls=[c for c in result['calls'] if c['entry'] not in (0x2e0478,0x2e0342)]))
        rows.append(row(release['after'],release['banks'],0x123708,None,f'{n}-declared-post-release-continuation'))
        expected.append(dict(abi=result['abi'],banks=result['banks'],stores=[],calls=[]))
        receipt['insertion_calls']=sum(c['entry']==0x1232c0 for c in result['calls'])
        receipt['comparison_calls']=sum(c['entry']==0x1ab330 for c in result['calls'])
        if receipt['insertion_calls']!=int(receipt['valid']) or (result['abi']['gp'][0]&255)!=int(receipt['valid']):
            raise ValueError(f'Promotion fixture did not reach its declared valid/no-op path: {n} {receipt}')
    for n,r in enumerate(rows):r['id']=n
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x1236a0,0x1232c0,0x1ab330),regions=REGIONS,
        trace_calls=[dict(entry=e,arguments=0,gp=True,xmm=True) for e in (0x1232c0,0x1ab330)])
    for n,(actual,wanted) in enumerate(zip(physical['cases'],expected,strict=True)):
        abi=dict(gp=actual['boundary_gp'],eflags=actual['boundary_eflags'],fp=actual['after_fp'],xmm=actual['boundary_xmm'])
        calls=[dict(entry=c['entry'],abi=dict(gp=c['gp'],eflags=c['eflags'],fp=c['fp'],xmm=c['xmm'])) for c in actual['calls']]
        observed=dict(abi=abi,banks=[bytes.fromhex(b['after']) for b in actual['banks']],stores=actual['writes'],calls=calls)
        if not actual['completed'] or not actual['live_x87_unchanged']:raise ValueError(f'Original promotion segment{n} incomplete')
        if observed!=wanted:
            differences=[key for key in wanted if observed[key]!=wanted[key]]
            detail=f' ABI {abi} vs {wanted["abi"]}' if 'abi' in differences else ''
            raise ValueError(f'Original promotion segment{n} {rows[n]["name"]} differs: {differences}{detail}')
    rejected=0
    if args.require_binding:
        import native_async_promote_replacements as binding
        source=primary.read_text();bind=binding.install_native_async_promote
        if bind(source)!=source:raise ValueError('Promotion native-default binding absent')
        body=binding.authenticate(source)[0][0]
        for candidate in (body,binding.canonical_body(body)):
            for old,new in (('0x4ecc38u','0x4ecc3cu'),('0x2e0478u','0x2e0342u'),
                            ('(0x8u+s.gp[4])','(0xcu+s.gp[4])')):
                changed=candidate.replace(old,new,1)
                if changed==candidate:raise ValueError('Missing promotion body mutation witness')
                try:bind(source.replace(body,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Promotion binder admitted semantic body drift')
    paths=('src/platform/native_tu15_async_promote.hpp','tools/native_async_promote_replacements.py',
        'tools/validate_native_async_promote_tu15.py','tests/native_tu15_async_promote_abi_tests.cpp',
        'src/platform/native_tu15_async_order.hpp','tools/native_async_insert_replacements.py','tools/native_async_order_replacements.py',
        'src/platform/native_synchronization.cpp','include/halo2/native_synchronization.hpp','src/physics/physics_game_arena_native.cpp',
        'src/physics/physics_live_support_tu15_generated.cpp','src/physics/physics_game_arena_runtime_generated.cpp',
        'src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','tools/validate_native_tu15_prt_helpers.py',
        'cmake/NativeTu15AsyncPromotion.cmake')
    report=dict(passed=True,cases=receipts,physical_original=proof,segmented_physical_cases=len(rows),
        declared_post_wait_continuation_inputs=len(receipts),declared_post_release_continuation_inputs=len(receipts),
        whole_original_mutex_path_qualified=False,xbox_kernel_mutex_equivalence_claimed=False,
        segmented_instruction_patches=0,segmented_supplied_callee_returns=0,legacy_map_pack_image_qualified=False,
        native_synchronization_policy='Actual fresh native mutex wait/release with existing Arena GP/flags/stack adaptation',
        native_mutex_waits_and_releases_per_mode=len(receipts),native_mutex_ownership_checkpoint_preserved=True,
        normal_abi_banks_stores_and_child_entries_exact=True,compared_bytes=len(streams[0]),comparison_sha256=hashlib.sha256(streams[0]).hexdigest(),
        canonical_ordered_access_cursor_exact=True,access_stream_bytes=len(access[0]),access_stream_sha256=hashlib.sha256(access[0]).hexdigest(),
        rejected_semantic_mutations=rejected,exceptional_partial_abi_qualified=False,physical_implicit_read_stream_claimed=False,
        source_sha256={path:digest(ROOT/path) for path in paths},scratch_owner=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS {len(receipts)} native async promotion cases and {len(rows)} segmented original i386 boundaries')
    return 0


if __name__=='__main__':sys.exit(main())
