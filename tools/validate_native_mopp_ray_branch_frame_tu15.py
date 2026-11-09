#!/usr/bin/env python3
"""Compare private native ray planes/branch decisions before crossing traversal; no production routing."""
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
from validate_native_mopp_ray_caller_frame_tu15 import FILES,REGIONS as CALLER_REGIONS
REGIONS=((0x46a000,0x1000),)+CALLER_REGIONS

BRANCH_FILES=FILES+(
    'include/halo2/native_tu15_mopp_ray_branch_kernel.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_branch_frame.hpp',
    'tests/native_tu15_mopp_ray_branch_frame_tests.cpp',
    'tools/validate_native_mopp_ray_branch_frame_tu15.py',
    'include/halo2/native_tu15_mopp_ray_branch_kernel.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_branch_frame.hpp',
)


def hashes():
    return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in BRANCH_FILES}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--pattern')
    args=parser.parse_args()
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),
                retained_large_artifacts=[],scope='Native split-plane and segment classification; selected next-opcode/crossing boundaries, no recursive/full-parent claim')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN
        image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x30aa70,)
        metadata=common.metadata(image)
        code=common._Code(image,'.text')
        for site in metadata['sites']:
            if code.at(site['instruction']).mnemonic in ('fst','fstp'):
                for operand in site['operands']:operand.update(read=False,write=True)
        io=[site['instruction'] for site in metadata['sites']
            if site['instruction'] not in (0x30aaa0,0x30aaa7) and
            (site['operands'] or code.at(site['instruction']).mnemonic in ('push','pop','ret','call'))]
        live=('0000000000000080ff3f','00000000000000000080',
              '35c26821a2da0fc90040','45230100000000c0ff7f',
              '00000000000000800040','00000000000000c0ff3f',
              '00000000000000000000','0000000000000080ffbf')
        rng=random.Random(0x30ade4);rows=[]
        bits=(0,0x80000000,1,0x007fffff,0x00800000,0x3f800001,0x7f7fffff,
              0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffc23456,
              0xff823456,0x3eaaaaab,0x80800000,0x807fffff)
        for opcode in list(range(0x10,0x1d))+list(range(0x20,0x26)):
            variants=[(f'basic_{n}',None,n) for n in range(16)]
            variants += [(f'bounds_alias_{offset}',offset,n) for n,offset in enumerate((-16,0,0x14,0x18,0x1c,0x20,0x24,0x3c,0x40,0x64))]
            for name,alias,n in variants:
                a=(0,1,64,127,128,170,254,255)[n%8];b=(255,254,170,128,127,64,1,0)[n%8]
                program=bytes([opcode,a,b,n%4,0,0,0,0]) if opcode<0x20 else bytes([opcode,a,n%4,0,0]) if opcode<0x23 else bytes([opcode,a,b,0,n%4,0,1+n%4,0,0,0,0,0])
                banks={base:bytearray(rng.randbytes(size)) for base,size in REGIONS}
                banks[0x46a000]=bytearray(image.read(0x46a000,0x1000))
                banks[0x61000000][0x100:0x100+len(program)]=program
                for lane in range(8):banks[0x61000000][0x900+lane*4:0x904+lane*4]=bits[(n+lane)%16].to_bytes(4,'little')
                ordinal=len(rows);sp=0x62006000+ordinal%16
                bounds=0x61000900 if alias is None else ((sp-4)&0xfffffff0)-0x194-12+alias
                arguments=[0x63000000,rng.getrandbits(32),0x61000100,bounds]
                for k,value in enumerate(arguments):
                    offset=sp-0x62000000+k*4;banks[0x62000000][offset:offset+4]=value.to_bytes(4,'little')
                gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=sp
                depth=(0,1,4,6)[(ordinal//16)%4]
                rows.append(dict(id=ordinal,name=f'plane_{opcode:x}_{name}',entry=0x30aa70,phase='branch_boundary',program_opcode=opcode,
                    gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                    xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],live_x87=list(live[:depth]),
                    before_fp=[(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10),
                               ((ordinal%8)<<11)|(ordinal&0x473f),
                               0x1f80|((ordinal%4)<<13)|(0,0x40,0x8000,0x8040)[ordinal%4]],
                    banks=[dict(address=base,before=bytes(data).hex()) for base,data in banks.items()],
                    matrix=arguments[1],source=arguments[2],bounds=arguments[3],max_steps=1000,
                    stop_before_entries=[0x30ae02,0x30ae75]))
        if args.pattern:
            rows=[row for row in rows if args.pattern in row['name']]
            assert rows,'No matching plane input'
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=(0x30aa70,),regions=REGIONS,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=False,allow_unbalanced_selected_stop=True)
        prefixes=[]
        prefix_indices=[];seen=set()
        for index,row in enumerate(rows):
            if index<16 or row['program_opcode'] not in seen:prefix_indices.append(index);seen.add(row['program_opcode'])
        for index in prefix_indices:
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
            entries=(0x30aa70,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=False,allow_unbalanced_selected_stop=True)
        boundary_count=len(rows);rows+=prefixes;observed['cases']+=prefix_observed['cases'];executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='branch_boundary')
            if row['phase']=='branch_boundary':assert actual['stopped_at'] in (0x30ae02,0x30ae75)
            executed.update(actual['executed_sites'])
            row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=actual['stopped_at'] if row['phase']=='branch_boundary' else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads'] if event['instruction'] not in (0x30aaa0,0x30aaa7)]
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir()
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(ray_branch LANGUAGES CXX)
include(CTest)
add_executable(ray_branch "{ROOT/'tests/native_tu15_mopp_ray_branch_frame_tests.cpp'}")
target_include_directories(ray_branch PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(ray_branch PRIVATE cxx_std_20)
target_compile_options(ray_branch PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME ray_branch COMMAND ray_branch "{fixture}")
'''
        if args.sanitizers:cmake+='target_compile_options(ray_branch PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(ray_branch PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),
                '--target','ray_branch','--filter','^ray_branch$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-4000:]
        report.update(passed=True,sanitizers=args.sanitizers,original_cases=len(rows),selected_branch_boundaries=boundary_count,
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
