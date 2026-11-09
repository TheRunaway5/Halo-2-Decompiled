#!/usr/bin/env python3
"""Compare native tick conversion and wide arithmetic with unchanged i386."""
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
from validate_native_async_order_tu15 import compare

LIVE=('0000000000000080ff3f','01000000000000800040','35c26821a2da0fc90040','0100000000000080ffbf',
      '0300000000000080f03f','07000000000000c00f40','01000000000000800140','09000000000000a00240')
SPECIAL=('01000000000000c0ff7f','0100000000000080ff7f','0000000000000080ff7f','0000000000000080ffff',
         '01000000000000000000','00000000000000000080','00000000000000000000','01000000000000800000')


def cohort(root,scratch,binary,xbe,kind,sanitizers,require_binding):
    from validate_native_tu15_prt_helpers import physical_original
    scratch.mkdir();rng=random.Random(0x22a5c0+(kind=='integer'));rows=[];receipts=[]
    inputs=scratch/'inputs';expected=scratch/'expected'
    numbers=(0,1,-1,733333332,733333333,733333334,-733333332,-733333333,-733333334,
        2**31-1,2**32,2**53+1,2**63-1,-2**63,0x100000001,0x7fff0000ffffffff)
    with inputs.open('wb') as out:
        for ordinal in range(768 if kind=='conversion' else 640):
            entry=0x22a5c0 if kind=='conversion' else (0x32ae70 if ordinal%2==0 else 0x32acd0)
            a=numbers[(ordinal//12)%len(numbers)] if kind=='conversion' and ordinal<384 else numbers[ordinal%len(numbers)] if ordinal<256 else rng.getrandbits(64)
            b=numbers[(ordinal//2)%len(numbers)] if ordinal<256 else rng.getrandbits(64)
            a&=0xffffffffffffffff;b&=0xffffffffffffffff
            if not b:b=1
            # Ensure signed division receives all value families; odd/even
            # selection otherwise correlates helper identity with sign cases.
            if kind=='integer' and ordinal%32>=16:a,b=b,a or 1
            if kind=='integer' and ordinal<96:
                entry=0x32ae70;divisor=(0x100000001,0x180000001,0x100000003,0x1ffffffff)[(ordinal//4)%4]
                quotient=(1,2,17)[(ordinal//16)%3];a=divisor*quotient-1;b=divisor
                if ordinal%4&1:a=(-a)&0xffffffffffffffff
                if ordinal%4&2:b=(-b)&0xffffffffffffffff
            if not b:b=1
            stack=0x62001000+ordinal%16;bank=bytearray(rng.randbytes(8192))
            args=(0x63000000,a&0xffffffff,a>>32) if kind=='conversion' else (0x63000000,a&0xffffffff,a>>32,b&0xffffffff,b>>32)
            for i,value in enumerate(args):struct.pack_into('<I',bank,stack-0x62000000+4*i,value)
            gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=stack
            flags=0x202|(rng.getrandbits(12)&0xcd5);mxcsr=0x1f80|((ordinal%4)<<13)
            cw=(0x7f,0x27f,0x37f)[ordinal%3]|(((ordinal//3)%4)<<10)
            depths=(0,1,4,6) if kind=='conversion' else (0,1,4,6,8);depth=depths[(ordinal//16)%len(depths)]
            status=((ordinal//4)%8)<<11|((ordinal%4)*0x200)|((ordinal//32)%2)*0x4000|((ordinal//7)%64)
            live_values=(LIVE,SPECIAL)[(ordinal//64)%2]
            live=[live_values[(i+ordinal//128)%len(live_values)] for i in range(depth)]
            xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)]
            out.write(struct.pack('<13I',entry,*gp,flags,mxcsr,cw,status))
            for lane in xmm:out.write(struct.pack('<4I',*lane))
            out.write(struct.pack('<I',depth));out.write(b''.join(bytes.fromhex(v) for v in live));out.write(bank)
            rows.append(dict(id=ordinal,name=f'{kind}-{ordinal}-{entry:x}',entry=entry,gp=gp,eflags=flags,
                matrix=args[1],source=args[2],max_steps=1000,xmm=xmm,before_fp=[cw,status,mxcsr],live_x87=live,
                banks=[dict(address=0x62000000,before=bank.hex())]))
            receipts.append(dict(entry=f'{entry:x}',a=f'{a:016x}',b=f'{b:016x}',depth=depth))
    physical,proof=physical_original(xbe,rows,scratch,entries=(0x22a5c0,0x32ae70,0x32acd0) if kind=='conversion' else (0x32ae70,0x32acd0),
        regions=((0x62000000,8192),),return_x87=kind=='conversion',
        trace_calls=[dict(entry=pc,arguments=4 if pc in (0x32ae70,0x32acd0) else 0,gp=True,xmm=True,x87_delta=delta)
            for pc,delta in ((0x32ae70,0),(0x32acd0,0),(0x22a601,1),(0x22a615,2),(0x22a619,1),(0x22a61e,1))] if kind=='conversion' else None,
        trace_extended=kind=='conversion')
    def emit_abi(out,gp,flags,fp,xmm):
        out.write(struct.pack('<12I',*gp,flags,fp[2],fp[1],fp[0]))
        for lane in xmm:out.write(struct.pack('<4I',*lane))
    with expected.open('wb') as out:
        for receipt,original,row in zip(receipts,rows,physical['cases'],strict=True):
            if not row['completed']:raise ValueError('Original tick arithmetic did not return')
            receipt['offset']=out.tell();out.write(struct.pack('<I',0));emit_abi(out,row['boundary_gp'],row['boundary_eflags'],row['after_fp'],row['boundary_xmm'])
            live=original['live_x87'];values=([row['returned_extended']]+live) if kind=='conversion' else live
            out.write(struct.pack('<I',len(values)));out.write(b''.join(bytes.fromhex(v) for v in values))
            out.write(bytes.fromhex(row['banks'][0]['after']));out.write(struct.pack('<I',len(row['writes'])))
            for event in row['writes']:
                before,written=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                out.write(struct.pack('<III',event['instruction'],event['address'],len(written))+before.ljust(4,b'\0')+written.ljust(4,b'\0'))
            out.write(struct.pack('<I',len(row['calls'])))
            for call in row['calls']:
                out.write(struct.pack('<I',call['entry']));emit_abi(out,call['gp'],call['eflags'],call['fp'],call['xmm'])
                out.write(struct.pack('<I',len(call['live_x87'])));out.write(b''.join(bytes.fromhex(value) for value in call['live_x87']))
            receipt['bytes']=out.tell()-receipt['offset']
    env=dict(os.environ)
    for key in ('HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_RAW_ALLOCATOR','HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
    if sanitizers:env['ASAN_OPTIONS']='detect_leaks=0'
    with inputs.open('rb') as inp:result=subprocess.run([str(binary)],stdin=inp,capture_output=True,env=env,check=True)
    compare(result.stdout,expected.read_bytes(),receipts,'Tick '+kind)
    owner_dispatch=None
    if require_binding:
        owner_expected=bytearray();owner_receipts=[];raw=expected.read_bytes()
        for receipt,row in zip(receipts,physical['cases'],strict=True):
            packet=raw[receipt['offset']:receipt['offset']+receipt['bytes']];trailer=4+sum(184+10*len(call['live_x87']) for call in row['calls'])
            projected=packet[:-trailer]+struct.pack('<I',0)
            owner_receipts.append(dict(receipt,offset=len(owner_expected),bytes=len(projected)));owner_expected+=projected
        with inputs.open('rb') as inp:owner_result=subprocess.run([str(binary),'--owner-dispatch'],stdin=inp,capture_output=True,env=env,check=True)
        compare(owner_result.stdout,owner_expected,owner_receipts,'Tick actual owner dispatch '+kind)
        raw_reference=dict(env,HALO2_REFERENCE_RAW_ALLOCATOR='1')
        with inputs.open('rb') as inp:independent=subprocess.run([str(binary),'--owner-dispatch'],stdin=inp,capture_output=True,env=raw_reference,check=True)
        compare(independent.stdout,owner_expected,owner_receipts,'Tick independent raw reference control '+kind)
        owner_dispatch=dict(cases=len(rows),compared_bytes=len(owner_result.stdout),comparison_sha256=hashlib.sha256(owner_result.stdout).hexdigest(),activity_checked=True,
            raw_reference_control_independent=True,no_external_callee_outcomes_requested=True,child_entry_snapshots='Separately compared by the identical native kernel with its passive Before observer')
    with inputs.open('rb') as inp:read_result=subprocess.run([str(binary),'--reads'],stdin=inp,capture_output=True,env=env,check=True)
    read_sites={event['instruction'] for row in physical['cases'] for event in row['reads']};cursor=0;explicit_reads=0
    for receipt,row in zip(receipts,physical['cases'],strict=True):
        count=struct.unpack_from('<I',read_result.stdout,cursor)[0];cursor+=4
        events=[struct.unpack_from('<III',read_result.stdout,cursor+12*i) for i in range(count)];cursor+=12*count
        actual=[event for event in events if event[0] in read_sites]
        wanted=[(event['instruction'],event['address'],event['bytes']) for event in row['reads']]
        if actual!=wanted:raise ValueError('Tick explicit operand-read stream differs: '+str(receipt))
        explicit_reads+=len(actual)
    if cursor!=len(read_result.stdout):raise ValueError('Tick read stream has trailing/missing cases')
    return dict(cases=len(rows),physical_original=proof,compared_bytes=len(result.stdout),comparison_sha256=hashlib.sha256(result.stdout).hexdigest(),
        complete_callee_entry_abi_packets=sum(call['entry'] in (0x32ae70,0x32acd0) for row in physical['cases'] for call in row['calls']),
        intermediate_fp_phase_packets=sum(call['entry'] not in (0x32ae70,0x32acd0) for row in physical['cases'] for call in row['calls']),
        physical_explicit_operand_reads=explicit_reads,physical_implicit_stack_reads_claimed=False,
        actual_owner_dispatch=owner_dispatch,
        executed_sites=[f'{pc:x}' for pc in sorted({pc for row in physical['cases'] for pc in row['executed_sites']})])


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true')
    parser.add_argument('--compiler',default='c++');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    from validate_native_tu15_prt_helpers import PIN
    if not args.xbe.is_file():print('SKIP missing original TU1.5 image');return 77
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest()!=PIN:raise ValueError('Wrong English TU1.5 image')
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH']);binary=scratch/'native-tick-conversion'
    command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(root/'include'),'-I'+str(root/'src'),
        str(root/'tests/native_tu15_tick_conversion_tests.cpp'),'-o',str(binary)]
    binding=None
    if args.require_binding:
        from specialize_native_operands import specialize,TICK_CONVERSION_ACTUAL_OWNER_IMAGES
        source=root/'src/physics/physics_live_support_tu15_generated.cpp';text=source.read_text();specialized,receipt=specialize(text)
        if set(receipt.get('native_tick_conversion_actual_owner_entries',[]))!={'22a5c0','32ae70','32acd0'}:raise ValueError('Tick conversion actual owner binding missing')
        for entry,(length,pin) in TICK_CONVERSION_ACTUAL_OWNER_IMAGES.items():
            sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
            if hashlib.sha256(Xbe(args.xbe).read(entry,length)).hexdigest()!=pin:raise ValueError('Tick actual whole original body identity changed')
        binding=dict(source_sha256=hashlib.sha256(text.encode()).hexdigest(),specialized_sha256=hashlib.sha256(specialized.encode()).hexdigest(),
            actual_entries=receipt['native_tick_conversion_actual_owner_entries'])
        dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',text)[0]
        for changed in (dispatch.replace('{','{if(target==0)return;',1),dispatch.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1)):
            if changed==dispatch:raise ValueError('Tick dispatch drift witness missing')
            try:specialize(text.replace(dispatch,changed))
            except ValueError as error:
                if str(error)!='Complete13FE50 dispatcher drift':raise
            else:raise ValueError('Changed complete dispatcher retained tick binding')
        binding['complete_dispatch_drift_rejections']=2
        command+=['-DHALO2_TICK_WITH_OWNER_DISPATCH',*[str(root/'src/physics'/name) for name in (
            'native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp',
            'native_tu15_physical_epoch.cpp')]]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    subprocess.run(command,check=True)
    sys.path.insert(0,str(root/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe)
    integer=cohort(root,scratch/'integer',binary,image,'integer',args.sanitizers,args.require_binding)
    conversion=cohort(root,scratch/'conversion',binary,image,'conversion',args.sanitizers,args.require_binding)
    env=dict(os.environ)
    for key in ('HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_RAW_ALLOCATOR','HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
    if args.sanitizers:env['ASAN_OPTIONS']='detect_leaks=0'
    guard=subprocess.run([str(binary),'--guard-tests'],env=env,text=True,capture_output=True,check=True)
    forced_reference=subprocess.run([str(binary),'--reference-guard'],env=dict(env,HALO2_REFERENCE_TICK_CONVERSION='1'),text=True,capture_output=True,check=True)
    report=dict(passed=True,integer=integer,conversion=conversion,address_undefined_sanitizers=args.sanitizers,
        compiler=subprocess.run([args.compiler,'--version'],capture_output=True,text=True,check=True).stdout.splitlines()[0],
        caller_x87_profiles=['finite full-precision','qNaN','sNaN','positive/negative infinity','denormal','negative/positive zero'],
        original_function_bytes={f'{start:x}':dict(bytes=end-start,sha256=hashlib.sha256(image.read(start,end-start)).hexdigest())
            for start,end in ((0x22a5c0,0x22a624),(0x32ae70,0x32af1a),(0x32acd0,0x32ad04))},
        production_bound=args.require_binding,binding=binding,admission=json.loads(guard.stdout),forced_reference=json.loads(forced_reference.stdout),exceptional_partial_abi_qualified=False,complete_clock_owner_qualified=False,retained_large_artifacts=[],
        source_sha256={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (
            Path(__file__).resolve(),root/'src/platform/native_tu15_tick_conversion.hpp',root/'tests/native_tu15_tick_conversion_tests.cpp',
            root/'tools/validate_native_tu15_prt_helpers.py',
            root/'src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp',root/'tools/specialize_native_operands.py',
            root/'cmake/NativeTu15TickConversion.cmake',root/'CMakeLists.txt')})
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,integer_cases=integer['cases'],conversion_cases=conversion['cases'])));return 0


if __name__=='__main__':sys.exit(main())
