#!/usr/bin/env python3
"""Compare native post-child interpolation, cache shortening and real early returns; no production routing."""
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
REGIONS=((0x41c000,0x1000),(0x46a000,0x1000))+CALLER_REGIONS

CROSSING_FILES=FILES+(
    'src/physics/runtime/native_tu15_mopp_ray_crossing_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_recursion_frame.hpp',
    'include/halo2/native_tu15_mopp_ray_crossing_kernel.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_resume_frame.hpp',
    'include/halo2/native_tu15_mopp_ray_resume_kernel.hpp',
    'tests/native_tu15_mopp_ray_resume_frame_tests.cpp',
    'tools/validate_native_mopp_ray_resume_frame_tu15.py',
    'src/physics/runtime/native_tu15_mopp_ray_controller.hpp',
    'include/halo2/native_tu15_mopp_ray_leaf_kernel.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_leaf_frame.hpp',
    'tests/native_tu15_mopp_ray_controller_tests.cpp',
)


def hashes():
    return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in CROSSING_FILES}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--pattern')
    parser.add_argument('--composed',action='store_true')
    args=parser.parse_args()
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),
                retained_large_artifacts=[],scope='Native post-child bounds interpolation/cache shortening; genuine native child and parent early returns, no full-parent claim')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN
        image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x30aa70,)
        original_metadata=common.metadata
        code=common._Code(image,'.text')
        def recursive_metadata(image):
            result=original_metadata(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if code.at(site['instruction']).mnemonic in ('fst','fstp'):
                    for operand in site['operands']:operand.update(read=False,write=True)
            return result
        common.metadata=recursive_metadata
        metadata=recursive_metadata(image)
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
            variants=[(f'cross_{direction}_{n}',None,16+n+8*direction) for direction in range(2) for n in range(8)]
            # A shortened endpoint beyond the high plane must continue after
            # the real child. The old fractions covered full/early-return and
            # unordered paths, but skipped Original's 30B01D continuation.
            if opcode in (0x10,0x11,0x12):
                variants += [(f'cross_0_continuation_{n}',None,16+n) for n in range(2)]
            for name,alias,n in variants:
                a=(0,1,64,127,128,170,254,255)[n%8];b=(255,254,170,128,127,64,1,0)[n%8]
                if name.startswith('cross_'):a,b=64,192
                program=(bytes([opcode,a,b,7]) if opcode<0x20 else bytes([opcode,a,7]) if opcode<0x23 else bytes([opcode,a,b,0,7,0,11]))+bytes(32)
                banks={base:bytearray(rng.randbytes(size)) for base,size in REGIONS}
                banks[0x46a000]=bytearray(image.read(0x46a000,0x1000))
                banks[0x41c000]=bytearray(image.read(0x41c000,0x1000))
                banks[0x61000000][0x100:0x100+len(program)]=program
                for lane in range(8):banks[0x61000000][0x900+lane*4:0x904+lane*4]=bits[(n+lane)%16].to_bytes(4,'little')
                if name.startswith('cross_'):
                    first_axis={0x13:2,0x14:1,0x15:2,0x16:0,0x17:1,0x18:0,0x19:2,0x1a:1,0x1b:0,0x1c:0}.get(opcode,
                        opcode-0x10 if opcode<0x20 else opcode-0x20 if opcode<0x23 else opcode-0x23)
                    reverse=int(name.split('_')[1]);start=[0,0,0,0x3f800000];end=[0,0,0,0xbf800000]
                    start[first_axis]=0x447a0000 if reverse else 0xc47a0000
                    end[first_axis]=0xc47a0000 if reverse else 0x447a0000
                    for lane,value in enumerate(start+end):banks[0x61000000][0x900+lane*4:0x904+lane*4]=value.to_bytes(4,'little')
                ordinal=len(rows);sp=0x62006000+ordinal%16
                bounds=0x61000900 if alias is None else ((sp-4)&0xfffffff0)-0x194-12+alias
                variables=0x61000b00;cache=0x61000c00;code_info=0x61000d00
                for lane,value in enumerate([0,0,0,0,0,0x3f800000,0,0]):
                    banks[0x61000000][0xb00+lane*4:0xb04+lane*4]=value.to_bytes(4,'little')
                banks[0x61000000][0xc10:0xc14]=code_info.to_bytes(4,'little')
                banks[0x61000000][0xc20:0xc40]=banks[0x61000000][0x900:0x920]
                fraction=(0x3f800000,0x3f000000,0,0xbf800000,0x7fc12345,0x7f800000,1,0x80000000)[n%8]
                if '_continuation_' in name:
                    fraction=(0x3f400000,0x3f666666)[n%2]
                banks[0x61000000][0xc54:0xc58]=fraction.to_bytes(4,'little')
                banks[0x61000000][0xd00:0xd10]=bytes(16)
                arguments=[0x63000000,variables,0x61000100,bounds]
                for k,value in enumerate(arguments):
                    offset=sp-0x62000000+k*4;banks[0x62000000][offset:offset+4]=value.to_bytes(4,'little')
                gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=sp;gp[1]=cache
                depth=(0,1,4,6)[(ordinal//16)%4]
                rows.append(dict(id=ordinal,name=f'plane_{opcode:x}_{name}',entry=0x30aa70,phase='crossing_boundary',program_opcode=opcode,
                    gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                    xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],live_x87=list(live[:depth]),
                    before_fp=[(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10),
                               ((ordinal%8)<<11)|(ordinal&0x473f),
                               0x1f80|((ordinal%4)<<13)|(0,0x40,0x8000,0x8040)[ordinal%4]],
                    banks=[dict(address=base,before=bytes(data).hex()) for base,data in banks.items()],
                    matrix=arguments[1],source=arguments[2],bounds=arguments[3],max_steps=1000,
                    stop_before_entries=[0x30ae02,0x63000000],alignment_fault=bool(bounds&15)))
        if args.pattern:
            rows=[row for row in rows if args.pattern in row['name']]
            assert rows,'No matching plane input'
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=(0x30aa70,),regions=REGIONS,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=False,allow_unbalanced_selected_stop=True)
        prefixes=[]
        prefix_indices=[];seen=set()
        for index,row in enumerate(rows):
            role=(row['program_opcode'],'cross_up' if '_cross_0_' in row['name'] else 'cross_down' if '_cross_1_' in row['name'] else 'other')
            if index<16 or role not in seen:prefix_indices.append(index);seen.add(role)
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
        # Keep the shared physical runner's finite per-process budget. Every
        # declared prefix still runs unchanged; batching changes no input or
        # instruction/outcome and is useful while the engine compiles too.
        prefix_observed={'cases':[]};prefix_authority=[]
        for first in range(0,len(prefixes),256):
            prefix_scratch=scratch/f'prefixes-{first}';prefix_scratch.mkdir()
            batch,authority_batch=physical_original_dense(image,prefixes[first:first+256],prefix_scratch,
                entries=(0x30aa70,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=False,allow_unbalanced_selected_stop=True)
            prefix_observed['cases']+=batch['cases'];prefix_authority.append(authority_batch)
        boundary_count=len(rows);rows+=prefixes;observed['cases']+=prefix_observed['cases'];executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual.get('boundary') and actual['boundary'].get('phase')=='alignment_fault':row['phase']='alignment_fault'
            assert actual['completed']==(row['phase']=='crossing_boundary')
            if row['phase']=='crossing_boundary':assert actual['stopped_at'] in (0x30ae02,0x63000000)
            executed.update(actual['executed_sites'])
            row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=actual['stopped_at'] if row['phase']=='crossing_boundary' else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads'] if event['instruction'] not in (0x30aaa0,0x30aaa7)]
            if row['phase']=='alignment_fault':
                site=next(site for site in metadata['sites'] if site['instruction']==row['expected_pc'])
                operands=site['operands'];assert code.at(row['expected_pc']).mnemonic=='movaps' and len(operands)==1
                operand=operands[0];assert operand['bytes']==16 and operand['read']
                row['alignment_fault_address']=(actual['boundary_gp'][operand['base']]+operand['displacement'])&0xffffffff
                row['execution_eflags_before_fault']=actual['calls'][-1]['eflags']
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir()
        driver=ROOT/'tests'/('native_tu15_mopp_ray_controller_tests.cpp' if args.composed else 'native_tu15_mopp_ray_resume_frame_tests.cpp')
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(ray_resume LANGUAGES CXX)
include(CTest)
add_executable(ray_resume "{driver}")
target_include_directories(ray_resume PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(ray_resume PRIVATE cxx_std_20)
target_compile_options(ray_resume PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME ray_resume COMMAND ray_resume "{fixture}")
'''
        if args.sanitizers:cmake+='target_compile_options(ray_resume PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(ray_resume PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),
                '--target','ray_resume','--filter','^ray_resume$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-4000:]
        report.update(passed=True,composed_native_parent=args.composed,supplied_child_results=False,native_child_owner="shared native empty-geometry caller frame",sanitizers=args.sanitizers,original_cases=len(rows),declared_entry_cases=boundary_count,
            selected_crossing_boundaries=sum(row['phase']=='crossing_boundary' for row in rows),
            selected_boundary_pcs={hex(pc):sum(row['phase']=='crossing_boundary' and row['expected_pc']==pc for row in rows) for pc in (0x30ae02,0x63000000)},
            actual_alignment_faults=sum(row['phase']=='alignment_fault' for row in rows),
            retained_prefixes=len(prefixes),native_comparisons_per_compiler=sum(4 if row['phase']=='crossing_boundary' else 2 for row in rows),
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
