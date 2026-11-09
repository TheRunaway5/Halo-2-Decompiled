#!/usr/bin/env python3
"""Qualify native1E3270 against the unchanged original lookup and full banks/ABI."""
import argparse
import hashlib
import json
import os
import platform
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
from native_record_lookup_replacements import canonical_body,replace_record_lookup,PREFIX
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x61000000,16384),(0x62000000,8192),(0x4f0000,8192),(0x62fff000,4096))
SOURCE='src/physics/physics_live_support_tu15_generated.cpp'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in ('x86_64','amd64','i386','i686'):
        print('SKIP unchanged original lookup requires Linux x86 ptrace');return 77
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if digest(args.xbe)!=PIN:raise ValueError('Original TU1.5 image pin differs')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=(ROOT/SOURCE).read_text()
    body=re.search(r'static void run_1e3270\(WorldState &s\) \{.*?\n\}',source,re.S)[0];original=canonical_body(body)
    raw=source.replace(body,original,1);native=replace_record_lookup(raw)
    if args.require_binding and native!=source:raise ValueError('Production lookup prefix missing')
    if replace_record_lookup(native)!=native:raise ValueError('Native lookup binding is not idempotent')
    rejected=0
    for victim in (raw,native):
        target=re.search(r'static void run_1e3270\(WorldState &s\) \{.*?\n\}',victim,re.S)[0]
        for before,after in (('s.reg(6,4,0)','s.reg(6,2,0)'),('0x5u,4)','0x4u,4)')):
            changed=target.replace(before,after,1)
            if changed==target:raise ValueError('Lookup mutation witness absent')
            try:replace_record_lookup(victim.replace(target,changed,1))
            except ValueError:rejected+=1
            else:raise ValueError('Lookup accepted whole-body drift')
    mini=replace_record_lookup(original)
    (scratch/'record_lookup_bodies.inc').write_text('namespace primary {\n'+mini+'\n}\nnamespace reference {\n'+original+'\n}\n')
    binary=scratch/'record-lookup';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_record_lookup_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,HALO2_REFERENCE_RECORD_LOOKUP='1',HALO2_RECORD_LOOKUP_REFUSAL_TEST='1'))
    rng=random.Random(0x1e3270);rows=[];requests=[];receipts=[]
    bounds=((0,0),(1,0),(1,1),(7,3),(7,0xffffffff),(7,0xfffffffe),(0xffffffff,0xfffffffe),
        (0x80000000,0),(0x7fffffff,0x7ffffffe),(0x7fffffff,0x80000000),(7,0x40000000),
        (0xffffffff,0xffffffff),(0x80000000,0x80000000),(0x80000000,0x7fffffff))
    modes=('found','absent','zero-datum','max-index-datum','signed-datum-overflow','negative-datum-nibble','one-datum',
        'unaligned','base-saved-si','global-saved-si','count-saved-si','object-saved-si')
    for count,index in bounds:
        for mode in modes:
            ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
            def put(at,value):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:struct.pack_into('<I',bank,at-base,value&0xffffffff);return
                raise ValueError('Lookup fixture escaped declared bank: '+hex(at))
            stack=0x62001000+ordinal%16;descriptor=0x61000100+(ordinal%4 if mode=='unaligned'else 0)
            slots=0x61002000+(ordinal%4 if mode=='unaligned'else 0);target=0x61003000+(ordinal%4 if mode=='unaligned'else 0)
            gp=[rng.getrandbits(32)for _ in range(8)]
            datum={'absent':0xffffffff,'zero-datum':0xabcd0000,'max-index-datum':0x1234ffff,
                'signed-datum-overflow':0x7fffffff,'negative-datum-nibble':0x8000000f,'one-datum':1}.get(mode,0x53420003)
            if mode=='base-saved-si':descriptor=stack-0x54;gp[6]=slots
            if mode=='global-saved-si':stack=0x4f0418
            if mode=='count-saved-si':descriptor=stack-0x50;slots=0x63000000;index_for_case=0xffffffff;gp[6]=3;count_for_case=3
            else:index_for_case=index;count_for_case=count
            if mode=='object-saved-si':target=stack-4
            objects=(target-((datum&0xffff)*16)-8)&0xffffffff
            if mode=='global-saved-si':gp[6]=objects
            put(descriptor+0x4c,0xffffffff if mode=='count-saved-si'else count_for_case);put(descriptor+0x50,slots+64 if mode=='base-saved-si'else slots)
            put((slots+index_for_case*32+4)&0xffffffff,datum);put(0x4f0414,objects+0x100 if mode=='global-saved-si'else objects);put(target,0x15000000+ordinal)
            put(stack,0x63000000);put(stack+4,descriptor);put(stack+8,index_for_case)
            gp[1]=index_for_case;gp[2]=descriptor;gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10);status=((ordinal%8)<<11)|((ordinal%8)<<8)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')
            live=list(live_values[:(0,1,4)[ordinal%3]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            row=dict(id=ordinal,name=f'{count:x}-{index_for_case:x}-{mode}',entry=0x1e3270,gp=gp,eflags=flags,matrix=descriptor,source=index_for_case,max_steps=1000,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)])
            rows.append(row);requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks));receipts.append(dict(case=row['name'],mode=mode,count=count_for_case,index=index_for_case,live_depth=len(live)))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x1e3270,),regions=REGIONS)
    inp,expected,actual=(scratch/name for name in ('lookup.in','lookup.expected','lookup.actual'))
    packet_starts=[]
    with inp.open('wb')as inputs,expected.open('wb')as outputs:
        for case,request in zip(physical['cases'],requests,strict=True):
            packet_starts.append(outputs.tell())
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original lookup completion/live FP contract failed')
            gp,flags,mxcsr,cw,status,live,xmm,banks=request
            inputs.write(struct.pack('<I12I',0,*gp,flags,mxcsr,cw,status));inputs.write(struct.pack('<I',len(live)))
            for value in live:inputs.write(bytes.fromhex(value))
            for lane in xmm:inputs.write(struct.pack('<4I',*lane))
            inputs.write(b''.join(banks));outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
            for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
            after={bank['address']:bank['after']for bank in case['banks']};outputs.write(b''.join(bytes.fromhex(after[base])for base,_ in REGIONS))
            outputs.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                outputs.write(struct.pack('<III',event['instruction'],event['address'],len(bytes.fromhex(event['written']))));outputs.write(bytes.fromhex(event['before']).ljust(4,b'\0'));outputs.write(bytes.fromhex(event['written']).ljust(4,b'\0'))
    environment=dict(os.environ);environment.pop('HALO2_REFERENCE_RECORD_LOOKUP',None)
    with inp.open('rb')as inputs,actual.open('wb')as outputs:subprocess.run([str(binary)],stdin=inputs,stdout=outputs,check=True,env=environment)
    if digest(actual)!=digest(expected):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
        packet=max(n for n,at in enumerate(packet_starts)if at<=first);at=packet_starts[packet]
        raise ValueError(f'Native lookup mismatch case{packet} {receipts[packet]} byte{first-at}: actual ABI {struct.unpack_from("<12I",a,at)} expected ABI {struct.unpack_from("<12I",b,at)}')
    files=(SOURCE,'src/physics/runtime/native_tu15_record_lookup.hpp','include/halo2/native_tu15_record_lookup_kernel.hpp','include/halo2/native_tu15_node_resource_kernel.hpp','tools/native_record_lookup_replacements.py','tests/native_tu15_record_lookup_tests.cpp','tools/validate_native_tu15_record_lookup.py','tools/validate_native_tu15_prt_helpers.py')
    source_sites={int(value,16)for value in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',original)}
    executed={pc for case in physical['cases']for pc in case['executed_sites']}
    if source_sites-executed:raise ValueError('Lookup hardware proof lost original source-site coverage')
    report=dict(passed=True,production_copies=1,comparisons=len(rows),cases=receipts,physical_original=proof,original_xbe_sha256=PIN,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,rejected_mutations=rejected,
        supplemental_canonical_read_store_alias_cases=8,admission_refusal_cases=6,normal_return_abi_verified=True,ordered_stores_verified=True,
        full_flags_compared_with_unchanged_hardware=True,supplemental_canonical_flags_compared=False,
        canonical_undefined_af_qualified=False,undefined_af_transport='Original CMP ECX,-1 / AND ECX,FFFF / SHL ECX,4 in one native asm block, captured by LAHF/SETO.',
        hardware_source_body_coverage=dict(total=len(source_sites),executed=len(source_sites&executed),unexecuted=sorted(source_sites-executed)),
        address_undefined_sanitizers=args.sanitizers,
        read_fault_partial_state_qualified=False,production_dependency_closure_qualified=False,visual_parity_verified=False,
        source_sha256={name:digest(ROOT/name)for name in files},scratch_directory=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(f'PASS native lookup {len(rows)} unchanged-i386 full ABI/bank/store cases');return 0


if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            output=Path(sys.argv[sys.argv.index('--report')+1]);output.parent.mkdir(parents=True,exist_ok=True)
            output.write_text(json.dumps(dict(passed=False,error=repr(error),command=sys.argv,
                scratch_directory=os.environ.get('HALO2_TEST_SCRATCH'),retained_large_artifacts=[]),indent=2)+'\n')
        raise
