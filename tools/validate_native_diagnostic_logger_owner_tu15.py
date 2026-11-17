#!/usr/bin/env python3
"""Private actual2E16D0 required-owner adapter and full unchanged-i386 ABI."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from validate_native_tu15_prt_helpers import physical_original,PIN

FILES=('include/halo2/native_tu15_diagnostic_logger_kernel.hpp',
    'src/runtime/native_tu15_diagnostic_logger.hpp','src/runtime/native_tu15_diagnostic_logger_owner.hpp',
    'tests/native_tu15_diagnostic_logger_owner_tests.cpp','tools/validate_native_diagnostic_logger_owner_tu15.py',
    'include/halo2/physics_contact_runtime.hpp','src/physics/runtime/physics_world_runtime_internal.hpp',
    'src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp')
def hashes():return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES}

def cases():
    rng=random.Random(0x2e16d0);rows=[]
    live=['0000000000000080ff3f','00000000000000000080','45230100000000c0ff7f',
          '01000000000000000000','0000000000000000ffbf','00000000000000800040',
          '01000000000000800000','4523010000000080ff7f']
    for index in range(128):
        sp=0x62001000+index%16;bank=bytearray([0xa5])*8192
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
        gp[1]=(0,1,7,8,0xfffffff8,0xfffffffc,0xffffffff,0x80000000,0x61000100)[index%9]
        arguments=[0x63000000,0x12340000+index,0x56780000+index]
        for n,value in enumerate(arguments):bank[sp-0x62000000+n*4:sp-0x62000000+n*4+4]=value.to_bytes(4,'little')
        rows.append(dict(id=index,name=f'getter-{index}',entry=0x2e16d0,gp=gp,
            eflags=0x202|(0x400 if index&1 else 0)|(0xd5 if index&2 else 0),fs_base=rng.getrandbits(32),
            matrix=arguments[1],source=arguments[2],max_steps=10,
            before_fp=[(0x7f,0x17f,0x27f,0x37f)[index%4]|((index%4)<<10),
                ((index//9)%8)<<11|(0,1,0x10,0x200,0x40)[index%5],
                0x1f80|((index%4)<<13)|(0x8040 if index&1 else 0)],
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            live_x87=live[:index%9],banks=[dict(address=0x62000000,before=bank.hex())]))
    for original in rows[:18]:
        row=copy.deepcopy(original);row.update(id=len(rows),name=original['name']+'-read-failure',read_failure=True,
            fault=dict(phase='before_read',ordinal=0));rows.append(row)
    return rows

def mutants(scratch,command,fixture):
    definitions=(
        ('wrong-stream-offset','include/halo2/native_tu15_diagnostic_logger_kernel.hpp','return logger+8u;','return logger+12u;',False),
        ('missing-abi-publication','src/runtime/native_tu15_diagnostic_logger_owner.hpp','run_private(state,publish);','run_private(state,[](auto&,auto){});',False),
        ('wrong-return-stack','src/runtime/native_tu15_diagnostic_logger.hpp','Base::pop(0x2e16d3u);','Base::pop(0x2e16d3u);Base::s_.gp[4]+=4u;',False),
        ('stale-cleared-callable','src/runtime/native_tu15_diagnostic_logger_owner.hpp','if(!services.instruction)return !c.position&&!c.literal_service_sites;','if(!services.instruction)return true;',False),
        ('reference-control-bypass','src/runtime/native_tu15_diagnostic_logger_owner.hpp','if(reference()){','if(false){',True))
    rows=[]
    for name,path,old,new,reference in definitions:
        text=(ROOT/path).read_text()
        if text.count(old)!=1:raise ValueError('Missing unique getter semantic mutant '+name)
        directory=scratch/('mutant-'+name);target=directory/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1))
        binary=directory/'tests';compile_command=list(command);compile_command.insert(1,'-I'+str(directory));compile_command[compile_command.index('-o')+1]=str(binary)
        compiled=subprocess.run(compile_command,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Getter mutant failed compilation '+name+': '+compiled.stderr[-3500:])
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');env.pop('HALO2_REFERENCE_DIAGNOSTIC_LOGGER',None)
        if reference:env['HALO2_REFERENCE_DIAGNOSTIC_LOGGER']='1'
        tested=subprocess.run([str(binary),'--guards'if name=='stale-cleared-callable'else str(fixture)],capture_output=True,text=True,env=env)
        if tested.returncode==0:raise ValueError('Compiled getter semantic mutant survived '+name)
        rows.append(dict(name=name,compiled=True,rejected=True,diagnostic=(tested.stdout+tested.stderr)[-2000:]))
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);before=hashes()
    report=dict(passed=False,production_binding=False,owned_scratch=str(scratch),source_sha256=before,retained_large_artifacts=[],compilers=[])
    try:
        image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
        assert image.read(0x2e16d0,4)==bytes.fromhex('8d4108c3')and image.read(0x41be04,4)==(0x2e16d0).to_bytes(4,'little')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original=common.metadata
        def metadata(image):
            result=original(image)
            for site in result['sites']:
                if site['instruction']==0x2e16d3:site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;rows=cases()
        proof,authority=physical_original(image,rows,scratch,entries=(0x2e16d0,),regions=((0x62000000,8192),),
            trace_calls=[dict(entry=0x2e16d0,gp=True,xmm=True),dict(entry=0x2e16d3,gp=True,xmm=True)],trace_extended=True,preserve_x87_prefix=True)
        for row,actual in zip(rows,proof['cases'],strict=True):
            assert actual['completed']==(not row.get('read_failure',False))
            assert actual['live_x87_unchanged'];row.update({key:actual[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads')})
            row['read_packet']=next(packet for packet in actual['calls']if packet['entry']==0x2e16d3);row['read_packet'].pop('arguments')
            for bank,after_bank in zip(row['banks'],actual['banks'],strict=True):bank['after']=after_bank['after']
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        for compiler in args.compiler or['g++']:
            binary=scratch/('getter-'+Path(compiler).name)
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_diagnostic_logger_owner_tests.cpp'),'-o',str(binary)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            compiled=subprocess.run(command,capture_output=True,text=True)
            if compiled.returncode:raise ValueError('Getter public-owner compilation failed '+compiled.stderr[-9000:])
            result=dict(compiler=compiler,checks=[])
            for name,arguments,reference in(('native',[str(fixture)],False),('reference',[str(fixture)],True),('guards',['--guards'],False),('reference_guards',['--guards'],True)):
                env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0');env.pop('HALO2_REFERENCE_DIAGNOSTIC_LOGGER',None)
                if reference:env['HALO2_REFERENCE_DIAGNOSTIC_LOGGER']='1'
                tested=subprocess.run([str(binary),*arguments],capture_output=True,text=True,env=env)
                check=dict(name=name,passed=tested.returncode==0,diagnostic=(tested.stdout+tested.stderr)[-8000:]);result['checks'].append(check)
                if tested.returncode:raise ValueError('Getter public-owner comparison failed '+check['diagnostic'])
                check['result']=json.loads(tested.stdout)
            if args.semantic_mutants:
                mutation_root=scratch/('mutants-'+Path(compiler).name);mutation_root.mkdir()
                result['semantic_mutants']=mutants(mutation_root,command,fixture)
            report['compilers'].append(result)
        if hashes()!=before:raise ValueError('Getter public-owner source drift')
        report.update(passed=True,physical_original=authority,original_cases=len(rows),normal_cases=128,read_prefix_cases=18,
            instruction_sites=2,original_bytes='8d4108c3',authoring_vtable_slot='41be04 ->2e16d0',
            scope='Private fixed-TU1.5 childless required-owner adapter; actual World hook absent. Complete source ABI/reference/IO publication; no initializer, platform output or older-image claim.',
            unsupported_domains=['ActiveMMX','Unmasked/pending input FP or invalid depth','Unknown instruction-policy callback','Actual signal/MMU fault envelope','Production World route not installed'])
    except Exception as error:report.update(passed=False,error=str(error)[-14000:])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=report['passed'],error=report.get('error'),report=str(args.report))));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
