#!/usr/bin/env python3
"""Private real24DEF0/24DF20 returns and genuine16C000 entry qualification."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys
from oracle_smg_firing_1_1 import ROOT,Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense
from validate_native_mopp_ray_caller_frame_tu15 import FILES

REGIONS=((0x46a000,0x1000),(0x4d4000,0x1000),(0x4f4000,0x1000),(0x514000,0x1000),(0x61000000,0x1000),(0x62000000,0x8000))
OWN_FILES=FILES+('include/halo2/native_tu15_instanced_ray_callback_value.hpp','include/halo2/native_tu15_instanced_ray_callback_kernel.hpp','src/physics/runtime/native_tu15_instanced_ray_callback_frame.hpp',
    'tools/native_instanced_ray_call_failure.py','tests/native_tu15_instanced_ray_call_failure_tests.cpp','tools/native_instanced_ray_callback_mutations.py','tests/native_tu15_instanced_ray_callback_frame_tests.cpp','tools/validate_native_instanced_ray_callback_frame_tu15.py')

def hashes():return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in OWN_FILES}

def corpus(image,smoke):
    rng=random.Random(0x24df20);rows=[]
    live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f',
          '00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
    for entry in (0x24def0,0x24df20):
        variants=[('basic',k) for k in range(8 if smoke else 64)]
        if not smoke:variants += [(name,k) for name in ('stamp_saved_bx','stamp_saved_si','collector_frame','collector_args','body_saved') for k in (0,1)]
        for name,k in variants:
            ordinal=len(rows);sp=0x62006000+ordinal%16
            banks={base:bytearray(rng.randbytes(size)) for base,size in REGIONS}
            banks[0x46a000]=bytearray(image.read(0x46a000,0x1000))
            def put(address,value):
                base=next(base for base,size in REGIONS if base<=address and address+4<=base+size)
                banks[base][address-base:address-base+4]=int(value).to_bytes(4,'little')
            collector=0x61000600;body=0x61000400;table=0x61000500;packet=0x61000800
            index=(0,1,0xffff,0x7fff,0x8000)[k%5];signed=index if index<0x8000 else index-0x10000
            instance=(rng.getrandbits(16)<<16)|index;epoch=rng.getrandbits(32)
            if name=='stamp_saved_bx':sp=0x4f44f0+signed*4+4
            elif name=='stamp_saved_si':sp=0x4f44f0+signed*4+8
            if name=='collector_frame':collector=(((sp-4)&0xfffffff0)-0x20) if entry==0x24def0 else sp-0x20
            elif name=='collector_args':collector=sp-8
            elif name=='body_saved':body=sp-8
            put(body+4,instance);put(0x4f44ec,epoch);put(0x4f44f0+signed*4,epoch if k%2==0 else epoch^1)
            put(collector,table);put(table,0x24df20)
            for offset,value in ((4,0x3f800000),(8,0x13579),(12,0x61000200),(16,0x61000210),(20,0x61000220),(28,0x61000700)):put(collector+offset,value)
            arguments=[0x63000000,0x61000900,body,collector] if entry==0x24def0 else [0x63000000,body,packet]
            for n,value in enumerate(arguments):put(sp+n*4,value)
            gp=[rng.getrandbits(32) for _ in range(8)];gp[1]=collector;gp[4]=sp
            if name=='body_saved' and entry==0x24def0:gp[5]=0xabcd0001
            depth=(0,1,4,8)[(ordinal//4)%4]
            rows.append(dict(id=ordinal,name=f'{entry:x}_{name}_{"duplicate" if k%2==0 else "query"}_{k}',entry=entry,phase='normal',
                gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],live_x87=list(live[:depth]),
                before_fp=[(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10),((ordinal%8)<<11)|(ordinal&0x473f),
                           0x1f80|((ordinal%4)<<13)|(0,0x40,0x8000,0x8040)[ordinal%4]],
                banks=[dict(address=base,before=bytes(data).hex()) for base,data in banks.items()],matrix=arguments[1],source=arguments[2],
                max_steps=1000,stop_before_entries=[0x16c000]))
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--smoke',action='store_true');parser.add_argument('--compiler',action='append',choices=('g++','clang++'))
    parser.add_argument('--negative-controls',action='store_true')
    parser.add_argument('--retained-call-failure',action='store_true')
    args=parser.parse_args()
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,full_postquery_continuation=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x24def0,0x24df20);original=common.metadata;code=common._Code(image,'.text')
        def metadata(image):
            result=original(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;sites=metadata(image)['sites'];io=[site['instruction'] for site in sites if site['operands'] or code.at(site['instruction']).mnemonic in ('push','pop','ret','call')]
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io];rows=corpus(image,args.smoke)
        observed,authority=physical_original_dense(image,rows,scratch,entries=(0x24def0,0x24df20),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        if not args.smoke:
            for row,actual in zip(rows,observed['cases'],strict=True):
                if int(row['name'].rsplit('_',1)[1])>=2:continue
                for ordinal,event in enumerate(actual['reads']):
                    prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_read_{ordinal}',phase='before_read',native_fault_ordinal=ordinal,
                        fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(prefix)
                for ordinal,event in enumerate(actual['writes']):
                    for phase in ('before_write','after_write'):
                        prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_{phase}_{ordinal}',phase=phase,native_fault_ordinal=ordinal,
                            fault=dict(phase=phase,ordinal=ordinal))
                        if phase=='before_write' and event['instruction']==0x24df14:
                            prefix.update(phase='before_instruction',native_fault_pc=0x24df14)
                        prefixes.append(prefix)
        prefix_authority=None
        if prefixes:
            directory=scratch/'prefixes';directory.mkdir()
            more,prefix_authority=physical_original_dense(image,prefixes,directory,entries=(0x24def0,0x24df20),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
            observed['cases']+=more['cases']
        normal_count=len(rows);rows+=prefixes;executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='normal'),row['name']
            row['returned']=row['phase']=='normal' and actual['stopped_at']==0x63000000
            if row['phase']=='normal':assert actual['stopped_at'] in (0x63000000,0x16c000)
            executed.update(actual['executed_sites']);row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','reads','calls')})
            row['expected_pc']=(0x24df19 if row['entry']==0x24def0 else 0x24dfab) if row['returned'] else 0x16c000 if row['phase']=='normal' else actual['boundary']['instruction']
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir();cmake=f'''cmake_minimum_required(VERSION 3.20)
project(instanced_callback LANGUAGES CXX)
include(CTest)
add_executable(instanced_callback "{ROOT/'tests/native_tu15_instanced_ray_callback_frame_tests.cpp'}")
target_include_directories(instanced_callback PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(instanced_callback PRIVATE cxx_std_20)
target_compile_options(instanced_callback PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME instanced_callback COMMAND instanced_callback "{fixture}" "{scratch/'native-report.json'}")
'''
        if args.sanitizers:cmake+='target_compile_options(instanced_callback PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(instanced_callback PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in args.compiler or ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),
                '--target','instanced_callback','--filter','^instanced_callback$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);results.append(dict(compiler=compiler,exit_code=result.returncode))
            assert result.returncode==0,result.stdout[-8000:]
            native=json.loads((scratch/'native-report.json').read_text())
            assert native['passed'] and native['comparisons']==normal_count*4+len(prefixes)*2
            results[-1]['native_observations']=native
        controls=None
        if args.negative_controls:
            from native_instanced_ray_callback_mutations import qualify
            controls=qualify(scratch,args.output,rows,io)
        retained=None
        if args.retained_call_failure:
            from native_instanced_ray_call_failure import qualify as qualify_call
            retained=qualify_call(scratch,args.output,rows,io)
        report.update(passed=True,retained_call_provider_failure=retained,compiled_controls=controls,sanitizers=args.sanitizers,physical_paths=len(rows),normal_paths=normal_count,complete_returns=sum(row['returned'] for row in rows),
            genuine_query_entries=sum(row['phase']=='normal' and not row['returned'] for row in rows),retained_prefixes=len(prefixes),
            memcall_before_instruction_prefixes=sum(row['phase']=='before_instruction' for row in rows),post_target_stack_write_failure_qualified=False,
            comparisons_per_compiler=normal_count*4+len(prefixes)*2,complete_original_pre_io_packets=sum(len(row['calls']) for row in rows),
            executed_sites=sorted(executed),authority=authority,prefix_authority=prefix_authority,compilers=results)
    except BaseException as error:report['error']=str(error) or repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
