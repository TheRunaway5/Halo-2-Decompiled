#!/usr/bin/env python3
"""Qualify native33B590 clock reads and genuine external-input service boundaries."""
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
from native_millisecond_clock_replacements import canonical_body,replace_millisecond_clock,PREFIX
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x41b000,4096),(0x61000000,8192),(0x62000000,8192),(0x80000000,4096))
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
    sources=(SOURCE,'src/input/source_input_startup_tu15_generated.cpp','src/gameplay/units/source_combat_action_tu15_generated.cpp');bodies=[];rejected=0
    for source_name in sources:
        source=(ROOT/source_name).read_text()
        body=re.search(r'static void run_33b590\(WorldState &s\) \{.*?\n\}',source,re.S)[0];original=canonical_body(body)
        raw=source.replace(body,original,1);native=replace_millisecond_clock(raw)
        if args.require_binding and native!=source:raise ValueError('Production millisecond clock prefix missing: '+source_name)
        if replace_millisecond_clock(native)!=native:raise ValueError('Native binding is not idempotent')
        for victim in (raw,native):
            target=re.search(r'static void run_33b590\(WorldState &s\) \{.*?\n\}',victim,re.S)[0]
            for before,after in ((',4),4,0);',',2),4,0);'),('0x33b597u','0x33b598u')):
                changed=target.replace(before,after,1)
                if changed==target:raise ValueError('Resource search mutation witness absent')
                try:replace_millisecond_clock(victim.replace(target,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Resource search accepted whole-body drift')
        if 'selected('in original:
            for old,new in (('case 0x41b774u:return 0x41b774u;','case 0x41b774u:return 0x41b778u;'),('switch(p)','p^=1u;switch(p)')):
                changed=raw.replace(old,new,1)
                if changed==raw:raise ValueError('Selected-map mutation witness missing')
                try:replace_millisecond_clock(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Resource search accepted complete resolver drift')
        native_body=re.search(r'static void run_33b590\(WorldState &s\) \{.*?\n\}',native,re.S)[0]
        literal=lambda text:re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',text)
        bodies.append((literal(original),literal(native_body)))
    original=bodies[0][0]
    if any(body[0]!=original for body in bodies):raise ValueError('Compatible millisecond clock copies diverged after authenticated identity resolution')
    (scratch/'millisecond_clock_bodies.inc').write_text(''.join('namespace '+name+' {\n'+body[1]+'\n}\n'for name,body in zip(('primary','input','combat'),bodies,strict=True))+'namespace reference {\n'+original+'\n}\n')
    binary=scratch/'millisecond-clock';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-pthread',
        '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_millisecond_clock_tests.cpp'),str(ROOT/'src/platform/native_tu15_rng_startup.cpp'),str(ROOT/'src/runtime/source_execution_inputs.cpp'),str(ROOT/'src/runtime/source_runtime_memory.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:raise ValueError('Clock fixture compile failed: '+result.stderr[-5000:])
    subprocess.run([str(binary)],check=True,env=dict(os.environ,HALO2_REFERENCE_MILLISECOND_CLOCK='1',HALO2_MILLISECOND_CLOCK_REFUSAL_TEST='1'))
    result=subprocess.run([str(binary),'--live'],capture_output=True,text=True)
    if result.returncode:raise ValueError('Real backend/input-journal clock integration failed: '+result.stdout+result.stderr)
    live_result=json.loads(result.stdout)
    rng=random.Random(0x33b590);rows=[];requests=[];receipts=[]
    modes=('genuine-cell','ordinary','unaligned-cell','pointer-arg1','pointer-arg2','value-arg1','value-arg2','value-return','self-pointer')
    for value in (0,1,0x12345678,0x7fffffff,0x80000000,0xffffffff):
        for mode in modes:
            ordinal=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
            def put(at,value):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:struct.pack_into('<I',bank,at-base,value&0xffffffff);return
                raise ValueError('Clock fixture escaped declared bank: '+hex(at))
            stack=0x62001000+ordinal%16;cell=0x8000009c if mode=='genuine-cell'else 0x61000100
            matrix=0x10203040;source=0x50607080;counter=value
            if mode=='unaligned-cell':cell+=ordinal%3+1
            if mode=='pointer-arg1':stack=0x41b770;matrix=cell
            if mode=='pointer-arg2':stack=0x41b76c;source=cell
            if mode=='value-arg1':cell=stack+4;matrix=counter
            if mode=='value-arg2':cell=stack+8;source=counter
            if mode=='value-return':cell=stack;counter=0x63000000
            if mode=='self-pointer':cell=0x41b774;counter=cell
            put(cell,counter);put(0x41b774,cell);put(stack,0x63000000);put(stack+4,matrix);put(stack+8,source)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)|(0x8040 if ordinal&4 else 0)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10);status=((ordinal%8)<<11)|((ordinal%8)<<8)|((ordinal>>4)&63)
            live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')
            live_x87=list(live_values[:(0,1,4)[ordinal%3]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            row=dict(id=ordinal,name=f'{value:x}-{mode}',entry=0x33b590,gp=gp,eflags=flags,matrix=matrix,source=source,max_steps=100,
                xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live_x87,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)])
            rows.append(row);requests.append((gp,flags,mxcsr,cw,status,live_x87,xmm,banks));receipts.append(dict(case=row['name'],mode=mode,cell=cell,counter=counter,live_depth=len(live_x87)))
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    physical,proof=physical_original(Xbe(args.xbe),rows,scratch,entries=(0x33b590,),regions=REGIONS)
    inp,expected,actual=(scratch/name for name in ('millisecond-clock.in','millisecond-clock.expected','millisecond-clock.actual'))
    packet_starts=[]
    with inp.open('wb')as inputs,expected.open('wb')as outputs:
        for copy in range(3):
          for case,request in zip(physical['cases'],requests,strict=True):
              packet_starts.append(outputs.tell())
              if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original lookup completion/live FP contract failed')
              gp,flags,mxcsr,cw,status,live,xmm,banks=request
              inputs.write(struct.pack('<I12I',copy,*gp,flags,mxcsr,cw,status));inputs.write(struct.pack('<I',len(live)))
              for value in live:inputs.write(bytes.fromhex(value))
              for lane in xmm:inputs.write(struct.pack('<4I',*lane))
              inputs.write(b''.join(banks));outputs.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
              return_sites=set(case['executed_sites'])&{0x33b597}
              if len(return_sites)!=1:raise ValueError('Original millisecond clock return-site ownership is ambiguous')
              outputs.write(struct.pack('<I',return_sites.pop()))
              for lane in case['boundary_xmm']:outputs.write(struct.pack('<4I',*lane))
              after={bank['address']:bank['after']for bank in case['banks']};outputs.write(b''.join(bytes.fromhex(after[base])for base,_ in REGIONS))
              outputs.write(struct.pack('<I',len(case['writes'])))
              for event in case['writes']:
                  outputs.write(struct.pack('<III',event['instruction'],event['address'],len(bytes.fromhex(event['written']))));outputs.write(bytes.fromhex(event['before']).ljust(4,b'\0'));outputs.write(bytes.fromhex(event['written']).ljust(4,b'\0'))
    environment=dict(os.environ);environment.pop('HALO2_REFERENCE_MILLISECOND_CLOCK',None)
    with inp.open('rb')as inputs,actual.open('wb')as outputs:subprocess.run([str(binary)],stdin=inputs,stdout=outputs,check=True,env=environment)
    if digest(actual)!=digest(expected):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
        packet=max(n for n,at in enumerate(packet_starts)if at<=first);at=packet_starts[packet];packet%=len(rows)
        raise ValueError(f'Native lookup mismatch case{packet} {receipts[packet]} byte{first-at}: actual ABI {struct.unpack_from("<12I",a,at)} expected ABI {struct.unpack_from("<12I",b,at)}')
    files=(*sources,'include/halo2/native_tu15_millisecond_clock_kernel.hpp','src/platform/native_tu15_millisecond_clock.hpp','src/platform/native_tu15_rng_startup.cpp','include/halo2/native_tu15_rng_startup.hpp','src/runtime/source_execution_inputs.cpp','include/halo2/source_execution_inputs.hpp','src/runtime/source_runtime_memory.cpp','src/runtime/source_runtime_write_protection.inc','tools/native_millisecond_clock_replacements.py','tests/native_tu15_millisecond_clock_tests.cpp','tools/validate_native_tu15_millisecond_clock.py','tools/validate_native_tu15_prt_helpers.py')
    source_sites={int(value,16)for value in re.findall(r's\.services\.instruction\((0x[0-9a-f]+)u\)',original)}
    executed={pc for case in physical['cases']for pc in case['executed_sites']}
    # Branch instructions execute; their negative-index/sentinel arms cannot
    # be reached after a nonnegative signed index passed the fresh signed bound.
    if source_sites-executed:raise ValueError('Resource search hardware proof lost original source-site coverage: '+str(sorted(source_sites-executed)))
    report=dict(passed=True,production_copies=3,comparisons=3*len(rows),cases=receipts,physical_original=proof,original_xbe_sha256=PIN,
        compared_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_service_callbacks_per_static_case=1,native_service_observation_sites_per_call=3,live_clock=live_result,rejected_mutations=rejected,
        static_callback_scope='Read-only observation of595; static-bank hardware cases supply no clock producer or time outcome.',native_compile_command=command,
        supplemental_canonical_read_store_alias_cases=6,admission_refusal_cases=2,normal_return_abi_verified=True,ordered_stores_verified=True,
        full_flags_compared_with_unchanged_hardware=True,supplemental_canonical_flags_compared=True,final_source_return_cursor_verified=True,
        hardware_source_body_coverage=dict(total=len(source_sites),executed=len(source_sites&executed),unexecuted=sorted(source_sites-executed)),
        address_undefined_sanitizers=args.sanitizers,
        read_fault_partial_state_qualified=False,host_time_generated_in_kernel=False,actual_backend="native_tu15_rng_clock().monotonic_milliseconds; genuine SourceExecutionInputs record/replay",production_dependency_closure_qualified=False,visual_parity_verified=False,
        source_sha256={name:digest(ROOT/name)for name in files},scratch_directory=str(scratch),retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(f'PASS native millisecond clock {3*len(rows)} unchanged-i386 full ABI/bank/store cases');return 0


if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            output=Path(sys.argv[sys.argv.index('--report')+1]);output.parent.mkdir(parents=True,exist_ok=True)
            output.write_text(json.dumps(dict(passed=False,error=repr(error),command=sys.argv,
                scratch_directory=os.environ.get('HALO2_TEST_SCRATCH'),retained_large_artifacts=[]),indent=2)+'\n')
        raise
