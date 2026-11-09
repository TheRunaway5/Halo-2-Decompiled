#!/usr/bin/env python3
"""Qualify the private BF0C0 hierarchy stage against pinned canonical DATA/ABI.

This does not bind the parent or qualify the unported physics/animation owners.
Compiler products stay under the enclosing run_test.py scratch owner.
"""
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
from native_object_node_rebuild_replacements import canonical_body as object_node_body

ROOT=Path(__file__).resolve().parents[1]
SOURCE='src/physics/physics_live_support_tu15_generated.cpp'
PINS={'bf0c0':'8bf592452daafe279643d1d0a060a3cc4daaed26add47339ffc0aa6ac702b950',
      '144cf0':'e680e131154417f038174f1534479663acbac718ce59da3e04e151a632000b88',
      '145560':'759d452a7d5ff296864ccd8e823526bdcb3e7fe3ee57ae7222624201619ddece'}

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def reference(scratch):
    source=(ROOT/SOURCE).read_text()
    bodies={match[1]:body for match,body in function_bodies(source)if match[1]in PINS}
    bodies['bf0c0']=object_node_body('bf0c0',bodies['bf0c0'])
    for name,pin in PINS.items():
        if hashlib.sha256(bodies[name].encode()).hexdigest()!=pin:raise ValueError('Complete original '+name+' body drift')
    original=bodies['bf0c0']
    stage=original[original.index('Lbf53c:'):original.index('Lbf5c3:')]
    stage+=original[original.index('Lbf625:'):original.index('throw std::runtime_error')]
    stage='static void run_stage(WorldState &s) {\n'+stage+'}\n'
    helpers=source[source.index('__m128 native_load_half('):source.index('long double native_fpatan(')]
    helper_pin=hashlib.sha256(helpers.encode()).hexdigest()
    if helper_pin!='1e2353f8b322f6cb4b9509626c6490add0a6b1b36ce3f26116eeb8a43819dc4c':raise ValueError('Canonical matrix half-access helper drift')
    # Exact source-owned half-load/store helpers are retained; no native prefix
    # or specialized body participates in the canonical reference owner.
    sign='std::uint32_t sign_extend(std::uint32_t v,unsigned n){return n==1?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(v)))):std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(v))));}\n'
    (scratch/'object_node_hierarchy_reference.inc').write_text(helpers+sign+bodies['144cf0']+'\n'+bodies['145560']+'\n'+stage)
    return dict(whole_body_sha256=PINS,half_access_helpers_sha256=helper_pin,stage_sha256=hashlib.sha256(stage.encode()).hexdigest(),entry='bf53c',normal_return_sites=['bf5c0','bf62f'])

REGIONS=((0x45f000,4096),(0x46a000,4096),(0x56e000,4096),(0x61000000,65536),(0x62000000,8192))
HALF_STORES={0x144d55:'0f134104',0x144d59:'0f17410c',0x144d5d:'0f134114',0x144d61:'0f17411c',
    0x144dd1:'0f174108',0x144dda:'0f134118',0x144e3f:'0f134128',0x1455d2:'0f17402c',
    0x145605:'0f174020',0x145638:'0f174014',0x145666:'0f174008'}

def hardware(args,scratch,binary):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    if digest(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    image=Xbe(args.xbe);rng=random.Random(0xbf0c0);rows=[];requests=[]
    original=next(body for match,body in function_bodies((ROOT/SOURCE).read_text())if match[1]=='bf0c0')
    stage=original[original.index('Lbf53c:'):original.index('Lbf5c3:')]+original[original.index('Lbf625:'):]
    expected_sites={int(pc,16)for pc in re.findall(r'services.instruction\((0x[0-9a-f]+)u\)',stage)}
    for count in (0,1,2,7,33,0xffffffff,0x80000000):
        for mask in (0,0xffffffff,0xaaaaaaaa):
            n=len(rows);banks=[bytearray(image.read(base,size))if base<0x61000000 else bytearray(rng.randbytes(size))for base,size in REGIONS]
            def put(address,value,width=4):
                for (base,size),bank in zip(REGIONS,banks):
                    if base<=address and address+width<=base+size:
                        bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Hierarchy input escaped declared backing')
            stack=0x62001000+n%16;model=0x61000100;records=0x61001000;locals=0x61004000+n%4;matrices=0x61006000+n%4
            put(model+0x48,count);put(model+0x4c,records);put(stack+0x14,model);put(stack+0x18,locals);put(stack+0x1c,matrices)
            for k in range(8):put(stack+0x70+k*4,mask)
            # The unchanged physical observer declares these three words at
            # its initial entry. Here they are saved-register words, not a
            # replacement function result or an invented child continuation.
            put(stack,0x63000000);put(stack+4,0x12345678);put(stack+8,0x23456789);put(stack+12,0x3456789a)
            put(stack+0xf8,0x63000000);put(stack+0xfc,0x456789ab)
            for node in range(64):
                put(records+node*96+4,node-1 if node else 0xffff,2)
                q=((0,0,0,0x3f800000),(0,0,0,0),(0x3e800000,0xbe000000,0x3f000000,0x3f400000))[(n//3)%3]
                for k,value in enumerate(q):put(locals+node*32+k*4,value)
                for k in range(3):put(locals+node*32+16+k*4,struct.unpack('<I',struct.pack('<f',(node-k)/8))[0])
                put(locals+node*32+28,0x3f800000)
                for k in range(13):put(matrices+node*52+k*4,0x3f800000 if k in (0,1,5,9)else 0)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack;gp[5]=model
            flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10);status=0x4100|((n%8)<<11)
            mxcsr=0x1f80|((n%4)<<13)|(0x8040 if n&4 else 0)
            live_values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
            live=list(live_values[:(0,1,4)[n%3]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            rows.append(dict(id=n,name=f'{count}-{mask:x}-{n}',entry=0xbf53c,gp=gp,eflags=flags,matrix=0x12345678,source=0x23456789,
                before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=100000,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
            requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks))
    # Capstone5 marks MOVLPS/MOVHPS memory destinations as reads. Author the
    # real write operand only for these complete, exact unchanged opcodes.
    # Keep every architectural store, including stores whose bytes stay equal.
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata;authored=[]
    def metadata_with_real_half_stores(owner_image):
        metadata=original_metadata(owner_image)
        for site in metadata['sites']:
            pc=site['instruction']
            if pc not in HALF_STORES:continue
            if site['bytes']!=HALF_STORES[pc]or owner_image.read(pc,len(bytes.fromhex(site['bytes']))).hex()!=HALF_STORES[pc]:raise ValueError('Original half-store opcode drift')
            if len(site['operands'])!=1 or site['operands'][0]['bytes']!=8:raise ValueError('Original half-store addressing operand drift')
            operand=site['operands'][0]
            authored.append(dict(instruction=f'{pc:x}',bytes=site['bytes'],original_metadata=dict(operand),read=False,write=True))
            operand.update(read=False,write=True)
        if len(authored)!=len(HALF_STORES):raise ValueError('Original half-store owner coverage drift')
        return metadata
    common.metadata=metadata_with_real_half_stores
    try:
        observed,proof=physical_original(image,rows,scratch,entries=(0xbf0c0,0x144cf0,0x145560),regions=REGIONS,
            trace_calls=[dict(entry=e,arguments=2 if e==0x144cf0 else 1,gp=True,xmm=True)for e in (0x144cf0,0x145560)],trace_extended=True)
    finally:common.metadata=original_metadata
    executed={pc for case in observed['cases']for pc in case['executed_sites']}
    if not expected_sites<=executed:raise ValueError('Original hierarchy stage site coverage incomplete')
    inputs,expected,actual=(scratch/name for name in ('hierarchy.in','hierarchy.expected','hierarchy.actual'))
    def emit_abi(out,gp,flags,fp,xmm):
        out.write(struct.pack('<12I',*gp,flags,fp[2],fp[1],fp[0]))
        for lane in xmm:out.write(struct.pack('<4I',*lane))
    entries=0;segments=[]
    with inputs.open('wb')as inp,expected.open('wb')as out:
        for case,request in zip(observed['cases'],requests,strict=True):
            name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original hierarchy failed to return with its live FP bank')
            gp,flags,mxcsr,cw,status,live,xmm,banks=request
            inp.write(struct.pack('<I12I',2,*gp,flags,mxcsr,cw,status));inp.write(struct.pack('<I',len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for lane in xmm:inp.write(struct.pack('<4I',*lane))
            inp.write(b''.join(banks));segments.append((out.tell(),name,'return_abi'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'])
            out.write(b''.join(bytes.fromhex(bank['after'])for bank in case['banks']));out.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);out.write(struct.pack('<III',event['instruction'],event['address'],len(raw)))
                out.write(bytes.fromhex(event['before']));out.write(raw)
            returns=set(case['executed_sites'])&{0xbf5c0,0xbf62f}
            if len(returns)!=1:raise ValueError('Hierarchy executed an unexpected parent return path')
            last=returns.pop()
            out.write(struct.pack('<5I',last,len(case['calls']),0,0,len(case['calls'])))
            for call in case['calls']:
                segments.append((out.tell(),name,f"callee_{call['entry']:x}"))
                out.write(struct.pack('<I',call['entry']));emit_abi(out,call['gp'],call['eflags'],call['fp'],call['xmm'])
                out.write(struct.pack('<I',len(live)))
                for value in call['live_x87']:out.write(bytes.fromhex(value))
                entries+=1
    with inputs.open('rb')as inp,actual.open('wb')as out:
        result=subprocess.run([str(binary)],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if result.returncode:raise RuntimeError('Hardware hierarchy native failed: '+result.stderr.decode()[-6000:])
    if digest(expected)!=digest(actual):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((i for i,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
        segment=next(x for x in reversed(segments)if x[0]<=first)
        diagnostic=''
        if segment[2]=='return_abi'and first-segment[0]==176+sum(size for _,size in REGIONS):
            def events(blob):
                at=first;count=struct.unpack_from('<I',blob,at)[0];at+=4;result=[]
                for _ in range(count):
                    pc,address,width=struct.unpack_from('<III',blob,at);at+=12+width*2;result.append((f'{pc:x}',f'{address:x}',width))
                return result
            diagnostic=' native stores='+str(events(a))+' original stores='+str(events(b))
        raise ValueError(f'Hierarchy hardware mismatch byte {first}, segment {segment}, relative {first-segment[0]}: {a[first:first+32].hex()} vs {b[first:first+32].hex()}'+diagnostic)
    return dict(passed=True,cases=len(rows),exact_real_matrix_callee_entry_packets=entries,original_xbe_sha256=PIN,physical_original=proof,
        exact_opcode_authored_half_store_operands=authored,
        hierarchy_sites_executed=len(expected_sites),original_stage_sites=[f'{pc:x}'for pc in sorted(expected_sites)],
        comparison_bytes=actual.stat().st_size,comparison_sha256=digest(actual),native_instruction_callbacks=0,reference_children=0,
        full_return_abi_banks_ordered_stores_verified=True,physical_pre_io_and_fault_atomicity_verified=False)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--xbe',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    paths=(SOURCE,'include/halo2/native_tu15_object_node_hierarchy_kernel.hpp',
        'src/physics/runtime/native_tu15_object_node_hierarchy_abi.hpp',
        'src/physics/runtime/native_tu15_matrix_leaves.hpp',
        'src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp',
        'src/physics/runtime/physics_step_internal.hpp','tools/generated_routine_ownership.py',
        'tools/validate_native_tu15_prt_helpers.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp',
        'tests/native_tu15_object_node_hierarchy_tests.cpp',str(Path(__file__).relative_to(ROOT)))
    pins={name:digest(ROOT/name)for name in paths}
    report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[],source_sha256=pins)
    try:
        report['canonical_reference']=reference(scratch)
        binary=scratch/'object-node-hierarchy'
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),
            str(ROOT/'tests/native_tu15_object_node_hierarchy_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['compiler_command']=command
        subprocess.run(command,check=True)
        result=subprocess.run([str(binary)],input=b'',capture_output=True)
        report['result']=result.stderr.decode()
        if result.returncode:raise RuntimeError('Hierarchy probe failed: '+report['result'][-6000:])
        report['source_hashes_unchanged']=pins=={name:digest(ROOT/name)for name in paths}
        if not report['source_hashes_unchanged']:raise ValueError('Hierarchy proof input changed during validation')
        report.update(passed=True,canonical_complete_banks_reads_stores_return_abi_verified=True,
            unchanged_original_hardware_verified=False,physical_intermediate_flags_verified=False,
            retained_fault_prefixes_verified=False,physical_page_fault_atomicity_verified=False,
            whole_parent_controller_verified=False,native_dependency_closure_verified=False)
        if args.xbe:
            report['unchanged_original_hardware']=hardware(args,scratch,binary)
            report['unchanged_original_hardware_verified']=True
        if pins!={name:digest(ROOT/name)for name in paths}:raise ValueError('Hierarchy proof input changed during hardware validation')
    except Exception as error:
        report['passed']=False;report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(report['result'].strip());return 0

if __name__=='__main__':raise SystemExit(main())
