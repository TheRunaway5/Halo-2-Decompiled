#!/usr/bin/env python3
"""Compare all nativeBCEE0 copies against unchanged TU1.5 hardware execution."""
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
from native_object_scan_replacements import (canonical_body,replace_object_scan,PREFIX)
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x4ec000,4096),(0x61000000,16384),(0x62000000,8192))
COPIES=('src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp',
        'src/gameplay/vehicles/source_vehicle_director_tu15_generated.cpp',
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
    for name,path in zip(('scene','vehicle','frame'),COPIES):
        source=(ROOT/path).read_text();body=next(body for match,body in function_bodies(source)if match[1]=='bcee0')
        original=canonical_body(body);raw=source.replace(body,original,1);native=replace_object_scan(raw)
        if args.require_binding and native!=source:raise ValueError('Native object scan production prefix missing: '+path)
        if replace_object_scan(native)!=native:raise ValueError('Object scan binding is not idempotent')
        selected=next((line for line in source.splitlines()if line.startswith('std::uint32_t selected(')),'')
        mini=replace_object_scan(selected+'\n'+original)
        bound.append('namespace '+name+' {\n'+mini+'\n}\n')
        if name=='vehicle':bound.append('namespace reference {\n'+original+'\n}\n')
        literal=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)
        for shape in (original,literal):
            candidate=raw.replace(original,shape,1);replace_object_scan(candidate)
            for victim in (candidate,replace_object_scan(candidate)):
                target=re.search(r'static void run_bcee0\(WorldState &s\) \{.*?\n\}',victim,re.S)[0]
                changed_body=target.replace('s.gp[5]),2)', 's.gp[5]),1)',1)
                if changed_body==target:raise ValueError('Object scan mutation witness missing')
                changed=victim.replace(target,changed_body,1)
                try:replace_object_scan(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Object scan accepted changed complete body')
        if 'selected('in original:
            changed=raw.replace('{if(image==SourcePhysicsContactImage::MapPack11)',
                '{if(p==0)return 1u;if(image==SourcePhysicsContactImage::MapPack11)',1)
            try:replace_object_scan(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Object scan accepted altered selected resolver')
    (scratch/'object_scan_bodies.inc').write_text('\n'.join(bound))
    binary=scratch/'object-scan'
    command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_object_scan_abi_tests.cpp'),str(ROOT/'src/physics/native_tu15_bsp_object_association.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    refusal=subprocess.run([str(binary)],check=True,capture_output=True,text=True,
        env=dict(os.environ,HALO2_REFERENCE_OBJECT_SCAN='1',HALO2_OBJECT_SCAN_REFUSAL_TEST='1'))
    reference_result=json.loads(refusal.stdout)
    if not reference_result['reference_admission_refused'] or reference_result['literal_callbacks']<=0 or reference_result['last_literal_pc']!=0xbcf65 or reference_result['returned_eax']!=0:
        raise ValueError('Forced object-scan reference route did not provide actual refusal/literal evidence')
    rng=random.Random(0xbcee0);rows=[];requests=[];receipts=[]
    cursors=(0,1,7,255,0x7ffe,0x7fff,0x8000,0xffff)
    modes=('initial-bound','match-first','match-third','exhaust','zero-generation','mask-zero','requested-fail','requested-pass',
        'null-object','kind31','kind32','kind255','salt8000','saltffff','unaligned','row-scan-alias',
        'saved-bx','saved-si','saved-di','cursor-bx','cursor-bp','cursor-si','cursor-di',
        'pool-rows-bx','pool-rows-bp','pool-rows-si','pool-rows-di','global-saved-bx')
    for initial in cursors:
        for mode in modes:
            ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
            def view(at,n):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at and at+n<=base+size:return bank,at-base
                raise ValueError(f'Scan fixture escaped declared bank: {at:x}+{n}')
            def put(at,value,n=4):
                bank,offset=view(at,n);bank[offset:offset+n]=(value&((1<<(n*8))-1)).to_bytes(n,'little')
            def get(at,n=4):
                bank,offset=view(at,n);return int.from_bytes(bank[offset:offset+n],'little')
            index=initial if initial<0x8000 else initial-0x10000
            stack=0x62001000+ordinal%4;scan=0x61000300;pool=0x61000100;first=0x61002000
            cursor=initial;limit=(index+4)&0xffffffff;kind=7;generation=1;mask=0x80;requested=0;flags_byte=0
            gp=[rng.getrandbits(32)for _ in range(8)]
            if mode=='initial-bound':limit=index&0xffffffff
            if mode=='kind31':kind=31;mask=0x80000000
            if mode=='kind32':kind=32;mask=1
            if mode=='kind255':kind=255;mask=0x80000000
            if mode=='salt8000':generation=0x8000
            if mode=='saltffff':generation=0xffff
            if mode=='requested-fail':requested=2
            if mode=='requested-pass':requested=3;flags_byte=7
            if mode=='mask-zero':mask=0
            if mode=='unaligned':scan+=1;pool+=2;first+=3
            if mode=='global-saved-bx':stack=0x4ecbd4
            if mode=='row-scan-alias':scan=first;cursor=0;index=0;limit=1;mask=0x8001;generation=0x8001;kind=0
            if mode=='saved-bx':scan=stack-12;gp[6]=mask;gp[5]=(initial<<16)|requested
            if mode=='saved-si':scan=stack-20;gp[7]=(initial<<16)|requested
            if mode=='saved-di':scan=stack-24
            if mode.startswith('cursor-'):
                offsets={'cursor-bx':4,'cursor-bp':8,'cursor-si':12,'cursor-di':16}
                scan=stack-offsets[mode]-6
            if mode in ('exhaust','zero-generation','mask-zero','requested-fail')or mode.startswith('cursor-'):
                limit=(index+min(4,0x7fff-index))&0xffffffff
            if mode.startswith('pool-rows-'):
                offsets={'pool-rows-bx':4,'pool-rows-bp':8,'pool-rows-si':12,'pool-rows-di':16}
                pool=stack-offsets[mode]-0x44
            rows_base=(first-index*12)&0xffffffff
            put(0x4ecbd0,pool);put(pool+0x38,limit);put(pool+0x44,rows_base)
            put(scan,mask);put(scan+4,requested,1);put(scan+6,cursor,2);put(scan+8,0xffffffff)
            gp[4]=stack
            if mode=='saved-bx':gp[6]=mask;gp[5]=(initial<<16)|requested
            if mode=='saved-si':gp[7]=(initial<<16)|requested
            if mode.startswith('cursor-'):
                register={'cursor-bx':3,'cursor-bp':5,'cursor-si':6,'cursor-di':7}[mode]
                gp[register]=(gp[register]&0xffff0000)|initial
            if mode in ('pool-rows-bx','pool-rows-bp','pool-rows-si'):
                register={'pool-rows-bx':3,'pool-rows-bp':5,'pool-rows-si':6}[mode];gp[register]=rows_base
                if mode=='pool-rows-bx':gp[7]=limit
            put(stack,0x63000000);put(stack+4,scan);put(stack+8,0)
            # Only simulate prefix publications to place authored rows at the
            # address actually selected by aliased caller DATA. No result or
            # branch outcome is supplied to native or original execution.
            saved=[]
            for offset,register in ((4,3),(8,5),(12,6),(16,7)):
                saved.append((stack-offset,get(stack-offset)));put(stack-offset,gp[register])
            effective_rows=get(pool+0x44)
            # Original rows read precedes the final EDI PUSH.
            if mode=='pool-rows-di':effective_rows=rows_base
            effective_cursor=get(scan+6,2);signed_cursor=effective_cursor if effective_cursor<0x8000 else effective_cursor-0x10000
            effective_first=(effective_rows+signed_cursor*12)&0xffffffff
            for at,value in reversed(saved):put(at,value)
            if mode.startswith('pool-rows-'):put(pool+0x44,rows_base)
            for n in range(4):
                active=mode not in ('exhaust','zero-generation','mask-zero','requested-fail')and not mode.startswith('cursor-')
                gen=generation if active and (mode!='match-third' or n==2)else 0
                put(effective_first+n*12,gen,2);put(effective_first+n*12+2,flags_byte,1);put(effective_first+n*12+3,kind,1)
                put(effective_first+n*12+8,0 if mode=='null-object'else 0x90000000+ordinal*4+n)
            # Row/scan shared fields are authored jointly, after row creation.
            if mode=='row-scan-alias':put(scan,mask);put(scan+4,requested,1);put(scan+6,0,2)
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10)
            status=((ordinal%8)<<11)|((ordinal%8)<<8)|(0x4000 if ordinal&8 else 0)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')
            live=list(live_values[:(0,1,4)[ordinal%3]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            row=dict(id=ordinal,name=f'{initial:x}-{mode}',entry=0xbcee0,gp=gp,eflags=flags,matrix=scan,source=0,max_steps=10000,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,
                banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)])
            rows.append(row);requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks));receipts.append(dict(case=row['name'],cursor=initial,mode=mode,live_depth=len(live)))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0xbcee0,),regions=REGIONS)
    inp,expected,actual=(scratch/name for name in ('record.in','record.expected','record.actual'))
    with inp.open('wb')as inputs,expected.open('wb')as outputs:
        for copy in range(3):
            for case,request in zip(physical['cases'],requests,strict=True):
                if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original object scan completion/live-FP contract changed')
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
        raise ValueError(f'Object scan mismatch byte{first}: {a[first:first+16].hex()} vs {b[first:first+16].hex()}')
    paths=(*COPIES,'src/physics/runtime/native_tu15_object_scan.hpp','tools/native_object_scan_replacements.py',
        'include/halo2/native_tu15_bsp_object_association.hpp','src/physics/native_tu15_bsp_object_association.cpp',
        'include/halo2/native_tu15_object_scan_kernel.hpp','include/halo2/native_tu15_retained_store_observation.hpp',
        'tests/native_tu15_object_scan_abi_tests.cpp','tools/validate_native_object_scan_tu15.py','tools/native_selected_operand_binding.py',
        'tools/validate_native_tu15_prt_helpers.py')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=True,cases=receipts,scratch_owner=str(scratch),
        production_copies=3,comparisons=len(rows)*3,original_xbe_sha256=PIN,physical_original=proof,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,
        rejected_mutations=rejected,reference_admission_refused=reference_result['reference_admission_refused'],
        forced_reference_result=reference_result,source_sha256={path:digest(ROOT/path)for path in paths},
        supplemental_canonical_read_store_alias_cases=12,retained_domain_public_events_verified=True,admission_refusal_cases=6,
        canonical_ordered_read_store_cases=len(rows)*3,canonical_final_instruction_cursor_verified=True,
        normal_return_abi_verified=True,ordered_stores_verified=True,memory_fault_prefix_abi_verified=False,
        ordered_read_fault_semantics_verified=False,visual_parity_verified=False,native_production_closure_verified=False,
        retained_large_artifacts=[]),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(rows)*3} native object-scan comparisons against{len(rows)} unchanged i386 cases')
    return 0


if __name__=='__main__':raise SystemExit(main())
