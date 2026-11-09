#!/usr/bin/env python3
"""Qualify actual native MOVAPS store alignment and ordinary-provider atomicity."""
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
from validate_native_mopp_ray_caller_frame_tu15 import FILES,REGIONS

KEY_FILES=FILES+(
    'tests/native_tu15_mopp_ray_vector_store_frame_tests.cpp',
    'tools/validate_native_mopp_ray_vector_store_frame_tu15.py',
)


def hashes():
    return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in KEY_FILES}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    args=parser.parse_args()
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),
                retained_large_artifacts=[],scope='One unchanged original MOVAPS store, true alignment faults, full ABI/live80 and ordinary-provider write/observer boundaries; no full-parent claim')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN
        image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x30aa70,)
        metadata=common.metadata(image)
        code=common._Code(image,'.text')
        io=[site['instruction'] for site in metadata['sites']
            if site['instruction'] not in (0x30aaa0,0x30aaa7) and
            (site['operands'] or code.at(site['instruction']).mnemonic in ('push','pop','ret','call'))]
        live=('0000000000000080ff3f','00000000000000000080',
              '35c26821a2da0fc90040','45230100000000c0ff7f',
              '00000000000000800040','00000000000000c0ff3f',
              '00000000000000000000','0000000000000080ffbf')
        rng=random.Random(0x30af0e);rows=[]
        for alignment in range(16):
            for n in range(8):
                ordinal=len(rows);banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
                sp=0x62005f70+alignment;key=rng.getrandbits(32);source=rng.getrandbits(32)
                for k,value in enumerate((0x63000000,key,source)):
                    offset=sp-REGIONS[1][0]+k*4;banks[1][offset:offset+4]=value.to_bytes(4,'little')
                gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=sp
                depth=(0,1,4,8)[n%4]
                rows.append(dict(id=ordinal,name=f'vector_store_{alignment}_{n}',entry=0x30af0e,phase='store_boundary',
                    gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                    xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],live_x87=list(live[:depth]),
                    before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(ordinal&0x473f),
                               0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],
                    banks=[dict(address=base,before=bytes(data).hex()) for (base,_),data in zip(REGIONS,banks)],
                    matrix=key,source=source,max_steps=100,stop_before_entries=[0x30af16],alignment_fault=bool(alignment)))
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=(0x30aa70,),regions=REGIONS,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        for index in range(8):
            row,actual=rows[index],observed['cases'][index];ordinal=0
            for physical_ordinal,event in enumerate(actual['reads']):
                if event['instruction'] in (0x30aaa0,0x30aaa7):continue
                prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_read_{ordinal}',
                    phase='before_read',native_fault_ordinal=ordinal,fault=dict(phase='before_read',ordinal=physical_ordinal))
                prefixes.append(prefix);ordinal+=1
            for ordinal,event in enumerate(actual['writes']):
                for phase in ('before_write','after_write'):
                    prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_{phase}_{ordinal}',
                        phase=phase,native_fault_ordinal=ordinal,fault=dict(phase=phase,ordinal=ordinal))
                    prefixes.append(prefix)
        prefix_scratch=scratch/'prefixes';prefix_scratch.mkdir()
        prefix_observed,prefix_authority=physical_original_dense(image,prefixes,prefix_scratch,
            entries=(0x30aa70,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        boundary_count=len(rows);rows+=prefixes;observed['cases']+=prefix_observed['cases'];executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual.get('boundary') and actual['boundary'].get('phase')=='alignment_fault':row['phase']='alignment_fault'
            assert actual['completed']==(row['phase']=='store_boundary')
            assert actual['live_x87_unchanged']
            if row['phase']=='store_boundary':assert actual['stopped_at']==0x30af16
            executed.update(actual['executed_sites'])
            row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=0x30af16 if row['phase']=='store_boundary' else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads'] if event['instruction'] not in (0x30aaa0,0x30aaa7)]
            if row['phase']=='alignment_fault':
                row['alignment_fault_address']=(actual['boundary_gp'][4]+0x90)&0xffffffff
                row['execution_eflags_before_fault']=actual['calls'][-1]['eflags']
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir()
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(vector_store LANGUAGES CXX)
include(CTest)
add_executable(vector_store "{ROOT/'tests/native_tu15_mopp_ray_vector_store_frame_tests.cpp'}")
target_include_directories(vector_store PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(vector_store PRIVATE cxx_std_20)
target_compile_options(vector_store PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME vector_store COMMAND vector_store "{fixture}")
'''
        if args.sanitizers:cmake+='target_compile_options(vector_store PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(vector_store PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),
                '--target','vector_store','--filter','^vector_store$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-4000:]
        report.update(passed=True,actual_alignment_faults=sum(row["phase"]=="alignment_fault" for row in rows),sanitizers=args.sanitizers,original_cases=len(rows),declared_entry_cases=boundary_count,
            retained_prefixes=len(prefixes),native_comparisons_per_compiler=boundary_count*4+len(prefixes)*2,
            complete_original_pre_io_packets=sum(len(row['calls']) for row in rows),executed_sites=sorted(executed),
            authority=authority,prefix_authority=prefix_authority,compiler_results=results)
    except BaseException as error:
        report['error']=str(error) or repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
