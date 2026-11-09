#!/usr/bin/env python3
"""Qualify12EF10 list traversal and its genuine dynamic callback entries."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if (seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
import validate_native_array_growth_tu15 as reference
from validate_native_physical_allocation_abi_tu15 import physical_inputs
import generate_physics_live_support_tu15 as generator
from specialize_native_operands import leaf_body


def recover(image,scratch):
    code=reference._Code(image,'.text');body=reference.complete_body(code,0x12ef10);addresses={}
    assert len(body.code)==31 and hashlib.sha256(image.read(0x12ef10,72)).hexdigest()=='e2a45e00a102c780309ed5fb998855a6ee64302327fc027d57e531ad43b6ef6d'
    for pc in body.code:
        ins=code.at(pc);addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
        for operand in ins.operands:
            if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
            if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=reference.NEW_PIN,entries={'0012ef10':0x12ef10},
        body_sites={'0012ef10':sorted(body.code)},instruction_map={f'{pc:08x}':pc for pc in body.code},
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=0x12ef10,tables=[],byte_tables=[])])
    pairing=scratch/'callbacks-pairing.json';pairing.write_text(json.dumps(paired))
    args=SimpleNamespace(output=scratch/'callbacks-reference.cpp',manifest=scratch/'callbacks-reference.json',correspondence=pairing,additional_entries=pairing)
    generator.generate(args,paired,complete_listings=True);pure=leaf_body(args.output.read_text(),'12ef10')[0]
    (scratch/'physical_callbacks_reference.inc').write_text(pure+'\n');start=pure.index('{\n')+2
    bound=pure[:start]+('    if(native_physical_callbacks_detail::run(s,[](auto &v,std::uint32_t owner,std::uint32_t site,bool tail){dispatch(v,owner,site,tail);}))return;\n'
        '    ++translated_fallbacks;\n')+pure[start:]
    (scratch/'physical_callbacks_body.inc').write_text(bound+'\n');return body


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,scratch_directory=str(scratch),production_bound=False,retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);body=recover(image,scratch)
        banks,provenance=physical_inputs(image);rows=[]
        for alignment in range(16):
            common=dict(entry=0x12ef10,arguments=[],receiver=0x486080,header=reference.HEADER,full_abi=True,
                stack_pointer=reference.SP+alignment,direction_flag=bool(alignment&1),abi_entries=[0x12f910],
                input_registers=dict(eax=0x12345678,edx=0xa5a5a5a5,ebx=0xbabababa,ebp=0x98765432,esi=0x11223344,edi=0x55667788))
            rows.append(dict(common,name=f'genuine_empty_callbacks_{alignment}',cohort='genuine-empty-list'))
            for count,callback in ((1,-1),(3,-1),(3,0),(3,1)):
                words=[(0x4f2d2c,reference.UI+0x1000)]
                for index in range(count):
                    node=reference.UI+0x1000+index*64
                    words.extend(((node+8,reference.UI+0x3000+index*32),(node+16,0x12345678+index),
                        (node+20,0x12f910 if index==callback else 0),(node+32,node+64 if index+1<count else 0)))
                rows.append(dict(common,name=f'declared_callback_list_{count}_{callback}_{alignment}',cohort='declared-list-real-entry'if callback>=0 else'declared-list-no-callback',
                    input_words=words,**(dict(required_entries=[0x12f910])if callback>=0 else{})))
        reference.OWNERS=(0x12ef10,);directory,authority=reference.original(image,banks,rows,scratch)
        fixture=directory/'callbacks-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
        binary=scratch/'physical-callbacks';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_physical_callbacks_abi_tests.cpp'),
            str(ROOT/'src/physics/native_tu15_physical_callbacks.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        subprocess.run(command,check=True);modes={}
        for mode in ('native','reference'):
            env=dict(os.environ)
            for key in ('HALO2_REFERENCE_PHYSICAL_CALLBACKS','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
            if mode=='reference':env['HALO2_REFERENCE_PHYSICAL_CALLBACKS']='1'
            result=subprocess.run([str(binary),str(fixture)],env=env,text=True,capture_output=True)
            if result.returncode:report['comparison_errors']=result.stderr[-4000:];sys.stderr.write(result.stderr);result.check_returncode()
            modes[mode]=json.loads(result.stdout)
        executed={pc for row in rows for pc in row['executed_sites']}
        files=[ROOT/'include/halo2/native_tu15_physical_callbacks.hpp',ROOT/'src/physics/native_tu15_physical_callbacks.cpp',
            ROOT/'src/physics/runtime/native_tu15_physical_callbacks_abi.hpp',ROOT/'tests/native_tu15_physical_callbacks_abi_tests.cpp',Path(__file__),
            Path(reference.__file__),ROOT/'tools/observe_native_array_growth_tu15.cpp',Path(generator.__file__),
            ROOT/'tools/validate_native_physical_allocation_abi_tu15.py',ROOT/'tools/specialize_native_operands.py']
        report.update(passed=True,modes=modes,input_provenance=provenance,source_authority=authority,original_instructions_unchanged=True,
            callback_outcomes_supplied_to_original=False,production_callback_closure_qualified=False,exceptional_partial_abi_qualification=False,
            physical_body_coverage=dict(total=len(body.code),executed=len(executed&set(body.code))),
            canonical_seam_uses_declared_independently_backed_callbacks=True,
            canonical_seam_callback_policies=['preserve list','unlink current from live list','clear head','remove head','replace with suffix','write caller saved ESI'],
            canonical_seam_test_store_sites='FE0000..FE0005 are declared test-owner publications; no such provider enters production.',
            address_undefined_sanitizers=args.sanitizers,source_sha256={str(p.relative_to(ROOT)):reference.sha(p)for p in files})
        report['recovered_reference_sha256']={name:reference.sha(scratch/name)for name in ('physical_callbacks_reference.inc','physical_callbacks_body.inc')}
        print(json.dumps(report,indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
