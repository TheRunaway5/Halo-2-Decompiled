#!/usr/bin/env python3
"""Qualify complete private16C000 DATA/raw frame with genuine live child algorithms."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import re
import subprocess
import sys

from oracle_smg_firing_1_1 import ROOT,Xbe
from validate_native_tu15_prt_helpers import PIN
from native_mopp_ray_dense_original import physical_original_dense
from validate_native_mopp_ray_caller_frame_tu15 import FILES,REGIONS as CALLER_REGIONS
REGIONS=()

KEY_FILES=FILES+(
    'include/halo2/native_tu15_camera_instance_query_kernel.hpp',
    'src/physics/runtime/native_tu15_camera_instance_query_frame.hpp',
    'src/physics/runtime/native_tu15_camera_instance_admission.hpp',
    'src/physics/runtime/native_tu15_camera_instance_bitmap.hpp',
    'src/physics/runtime/native_tu15_camera_collision_publication.hpp',
    'src/physics/runtime/native_tu15_segment_sphere.hpp',
    'include/halo2/native_tu15_segment_sphere_kernel.hpp',
    'tools/native_camera_instance_query_children.py',
    'tools/native_camera_instance_query_inputs.py',
    'tools/native_camera_instance_query_replacements.py',
    'src/physics/physics_live_support_tu15_generated.cpp',
    'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
    'src/runtime/source_map_world_support_tu15_generated.cpp',
    'tests/native_tu15_camera_instance_query_complete_tests.cpp',
    'tests/native_tu15_camera_instance_query_admission_tests.cpp',
    'tests/native_tu15_camera_instance_query_backend_tests.cpp',
    'tools/validate_native_camera_instance_query_complete_tu15.py',
)


def hashes():
    result={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in KEY_FILES}
    # This probe copies only the authenticated inverse owner and its resolver
    # from World. Unrelated live simplex-owner edits cannot change that copy.
    name='src/physics/runtime/physics_world_runtime_generated.cpp'
    source=(ROOT/name).read_text()
    patterns={'run_142d00':r'static void run_142d00\(ObservedWorldState &s\) \{.*?\n\}',
              'selected':r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}'}
    for symbol,pattern in patterns.items():
        result[name+'::'+symbol]=hashlib.sha256(re.search(pattern,source,re.S)[0].encode()).hexdigest()
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--semantic-mutants',action='store_true')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),
                retained_large_artifacts=[],scope='Complete16C000 DATA/raw frame with genuine native admission/sphere/bitmap/BSP/publication and retained inverse; default binding verified when requested')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN
        image=Xbe(args.xbe)
        import validate_native_camera_instanced_query_tu15 as composition
        selected=[c for c in composition.cases(image)if c['entry']==0x16c000 and not c['name'].startswith('fault_seed_')]
        pages={p for c in selected for b in c['banks'] for p in range(b['address']&~0xfff,(b['address']+len(bytes.fromhex(b['before']))+0xfff)&~0xfff,0x1000)}
        pages.update(range(0x2000000,0x2010000,0x1000));pages.add(0x46a000)
        regions=[]
        for p in sorted(pages):
            if regions and regions[-1][0]+regions[-1][1]==p:regions[-1]=(regions[-1][0],regions[-1][1]+0x1000)
            else:regions.append((p,0x1000))
        regions=tuple(regions)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=composition.OWNERS
        original_metadata=common.metadata;code=common._Code(image,'.text')
        def support_metadata(image):
            result=original_metadata(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=support_metadata;metadata=support_metadata(image)
        io=[site['instruction'] for site in metadata['sites']
            if site['instruction'] not in (0x30aaa0,0x30aaa7,0x1441b7) and
            (site['operands'] or code.at(site['instruction']).mnemonic in ('push','pop','ret','call'))]
        live=('0000000000000080ff3f','00000000000000000080',
              '35c26821a2da0fc90040','45230100000000c0ff7f',
              '00000000000000800040','00000000000000c0ff3f',
              '00000000000000000000','0000000000000080ffbf')
        rng=random.Random(0x16c074);entries=[]
        for ordinal,c in enumerate(selected):
            banks={base:bytearray(size)for base,size in regions}
            try:banks[0x46a000][:]=image.read(0x46a000,len(banks[0x46a000]))
            except KeyError:pass
            def put(address,data):
                base=next(base for base,size in regions if base<=address and address+len(data)<=base+size)
                banks[base][address-base:address-base+len(data)]=data
            for bank in c['banks']:put(bank['address'],bytes.fromhex(bank['before']))
            sp=0x200f000+ordinal%16
            for n,value in enumerate((0x63000000,*c['arguments'])):put(sp+n*4,int(value).to_bytes(4,'little'))
            gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=c['entry_registers']['eax'];gp[3]=c['entry_registers']['ebx'];gp[4]=sp
            entries.append(dict(id=ordinal,name=c['name'],entry=0x16c000,phase='query_return',gp=gp,eflags=0x202,
                xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],live_x87=[],before_fp=c['before_fp'],
                banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],
                return_pc=0x63000000,matrix=c['arguments'][0],source=c['arguments'][1],max_steps=10000,stop_before_entries=[0x63000000]))
        rows=entries;actual_parent_captures=len(rows)
        # The BSP subtree owns at most two additional x87 slots. Exercise the
        # complete parent with genuine unchanged-i386 live caller values in
        # every admitted dense occupancy class, rather than infer child ABI.
        for seed in entries[:]:
            for depth in (1,4,6):
                row=copy.deepcopy(seed);row.update(id=len(rows),name=seed['name']+f'_depth{depth}',live_x87=list(live[:depth]))
                rows.append(row)
        from native_camera_instance_query_children import prepare
        child_receipts=prepare(ROOT,scratch)
        from native_camera_instance_query_inputs import prepare as prepare_routes
        source_routes=prepare_routes(ROOT,scratch,args.require_binding)
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=composition.OWNERS,regions=regions,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        for index in range(min(4,len(rows))):
            row,actual=rows[index],observed['cases'][index];ordinal=0
            for physical_ordinal,event in enumerate(actual['reads']):
                if not 0x16c000<=event['instruction']<=0x16c28b:continue
                prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_read_{ordinal}',
                    phase='before_read',native_fault_ordinal=physical_ordinal,fault=dict(phase='before_read',ordinal=physical_ordinal))
                prefixes.append(prefix);ordinal+=1
            for ordinal,event in enumerate(actual['writes']):
                if not 0x16c000<=event['instruction']<=0x16c28b:continue
                for phase in ('before_write','after_write'):
                    prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'_{phase}_{ordinal}',
                        phase=phase,native_fault_ordinal=ordinal,fault=dict(phase=phase,ordinal=ordinal))
                    prefixes.append(prefix)
        prefix_scratch=scratch/'prefixes';prefix_scratch.mkdir()
        prefix_observed,prefix_authority=physical_original_dense(image,prefixes,prefix_scratch,
            entries=composition.OWNERS,regions=regions,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        boundary_count=len(rows);rows+=prefixes;observed['cases']+=prefix_observed['cases'];executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='query_return')
            assert actual['live_x87_unchanged']
            executed.update(actual['executed_sites'])
            row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=actual['stopped_at'] if row['phase']=='query_return'else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads'] if event['instruction'] not in (0x30aaa0,0x30aaa7)]
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir();mutants=[]
        if args.semantic_mutants:
            header=(ROOT/'src/physics/runtime/native_tu15_camera_instance_query_frame.hpp').read_text()
            for name,needle,replacement,guard in (
                ('origin_pointer','g[4]+0x480u);break;','g[4]+0x484u);break;',False),
                ('displacement_store','pc=0x16c1f9u;offset=0x58u','pc=0x16c1f9u;offset=0x54u',False),
                ('material_table','address=g[0]+0x10u;reg=0','address=g[0]+0x14u;reg=0',False),
                ('live_callback','s.services.instruction.target_type()!=*c->callback_type||','',True),
                ('reference_control','reference()||s.image','s.image',True),
                ('oversized_stack','||s.fp.count>6u||','||',True)):
                assert header.count(needle)==1,('Missing unique query mutation',name)
                directory=scratch/'mutants'/name;target=directory/'physics/runtime/native_tu15_camera_instance_query_frame.hpp'
                target.parent.mkdir(parents=True);target.write_text(header.replace(needle,replacement,1))
                mutants.append(dict(name=name,directory=directory,guard=guard))
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(camera_instance_query LANGUAGES CXX)
include(CTest)
add_executable(polygon_support "{ROOT/'tests/native_tu15_camera_instance_query_complete_tests.cpp'}")
target_include_directories(polygon_support PRIVATE "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(polygon_support PRIVATE cxx_std_20)
target_compile_options(polygon_support PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME polygon_support COMMAND polygon_support "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/frame.json")
add_executable(camera_query_admission "{ROOT/'tests/native_tu15_camera_instance_query_admission_tests.cpp'}")
target_include_directories(camera_query_admission PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(camera_query_admission PRIVATE cxx_std_20)
target_compile_options(camera_query_admission PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME camera_query_reference COMMAND ${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_CAMERA_INSTANCE_QUERY=1 HALO2_CAMERA_INSTANCE_QUERY_REFERENCE_TEST=1 $<TARGET_FILE:polygon_support> "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/reference-frame.json")
add_test(NAME camera_query_admission COMMAND camera_query_admission)
add_test(NAME camera_query_reference_guard COMMAND ${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_CAMERA_INSTANCE_QUERY=1 $<TARGET_FILE:camera_query_admission> --reference)
add_executable(camera_query_backend "{ROOT/'tests/native_tu15_camera_instance_query_backend_tests.cpp'}")
target_include_directories(camera_query_backend PRIVATE "{ROOT/'src'}")
target_compile_features(camera_query_backend PRIVATE cxx_std_20)
add_test(NAME camera_query_backend COMMAND camera_query_backend)
'''
        targets=['polygon_support','camera_query_admission','camera_query_backend']
        for mutant in mutants:
            name='camera_query_mutant_'+mutant['name'];targets.append(name)
            driver='native_tu15_camera_instance_query_admission_tests.cpp'if mutant['guard']else'native_tu15_camera_instance_query_complete_tests.cpp'
            cmake+=f'add_executable({name} "{ROOT/"tests"/driver}")\ntarget_include_directories({name} PRIVATE "{mutant["directory"]}" "{scratch}" "{ROOT/"include"}" "{ROOT/"src"}")\ntarget_compile_features({name} PRIVATE cxx_std_20)\ntarget_compile_options({name} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\n'
            if mutant['name']=='reference_control':command=f'${{CMAKE_COMMAND}} -E env HALO2_REFERENCE_CAMERA_INSTANCE_QUERY=1 $<TARGET_FILE:{name}> --reference'
            elif mutant['guard']:command=f'$<TARGET_FILE:{name}>'
            else:command=f'$<TARGET_FILE:{name}> "{fixture}"'
            cmake+=f'add_test(NAME {name} COMMAND {command})\nset_tests_properties({name} PROPERTIES WILL_FAIL TRUE)\n'
        if args.sanitizers:
            for target in targets:
                cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in args.compiler or ('g++','clang++'):
            compiler_tag=Path(compiler).name
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler_tag),
                *[value for target in targets for value in ('--target',target)],'--filter','^(polygon_support|camera_query_.*)$','--retain-report','frame.json','--retain-report','reference-frame.json',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-4000:]
            results[-1].update(frame=json.loads((args.output/compiler_tag/'evidence/frame.json').read_text()),reference=json.loads((args.output/compiler_tag/'evidence/reference-frame.json').read_text()))
        report.update(passed=True,production_binding=args.require_binding,sanitizers=args.sanitizers,original_cases=len(rows),declared_entry_cases=boundary_count,
            retained_prefixes=len(prefixes),native_comparisons_per_compiler=results[0]['frame']['comparisons'],
            complete_original_pre_io_packets=sum(len(row['calls']) for row in rows),executed_sites=sorted(executed),
            actual_parent_captures=actual_parent_captures,declared_dense_continuations=boundary_count-actual_parent_captures,
            authority=authority,prefix_authority=prefix_authority,compiler_results=results,genuine_child_sources=child_receipts,source_routes=source_routes,compiled_mutants=[row['name']for row in mutants],retained_child_exception_scope="Parent faults only; retained inverse child fault/hostile-provider domains remain separately owned")
    except BaseException as error:
        report['error']=str(error) or repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
