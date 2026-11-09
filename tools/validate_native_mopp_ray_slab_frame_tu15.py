#!/usr/bin/env python3
"""Qualify the private native ray slab decode/clipping against unchanged i386."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

from oracle_smg_firing_1_1 import ROOT,Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense
from validate_native_mopp_ray_caller_frame_tu15 import FILES,REGIONS as CALLER_REGIONS
REGIONS=((0x41c000,0x1000),(0x46a000,0x1000))+CALLER_REGIONS
SLAB_FILES=FILES+(
    'include/halo2/native_tu15_mopp_ray_slab_kernel.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_slab_frame.hpp',
    'tests/native_tu15_mopp_ray_slab_frame_tests.cpp',
    'tools/validate_native_mopp_ray_slab_frame_tu15.py',
    'tools/native_mopp_ray_slab_mutations.py',
    'src/physics/runtime/native_tu15_mopp_ray_controller.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_plane_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_branch_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_crossing_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_recursion_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_resume_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_primitive_key_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_leaf_frame.hpp',
)

def hashes():
    return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in SLAB_FILES}

def corpus(image,smoke=False):
    rng=random.Random(0x30b17d);rows=[]
    live=('0000000000000080ff3f','00000000000000000080',
          '35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000800040')
    pairs=((-5,15),(15,-5),(1,9),(9,1),(-5,-1),(11,15),(-1,-5),(15,11),(0,10),(10,0))
    bits=(0,0x80000000,1,0x007fffff,0x00800000,0x3f800001,0x7f7fffff,
          0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffc23456,
          0xff823456,0x3eaaaaab,0x80800000,0x807fffff)
    for opcode in range(0x26,0x2c):
        axis=(opcode-0x26)%3;wide=opcode>=0x29
        variants=[(f'basic_{n}',None,n) for n in range(2 if smoke else 64)]
        if not smoke:variants += [(f'bounds_alias_{offset:x}',offset,n) for n,offset in enumerate((0,0x10,0x20,0x30,0x40,0x60))]
        if not smoke:
            variants += [(f'unaligned_{n}','unaligned',n) for n in range(1,16)]
            variants += [(name,name,0) for name in ('program_frame','variables_frame','cache_frame','bounds_variables','bounds_constant','program_bounds','dual_nan')]
        for name,alias,n in variants:
            banks={base:bytearray(rng.randbytes(size)) for base,size in REGIONS}
            for base in (0x41c000,0x46a000):banks[base]=bytearray(image.read(base,0x1000))
            ordinal=len(rows);sp=0x62006000+ordinal%16
            frame=((sp-4)&0xfffffff0)-0x194-12
            bounds=frame+alias if isinstance(alias,int) else 0x61000900
            variables=0x61000200;program_address=0x61000100;cache=0x61000300
            if alias=='unaligned':bounds+=n
            elif alias=='program_frame':program_address=frame+0x10
            elif alias=='variables_frame':variables=frame
            elif alias=='cache_frame':cache=frame
            elif alias=='bounds_variables':bounds=variables
            elif alias=='bounds_constant':bounds=0x41c500
            elif alias=='program_bounds':program_address=bounds
            low,high=(1,10)
            if wide and name.startswith('basic_') and n>=10:
                values=(0,0xff,0x100,0x8000,0x123456,0xfe0001,0xffffff)
                low=values[(n-10)%len(values)];high=values[(n-8)%len(values)]
            program=bytes([opcode])+low.to_bytes(3,'big')+high.to_bytes(3,'big')+bytes([0]) if wide else bytes([opcode,low,high,0])
            def put(address,value):
                base=next(base for base,size in REGIONS if base<=address<base+size)
                banks[base][address-base:address-base+4]=int(value).to_bytes(4,'little')
            def fp(address,value):put(address,struct.unpack('<I',struct.pack('<f',value))[0])
            for lane in range(8):put(bounds+lane*4,bits[(n+lane)%16])
            start,end=pairs[0 if alias=="unaligned" else n%len(pairs)]
            fp(bounds+axis*4,start);fp(bounds+16+axis*4,end)
            if name.startswith('basic_') and n>=32:
                put(bounds+axis*4,bits[(n-32)%len(bits)]);put(bounds+16+axis*4,bits[(n-27)%len(bits)])
            if name=='dual_nan':put(bounds+12,0x7fc12345);put(bounds+28,0xffc54321)
            for lane in range(3):fp(variables+lane*4,0)
            fp(variables+0x14,2);fp(cache+0x14,.5)
            if wide and name.startswith('basic_') and n>=20:
                put(variables+0x14,bits[(n-20)%len(bits)]);put(cache+0x14,bits[(n-15)%len(bits)])
                put(variables+axis*4,bits[(n-12)%len(bits)])
            base=next(base for base,size in REGIONS if base<=program_address<base+size)
            banks[base][program_address-base:program_address-base+len(program)]=program
            arguments=[0x63000000,variables,program_address,bounds]
            for k,value in enumerate(arguments):put(sp+k*4,value)
            gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=sp;gp[1]=cache
            depth=(0,1,4,5)[(ordinal//6)%4]
            rows.append(dict(id=ordinal,name=f'slab_{opcode:x}_{name}',entry=0x30aa70,phase='slab_boundary',program_opcode=opcode,
                gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],
                live_x87=list(live[:depth]),before_fp=[(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10),
                ((ordinal%8)<<11)|(ordinal&0x473f),0x1f80|((ordinal%4)<<13)|(0,0x40,0x8000,0x8040)[ordinal%4]],
                banks=[dict(address=base,before=bytes(data).hex()) for base,data in banks.items()],
                matrix=variables,source=program_address,bounds=bounds,max_steps=1000,stop_before_entries=[0x30ae02]))
            if alias=='unaligned':rows[-1].update(phase='alignment_fault',alignment_fault=True)
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--smoke',action='store_true')
    parser.add_argument('--negative-controls',action='store_true')
    parser.add_argument('--pattern');parser.add_argument('--compiler',action='append',choices=('g++','clang++'))
    args=parser.parse_args()
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[],
                scope='Native slab decode/clipping, real reject return or next-opcode numerical boundary; no full-parent claim')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x30aa70,);original_metadata=common.metadata;code=common._Code(image,'.text')
        def metadata(image):
            result=original_metadata(image)
            for site in result['sites']:
                mnemonic=code.at(site['instruction']).mnemonic
                if mnemonic in ('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if mnemonic in ('fst','fstp','fist','fistp','fisttp'):
                    for operand in site['operands']:operand.update(read=False,write=True)
            return result
        common.metadata=metadata;sites=metadata(image)['sites']
        io=[site['instruction'] for site in sites if site['instruction'] not in (0x30aaa0,0x30aaa7) and
            (site['operands'] or code.at(site['instruction']).mnemonic in ('push','pop','ret','call'))]
        rows=corpus(image,args.smoke)
        if args.pattern:rows=[row for row in rows if args.pattern in row['name']]
        assert rows,'No matching slab input'
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=(0x30aa70,),regions=REGIONS,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=False,allow_unbalanced_selected_stop=True)
        prefixes=[]
        if not args.smoke:
            indices=[i for i,row in enumerate(rows) if row['name'].endswith(('basic_0','basic_1','basic_4','basic_5'))]
            for index in indices:
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
        prefix_authority=None
        if prefixes:
            prefix_scratch=scratch/'prefixes';prefix_scratch.mkdir()
            prefix_observed,prefix_authority=physical_original_dense(image,prefixes,prefix_scratch,entries=(0x30aa70,),regions=REGIONS,
                trace_calls=traces,trace_extended=True,preserve_x87_prefix=False,allow_unbalanced_selected_stop=True)
            observed['cases']+=prefix_observed['cases']
        boundary_count=sum(row['phase']=='slab_boundary' for row in rows);alignment_count=sum(row['phase']=='alignment_fault' for row in rows)
        rows+=prefixes;executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='slab_boundary'),row['name']
            if row['phase']=='slab_boundary':assert actual['stopped_at'] in (0x30ae02,0x63000000),row['name']
            row['returned']=row['phase']=='slab_boundary' and actual['stopped_at']==0x63000000
            executed.update(actual['executed_sites'])
            row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=(0x30b6b5 if row['returned'] else 0x30ae02) if row['phase']=='slab_boundary' else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads'] if event['instruction'] not in (0x30aaa0,0x30aaa7)]
            if row['phase']=='alignment_fault':
                assert code.at(row['expected_pc']).mnemonic=='movaps'
                operands=next(site for site in sites if site['instruction']==row['expected_pc'])['operands']
                assert len(operands)==1 and operands[0]['read'] and operands[0]['bytes']==16
                operand=operands[0]
                row['alignment_fault_address']=(row['boundary_gp'][operand['base']]+operand['displacement'])&0xffffffff
                row['execution_eflags_before_fault']=row['calls'][-1]['eflags']
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        slab_sites={site['instruction'] for site in sites if 0x30b17d<=site['instruction']<=0x30b323}
        assert slab_sites<=executed,'Unqualified source slab instructions: '+','.join(hex(pc) for pc in sorted(slab_sites-executed))
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir()
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(ray_slab LANGUAGES CXX)
include(CTest)
add_executable(ray_slab "{ROOT/'tests/native_tu15_mopp_ray_slab_frame_tests.cpp'}")
target_include_directories(ray_slab PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(ray_slab PRIVATE cxx_std_20)
target_compile_options(ray_slab PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME ray_slab COMMAND ray_slab "{fixture}")
'''
        if args.sanitizers:cmake+='target_compile_options(ray_slab PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(ray_slab PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in args.compiler or ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
                '--output',str(args.output/compiler),'--target','ray_slab','--filter','^ray_slab$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-7000:]
        controls=None
        if args.negative_controls:
            from native_mopp_ray_slab_mutations import qualify
            controls=qualify(scratch,args.output,rows,io)
        report.update(passed=True,negative_controls=controls,direct_and_composed=True,slab_source_sites=len(slab_sites),sanitizers=args.sanitizers,original_cases=len(rows),slab_boundaries=boundary_count,
            real_returns=sum(row['returned'] for row in rows),retained_prefixes=len(prefixes),actual_alignment_faults=alignment_count,
            native_comparisons_per_compiler=boundary_count*8+(len(prefixes)+alignment_count)*4,complete_original_pre_io_packets=sum(len(row['calls']) for row in rows),
            executed_sites=sorted(executed),authority=authority,prefix_authority=prefix_authority,compiler_results=results)
    except BaseException as error:report['error']=str(error) or repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
