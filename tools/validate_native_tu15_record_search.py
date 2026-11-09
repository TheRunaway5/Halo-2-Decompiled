#!/usr/bin/env python3
"""Compare all native1E5810 copies against unchanged TU1.5 hardware execution."""
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
from generated_routine_ownership import function_bodies
from native_record_search_replacements import (canonical_body,replace_record_search,PREFIX)
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x61000000,16384),(0x62000000,8192))
COPIES=('src/physics/physics_live_support_tu15_generated.cpp',
        'src/campaign/ai/source_ai_actor_tu15_generated.cpp',
        'src/campaign/ai/source_ai_frame_tu15_generated.cpp')


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);bound=[];rejected=0
    for name,path in zip(('primary','actor','frame'),COPIES):
        source=(ROOT/path).read_text();body=next(body for match,body in function_bodies(source)if match[1]=='1e5810')
        original=canonical_body(body);raw=source.replace(body,original,1);native=replace_record_search(raw)
        if args.require_binding and native!=source:raise ValueError('Native record search production prefix missing: '+path)
        if replace_record_search(native)!=native:raise ValueError('Record search binding is not idempotent')
        selected=next((line for line in source.splitlines()if line.startswith('std::uint32_t selected(')),'')
        mini=replace_record_search(selected+'\n'+original)
        bound.append('namespace '+name+' {\n'+mini+'\n}\n')
        if name=='primary':bound.append('namespace reference {\n'+original+'\n}\n')
        literal=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)
        for shape in (original,literal):
            candidate=raw.replace(original,shape,1);replace_record_search(candidate)
            for victim in (candidate,replace_record_search(candidate)):
                target=re.search(r'static void run_1e5810\(WorldState &s\) \{.*?\n\}',victim,re.S)[0]
                changed_body=target.replace('s.gp[5]),4)', 's.gp[5]),2)',1)
                if changed_body==target:raise ValueError('Record search mutation witness missing')
                changed=victim.replace(target,changed_body,1)
                try:replace_record_search(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Record search accepted changed complete body')
        if 'selected('in original:
            changed=raw.replace('{if(image==SourcePhysicsContactImage::MapPack11)',
                '{if(p==0)return 1u;if(image==SourcePhysicsContactImage::MapPack11)',1)
            try:replace_record_search(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Record search accepted altered selected resolver')
    (scratch/'record_search_bodies.inc').write_text('\n'.join(bound))
    binary=scratch/'record-search'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_record_search_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,HALO2_REFERENCE_RECORD_SEARCH='1',HALO2_RECORD_SEARCH_REFUSAL_TEST='1'))
    rng=random.Random(0x1e5810);rows=[];requests=[];receipts=[]
    modes=('first','middle','last','miss-low','miss-high','duplicates','negative-stride','zero-stride',
        'unaligned','stack-records','saved-header','result-record','key-record','negative-count','min-count')
    for count in (0,1,2,3,7,16,31,64):
        for mode in modes:
            ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
            def put(at,value):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:struct.pack_into('<I',bank,at-base,value&0xffffffff);return
                raise ValueError('Record fixture escaped declared bank')
            stack=0x62001000+ordinal%16;table=0x61000100+(ordinal%4 if mode=='unaligned'else 0)
            records=0x61002000+(ordinal%4 if mode=='unaligned'else 0);stride=4
            values=[(i*3-90)&0xffffffff for i in range(max(1,count))]
            index={'first':0,'middle':count//2,'last':max(0,count-1)}.get(mode,count//2)
            key=values[min(index,len(values)-1)]
            if mode=='miss-low':key=0x80000000
            if mode=='miss-high':key=0x7fffffff
            if mode=='duplicates':values=[((i//3)*7-20)&0xffffffff for i in range(max(1,count))];key=values[min(index,len(values)-1)]
            if mode=='negative-stride':stride=0xfffffffc;records+=count*4
            if mode=='zero-stride':stride=0;values=[0]*max(1,count);key=0
            if mode=='stack-records':records=stack+0x40
            if mode=='result-record':records=stack+4;key=0
            if mode=='key-record':records=stack+8
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack
            if mode=='saved-header':table=stack-8;gp[3]=records;gp[5]=count
            raw_count=count
            if mode=='negative-count':raw_count=0xffffffff
            if mode=='min-count':raw_count=0x80000000;stride=0;values=[0];key=0
            for i,value in enumerate(values):put(records+i*stride&0xffffffff,value)
            put(table,raw_count);put(table+4,records)
            put(stack,0x63000000);put(stack+4,table);put(stack+8,key);put(stack+12,stride)
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10)
            status=((ordinal%8)<<11)|((ordinal%8)<<8)|(0x4000 if ordinal&8 else 0)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')
            live=list(live_values[:(0,1,4)[ordinal%3]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            row=dict(id=ordinal,name=f'{count}-{mode}',entry=0x1e5810,gp=gp,eflags=flags,matrix=table,source=key,max_steps=10000,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,
                banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)])
            rows.append(row);requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks));receipts.append(dict(case=row['name'],count=raw_count,mode=mode,live_depth=len(live)))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x1e5810,),regions=REGIONS)
    inp,expected,actual=(scratch/name for name in ('record.in','record.expected','record.actual'))
    with inp.open('wb')as inputs,expected.open('wb')as outputs:
        for copy in range(3):
            for case,request in zip(physical['cases'],requests,strict=True):
                if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original record search completion/live-FP contract changed')
                gp,flags,mxcsr,cw,status,live,xmm,banks=request
                inputs.write(struct.pack('<I12I',copy,*gp,flags,mxcsr,cw,status));inputs.write(struct.pack('<I',len(live)))
                for value in live:inputs.write(bytes.fromhex(value))
                for lane in xmm:inputs.write(struct.pack('<4I',*lane))
                inputs.write(b''.join(banks))
                outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
                for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
                outputs.write(b''.join(bytes.fromhex(bank['after'])for bank in case['banks']))
                outputs.write(struct.pack('<I',len(case['writes'])))
                for event in case['writes']:
                    outputs.write(struct.pack('<III',event['instruction'],event['address'],len(bytes.fromhex(event['written']))))
                    outputs.write(bytes.fromhex(event['before']).ljust(4,b'\0'));outputs.write(bytes.fromhex(event['written']).ljust(4,b'\0'))
    with inp.open('rb')as inputs,actual.open('wb')as outputs:subprocess.run([str(binary)],stdin=inputs,stdout=outputs,check=True)
    if digest(actual)!=digest(expected):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
        raise ValueError(f'Record search mismatch byte{first}: {a[first:first+16].hex()} vs {b[first:first+16].hex()}')
    paths=(*COPIES,'src/physics/runtime/native_tu15_record_search.hpp','tools/native_record_search_replacements.py',
        'tests/native_tu15_record_search_tests.cpp','tools/validate_native_tu15_record_search.py','tools/native_selected_operand_binding.py',
        'tools/validate_native_tu15_prt_helpers.py')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=True,cases=receipts,
        production_copies=3,comparisons=len(rows)*3,original_xbe_sha256=PIN,physical_original=proof,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,
        rejected_mutations=rejected,reference_admission_refused=True,source_sha256={path:digest(ROOT/path)for path in paths},
        supplemental_canonical_read_store_alias_cases=8,admission_refusal_cases=6,
        normal_return_abi_verified=True,ordered_stores_verified=True,memory_fault_prefix_abi_verified=False,
        ordered_read_fault_semantics_verified=False,visual_parity_verified=False,native_production_closure_verified=False,
        retained_large_artifacts=[]),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(rows)*3} native record-search comparisons against{len(rows)} unchanged i386 cases')
    return 0


if __name__=='__main__':raise SystemExit(main())
