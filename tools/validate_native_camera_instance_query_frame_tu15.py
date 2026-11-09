#!/usr/bin/env python3
"""Compare complete native MapPack1.1 1F1D20 polygon-support selection against unchanged original execution. Private and unbound."""
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
REGIONS=((0x4ec000,0x1000),)+CALLER_REGIONS

KEY_FILES=FILES+(
    'include/halo2/native_tu15_camera_instance_query_kernel.hpp',
    'src/physics/runtime/native_tu15_camera_instance_query_frame.hpp',
    'src/physics/runtime/native_tu15_camera_instance_admission.hpp',
    'tests/native_tu15_camera_instance_query_frame_tests.cpp',
    'tools/validate_native_camera_instance_query_frame_tu15.py',
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
                retained_large_artifacts=[],scope='Private16C000 metadata/admission/genuine sphere entry and complete rejection return; later controller ABI unqualified')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN
        image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x16c000,0x16b810)
        original_metadata=common.metadata;code=common._Code(image,'.text')
        def support_metadata(image):
            result=original_metadata(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=support_metadata;metadata=support_metadata(image)
        io=[site['instruction'] for site in metadata['sites']
            if site['instruction'] not in (0x30aaa0,0x30aaa7) and
            (site['operands'] or code.at(site['instruction']).mnemonic in ('push','pop','ret','call'))]
        live=('0000000000000080ff3f','00000000000000000080',
              '35c26821a2da0fc90040','45230100000000c0ff7f',
              '00000000000000800040','00000000000000c0ff3f',
              '00000000000000000000','0000000000000080ffbf')
        import struct
        rng=random.Random(0x16c000);rows=[]
        for index in (0,2,0xffffffff,0x12345):
            for definition_index in (0,1,0xffff,0x8000):
                for variant in range(8):
                    ordinal=len(rows);sp=0x62007000+ordinal%16
                    banks={base:bytearray(rng.randbytes(size))for base,size in REGIONS}
                    scenario=0x61000040;instance=0x61000400;definition=0x61000800;parts=0x61000b00;table=0x61000c00;mapping_entry=0x61000e00
                    def put(address,value,width=4):
                        base=next(base for base,size in REGIONS if base<=address and address+width<=base+size)
                        banks[base][address-base:address-base+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
                    put(0x4ecc18,scenario)
                    put(scenario+0x144,(instance-index*0x58)&0xffffffff)
                    signed=definition_index if definition_index<0x8000 else definition_index-0x10000
                    put(scenario+0x13c,(definition-signed*0xc8)&0xffffffff)
                    put(instance+0x34,definition_index,2);put(instance+0x48,0x42c80000)
                    count=(0,0xffffffff,1,1,1,1,1,1)[variant];mask=0 if variant<4 else 0x40000
                    put(definition+0x98,count);put(definition+0x9c,parts);put(parts+4,0x10 if variant==2 else 0,1)
                    put(scenario+0xc4,0 if variant==4 else 0xffffffff if variant==7 else 1);put(scenario+0xc8,table)
                    put(table+0x40,(index+1)&0xffffffff);put(table+0x44,(mapping_entry-index*4)&0xffffffff)
                    put(mapping_entry,0xffff if variant==5 else 0,2)
                    formals=(mask,1,0x61000f00,0x61000f20)
                    for n,value in enumerate((0x63000000,*formals)):put(sp+n*4,value)
                    gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=index;gp[3]=0x61000f40;gp[4]=sp
                    depth=(0,1,4,8)[ordinal%4]
                    rows.append(dict(id=ordinal,name=f'query_{index:x}_{definition_index:x}_{variant}',entry=0x16c000,phase='pending',gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],live_x87=list(live[:depth]),
                        before_fp=[(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10),((ordinal%8)<<11)|(ordinal&0x473f),0x1f80|((ordinal%4)<<13)],
                        banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],matrix=formals[0],source=formals[1],max_steps=5000,stop_before_entries=[0x63000000,0x120fa0]))
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=(0x16c000,0x16b810),regions=REGIONS,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        for index in (0,2,3,6,32,34,35,38):
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
            entries=(0x16c000,0x16b810),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        boundary_count=len(rows);rows+=prefixes;observed['cases']+=prefix_observed['cases'];executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            if row['phase']=='pending':row['phase']='query_return' if actual['stopped_at']==0x63000000 else 'sphere_entry'
            assert actual['completed']==(row['phase']in('query_return','sphere_entry'))
            assert actual['live_x87_unchanged']
            executed.update(actual['executed_sites'])
            row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=actual['stopped_at'] if row['phase']in('query_return','sphere_entry')else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads'] if event['instruction'] not in (0x30aaa0,0x30aaa7)]
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir()
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(polygon_support LANGUAGES CXX)
include(CTest)
add_executable(polygon_support "{ROOT/'tests/native_tu15_camera_instance_query_frame_tests.cpp'}")
target_include_directories(polygon_support PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(polygon_support PRIVATE cxx_std_20)
target_compile_options(polygon_support PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME polygon_support COMMAND polygon_support "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/frame.json")
'''
        if args.sanitizers:cmake+='target_compile_options(polygon_support PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(polygon_support PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),
                '--target','polygon_support','--filter','^polygon_support$','--retain-report','frame.json',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-4000:]
            results[-1].update(frame=json.loads((args.output/compiler/'evidence/frame.json').read_text()))
        report.update(passed=True,production_binding=False,sanitizers=args.sanitizers,original_cases=len(rows),declared_entry_cases=boundary_count,
            retained_prefixes=len(prefixes),native_comparisons_per_compiler=sum(4 if row["phase"] in ("query_return","sphere_entry") else 2 for row in rows),
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
