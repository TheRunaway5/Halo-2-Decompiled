#!/usr/bin/env python3
"""Compare native model-default pose gathers against unchanged TU1.5 hardware."""
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
from native_model_default_pose_replacements import canonical_body,replace_model_default_pose
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x46a000,4096),(0x61000000,65536),(0x62000000,8192))
COPIES=('src/physics/physics_live_support_tu15_generated.cpp',
        'src/campaign/ai/source_ai_frame_tu15_generated.cpp')
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--compiler',default='c++')
    p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);bound=[];rejected=0
    for name,path in zip(('primary','frame'),COPIES):
        source=(ROOT/path).read_text();body=next(body for match,body in function_bodies(source)if match[1]=='1707a0')
        original=canonical_body(body);raw=source.replace(body,original,1);native=replace_model_default_pose(raw)
        if args.require_binding and native!=source:raise ValueError('Native model pose production prefix missing: '+path)
        if replace_model_default_pose(native)!=native:raise ValueError('Model pose binding is not idempotent')
        selected=next((line for line in source.splitlines()if line.startswith('std::uint32_t selected(')),'')
        bound.append('namespace '+name+' {\n'+replace_model_default_pose(selected+'\n'+original)+'\n}\n')
        if name=='primary':bound.append('namespace reference {\n'+original+'\n}\n')
        for victim in (raw,native):
            target=re.search(r'static void run_1707a0\(WorldState &s\) \{.*?\n\}',victim,re.S)[0]
            changed=victim.replace(target,target.replace('u(_mm_cvtss_f32(s.xmm[0]))','0u',1),1)
            if changed==victim:raise ValueError('Pose body mutation witness missing')
            try:replace_model_default_pose(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Pose binding accepted changed body')
        if 'selected('in original:
            changed=raw.replace('{if(image==SourcePhysicsContactImage::MapPack11)',
                '{if(p==0)return 1u;if(image==SourcePhysicsContactImage::MapPack11)',1)
            try:replace_model_default_pose(changed)
            except ValueError:rejected+=1
            else:raise ValueError('Pose binding accepted changed resolver')
    (scratch/'model_default_pose_bodies.inc').write_text('\n'.join(bound));binary=scratch/'model-default-pose'
    command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_model_default_pose_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    forced=subprocess.run([str(binary)],text=True,capture_output=True,
        env=dict(os.environ,HALO2_REFERENCE_MODEL_DEFAULT_POSE='1',HALO2_MODEL_DEFAULT_POSE_REFUSAL_TEST='1'))
    if forced.returncode:raise ValueError('Forced reference failed: '+forced.stderr[-3000:])
    faults=subprocess.run([str(binary)],text=True,capture_output=True,
        env=dict(os.environ,HALO2_MODEL_DEFAULT_POSE_FAULT_TEST='1'))
    if faults.returncode:raise ValueError('Retained fault proof failed: '+faults.stderr[-4000:])
    rng=random.Random(0x1707a0);rows=[];requests=[];receipts=[]
    modes=('normal','unaligned','overlap-forward','overlap-back','future-row','saved-frame','descriptor','count-cell')
    for count in (0,1,2,3,7,16,31,64,127,255,0xffffffff,0x80000000):
        for mode in modes:
            ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
            def put(at,value):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:struct.pack_into('<I',bank,at-base,value&0xffffffff);return
                raise ValueError('Pose fixture escaped declared bank')
            stack=0x62001000+ordinal%16;model=0x61000100;records=0x61001000;destination=0x61008000
            effective=count
            if mode=='unaligned':model+=1;records+=3;destination+=2
            if mode=='overlap-forward':destination=records+0x1c
            if mode=='overlap-back':destination=records+0x10
            if mode=='future-row':destination=records+0x60
            if mode in ('saved-frame','descriptor','count-cell')and count not in (0,0xffffffff,0x80000000):effective=1
            if mode=='saved-frame':destination=stack-16
            if mode=='descriptor':destination=model+0x4c
            if mode=='count-cell':destination=model+0x48;put(records+0x18,1)
            if mode=='saved-frame':put(records+0x0c,0x63000000);put(records+0x10,model)
            put(model+0x48,effective);put(model+0x4c,records);put(0x46a0d0,(0x3f800000,0x80000000,0x7f800001,0x7fc12345)[ordinal%4])
            put(stack,0x63000000);put(stack+4,model);put(stack+8,destination)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10);status=((ordinal%8)<<11)|((ordinal%8)<<8)|(0x4000 if ordinal&8 else 0)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')
            live=list(live_values[:(0,1,4)[ordinal%3]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            rows.append(dict(id=ordinal,name=f'{count}-{mode}',entry=0x1707a0,gp=gp,eflags=flags,matrix=model,source=destination,max_steps=10000,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
            requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks));receipts.append(dict(case=rows[-1]['name'],count=effective,mode=mode,live_depth=len(live)))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);body_pin=hashlib.sha256(image.read(0x1707a0,0x76)).hexdigest()
    if body_pin!='74779002b574468de312aef48d332cfa8a3bb7c6aecf09967efd5302c63a61ea':raise ValueError('Whole original model pose body changed')
    physical,proof=physical_original(image,rows,scratch,entries=(0x1707a0,),regions=REGIONS)
    executed=sorted({pc for row in physical['cases']for pc in row['executed_sites']})
    if len(executed)!=proof['owners']['0x1707a0']['sites']:raise ValueError('Model pose original site coverage incomplete')
    inp,expected,actual=(scratch/name for name in ('pose.in','pose.expected','pose.actual'))
    with inp.open('wb')as inputs,expected.open('wb')as outputs:
        for copy in range(4):
            for case,request in zip(physical['cases'],requests,strict=True):
                if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original pose completion/live-FP contract changed: '+case['name'])
                gp,flags,mxcsr,cw,status,live,xmm,banks=request
                inputs.write(struct.pack('<I12I',copy,*gp,flags,mxcsr,cw,status));inputs.write(struct.pack('<I',len(live)))
                for value in live:inputs.write(bytes.fromhex(value))
                for lane in xmm:inputs.write(struct.pack('<4I',*lane))
                inputs.write(b''.join(banks))
                outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
                for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
                outputs.write(b''.join(bytes.fromhex(bank['after'])for bank in case['banks']))
                writes=case['writes']if copy<2 else []
                outputs.write(struct.pack('<I',len(writes)))
                for event in writes:
                    outputs.write(struct.pack('<III',event['instruction'],event['address'],len(bytes.fromhex(event['written']))))
                    outputs.write(bytes.fromhex(event['before']).ljust(4,b'\0'));outputs.write(bytes.fromhex(event['written']).ljust(4,b'\0'))
    with inp.open('rb')as inputs,actual.open('wb')as outputs:subprocess.run([str(binary)],stdin=inputs,stdout=outputs,check=True)
    if digest(actual)!=digest(expected):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
        raise ValueError(f'Default pose mismatch byte{first}: {a[first:first+16].hex()} vs {b[first:first+16].hex()}')
    paths=(*COPIES,'include/halo2/native_tu15_model_default_pose_kernel.hpp','src/physics/runtime/native_tu15_model_default_pose.hpp',
        'tools/native_model_default_pose_replacements.py','tests/native_tu15_model_default_pose_tests.cpp','tools/validate_native_model_default_pose_tu15.py')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(dict(passed=True,cases=receipts,
        production_bound=args.require_binding,production_copies=2,comparisons=len(rows)*4,original_xbe_sha256=PIN,physical_original=proof,
        original_function_bytes=dict(bytes=0x76,sha256=body_pin),executed_sites=[f'{pc:x}'for pc in executed],
        compiler=subprocess.run([args.compiler,'--version'],capture_output=True,text=True,check=True).stdout.splitlines()[0],
        address_undefined_sanitizers=args.sanitizers,forced_reference=json.loads(forced.stdout),scratch_owner=str(scratch),
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,rejected_mutations=rejected,
        source_sha256={path:digest(ROOT/path)for path in paths},supplemental_canonical_read_store_alias_cases=10,admission_refusals=8,
        actual_default_empty_observer_comparisons=len(rows)*2,
        normal_return_abi_verified=True,ordered_stores_verified=True,retained_memory_fault_prefix_abi_verified=True,
        retained_faults=json.loads(faults.stdout),physical_page_fault_atomicity_verified=False,
        visual_parity_verified=False,native_production_closure_verified=False,retained_large_artifacts=[]),indent=2,sort_keys=True)+'\n')
    print(f'PASS {len(rows)*4} native default-pose comparisons against{len(rows)} unchanged i386 cases');return 0

if __name__=='__main__':raise SystemExit(main())
