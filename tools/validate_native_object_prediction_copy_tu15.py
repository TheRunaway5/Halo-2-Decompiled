#!/usr/bin/env python3
"""ADCD0 shared DWORD-copy kernel and real owner dispatcher versus unchanged TU1.5."""
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
from validate_native_object_node_rebuild_tu15 import prediction_provenance
from native_object_node_validation_support import admission

ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x61000000,65536),(0x62000000,16384))
ORIGINAL='568b74240c578b7c241483c668b90a000000f3a55fb0015ec20c00'
BODY_PIN='8308d639cb2a5de597db3c3e85dec6553185384e6e58adfe074431c268986310'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def hardware(args,scratch,binary):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image=Xbe(args.xbe)
    if image.read(0xadcd0,27).hex()!=ORIGINAL:raise ValueError('Complete ADCD0 image drift')
    rng=random.Random(0xadcd0);rows=[];requests=[]
    labels=('separate','overlap-forward','overlap-backward','self','source-saved-words','saved-si','saved-di','caller-arguments','caller-return')
    for alignment in range(16):
        for backwards in (False,True):
            for label in labels:
                n=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
                def put(address,value):
                    for (base,size),bank in zip(REGIONS,banks):
                        if base<=address and address+4<=base+size:struct.pack_into('<I',bank,address-base,value);return
                    raise ValueError('Copy fixture input escaped declared bank')
                sp=0x62002000+alignment;source=0x61001000+alignment;start=source+0x68;output=0x61002000+alignment
                if label=='overlap-forward':output=start+4
                if label=='overlap-backward':output=start-4
                if label=='self':output=start
                if label=='source-saved-words':source=sp-0x78
                if label=='saved-si':output=sp-4
                if label=='saved-di':output=sp-8
                if label=='caller-arguments':output=sp+4
                if label=='caller-return':output=sp
                delta=-4 if backwards else 4
                # These are declared source payload words. The unchanged copy
                # computes every result; a word that will become the caller's
                # return address is a real input pointing at the stop owner.
                for k in range(10):
                    if output+delta*k==sp:put(source+0x68+delta*k,0x63000000)
                metadata=rng.getrandbits(32)
                put(sp,0x63000000);put(sp+4,metadata);put(sp+8,source);put(sp+12,output)
                gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
                flags=0x202|(rng.getrandbits(12)&0x8d5)|(1024 if backwards else 0)
                cw=(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10);status=0x4100|((n%8)<<11)|(n&0x3f)
                mxcsr=0x1f80|((n%4)<<13)|(0x8040 if n&4 else 0)|(n&0x3f)
                values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f',
                    '0000000000000080ffbf','00000000000000000000','01000000000000000000','efcdab00000000c0ff7f')
                live=list(values[:(0,1,4,8)[n%4]]);xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
                rows.append(dict(id=n,name=f'{label}-align{alignment}-DF{int(backwards)}',entry=0xadcd0,gp=gp,eflags=flags,matrix=metadata,source=source,
                    before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=100,
                    banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
                requests.append((gp,flags,mxcsr,cw,status,live,xmm,banks))
    observed,proof=physical_original(image,rows,scratch,entries=(0xadcd0,),regions=REGIONS)
    inputs,expected,actual=(scratch/name for name in ('copy.in','copy.expected','copy.actual'))
    segments=[]
    with inputs.open('wb')as inp,expected.open('wb')as out:
        for case,request in zip(observed['cases'],requests,strict=True):
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original prediction copy failed a genuine return')
            gp,flags,mxcsr,cw,status,live,xmm,banks=request
            inp.write(struct.pack('<12I',*gp,flags,mxcsr,cw,status));inp.write(struct.pack('<I',len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for lane in xmm:inp.write(struct.pack('<4I',*lane))
            inp.write(b''.join(banks));segments.append((out.tell(),rows[case['id']]['name']))
            out.write(struct.pack('<12I',*case['boundary_gp'],case['boundary_eflags'],case['after_fp'][2],case['after_fp'][1],case['after_fp'][0]))
            for lane in case['boundary_xmm']:out.write(struct.pack('<4I',*lane))
            out.write(b''.join(bytes.fromhex(bank['after'])for bank in case['banks']));out.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);out.write(struct.pack('<III',event['instruction'],event['address'],len(raw)))
                out.write(bytes.fromhex(event['before']));out.write(raw)
            out.write(struct.pack('<I',0xadce8))
    with inputs.open('rb')as inp,actual.open('wb')as out:result=subprocess.run([str(binary)],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if result.returncode:raise RuntimeError('Copy native transaction failed: '+result.stderr.decode()[-6000:])
    if digest(expected)!=digest(actual):
        a,b=actual.read_bytes(),expected.read_bytes();first=next((i for i,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)))
        segment=next(s for s in reversed(segments)if s[0]<=first)
        raise ValueError(f'Prediction copy physical mismatch at{first},case{segment[1]},relative{first-segment[0]}: {a[first:first+32].hex()} vs{b[first:first+32].hex()}')
    owner_proof=None
    if args.require_binding:
        env=dict(os.environ);env.pop('HALO2_REFERENCE_OBJECT_PREDICTION_COPY',None)
        controls=('default','unrelated-reference-controls')
        owner_rows=[]
        for control in controls:
            environment=dict(env)
            if control!='default':environment.update({name:'1'for name in ('HALO2_REFERENCE_RAW_ALLOCATOR','HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_OBJECT_NODE_REBUILD','HALO2_REFERENCE_OBJECT_ANIMATION')})
            path=scratch/f'owner-{control}.actual'
            with inputs.open('rb')as inp,path.open('wb')as out:owner=subprocess.run([str(binary),'--owner-dispatch'],stdin=inp,stdout=out,stderr=subprocess.PIPE,env=environment)
            if owner.returncode or digest(path)!=digest(expected):raise ValueError('True prediction owner dispatcher mismatch: '+owner.stderr.decode()[-4000:])
            owner_rows.append(dict(control=control,cases=len(rows),comparison_sha256=digest(path),activity_checked=True,external_outcomes_requested=False,
                retained_source_protocol_prefixes=510,canonical_normal_cases=10))
        owner_proof=dict(routes=owner_rows,admission=admission(binary,'HALO2_REFERENCE_OBJECT_PREDICTION_COPY','owner-'))
    executed={pc for case in observed['cases']for pc in case['executed_sites']}
    if len(executed)!=11:raise ValueError('Original prediction copy site coverage changed')
    return dict(passed=True,cases=len(rows),all_original_instruction_sites=11,labels=labels,stack_alignments=16,direction_flag_values=2,
        full_return_gp_flags_xmm_fp_live80_banks_ordered_stores=True,canonical_ordered_reads=True,
        physical_pre_io_and_page_fault_atomicity_verified=False,native_instruction_callbacks=0,
        comparison_bytes=actual.stat().st_size,comparison_sha256=digest(actual),original_xbe_sha256=PIN,physical_original=proof,
        canonical_result=result.stderr.decode().strip(),actual_owner_dispatch=owner_proof)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    paths=('include/halo2/native_tu15_word_copy_kernel.hpp','src/physics/runtime/native_tu15_object_prediction_copy.hpp','src/physics/runtime/native_tu15_object_node_admission.hpp',
        'tests/native_tu15_object_prediction_copy_tests.cpp','tests/native_tu15_object_prediction_copy_reference.inc',
        'tests/native_tu15_object_node_admission_checks.hpp','tools/native_object_node_validation_support.py',
        'tools/validate_native_object_node_rebuild_tu15.py','src/physics/runtime/physics_world_runtime_internal.hpp',
        'src/physics/runtime/native_tu15_matrix_leaves.hpp','tools/validate_native_tu15_prt_helpers.py',
        'tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp',str(Path(__file__).relative_to(ROOT)))
    if args.require_binding:paths+=('src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp','tools/specialize_native_operands.py','tools/native_object_node_rebuild_replacements.py','src/physics/physics_live_support_tu15_generated.cpp')
    pins={name:digest(ROOT/name)for name in paths};report=dict(passed=False,production_bound=args.require_binding,source_sha256=pins,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        report['genuine_prediction_callback_provenance']=prediction_provenance(args.xbe)
        text=(ROOT/'tests/native_tu15_object_prediction_copy_reference.inc').read_text()
        body=next(body for match,body in function_bodies(text)if match[1]=='adcd0')
        if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete original ADCD0 lowering drift')
        report['complete_reference_body_sha256']=BODY_PIN;binary=scratch/'prediction-copy'
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
            str(ROOT/'tests/native_tu15_object_prediction_copy_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if args.require_binding:
            from specialize_native_operands import specialize,OBJECT_PREDICTION_COPY_ACTUAL_OWNER_IMAGES
            source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();_,receipt=specialize(source)
            if receipt['native_object_prediction_copy_actual_owner_entries']!=['adcd0']:raise ValueError('Actual prediction owner binding missing')
            if OBJECT_PREDICTION_COPY_ACTUAL_OWNER_IMAGES!={0xadcd0:(27,hashlib.sha256(bytes.fromhex(ORIGINAL)).hexdigest())}:raise ValueError('Whole prediction original image identity drift')
            dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            for replacement in (dispatch.replace('{','{if(target==0)return;',1),dispatch.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1)):
                try:specialize(source.replace(dispatch,replacement,1))
                except ValueError as error:
                    if str(error)!='Complete13FE50 dispatcher drift':raise
                else:raise ValueError('Prediction actual owner accepted changed full dispatch context')
            report['binding']=dict(entries=receipt['native_object_prediction_copy_actual_owner_entries'],complete_dispatch_drift_rejections=2,owner_images=OBJECT_PREDICTION_COPY_ACTUAL_OWNER_IMAGES)
            command+=['-DHALO2_OBJECT_PREDICTION_WITH_OWNER_DISPATCH',*[str(ROOT/'src/physics'/name)for name in ('native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp','native_tu15_physical_epoch.cpp')]]
        report['compiler_command']=command;subprocess.run(command,check=True)
        report['admission']=admission(binary,'HALO2_REFERENCE_OBJECT_PREDICTION_COPY')
        result=subprocess.run([str(binary)],input=b'',capture_output=True)
        report['canonical_result']=result.stderr.decode().strip()
        if result.returncode:raise RuntimeError('Prediction copy canonical probe failed: '+report['canonical_result'])
        report['hardware']=hardware(args,scratch,binary)
        report['source_hashes_unchanged']=pins=={name:digest(ROOT/name)for name in paths}
        if not report['source_hashes_unchanged']:raise ValueError('Prediction copy proof input changed')
        report.update(passed=True,normal_hardware_verified=True,retained_source_protocol_prefixes_verified=True,physical_page_fault_atomicity_verified=False)
    except Exception as error:report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(report['canonical_result']);print('physical_prediction_copy_cases='+str(report['hardware']['cases']));return 0
if __name__=='__main__':raise SystemExit(main())
