#!/usr/bin/env python3
"""Qualify native126190 against the unchanged original lookup and full banks/ABI."""
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
from native_resource_record_search_replacements import canonical_body,replace_resource_record_search,PREFIX
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x61000000,65536),(0x62000000,8192),(0x4f0000,8192),(0x62fff000,4096))
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
    sources=(SOURCE,'src/campaign/ai/source_ai_frame_tu15_generated.cpp');bodies=[];rejected=0
    for source_name in sources:
        source=(ROOT/source_name).read_text()
        body=re.search(r'static void run_126190\(WorldState &s\) \{.*?\n\}',source,re.S)[0];original=canonical_body(body)
        raw=source.replace(body,original,1);native=replace_resource_record_search(raw)
        if args.require_binding and native!=source:raise ValueError('Production resource search prefix missing: '+source_name)
        if replace_resource_record_search(native)!=native:raise ValueError('Native binding is not idempotent')
        for victim in (raw,native):
            target=re.search(r'static void run_126190\(WorldState &s\) \{.*?\n\}',victim,re.S)[0]
            for before,after in (('s.reg(6,4,0)','s.reg(6,2,0)'),('0x1261f7u','0x1261f8u')):
                changed=target.replace(before,after,1)
                if changed==target:raise ValueError('Resource search mutation witness absent')
                try:replace_resource_record_search(victim.replace(target,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Resource search accepted whole-body drift')
        if 'selected('in original:
            for old,new in (('case 0x4f041cu:return 0x4f041cu;','case 0x4f041cu:return 0x4f0420u;'),('switch(p)','p^=1u;switch(p)')):
                changed=raw.replace(old,new,1)
                if changed==raw:raise ValueError('Selected-map mutation witness missing')
                try:replace_resource_record_search(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Resource search accepted complete resolver drift')
        native_body=re.search(r'static void run_126190\(WorldState &s\) \{.*?\n\}',native,re.S)[0]
        literal=lambda text:re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',text)
        bodies.append((literal(original),literal(native_body)))
    original=bodies[0][0]
    if bodies[1][0]!=original:raise ValueError('Compatible resource search copies diverged after authenticated identity resolution')
    (scratch/'resource_record_search_bodies.inc').write_text('namespace primary {\n'+bodies[0][1]+'\n}\nnamespace frame {\n'+bodies[1][1]+'\n}\nnamespace reference {\n'+original+'\n}\n')
    binary=scratch/'resource-record-search';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_resource_record_search_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,HALO2_REFERENCE_RESOURCE_RECORD_SEARCH='1',HALO2_RESOURCE_RECORD_SEARCH_REFUSAL_TEST='1'))
    rng=random.Random(0x126190);rows=[];requests=[];receipts=[]
    scenarios=('fast-zero','fast-handle','empty','negative-bound','signed-min-bound','empty-bitmap','found-first','found-last',
        'nonmatching','bit31','bit32','bit63','bit64','sparse','zero-generation','ffff-generation','high-count','stride-zero',
        'negative-stride','stride-one')
    modes=('ordinary','unaligned','count-saved-bp','global-saved-di','global-saved-bp','entry-saved-di','record-saved-si',
        'stride-saved-bp','bitmap-saved-si','records-saved-bp')
    for scenario in scenarios:
        for mode in modes:
            ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
            def put(at,value,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Resource search fixture escaped declared bank: '+hex(at))
            def get(at):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:return int.from_bytes(bank[at-base:at-base+4],'little')
                raise ValueError('Resource search fixture escaped declared bank')
            stack=0x62001000+ordinal%16;entry=0x61000100;pool=0x61000200;bitmap=0x61000400;records=0x61002000;stride=12
            count={'empty':0,'negative-bound':0xffffffff,'signed-min-bound':0x80000000,'high-count':0x7fffffff}.get(scenario,97)
            wanted={'bit31':31,'bit32':32,'bit63':63,'bit64':64,'found-last':96,'sparse':67}.get(scenario,0)
            if scenario=='stride-zero':stride=0
            if scenario=='negative-stride':stride=0xfffffff4;wanted=32;records=0x61004000
            if scenario=='stride-one':stride=1;wanted=64
            gp=[rng.getrandbits(32)for _ in range(8)]
            if mode=='unaligned':entry+=1;pool+=3;bitmap+=1;records+=3
            if mode=='global-saved-di':stack=0x4f0420;gp[7]=pool
            if mode=='global-saved-bp':stack=0x4f0424;gp[5]=0xdeadbeef
            if mode=='entry-saved-di':entry=stack-16
            if mode=='count-saved-bp':pool=stack-0x40;count=3;gp[5]=3
            if mode=='record-saved-si':records=stack-12;wanted=0;gp[7]=entry;count=1
            if mode=='stride-saved-bp':pool=stack-0x2c;gp[5]=stride
            if mode=='bitmap-saved-si':pool=stack-0x54;gp[6]=bitmap
            if mode=='records-saved-bp':pool=stack-0x4c;gp[5]=records;gp[7]=bitmap
            for n in range(32):put(bitmap+n*4,0)
            for n in range(97):
                at=(records+stride*n)&0xffffffff
                if mode=='record-saved-si'and n:break
                generation=0 if scenario=='zero-generation'else 0xffff if scenario=='ffff-generation'else (0x8000+n*53)&0xffff
                put(at,generation,2);put(at+8,entry if n==wanted and scenario not in ('nonmatching','empty-bitmap')else entry+1)
            if scenario not in ('empty-bitmap','empty','negative-bound','signed-min-bound'):
                for n in (0,2,17,30,31,32,63,64,67,96) if scenario=='sparse'else (wanted,):
                    at=bitmap+(n>>5)*4;put(at,get(at)|(1<<(n&31)))
            handle=0 if scenario=='fast-zero'else 0x12340001 if scenario=='fast-handle'else 0xffffffff
            put(entry+12,handle);put(pool+0x38,0xffffffff if mode=='count-saved-bp'else count)
            put(pool+0x24,0x12345678 if mode=='stride-saved-bp'else stride)
            put(pool+0x44,records+128 if mode=='records-saved-bp'else records)
            put(pool+0x48,bitmap+128 if mode=='bitmap-saved-si'else bitmap)
            put(0x4f041c,pool+0x100 if mode=='global-saved-di'else pool)
            put(stack,0x63000000);put(stack+4,records);put(stack+8,bitmap)
            gp[3]=entry;gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10);status=((ordinal%8)<<11)|((ordinal%8)<<8)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')
            live=list(live_values[:(0,1,4)[ordinal%3]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            row=dict(id=ordinal,name=scenario+'-'+mode,entry=0x126190,gp=gp,eflags=flags,matrix=records,source=bitmap,max_steps=10000,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)])
            rows.append(row);requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks));receipts.append(dict(case=row['name'],scenario=scenario,mode=mode,count=count,live_depth=len(live)))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x126190,),regions=REGIONS)
    inp,expected,actual=(scratch/name for name in ('resource-search.in','resource-search.expected','resource-search.actual'))
    packet_starts=[]
    with inp.open('wb')as inputs,expected.open('wb')as outputs:
        for copy in range(2):
          for case,request in zip(physical['cases'],requests,strict=True):
              packet_starts.append(outputs.tell())
              if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original lookup completion/live FP contract failed')
              gp,flags,mxcsr,cw,status,live,xmm,banks=request
              inputs.write(struct.pack('<I12I',copy,*gp,flags,mxcsr,cw,status));inputs.write(struct.pack('<I',len(live)))
              for value in live:inputs.write(bytes.fromhex(value))
              for lane in xmm:inputs.write(struct.pack('<4I',*lane))
              inputs.write(b''.join(banks));outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
              return_sites=set(case['executed_sites'])&{0x1261d1,0x1261f7}
              if len(return_sites)!=1:raise ValueError('Original resource search return-site ownership is ambiguous')
              outputs.write(struct.pack('<I',return_sites.pop()))
              for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
              after={bank['address']:bank['after']for bank in case['banks']};outputs.write(b''.join(bytes.fromhex(after[base])for base,_ in REGIONS))
              outputs.write(struct.pack('<I',len(case['writes'])))
              for event in case['writes']:
                  outputs.write(struct.pack('<III',event['instruction'],event['address'],len(bytes.fromhex(event['written']))));outputs.write(bytes.fromhex(event['before']).ljust(4,b'\0'));outputs.write(bytes.fromhex(event['written']).ljust(4,b'\0'))
    environment=dict(os.environ);environment.pop('HALO2_REFERENCE_RESOURCE_RECORD_SEARCH',None)
    with inp.open('rb')as inputs,actual.open('wb')as outputs:subprocess.run([str(binary)],stdin=inputs,stdout=outputs,check=True,env=environment)
    if digest(actual)!=digest(expected):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
        packet=max(n for n,at in enumerate(packet_starts)if at<=first);at=packet_starts[packet];packet%=len(rows)
        raise ValueError(f'Native lookup mismatch case{packet} {receipts[packet]} byte{first-at}: actual ABI {struct.unpack_from("<12I",a,at)} expected ABI {struct.unpack_from("<12I",b,at)}')
    files=(*sources,'include/halo2/native_tu15_resource_record_search_kernel.hpp','include/halo2/native_tu15_bitmap_scan_kernel.hpp','src/physics/runtime/native_tu15_resource_record_search.hpp','tools/native_resource_record_search_replacements.py','tests/native_tu15_resource_record_search_tests.cpp','tools/validate_native_tu15_resource_record_search.py','tools/validate_native_tu15_prt_helpers.py')
    source_sites={int(value,16)for value in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',original)}
    executed={pc for case in physical['cases']for pc in case['executed_sites']}
    # Branch instructions execute; their negative-index/sentinel arms cannot
    # be reached after a nonnegative signed index passed the fresh signed bound.
    if source_sites-executed:raise ValueError('Resource search hardware proof lost original source-site coverage: '+str(sorted(source_sites-executed)))
    report=dict(passed=True,production_copies=2,comparisons=2*len(rows),cases=receipts,physical_original=proof,original_xbe_sha256=PIN,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,rejected_mutations=rejected,
        supplemental_canonical_read_store_alias_cases=12,admission_refusal_cases=6,normal_return_abi_verified=True,ordered_stores_verified=True,
        full_flags_compared_with_unchanged_hardware=True,supplemental_canonical_flags_compared=True,final_source_return_cursor_verified=True,
        hardware_source_body_coverage=dict(total=len(source_sites),executed=len(source_sites&executed),unexecuted=sorted(source_sites-executed)),
        address_undefined_sanitizers=args.sanitizers,
        read_fault_partial_state_qualified=False,production_dependency_closure_qualified=False,visual_parity_verified=False,
        source_sha256={name:digest(ROOT/name)for name in files},scratch_directory=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(f'PASS native resource search {2*len(rows)} unchanged-i386 full ABI/bank/store cases');return 0


if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            output=Path(sys.argv[sys.argv.index('--report')+1]);output.parent.mkdir(parents=True,exist_ok=True)
            output.write_text(json.dumps(dict(passed=False,error=repr(error),command=sys.argv,
                scratch_directory=os.environ.get('HALO2_TEST_SCRATCH'),retained_large_artifacts=[]),indent=2)+'\n')
        raise
