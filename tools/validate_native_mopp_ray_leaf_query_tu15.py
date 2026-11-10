#!/usr/bin/env python3
"""Complete private MOPP collector leaves with genuinely returned scene queries."""
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
from validate_native_camera_instance_query_complete_tu15 import KEY_FILES as QUERY_FILES, hashes as query_hashes
from validate_native_mopp_ray_caller_frame_tu15 import FILES

KEY_FILES=tuple(dict.fromkeys(FILES+QUERY_FILES+(
    'include/halo2/native_tu15_mopp_ray_leaf_kernel.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_leaf_frame.hpp',
    'include/halo2/native_tu15_instanced_ray_callback_kernel.hpp',
    'src/physics/runtime/native_tu15_instanced_ray_callback_frame.hpp',
    'src/physics/runtime/native_tu15_instanced_ray_child_frame.hpp',
    'src/physics/runtime/native_tu15_instanced_ray_callback.hpp',
    'tools/native_instanced_ray_callback_replacements.py',
    'tools/native_instanced_ray_callback_inputs.py',
    'tools/native_instanced_ray_callback_return_mutations.py',
    'tools/native_mopp_ray_query_profiles.py',
    'src/physics/runtime/native_tu15_host_fp_io.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_controller.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_slab_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_diagnostic_frame.hpp',
    'src/physics/runtime/native_tu15_affine_inverse.hpp',
    'include/halo2/native_tu15_affine_inverse_kernel.hpp',
    'tests/native_tu15_mopp_ray_leaf_query_tests.cpp',
    'tests/native_tu15_instanced_ray_callback_admission_tests.cpp',
    'tests/native_tu15_instanced_ray_callback_backend_tests.cpp',
    'tools/validate_native_mopp_ray_leaf_query_tu15.py')))


def hashes():
    result={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in KEY_FILES}
    result.update(query_hashes());return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--smoke',action='store_true');parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--source-routes',action='store_true');parser.add_argument('--require-binding',action='store_true')
    parser.add_argument('--semantic-mutants',action='store_true')
    parser.add_argument('--compose-queries',action='store_true')
    parser.add_argument('--hostile-providers',action='store_true')
    parser.add_argument('--compiler',action='append');args=parser.parse_args()
    if args.hostile_providers and not args.compose_queries:parser.error('Hostile whole-query providers require --compose-queries')
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),
        retained_large_artifacts=[],supplied_child_outcomes=False,hostile_providers=args.hostile_providers,
        scope='Private complete MOPP collector leaf, genuine native provider/forwarder/collector/query/BSP/publication; retained inverse; parent unbound')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN;image=Xbe(args.xbe)
        import validate_native_camera_instanced_query_tu15 as composition
        selected=[c for c in composition.cases(image)if c['entry']==0x16c000 and not c['name'].startswith('fault_seed_')]
        if args.smoke:selected=[selected[n]for n in (0,18,21,34,41,61)]
        pages={p for c in selected for bank in c['banks']for p in range(bank['address']&~0xfff,(bank['address']+len(bytes.fromhex(bank['before']))+0xfff)&~0xfff,0x1000)}
        pages.update((0x41c000,0x466000,0x46a000,0x4f4000,0x61000000));pages.update(range(0x62000000,0x62008000,0x1000))
        regions=[]
        for page in sorted(pages):
            if regions and regions[-1][0]+regions[-1][1]==page:regions[-1]=(regions[-1][0],regions[-1][1]+0x1000)
            else:regions.append((page,0x1000))
        regions=tuple(regions);rng=random.Random(0x30b699);rows=[]
        live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040',
              '45230100000000c0ff7f','00000000000000800040','00000000000000c0ff3f')
        for c in selected:
            for variant in range(2 if args.smoke else 4):
                banks={base:bytearray(size)for base,size in regions}
                def put(address,data):
                    base=next(base for base,size in regions if base<=address and address+len(data)<=base+size)
                    banks[base][address-base:address-base+len(data)]=data
                def word(address,value,width=4):put(address,int(value&((1<<(width*8))-1)).to_bytes(width,'little'))
                for page in (0x41c000,0x466000,0x46a000):put(page,image.read(page,0x1000))
                for bank in c['banks']:put(bank['address'],bytes.fromhex(bank['before']))
                sp=0x62006000+len(rows)%16;variables=0x61000800;cache=0x61000900
                collection=0x61000b00;table=0x61000b40;collector=0x61000c00;collector_table=0x61000c80;parent=0x61000d00
                put(0x61000100,b'\x30');word(variables+0x18,c['entry_registers']['eax'])
                for address,value in ((cache+0x44,0),(cache+0x64,collection),(collection,table),
                    (table+0x34,0x24dec0),(collection+12,0x1006000),(cache+0x58,0),
                    (cache+0x5c,collector),(cache+0x60,parent),(parent+8,0x12345678),
                    (collector,collector_table),(collector_table,0x24df20),
                    (collector+4,0x3e800000 if variant==2 else 0x3f800000),
                    (collector+8,c['arguments'][0]|(0x10000000 if variant==3 else 0)),
                    (collector+12,c['arguments'][1]),(collector+16,c['arguments'][2]),
                    (collector+20,c['arguments'][3]),(collector+24,0),(collector+28,c['entry_registers']['ebx']),
                    (0x4f44ec,0x12345678),(0x4f44f0+c['entry_registers']['eax']*4,0x12345678 if variant==1 else 0x87654321)):
                    word(address,value)
                arguments=[0x63000000,variables,0x61000100,0x61000f00]
                for n,value in enumerate(arguments):word(sp+n*4,value)
                gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=cache;gp[4]=sp
                depth=0 if args.smoke else (0,1,4,6)[len(rows)%4]
                rows.append(dict(id=len(rows),name=c['name']+f'_collector{variant}',entry=0x30aa70,phase='normal',
                    gp=gp,eflags=0x202,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                    before_fp=c['before_fp'],live_x87=list(live[:depth]),
                    banks=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()],
                    matrix=arguments[1],source=arguments[2],max_steps=10000,stop_before_entries=[0x63000000]))
        if args.compose_queries:
            from native_mopp_ray_query_profiles import compose
            rows=compose(image,rows,args.smoke)
            report['scope']='Private complete authored split/recursive/returned-query MOPP controller, all native subtree algorithms; no production MOPP binding'
        owners=(0x30aa70,0x24dec0,0x24def0,0x24df20,*composition.OWNERS)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=owners;original=common.metadata;code=common._Code(image,'.text')
        def metadata(image):
            result=original(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in ('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if code.at(site['instruction']).mnemonic in ('fst','fstp'):
                    for operand in site['operands']:operand.update(read=False,write=True)
            return result
        common.metadata=metadata;meta=metadata(image)
        excluded=(0x30aaa0,0x30aaa7)if args.compose_queries else (0x30aaa0,0x30aaa7,0x1441b7)
        io=[site['instruction']for site in meta['sites']if site['instruction']not in excluded and
            (site['operands']or code.at(site['instruction']).mnemonic in ('push','pop','ret','call'))]
        traces=[dict(entry=pc,gp=True,xmm=True)for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=owners,regions=regions,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        normal_count=len(rows);prefixes=[];prefix_authority=None
        normal_sites=set().union(*(set(actual['executed_sites'])for actual in observed['cases']))
        if not args.smoke:
            callback_sites={site['instruction']for site in meta['sites']if 0x24def0<=site['instruction']<=0x24dfab}
            assert len(callback_sites)==62 and callback_sites<=normal_sites
            assert all(pc in normal_sites for pc in (0x24df6c,0x24df8d,0x24dfa0))
            # Exercise each real collector outcome and the duplicate fast return.
            # The query/BSP subtree has its own complete interruption proof; this
            # composition adds the newly returned parent/provider/callback seam.
            seeds={0,1}
            for pc in (0x24df5c,0x24df6c,0x24df8d,0x24dfa0):
                match=next((i for i,actual in enumerate(observed['cases'])if pc in actual['executed_sites']),None)
                if match is not None:seeds.add(match)
            def new_seam(pc):return 0x30b5f5<=pc<=0x30b6a8 or 0x24dec0<=pc<=0x24dfab
            by_site={site['instruction']:site for site in meta['sites']}
            for index in sorted(seeds):
                row,actual=rows[index],observed['cases'][index];native_ordinal=0
                for physical_ordinal,event in enumerate(actual['reads']):
                    pc=event['instruction']
                    if pc in (0x30aaa0,0x30aaa7):continue
                    if new_seam(pc):
                        prefix=copy.deepcopy(row);prefix.update(id=normal_count+len(prefixes),
                            name=row['name']+f'_read_{native_ordinal}',phase='before_read',
                            native_fault_ordinal=native_ordinal,fault=dict(phase='before_read',ordinal=physical_ordinal))
                        prefixes.append(prefix)
                    native_ordinal+=1
                for ordinal,event in enumerate(actual['writes']):
                    pc=event['instruction']
                    if not new_seam(pc):continue
                    for phase in ('before_write','after_write'):
                        # Before an indirect CALL instruction the target read
                        # has not happened. Do not supply that read's result.
                        site=by_site[pc]
                        native_phase='before_instruction'if phase=='before_write'and code.at(pc).mnemonic=='call'and any(operand['read']for operand in site['operands'])else phase
                        prefix=copy.deepcopy(row);prefix.update(id=normal_count+len(prefixes),
                            name=row['name']+f'_{phase}_{ordinal}',phase=native_phase,
                            native_fault_ordinal=ordinal,fault=dict(phase=phase,ordinal=ordinal))
                        prefixes.append(prefix)
            prefix_scratch=scratch/'prefixes';prefix_scratch.mkdir()
            prefix_observed,prefix_authority=physical_original_dense(image,prefixes,prefix_scratch,
                entries=owners,regions=regions,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
            rows+=prefixes;observed['cases']+=prefix_observed['cases']
        executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='normal')and actual['live_x87_unchanged']
            if row['phase']=='normal':assert actual['stopped_at']==0x63000000
            executed.update(actual['executed_sites'])
            row.update({key:actual[key]for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=0x63000000 if row['phase']=='normal'else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads']if event['instruction']not in (0x30aaa0,0x30aaa7)]
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after']for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        from native_camera_instance_query_children import prepare
        inverse_sources=prepare(ROOT,scratch)
        source_routes=None
        if args.source_routes:
            from native_instanced_ray_callback_inputs import prepare as prepare_callbacks
            source_routes=prepare_callbacks(ROOT,scratch,args.require_binding)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows,hostile_providers=args.hostile_providers)))
        project=scratch/'project';project.mkdir();cmake=f'''cmake_minimum_required(VERSION 3.20)
project(mopp_leaf_query LANGUAGES CXX)
include(CTest)
add_executable(mopp_leaf_query "{ROOT/'tests/native_tu15_mopp_ray_leaf_query_tests.cpp'}")
target_include_directories(mopp_leaf_query PRIVATE "{scratch}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(mopp_leaf_query PRIVATE cxx_std_20)
target_compile_options(mopp_leaf_query PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME mopp_leaf_query COMMAND mopp_leaf_query "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/leaf-query.json")
'''
        retained=['leaf-query.json']
        if args.source_routes:
            cmake+='target_compile_definitions(mopp_leaf_query PRIVATE HALO2_CALLBACK_SOURCE_ROUTES=1)\n'
            cmake+=f'''add_test(NAME mopp_leaf_query_reference COMMAND "${{CMAKE_COMMAND}}" -E env HALO2_REFERENCE_INSTANCED_RAY_CALLBACKS=1 HALO2_INSTANCED_RAY_CALLBACK_REFERENCE_TEST=1 $<TARGET_FILE:mopp_leaf_query> "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/leaf-query-reference.json")
add_test(NAME mopp_leaf_query_unrelated_control COMMAND "${{CMAKE_COMMAND}}" -E env HALO2_REFERENCE_INSTANCED_RAY_CHILD=1 $<TARGET_FILE:mopp_leaf_query> "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/leaf-query-unrelated-control.json")
add_executable(callback_admission "{ROOT/'tests/native_tu15_instanced_ray_callback_admission_tests.cpp'}")
target_include_directories(callback_admission PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(callback_admission PRIVATE cxx_std_20)
add_test(NAME callback_admission COMMAND callback_admission)
add_test(NAME callback_admission_reference COMMAND "${{CMAKE_COMMAND}}" -E env HALO2_REFERENCE_INSTANCED_RAY_CALLBACKS=1 $<TARGET_FILE:callback_admission> --reference)
add_executable(callback_backend "{ROOT/'tests/native_tu15_instanced_ray_callback_backend_tests.cpp'}")
target_include_directories(callback_backend PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(callback_backend PRIVATE cxx_std_20)
add_test(NAME callback_backend COMMAND callback_backend)
'''
            retained+=['leaf-query-reference.json','leaf-query-unrelated-control.json']
        if args.sanitizers:
            for target in ('mopp_leaf_query','callback_admission','callback_backend')if args.source_routes else ('mopp_leaf_query',):
                cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in args.compiler or ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),'--scratch-root',str(scratch/'builds'),
                '--output',str(args.output/Path(compiler).name),'--target','mopp_leaf_query','--filter','^(mopp_leaf_query(_reference|_unrelated_control)?|callback_admission(_reference)?|callback_backend)$',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            if args.source_routes:command+=['--target','callback_admission','--target','callback_backend']
            for path in retained:command+=['--retain-report',path]
            result=subprocess.run(command,capture_output=True,text=True);assert result.returncode==0,(result.stdout+result.stderr)[-6000:]
            observations={path:json.loads((args.output/Path(compiler).name/'evidence'/path).read_text())for path in retained}
            if not args.smoke:
                for result in observations.values():
                    assert all(result[key]>0 for key in ('real_queries','real_query_hits','duplicate_instance_returns','any_hit_stores','replacement_stores','keep_stores'))
            results.append(dict(compiler=compiler,passed=True,observations=observations))
        mutants=None
        if args.semantic_mutants:
            assert args.source_routes and not args.smoke,'Returned mutants require the full actual-source proof'
            from native_instanced_ray_callback_return_mutations import qualify
            mutants=qualify(scratch,args.output,rows,io)
        report.update(passed=True,original_paths=len(rows),executed_sites=sorted(executed),authority=authority,
            normal_paths=normal_count,retained_prefixes=len(prefixes),prefix_authority=prefix_authority,
            normal_executed_sites=sorted(normal_sites),
            indirect_call_post_target_read_provider_write_failure_qualified=False,
            original_pre_io_packets=sum(len(row['calls'])for row in rows),inverse_sources=inverse_sources,
            source_routes=source_routes,semantic_mutants=mutants,bsp_fp_io_installed=True,compilers=results)
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
