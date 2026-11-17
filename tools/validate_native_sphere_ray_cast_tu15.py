#!/usr/bin/env python3
"""Original-i386 qualification of the private native sphere ray caster."""
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

from oracle_smg_firing_1_1 import ROOT, Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense

REGIONS=((0x41c000,0x1000),(0x46a000,0x1000),(0x61000000,0x2000),(0x62000000,0x8000))
FILES=('include/halo2/native_tu15_sphere_ray_cast_kernel.hpp',
       'src/physics/runtime/native_tu15_sphere_ray_cast_frame.hpp',
       'src/physics/runtime/native_tu15_raw_x87_scope.hpp',
       'src/physics/runtime/native_tu15_mopp_ray_integer_frame.hpp',
       'src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
       'include/halo2/native_tu15_mopp_ray_traversal_kernel.hpp',
       'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
       'tests/native_tu15_sphere_ray_cast_tests.cpp',
       'tools/validate_native_sphere_ray_cast_tu15.py',
       'tools/native_sphere_ray_cast_mutations.py',
       'tools/native_mopp_ray_dense_original.py')
def hashes():return {n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in FILES}

def cases(image,smoke):
    rng=random.Random(0x2e8dd0);rows=[]
    geometry=(
        ((-2,0,0),(2,0,0),1),((-2,0,0),(-1.5,0,0),1),
        ((-2,0,0),(-3,0,0),1),((0,0,0),(2,0,0),1),
        ((-2,1,0),(2,1,0),1),((-2,2,0),(2,2,0),1),
        ((-1000,0,0),(1000,0,0),1),((-2,0.1,0),(2,0.1,0),1),
        ((-2,0,0),(-2,0,0),1),((-2,0,0),(2,0,0),-1),
        ((-2,0,0),(2,0,0),0),((-2,0,0),(2,0,0),2),
    )
    live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000800040')
    for n in range(12 if smoke else 320):
        banks={base:bytearray(rng.randbytes(size))for base,size in REGIONS}
        for page in (0x41c000,0x46a000):banks[page][:]=image.read(page,0x1000)
        def put(at,data):
            base=next(base for base,size in REGIONS if base<=at and at+len(data)<=base+size)
            banks[base][at-base:at-base+len(data)]=data
        def word(at,value):put(at,struct.pack('<I',value&0xffffffff))
        shape=0x61000200;ray=0x61000400;out=0x61000600;boolean=0x61000800;sp=0x62006000+n%16
        start,end,radius=geometry[n%len(geometry)];mode=n//len(geometry)
        alias='separate'if smoke else('separate','out_ray','out_shape','bool_out','out_locals')[mode%5]
        if alias=='out_ray':out=ray
        elif alias=='out_shape':out=shape
        elif alias=='bool_out':boolean=out+3
        elif alias=='out_locals':
            # Keep all24 result bytes below the saved EBP even at an
            # unrounded incoming SP; the tail still aliases live locals.
            out=((sp-4)&~15)-32
        for lane,value in enumerate((*start,1.0,*end,-1.0)):put(ray+lane*4,struct.pack('<f',value))
        put(shape+12,struct.pack('<f',radius))
        if n>=192:
            edges=(0,0x80000000,1,0x80000001,0x00800000,0x3f800001,0xbf800001,0x7f7fffff,
                   0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffc23456,0xff823456,0x3eaaaaab)
            word((ray,ray+16,shape+12)[(n//16)%3],edges[n%16])
        for i,value in enumerate((0x63000000,boolean,ray,out)):word(sp+i*4,value)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=shape;gp[4]=sp
        depth=0 if smoke else(0,1,4,5)[(n//3)%4]
        rows.append(dict(id=n,name=f'sphere_{n}_geometry{n%len(geometry)}_{alias}',entry=0x2e8dd0,phase='normal',
            gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            before_fp=[(0x7f,0x27f,0x37f)[mode%3]|(((mode//3)%4)<<10),((mode%8)<<11)|((n*0x101)&0x473f),0x1f80|(((mode//3)%4)<<13)|(n%64)],
            live_x87=list(live[:depth]),banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],
            matrix=boolean,source=ray,max_steps=1000,stop_before_entries=[0x63000000]))
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True);parser.add_argument('--compiler',action='append')
    parser.add_argument('--smoke',action='store_true');parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if args.smoke and args.semantic_mutants:parser.error('Sphere mutations require full alias/branch inputs')
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        owners=(0x2e8dd0,);common.ENTRIES=owners;original=common.metadata;code=common._Code(image,'.text')
        def metadata(image):
            result=original(image)
            for site in result['sites']:
                op=code.at(site['instruction']).mnemonic
                if op in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if op in('fst','fstp'):
                    for operand in site['operands']:operand.update(read=False,write=True)
            return result
        common.metadata=metadata;meta=metadata(image)
        io=[s['instruction']for s in meta['sites']if s['operands']or code.at(s['instruction']).mnemonic in('push','pop','ret')]
        traces=[dict(entry=pc,gp=True,xmm=True)for pc in io];rows=cases(image,args.smoke)
        observed,authority=physical_original_dense(image,rows,scratch,entries=owners,regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        normal_count=len(rows);prefixes=[];prefix_authority=None
        if not args.smoke:
            for index in range(12):
                row,actual=rows[index],observed['cases'][index]
                for ordinal,event in enumerate(actual['reads']):
                    p=copy.deepcopy(row);p.update(id=normal_count+len(prefixes),name=row['name']+f'_read{ordinal}',phase='before_read',native_fault_ordinal=ordinal,fault=dict(phase='before_read',ordinal=ordinal));prefixes.append(p)
                for ordinal,event in enumerate(actual['writes']):
                    for phase in('before_write','after_write'):
                        p=copy.deepcopy(row);p.update(id=normal_count+len(prefixes),name=row['name']+f'_{phase}{ordinal}',phase=phase,native_fault_ordinal=ordinal,fault=dict(phase=phase,ordinal=ordinal));prefixes.append(p)
            sub=scratch/'prefixes';sub.mkdir();stopped,prefix_authority=physical_original_dense(image,prefixes,sub,entries=owners,regions=REGIONS,trace_calls=traces,trace_extended=True)
            rows+=prefixes;observed['cases']+=stopped['cases']
        executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='normal')
            if row['phase']=='normal':assert actual['stopped_at']==0x63000000 and actual['live_x87_unchanged']
            executed.update(actual['executed_sites']);row.update({k:actual[k]for k in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=actual['stopped_at']if row['phase']=='normal'else actual['boundary']['instruction'];row['reads']=actual['reads']
            for packet in row['calls']:packet.pop('arguments')
            after={b['address']:b['after']for b in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir();cmake=f'''cmake_minimum_required(VERSION 3.20)
project(sphere_ray LANGUAGES CXX)
include(CTest)
add_executable(sphere_ray "{ROOT/'tests/native_tu15_sphere_ray_cast_tests.cpp'}")
target_include_directories(sphere_ray PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(sphere_ray PRIVATE cxx_std_20)
target_compile_options(sphere_ray PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME sphere_ray COMMAND sphere_ray "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/sphere.json")
'''
        if args.sanitizers:cmake+='target_compile_options(sphere_ray PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(sphere_ray PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);compilers=[];mutants=[]
        report.update(original_paths=len(rows),normal_paths=normal_count,prefixes=len(prefixes),executed_sites=sorted(executed),authority=authority,prefix_authority=prefix_authority)
        for compiler in args.compiler or('g++','clang++'):
            cmd=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),'--output',str(args.output/Path(compiler).name),'--target','sphere_ray','--filter','^sphere_ray$','--retain-report','sphere.json','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(cmd,capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-6000:]
            compilers.append(dict(compiler=compiler,passed=True,observations=json.loads((args.output/Path(compiler).name/'evidence/sphere.json').read_text())))
            if args.semantic_mutants:
                from native_sphere_ray_cast_mutations import qualify
                mutants.append(qualify(scratch,args.output,rows,io,compiler,args.sanitizers))
        parent_sites=set(common.complete_body(code,0x2e8dd0).code)
        report.update(passed=True,compilers=compilers,semantic_mutants=mutants,parent_sites=len(parent_sites),parent_executed_sites=len(parent_sites&executed))
        if not args.smoke:assert len(parent_sites)==147 and parent_sites<=executed
    except BaseException as error:report.update(passed=False,error=str(error)or repr(error));raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
