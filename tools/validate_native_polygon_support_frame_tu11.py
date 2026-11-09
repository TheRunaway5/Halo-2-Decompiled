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
from oracle_smg_firing_1_1 import PIN
from native_mopp_ray_dense_original import physical_original_dense
from validate_native_mopp_ray_caller_frame_tu15 import FILES,REGIONS as CALLER_REGIONS
REGIONS=((0x447000,0x1000),)+CALLER_REGIONS

KEY_FILES=FILES+(
    'include/halo2/native_tu15_polygon_support_kernel.hpp',
    'src/physics/runtime/native_tu15_polygon_support_frame.hpp',
    'src/physics/runtime/native_tu11_polygon_support_frame.hpp',
    'tests/native_tu11_polygon_support_frame_tests.cpp',
    'tests/native_tu11_polygon_support_binding_tests.cpp',
    'tools/native_polygon_support_replacements.py',
    'tools/specialize_native_operands.py',
    'src/physics/runtime/physics_contact_runtime_generated.cpp',
    'src/physics/runtime/physics_world_runtime_generated.cpp',
    'tools/validate_native_polygon_support_frame_tu11.py',
)


def hashes():
    return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in KEY_FILES}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--mutations',action='store_true')
    parser.add_argument('--source-routes',action='store_true')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if args.require_binding:args.source_routes=True
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),
                retained_large_artifacts=[],scope='Private native MapPack1.1 1F1D20 support-point selection, complete raw ABI, live80, aliases and actual alignment faults; no production binding')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN
        image=Xbe(args.xbe)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x1f1d20,)
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
        rng=random.Random(0x1f1d20);rows=[]
        edge=(0,0x80000000,1,0x007fffff,0x00800000,0x3f800001,0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0x7f812345,0xffc23456)
        names=('plain','output_vertex','output_saved','direction_vertex','shape_scratch','shape_unaligned','direction_unaligned','output_unaligned')
        for kind in names:
            for n in range(32):
                ordinal=len(rows);sp=0x62006000+ordinal%16;local=((sp-4)&0xfffffff0)-16
                banks={base:bytearray(rng.randbytes(size)) for base,size in REGIONS};banks[0x447000]=bytearray(image.read(0x447000,0x1000))
                shape=local-0x20 if kind=='shape_scratch' else 0x61000404 if kind=='shape_unaligned' else 0x61000400
                direction=shape+0x20 if kind=='direction_vertex' else 0x61000204 if kind=='direction_unaligned' else 0x61000200
                output=shape+0x20 if kind=='output_vertex' else local if kind=='output_saved' else 0x61000804 if kind=='output_unaligned' else 0x61000800
                count=(0,1,2,3,4,8,0xffffffff,0x80000000)[n%8]
                def put(address,value):
                    first=next(first for first,size in REGIONS if first<=address and address+4<=first+size)
                    banks[first][address-first:address-first+4]=int(value).to_bytes(4,'little')
                for index in range(32):
                    value=edge[(index+n)%len(edge)] if n&8 else struct.unpack('<I',struct.pack('<f',float(((index*7+n)%19)-9)))[0]
                    put(shape+0x20+index*4,value)
                for lane in range(4):put(direction+lane*4,edge[(n+lane)%len(edge)] if n&8 else struct.unpack('<I',struct.pack('<f',float(lane+1)))[0])
                if n>=16:
                    count=4
                    # Strict ties, reversed maxima and multiple NaN payloads
                    # make selection and reduction order observable.
                    for vertex in range(4):
                        values=(0x3f800000,)*4 if n%4==0 else tuple(struct.unpack('<I',struct.pack('<f',float((vertex if n%4==1 else 3-vertex)+lane)))[0] for lane in range(4))
                        if n%4==3:values=(0x7fc12345,0x7fc23456,0x3f800000,0)
                        for lane in range(4):put(shape+0x20+vertex*16+lane*4,values[lane])
                put(shape+0x14,count)
                for index,value in enumerate((0x63000000,direction,output)):put(sp+index*4,value)
                gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=sp;gp[1]=shape
                depth=(0,1,4,8)[ordinal%4]
                rows.append(dict(id=ordinal,name=f'{kind}_{n}',entry=0x1f1d20,phase='support_boundary',kind=kind,
                    gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                    xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],live_x87=list(live[:depth]),
                    before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(ordinal&0x473f),0x1f80|((n%4)<<13)|(0,0x40,0x8000,0x8040)[n%4]],
                    banks=[dict(address=base,before=bytes(data).hex()) for base,data in banks.items()],
                    matrix=direction,source=output,max_steps=5000,stop_before_entries=[0x63000000],alignment_fault=True))
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=(0x1f1d20,),regions=REGIONS,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        for index in [32*k+n for k in range(len(names)) for n in (0,1,3,11)]:
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
            entries=(0x1f1d20,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        boundary_count=len(rows);rows+=prefixes;observed['cases']+=prefix_observed['cases'];executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual.get('boundary') and actual['boundary'].get('phase')=='alignment_fault':row['phase']='alignment_fault'
            assert actual['completed']==(row['phase']=='support_boundary')
            assert actual['live_x87_unchanged']
            if row['phase']=='support_boundary':assert actual['stopped_at']==0x63000000
            executed.update(actual['executed_sites'])
            row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=0x63000000 if row['phase']=='support_boundary' else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads'] if event['instruction'] not in (0x30aaa0,0x30aaa7)]
            if row['phase']=='alignment_fault':
                site=next(site for site in metadata['sites'] if site['instruction']==row['expected_pc']);operand=site['operands'][0];g=actual['boundary_gp']
                row['alignment_fault_address']=(operand['displacement']+(g[operand['base']] if operand['base']>=0 else 0)+(g[operand['index']]*operand['scale'] if operand['index']>=0 else 0))&0xffffffff
                row['execution_eflags_before_fault']=actual['calls'][-1]['eflags']
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        source_include=scratch/'source_routes.inc';source_authority={}
        if args.source_routes:
            import re
            import native_polygon_support_replacements as binding
            from specialize_native_operands import specialize
            payload=[]
            for kind,name in (('contact','src/physics/runtime/physics_contact_runtime_generated.cpp'),('world','src/physics/runtime/physics_world_runtime_generated.cpp')):
                source=(ROOT/name).read_text();bound=binding.replace_polygon_support(source)
                assert binding.replace_polygon_support(bound)==bound
                assert binding.remove_polygon_support(bound)==binding.remove_polygon_support(source)
                prepared,receipt=specialize(bound)
                for suffix,value in (('raw',bound),('prepared',prepared)):
                    match=binding.BODY.search(value);assert match
                    body=match[0]
                    if args.require_binding:assert binding.replace_polygon_support(source)==source,'Owning support prefix is not installed'
                    else:
                        opening=body.index('{')+1
                        body=body[:opening]+'\n    if(halo2::native_polygon_support_legacy_detail::bridge(s))return;'+body[opening:]
                        body=body.replace('native_polygon_support_detail::ordered_','native_polygon_support_legacy_detail::ordered_')
                    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',value,re.S);assert resolver
                    body_authority=dict(body_sha256=hashlib.sha256(body.encode()).hexdigest(),source_sha256=hashlib.sha256(value.encode()).hexdigest())
                    if suffix=='prepared':body_authority['preparation']=receipt
                    source_authority[kind+'_'+suffix]=body_authority
                    # Count the copied retained body after its exact native
                    # prefix. Cursor-only callbacks alone cannot prove native
                    # ownership of a prepared reference route.
                    body=re.sub(r'(?=if\(s.services.instruction\)\{s.services.instruction\(|s\.instruction_literal\()',r'++literal_statements;if(stop_before_literal)throw RetainedBoundary{};',body)
                    aliases='using State=WorldState;' if kind=='contact' else 'using ObservedWorldState=SelectedWorldState;'
                    observation=''
                    if kind=='world':
                        observation=re.search(r'std::uint32_t world_observation_pc\(.*?\n\}',value,re.S)[0]
                    payload.append('namespace polygon_'+kind+'_'+suffix+' {\n'+aliases+'\nunsigned literal_statements{};\n'+resolver[0]+'\n'+observation+'\n'+body+'\n}\n')
            source_include.write_text('\n'.join(payload))
        project=scratch/'project';project.mkdir()
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(polygon_support LANGUAGES CXX)
include(CTest)
add_executable(polygon_support "{ROOT/'tests/native_tu11_polygon_support_frame_tests.cpp'}")
target_include_directories(polygon_support PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(polygon_support PRIVATE cxx_std_20)
target_compile_options(polygon_support PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME polygon_support COMMAND polygon_support "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/frame.json")
'''
        if args.sanitizers:cmake+='target_compile_options(polygon_support PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(polygon_support PRIVATE -fsanitize=address,undefined)\n'
        if args.source_routes:
            cmake+=f'''add_executable(polygon_support_bindings "{ROOT/'tests/native_tu11_polygon_support_binding_tests.cpp'}")
target_include_directories(polygon_support_bindings PRIVATE "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features(polygon_support_bindings PRIVATE cxx_std_20)
target_compile_definitions(polygon_support_bindings PRIVATE POLYGON_SOURCE_ROUTES="{source_include}")
target_compile_options(polygon_support_bindings PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME polygon_support_bindings COMMAND polygon_support_bindings "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/source-routes.json")
add_test(NAME polygon_support_bindings_reference COMMAND polygon_support_bindings "{fixture}" "${{CMAKE_CURRENT_BINARY_DIR}}/source-routes-reference.json")
set_tests_properties(polygon_support_bindings_reference PROPERTIES ENVIRONMENT "HALO2_REFERENCE_POLYGON_SUPPORT=1")
'''
            if args.sanitizers:cmake+='target_compile_options(polygon_support_bindings PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(polygon_support_bindings PRIVATE -fsanitize=address,undefined)\n'
        mutants={
            'wrong_bound':('s_.memory.read(0x447020u,bytes)','s_.memory.read(0x447024u,bytes)'),
            'wrong_namespace':('native_polygon_support_detail::Frame<State,Before,false>','native_polygon_support_detail::Frame<State,Before,true>'),
        }
        mutant_names=[]
        if args.mutations:
            original=(ROOT/'src/physics/runtime/native_tu11_polygon_support_frame.hpp').read_text()
            for name,(old,new) in mutants.items():
                assert original.count(old)>=1
                root=scratch/'mutants'/name;header=root/'physics/runtime/native_tu11_polygon_support_frame.hpp'
                header.parent.mkdir(parents=True);header.write_text(original.replace(old,new))
                target='polygon_legacy_mutant_'+name;mutant_names.append(target)
                cmake+=f'''add_executable({target} "{ROOT/'tests/native_tu11_polygon_support_frame_tests.cpp'}")
target_include_directories({target} BEFORE PRIVATE "{root}" "{ROOT/'include'}" "{ROOT/'src'}")
target_compile_features({target} PRIVATE cxx_std_20)
target_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME {target} COMMAND {target} "{fixture}")
set_tests_properties({target} PROPERTIES WILL_FAIL TRUE)
'''
                if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),
                '--target','polygon_support',*[arg for name in mutant_names for arg in ('--target',name)],
                *(['--target','polygon_support_bindings','--retain-report','source-routes.json','--retain-report','source-routes-reference.json'] if args.source_routes else []),
                '--filter','^polygon_(support(_bindings(_reference)?)?|legacy_mutant_.*)$','--retain-report','frame.json',
                '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-4000:]
            results[-1].update(frame=json.loads((args.output/compiler/'evidence/frame.json').read_text()))
            if args.source_routes:results[-1].update(source_routes=json.loads((args.output/compiler/'evidence/source-routes.json').read_text()),source_reference=json.loads((args.output/compiler/'evidence/source-routes-reference.json').read_text()))
        report.update(passed=True,production_binding=args.require_binding,rejected_compiled_mutants=list(mutants) if args.mutations else [],actual_alignment_faults=sum(row["phase"]=="alignment_fault" for row in rows),sanitizers=args.sanitizers,original_cases=len(rows),declared_entry_cases=boundary_count,
            retained_prefixes=len(prefixes),native_comparisons_per_compiler=sum(4 if row["phase"] in ("support_boundary","alignment_fault") else 2 for row in rows),
            complete_original_pre_io_packets=sum(len(row['calls']) for row in rows),executed_sites=sorted(executed),
            authority=authority,prefix_authority=prefix_authority,compiler_results=results,source_route_authority=source_authority)
    except BaseException as error:
        report['error']=str(error) or repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
