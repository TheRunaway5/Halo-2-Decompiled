#!/usr/bin/env python3
"""Private30E930 edge/point DATA and retained-ABI continuation qualification."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import random
import struct
import subprocess

from generated_routine_ownership import function_bodies

ROOT=Path(__file__).resolve().parents[1]
BODIES={
    'src/physics/runtime/physics_world_runtime_generated.cpp':'4831de546a5dc35bdbd910bbf393eb7438a992a4b2cb8d7c2718ba1a5fed8113',
    'src/physics/runtime/physics_contact_runtime_generated.cpp':'d62f815eac431c206770af5897ddbf25782acbc20dfa367805f4bc2d7771cc2f',
}
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def original_bodies():
    result={}
    for name,pin in BODIES.items():
        text=(ROOT/name).read_text()
        body=next(body for match,body in function_bodies(text)if match.symbol=='30ae40')
        if hashlib.sha256(body.encode()).hexdigest()!=pin:raise ValueError('Complete30E930 canonical body drift: '+name)
        result[name]=body
    return result
def direction_stage(body):
    return body[body.index('L30b576:'):body.index('L30b665:')]+body[body.index('L30b685:'):body.index('L30b784:')]
def reference(scratch):
    bodies=original_bodies();world=direction_stage(next(iter(bodies.values())))
    canonical=re.sub(r'world_observation_pc\((0x[0-9a-f]+u),0x[0-9a-f]+u,s\)',r'\1',world)
    contact=direction_stage(bodies['src/physics/runtime/physics_contact_runtime_generated.cpp'])
    if canonical!=contact:raise ValueError('World/contact edge continuation numerical body differs')
    actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',world)
    text='static void run_stage(WorldState&s,unsigned side){\nif(side==0)goto L30b685;if(side==1)goto L30b576;goto L30b763;\n'+actual+'\nL30b2e5:return;\n}\n'
    (scratch/'simplex_direction_reference.inc').write_text(text)
    return dict(complete_body_sha256=BODIES,stage_sha256=hashlib.sha256(text.encode()).hexdigest(),
        source_stage_sites=141,whole_owner_sites=1020,declared_continuation_entries=['30f175','30f066','30f253'],
        next_owner_instruction='30edd5',world_contact_numerical_body_equal=True)

def hardware(args,scratch,binary):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x30e930);rows=[];requests=[]
    base,size=0x61000000,0x4000
    finite=lambda value:struct.unpack('<I',struct.pack('<f',value))[0]
    edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,
        0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    for k in range(64):
        for side in range(3):
            bank=bytearray(rng.randbytes(size));work=base+0x100+(k%4)*16;sp=base+0x3c00+k%16
            def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
            for offset in (0x20,0x30,0xa0,0xb0):
                for lane in range(4):put(work+offset+lane*4,finite((rng.randrange(65)-32)/4)if k<32 else edges[(k+offset//16+lane*3)%len(edges)])
            if k<12:
                first=(0.,0.,0.,0.);endpoint=(1.,0.,0.,0.);opposite=((-1.,0.,0.,0.),(2.,0.,0.,0.),(.5,1.,0.,0.))[k%3]
                edge=0x20 if side==0 else 0xa0;other=0xa0 if side==0 else 0x20
                for offset,values in ((edge,first),(edge+0x10,endpoint),(other,opposite)):
                    for lane,value in enumerate(values):put(work+offset+lane*4,finite(value))
            put(work,2);put(work+4,2)
            put(sp,0x63000000);put(sp+4,0x12345678);put(sp+8,0x23456789)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp;gp[6]=work
            gp[7]=work+(4,0x120,0x20,0xa0,8,0x70)[k%6]+(1 if k>=48 and k&1 else 0)
            flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10)
            status=0x4100|((k%8)<<11);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f)
            depth=(0,1,4,8)[k%4];values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f')
            live=[values[n%4]for n in range(depth)];xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
            rows.append(dict(id=len(rows),name=f'direction-{side}-{k}',entry=(0x30f175,0x30f066,0x30f253)[side],gp=gp,
                eflags=flags,matrix=0x12345678,source=0x23456789,before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,
                max_steps=300,stop_before_entries=[0x30edd5],banks=[dict(address=base,before=bank.hex())]))
            requests.append((side,gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30e930,),regions=((base,size),),
        trace_calls=[],trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/name for name in ('direction.in','direction.expected','direction.actual'))
    segments=[];vector_opcodes={};source_events=runtime_events=0;sites=set();counterexample_inputs=[]
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(requests)))
        for case,request in zip(observed['cases'],requests,strict=True):
            side,gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']or case.get('stopped_at')!=0x30edd5:
                raise ValueError('Original direction did not reach its unchanged real continuation: '+name)
            inp.write(struct.pack('<14I',side,*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank);segments.append((out.tell(),name,'retained_abi'))
            counterexample_inputs.append(dict(offset=out.tell(),name=name,entry=f'{rows[case["id"]]["entry"]:x}',
                gp=gp,eflags=flags,before_fp=[cw,status,mxcsr],live80=live,
                point_words={f'{offset:x}':list(struct.unpack_from('<4I',bank,gp[6]-base+offset))for offset in (0x20,0x30,0xa0,0xb0)}))
            fp=case['after_fp'];out.write(struct.pack('<13I',*case['boundary_gp'],case['boundary_eflags'],fp[2],fp[1],fp[0],len(live)))
            for value in live:out.write(bytes.fromhex(value))
            for vector in case['boundary_xmm']:out.write(struct.pack('<4I',*vector))
            out.write(bytes.fromhex(case['banks'][0]['after']));events=[]
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before']);pc=event['instruction'];address=event['address'];source_events+=1
                if len(raw)==16:
                    # Every physical MOVAPS operand is retained. The existing
                    # runtime's8+8 protocol is represented explicitly, with no
                    # equal-value filtering or page-fault atomicity claim.
                    opcode=code.at(pc)
                    if opcode.bytes[:2]!=b'\x0f\x29'or opcode.mnemonic!='movaps':raise ValueError('Direction physical vector-store opcode drift')
                    vector_opcodes[f'{pc:x}']=opcode.bytes.hex()
                    events.extend((pc,address+at,before[at:at+8],raw[at:at+8])for at in (0,8))
                elif len(raw)==4:events.append((pc,address,before,raw))
                else:raise ValueError('Unexpected physical direction store width')
            runtime_events+=len(events);out.write(struct.pack('<I',len(events)))
            for pc,address,before,raw in events:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            last=set(case['executed_sites'])&{0x30f23c,0x30f130,0x30f26f}
            if len(last)!=1:raise ValueError('Direction final retained jump-site drift')
            out.write(struct.pack('<I',last.pop()));sites.update(case['executed_sites'])
    with inputs.open('rb')as inp,actual.open('wb')as out:
        result=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if result.returncode:raise RuntimeError('Native physical direction fixture failed: '+result.stderr.decode()[-6000:])
    expected_bytes=expected.read_bytes();actual_bytes=actual.read_bytes()
    if expected_bytes!=actual_bytes:
        offset=next((n for n,(a,b)in enumerate(zip(expected_bytes,actual_bytes))if a!=b),min(len(expected_bytes),len(actual_bytes)))
        name=next((row for row in reversed(segments)if row[0]<=offset),None)
        raise ValueError(f'Unchanged-i386 direction comparison differs byte{offset},segment{name},expected={expected_bytes[offset:offset+32].hex()},actual={actual_bytes[offset:offset+32].hex()}')
    expected_sites={int(pc,16)for pc in re.findall(r'services.instruction\((0x[0-9a-f]+)u\)',(scratch/'simplex_direction_reference.inc').read_text())}
    if sites!=expected_sites:raise ValueError(f'Direction physical coverage differs: {len(sites)}/{len(expected_sites)}')
    (scratch/'direction-case-offsets.json').write_text(json.dumps(counterexample_inputs))
    return dict(cases=len(rows),source_sites=len(sites),complete_gp_flags_xmm_live80_banks_verified=True,
        unchanged_original_proof=proof,physical_source_store_operands=source_events,runtime_store_events=runtime_events,
        comparison_sha256=hashlib.sha256(actual_bytes).hexdigest(),vector_protocol=dict(physical_operand_bytes=16,runtime_event_bytes=[8,8],
            exact_store_opcodes=vector_opcodes,no_equal_value_filtering=True),physical_page_fault_atomicity_verified=False,
        declared_midowner_continuations=True,whole30e930_hardware_verified=False)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report',required=True,type=Path);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--xbe',type=Path)
    args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():print('Authenticated TU1.5 XBE is not configured');return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    owned=[
        'include/halo2/native_tu15_simplex_direction_kernel.hpp',
        'src/physics/runtime/native_tu15_simplex_direction_abi.hpp',
        'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
        'src/physics/runtime/physics_world_runtime_internal.hpp',
        'src/physics/runtime/physics_contact_runtime_internal.hpp',
        'src/physics/runtime/physics_step_internal.hpp',
        'tests/native_tu15_simplex_direction_tests.cpp',
        'tools/generated_routine_ownership.py',str(Path(__file__).relative_to(ROOT)),
    ]
    pins={name:sha(ROOT/name)for name in owned}
    report=dict(passed=False,production_bound=False,whole30e930_controller_closed=False,
        scratch_owner=str(scratch),retained_large_artifacts=[],source_sha256=pins,
        original_physical_instructions_patched=False,supplied_child_outcomes=False)
    try:
        report['reference']=reference(scratch)
        binary=scratch/'simplex-direction'
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),
            str(ROOT/'tests/native_tu15_simplex_direction_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['compiler_command']=command
        built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise RuntimeError('Direction fixture build failed:\n'+built.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True)
        report['canonical_diagnostic']=result.stderr[-12000:]
        if result.returncode:raise RuntimeError('Direction fixture failed:\n'+report['canonical_diagnostic'])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
        if args.xbe:report['physical_original']=hardware(args,scratch,binary)
        text=(ROOT/'include/halo2/native_tu15_simplex_direction_kernel.hpp').read_text()
        mutations={
            'wrong_cross_orientation':('if(first_edge){','if(!first_edge){',1),
            'skip_paired_payload':('memory.vector_write(P::promote_payload,s,s.work+(first_edge?0x60u:0xe0u),s.payload);','/* deliberate missing paired payload */',1),
            'wrong_live_second_count':('memory.word_read(P::second_count,s,s.second_count_address);','(memory.word_read(P::second_count,s,s.second_count_address)^1u);',2),
        }
        report['canonical_semantic_mutants']={}
        for name,(needle,replacement,count)in mutations.items():
            if text.count(needle)!=count:raise ValueError('Direction semantic mutant source drift: '+name)
            directory=scratch/name;(directory/'halo2').mkdir(parents=True)
            variant=text.replace(needle,replacement);(directory/'halo2/native_tu15_simplex_direction_kernel.hpp').write_text(variant)
            mutant_binary=directory/'mutant';mutant_command=command.copy();mutant_command[mutant_command.index('-I'+str(ROOT/'include')):mutant_command.index('-I'+str(ROOT/'include'))]=['-I'+str(directory)]
            mutant_command[-1]=str(mutant_binary)
            built=subprocess.run(mutant_command,capture_output=True,text=True)
            if built.returncode:raise RuntimeError('Direction mutant did not compile: '+name+'\n'+built.stderr[-5000:])
            refused=subprocess.run([str(mutant_binary)],capture_output=True,text=True)
            if refused.returncode==0 or 'Native simplex direction mismatch'not in refused.stderr:
                raise ValueError('Direction semantic mutant was not rejected by comparison: '+name+'\n'+refused.stderr[-5000:])
            report['canonical_semantic_mutants'][name]=dict(rejected=True,mutant_sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=refused.stderr[-2000:])
        if args.xbe:
            needle='multiply(shuffle<0xc9>(p.from_endpoint),shuffle<0xd2>(p.from_first))'
            replacement='multiply(shuffle<0xd2>(p.from_first),shuffle<0xc9>(p.from_endpoint))'
            if text.count(needle)!=1:raise ValueError('Original positive cross-product operand seam drift')
            variant=text.replace(needle,replacement);directory=scratch/'reverse_positive_mul';(directory/'halo2').mkdir(parents=True)
            (directory/'halo2/native_tu15_simplex_direction_kernel.hpp').write_text(variant)
            target=directory/'mutant';changed=command.copy();at=changed.index('-I'+str(ROOT/'include'));changed[at:at]=['-I'+str(directory)];changed[-1]=str(target)
            built=subprocess.run(changed,capture_output=True,text=True)
            if built.returncode:raise RuntimeError('Operand-order counterexample mutant compile failed: '+built.stderr[-5000:])
            observed=scratch/'direction-reversed-positive.actual'
            with(scratch/'direction.in').open('rb')as inp,observed.open('wb')as out:
                result=subprocess.run([str(target),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
            if result.returncode:raise ValueError('Operand-order mutant must complete its real arithmetic: '+result.stderr.decode()[-5000:])
            expected_bytes=(scratch/'direction.expected').read_bytes();actual_bytes=observed.read_bytes()
            if expected_bytes==actual_bytes:raise ValueError('Original positive MUL operand reversal survived IEEE hardware corpus')
            offset=next((n for n,(a,b)in enumerate(zip(expected_bytes,actual_bytes))if a!=b),min(len(expected_bytes),len(actual_bytes)))
            inputs=json.loads((scratch/'direction-case-offsets.json').read_text());case=next(row for row in reversed(inputs)if row['offset']<=offset)
            depth=len(case['live80']);xmm_at=case['offset']+52+depth*10
            report['source_operand_order_counterexample']=dict(input=case,first_difference_byte_in_case=offset-case['offset'],
                unchanged_i386_xmm=[list(struct.unpack_from('<4I',expected_bytes,xmm_at+n*16))for n in range(8)],
                reversed_positive_mul_xmm=[list(struct.unpack_from('<4I',actual_bytes,xmm_at+n*16))for n in range(8)],
                mutant_sha256=hashlib.sha256(variant.encode()).hexdigest(),unchanged_original_authority=True,
                physical_positive_mul_sites=['30f1fd','30f0f4'],no_flag_or_nan_masking=True)
        report.update(complete_return_gp_flags_xmm_live80_verified=True,
            retained_ordered_reads_stores_banks_verified=True,
            retained_read_write_observer_fault_prefixes_verified=True,
            original_hardware_verified=bool(args.xbe),physical_pre_io_flags_verified=False,
            physical_movaps_fault_atomicity_verified=False,physical_rmw_fault_atomicity_verified=False,
            semantic_unprovided_payload_refusals_preserved=True,
            remaining=([]if args.xbe else['Unchanged physical-i386 direction-stage proof.'])+[
                'Full1020-site30E930 typed controller and retained ABI.',
                'Wider30D830/30DE60 and genuine shape virtual owner closure.',
                'Physical instruction/page-fault atomicity.'])
        original_bodies()
        report['proof_inputs_unchanged']=pins=={name:sha(ROOT/name)for name in owned}
        if not report['proof_inputs_unchanged']:raise ValueError('Direction proof inputs changed')
        report['passed']=True
    except Exception as error:report['error']=str(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report['canonical'],sort_keys=True));return 0
if __name__=='__main__':raise SystemExit(main())
