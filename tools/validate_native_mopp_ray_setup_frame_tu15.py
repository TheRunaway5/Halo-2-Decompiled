#!/usr/bin/env python3
"""Private complete actual 30B890 caller with a genuinely returned native child."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys

from oracle_smg_firing_1_1 import ROOT, Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense
from validate_native_mopp_ray_caller_frame_tu15 import FILES

REGIONS=((0x41c000,0x1000),(0x46a000,0x1000),(0x61000000,0x2000),(0x62000000,0x8000))
INPUTS=FILES+(
    'include/halo2/native_tu15_mopp_ray_setup_kernel.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_setup_frame.hpp',
    'tests/native_tu15_mopp_ray_frame_test_memory.hpp',
    'tests/native_tu15_mopp_ray_setup_frame_tests.cpp',
    'tools/validate_native_mopp_ray_setup_frame_tu15.py',
)


def hashes():
    return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}


def mutations(scratch,output,rows,io,compiler,sanitizers):
    project=scratch/('mutants-'+Path(compiler).name);project.mkdir()
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=[row for row in rows[:8]])))
    frame='src/physics/runtime/native_tu15_mopp_ray_setup_frame.hpp'
    kernel='include/halo2/native_tu15_mopp_ray_setup_kernel.hpp'
    defects=(
        ('inverse_multiply',frame,'asm volatile("fdivs %0"','asm volatile("fmuls %0"'),
        ('cached_end',kernel,'const auto current_end=view.vector(Vectors::current_end,d);','const auto current_end=d.end;'),
        ('wide_hit_store',frame,'event{pc,address,{},{std::byte(g[2])}};','event{pc,address,{},std::vector<std::byte>(4,std::byte(g[2]))};'),
    )
    wrapper=project/'reject.py';wrapper.write_text('''import subprocess,sys
r=subprocess.run(sys.argv[1:],capture_output=True,text=True)
assert r.returncode==1 and any(text in r.stderr for text in ("setup full packet differs", "setup ordered reads differ", "setup ordered writes differ", "setup store observer differs")),r.stdout+r.stderr
print("Rejected real setup defect")
''')
    cmake='cmake_minimum_required(VERSION 3.20)\nproject(setup_mutants LANGUAGES CXX)\ninclude(CTest)\n'
    for name,path,needle,replacement in defects:
        text=(ROOT/path).read_text();assert text.count(needle)==1,'Exact setup mutant seam drift '+name
        overlay=project/name;header=overlay/path;header.parent.mkdir(parents=True);header.write_text(text.replace(needle,replacement,1))
        cmake+=f'''add_executable({name} "{ROOT/'tests/native_tu15_mopp_ray_setup_frame_tests.cpp'}")
target_include_directories({name} PRIVATE "{overlay/'include'}" "{overlay/'src'}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({name} PRIVATE cxx_std_20)
target_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {name} COMMAND "{sys.executable}" "{wrapper}" $<TARGET_FILE:{name}> "{fixture}")
'''
        if sanitizers:cmake+=f'target_compile_options({name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({name} PRIVATE -fsanitize=address,undefined)\n'
    (project/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
        '--output',str(output/(Path(compiler).name+'-mutants')),'--filter','^(inverse_multiply|cached_end|wide_hit_store)$','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
    for name,*_ in defects:command+=['--target',name]
    result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-6000:]
    return dict(compiler=compiler,passed=True,rejected=[name for name,*_ in defects])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--semantic-mutants',action='store_true')
    parser.add_argument('--smoke',action='store_true')
    args=parser.parse_args()
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),
                supplied_child_outcomes=False,retained_large_artifacts=[],
                scope='Private actual30B890 full raw setup/ABI/faults and genuine native empty-geometry30AA70 child; no default/public facade change')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        owners=(0x30b890,0x30aa70);common.ENTRIES=owners;original=common.metadata;code=common._Code(image,'.text')
        def metadata(image):
            result=original(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if code.at(site['instruction']).mnemonic in ('fst','fstp'):
                    for operand in site['operands']:operand.update(read=False,write=True)
            return result
        common.metadata=metadata;meta=metadata(image)
        io=[site['instruction']for site in meta['sites']if site['instruction']not in (0x30aaa0,0x30aaa7)and
            (site['operands']or code.at(site['instruction']).mnemonic in ('push','pop','ret','call'))]
        rng=random.Random(0x30b890);rows=[]
        values=(0,0x80000000,1,0x00800000,0x3f800001,0xbf800000,0x3eaaaaab,0x7f7fffff,
                0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffc23456,0xff823456,0x40000000)
        live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040',
              '45230100000000c0ff7f','00000000000000800040','00000000000000c0ff3f','0000000000000080ffbf')
        for n in range(8 if args.smoke else 96):
            banks={base:bytearray(rng.randbytes(size))for base,size in REGIONS}
            for base in (0x41c000,0x46a000):banks[base][:]=image.read(base,0x1000)
            def put(at,value,width=4):
                base=next(base for base,size in REGIONS if base<=at and at+width<=base+size)
                banks[base][at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
            cache=0x61000200;shape_code=0x61000600;ray=0x61000900;collection=0x61000b00;context=0x61000c00;collector=0x61000d00
            mode=('ordinary','ray_cache','ray_code','collector_cache','ray_shifted_cache')[n%5]
            if mode=='ray_cache':ray=cache+0x20
            elif mode=='ray_code':ray=shape_code
            elif mode=='collector_cache':collector=cache+0x50
            elif mode=='ray_shifted_cache':ray=cache+0x10
            for lane in range(4):
                put(shape_code+lane*4,values[(n+lane)%16]);put(ray+lane*4,values[(n+lane+3)%16]);put(ray+16+lane*4,values[(n+lane+7)%16])
            put(shape_code+12,(0x3f800000,0x40000000,0x3f000000,0,0x7fc12345,0x7f812345,0x7f800000,1)[n%8])
            put(shape_code+0x30,0,1);put(collector+4,values[(n+4)%16]);put(ray+0x20,rng.getrandbits(32));put(ray+0x24,0)
            sp=0x62006000+n%16
            for index,value in enumerate((0x63000000,collection,shape_code,ray,context,collector)):put(sp+index*4,value)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=cache;gp[4]=sp;depth=(0,1,4,7)[n%4]
            rows.append(dict(id=n,name=f'setup_{n}_{mode}',entry=0x30b890,phase='normal',empty_child=True,
                gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|((n*0x101)&0x473f),
                           0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[(n//4)%4]|(n%64)],
                live_x87=list(live[:depth]),banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],
                matrix=collection,source=shape_code,max_steps=5000,stop_before_entries=[0x63000000]))
        traces=[dict(entry=pc,gp=True,xmm=True)for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=owners,regions=REGIONS,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        normals=len(rows);prefixes=[];prefix_authority=None
        if not args.smoke:
            for index in (0,3,7,11):
                row,actual=rows[index],observed['cases'][index];ordinal=0
                for physical_ordinal,event in enumerate(actual['reads']):
                    if event['instruction']in (0x30aaa0,0x30aaa7):continue
                    prefix=copy.deepcopy(row);prefix.update(id=normals+len(prefixes),name=row['name']+f'_read_{ordinal}',
                        phase='before_read',native_fault_ordinal=ordinal,fault=dict(phase='before_read',ordinal=physical_ordinal))
                    prefixes.append(prefix);ordinal+=1
                for ordinal,event in enumerate(actual['writes']):
                    for phase in ('before_write','after_write'):
                        prefix=copy.deepcopy(row);prefix.update(id=normals+len(prefixes),name=row['name']+f'_{phase}_{ordinal}',
                            phase=phase,native_fault_ordinal=ordinal,fault=dict(phase=phase,ordinal=ordinal));prefixes.append(prefix)
            prefix_scratch=scratch/'prefixes';prefix_scratch.mkdir()
            stopped,prefix_authority=physical_original_dense(image,prefixes,prefix_scratch,entries=owners,regions=REGIONS,
                trace_calls=traces,trace_extended=True)
            rows+=prefixes;observed['cases']+=stopped['cases']
        executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='normal')
            if row['phase']=='normal':
                assert actual['stopped_at']==0x63000000 and actual['live_x87_unchanged']
            executed.update(actual['executed_sites']);row.update({key:actual[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=actual['stopped_at']if row['phase']=='normal'else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads']if event['instruction']not in(0x30aaa0,0x30aaa7)]
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after']for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir();cmake=f'''cmake_minimum_required(VERSION 3.20)
project(mopp_setup LANGUAGES CXX)
include(CTest)
add_executable(mopp_setup "{ROOT/'tests/native_tu15_mopp_ray_setup_frame_tests.cpp'}")
target_include_directories(mopp_setup PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(mopp_setup PRIVATE cxx_std_20)
target_compile_options(mopp_setup PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME mopp_setup COMMAND mopp_setup "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/setup.json")
'''
        if args.sanitizers:cmake+='target_compile_options(mopp_setup PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(mopp_setup PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);compilers=[];mutants=[]
        for compiler in args.compiler or('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
                '--output',str(args.output/Path(compiler).name),'--target','mopp_setup','--filter','^mopp_setup$',
                '--retain-report','setup.json','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-6000:]
            compilers.append(dict(compiler=compiler,passed=True,observations=json.loads((args.output/Path(compiler).name/'evidence/setup.json').read_text())))
            if args.semantic_mutants:mutants.append(mutations(scratch,args.output,rows,io,compiler,args.sanitizers))
        parent_sites=set(common.complete_body(code,0x30b890).code)
        assert len(parent_sites)==66 and parent_sites<=executed
        report.update(passed=True,normal_paths=normals,prefixes=len(prefixes),original_paths=len(rows),executed_sites=sorted(executed),
            authority=authority,prefix_authority=prefix_authority,original_pre_io_packets=sum(len(row['calls'])for row in rows),
            compilers=compilers,semantic_mutants=mutants,parent_sites=len(parent_sites),parent_executed_sites=len(parent_sites&executed))
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_after']!=report['source_before']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
