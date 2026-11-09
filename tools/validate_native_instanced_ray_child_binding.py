#!/usr/bin/env python3
"""Compare native instanced child DATA and complete raw ABI with unchanged original24DEC0."""
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
import native_instanced_ray_child_replacements as binding
import re

KEY_FILES=FILES+(
    'include/halo2/native_tu15_instanced_ray_child_kernel.hpp',
    'src/physics/runtime/native_tu15_instanced_ray_child_frame.hpp',
    'tests/native_tu15_instanced_ray_child_binding_tests.cpp',
    'tools/validate_native_instanced_ray_child_binding.py',
    'src/physics/runtime/native_tu15_instanced_ray_child.hpp',
    'tools/native_instanced_ray_child_replacements.py',
    'tools/native_selected_operand_binding.py',
    'tools/native_halfword_bit_query_replacements.py',
    'tools/specialize_native_operands.py',
    'tools/generate_physics_live_support_tu15.py',
    'src/physics/physics_live_support_tu15_generated.cpp',
    'src/campaign/ai/source_ai_frame_tu15_generated.cpp',
)


def hashes():
    return {name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in KEY_FILES}


def instrument_retained_body(body):
    counter='++literal_body_statements;'
    pattern=r'if\(s\.services\.instruction\)\{s\.services\.instruction\(|(?<!\w)s\.instruction(?:_literal)?\('
    counted,count=re.subn(pattern,lambda match:counter+match[0],body)
    if count!=13 or counted.count(counter)!=13:raise ValueError('Missing complete retained child counter sites')
    return counted


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--require-binding',action='store_true')
    parser.add_argument('--mutations',action='store_true')
    args=parser.parse_args()
    if not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,production_binding=False,source_before=hashes(),owned_scratch=str(scratch),
                retained_large_artifacts=[],scope='Native24DEC0 admission and both complete source prefixes against unchanged original CPU')
    try:
        assert hashlib.sha256(args.xbe.read_bytes()).hexdigest()==PIN
        image=Xbe(args.xbe)
        source_include=[];prepared_owners=[]
        from specialize_native_operands import specialize
        for label,name in(('primary','src/physics/physics_live_support_tu15_generated.cpp'),('ai','src/campaign/ai/source_ai_frame_tu15_generated.cpp')):
            source=(ROOT/name).read_text();bound=binding.replace_instanced_ray_child(source)
            if args.require_binding and bound!=source:raise ValueError('Actual native child prefix not installed '+name)
            prepared,_=specialize(bound)
            for suffix,text in(('',bound),('_prepared',prepared)):
                if suffix:
                    match=binding.BODY.search(text);body=binding.canonical_body(match[0])
                    if binding.digest(body)!=binding.PREPARED_BODY_PIN or binding.PREFIX not in match[0]:raise ValueError('Exact derived child preparation drift')
                else:
                    match,body=binding.authenticate(text)
                    if binding.replace_instanced_ray_child(text)!=text:raise ValueError('Native child installation is not idempotent')
                resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',text,re.S)[0]
                if binding.digest(resolver)not in binding.RESOLVER_PINS:raise ValueError('Derived child resolver drift')
                # Test-owned counters sit after the exact native prefix and
                # count only actual retained literal statements. Prepared
                # callbacks may be cursor-only, which is not native execution.
                counted=instrument_retained_body(match[0])
                source_include.append('namespace instanced_child_'+label+suffix+'{using namespace halo2;using namespace halo2::physics_world_runtime_detail;using halo2::physics_step_detail::r;using halo2::physics_step_detail::w;\nunsigned literal_body_statements{};\n'+resolver+'\n'+counted+'\n}\n')
                prepared_owners.append(dict(source=name,prepared=bool(suffix),body_sha256=binding.digest(body),bound_sha256=binding.digest(match[0]),resolver_sha256=binding.digest(resolver)))
        (scratch/'instanced_ray_child_sources.inc').write_text(''.join(source_include))
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        common.ENTRIES=(0x24dec0,)
        original_metadata=common.metadata
        code=common._Code(image,'.text')
        def child_metadata(image):
            result=original_metadata(image)
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic=='ret':
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=child_metadata;metadata=child_metadata(image)
        io=[site['instruction'] for site in metadata['sites']
            if site['instruction'] not in (0x30aaa0,0x30aaa7) and
            (site['operands'] or code.at(site['instruction']).mnemonic in ('push','pop','ret','call'))]
        live=('0000000000000080ff3f','00000000000000000080',
              '35c26821a2da0fc90040','45230100000000c0ff7f',
              '00000000000000800040','00000000000000c0ff3f',
              '00000000000000000000','0000000000000080ffbf')
        rng=random.Random(0x24dec0);rows=[]
        for alias in range(8):
            for n in range(16):
                ordinal=len(rows);banks=[bytearray(rng.randbytes(size)) for _,size in REGIONS]
                sp=0x62006000+n;collection=0x61000700
                child=(0x61000800,collection,collection+12,0,sp+4,sp+8,sp+16,sp+32)[alias]
                key=(0,1,0xffffffff,rng.getrandbits(32))[n%4]
                banks[0][0x70c:0x710]=(0x61000d00).to_bytes(4,'little')
                for k,value in enumerate((0x63000000,key,child)):
                    offset=sp-REGIONS[1][0]+k*4;banks[1][offset:offset+4]=value.to_bytes(4,'little')
                gp=[rng.getrandbits(32) for _ in range(8)];gp[4]=sp;gp[1]=collection
                depth=(0,1,4,8)[(ordinal//8)%4]
                rows.append(dict(id=ordinal,name=f'child_{alias}_{n}',entry=0x24dec0,phase='child_boundary',
                    gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
                    xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)],live_x87=list(live[:depth]),
                    before_fp=[(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10),((ordinal%8)<<11)|(ordinal&0x473f),
                               0x1f80|((ordinal%4)<<13)|(0,0x40,0x8000,0x8040)[ordinal%4]],
                    banks=[dict(address=base,before=bytes(data).hex()) for (base,_),data in zip(REGIONS,banks)],
                    matrix=key,source=child,max_steps=1000,stop_before_entries=[0x63000000]))
        traces=[dict(entry=pc,gp=True,xmm=True) for pc in io]
        observed,authority=physical_original_dense(image,rows,scratch,entries=(0x24dec0,),regions=REGIONS,
            trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        prefixes=[]
        for index in [16*alias+n for alias in range(8) for n in (0,1)]:
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
            entries=(0x24dec0,),regions=REGIONS,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        boundary_count=len(rows);rows+=prefixes;observed['cases']+=prefix_observed['cases'];executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            assert actual['completed']==(row['phase']=='child_boundary')
            assert actual['live_x87_unchanged']
            if row['phase']=='child_boundary':assert actual['stopped_at']==0x63000000
            executed.update(actual['executed_sites'])
            row.update({key:actual[key] for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','boundary_live_x87','writes','calls')})
            row['expected_pc']=0x63000000 if row['phase']=='child_boundary' else actual['boundary']['instruction']
            row['reads']=[event for event in actual['reads'] if event['instruction'] not in (0x30aaa0,0x30aaa7)]
            for packet in row['calls']:packet.pop('arguments')
            after={bank['address']:bank['after'] for bank in actual['banks']}
            for bank in row['banks']:bank['after']=after[bank['address']]
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows)))
        project=scratch/'project';project.mkdir()
        cmake=f'''cmake_minimum_required(VERSION 3.20)
project(instanced_child LANGUAGES CXX)
include(CTest)
add_executable(instanced_child "{ROOT/'tests/native_tu15_instanced_ray_child_binding_tests.cpp'}")
target_include_directories(instanced_child PRIVATE "{ROOT/'include'}" "{ROOT/'src'}" "{scratch}")
target_compile_features(instanced_child PRIVATE cxx_std_20)
target_compile_options(instanced_child PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
add_test(NAME instanced_child_raw COMMAND instanced_child "{fixture}" raw)
add_test(NAME instanced_child_guards COMMAND instanced_child --guards)
add_test(NAME instanced_child_bridge COMMAND instanced_child "{fixture}" bridge)
add_test(NAME instanced_child_primary COMMAND instanced_child "{fixture}" primary)
add_test(NAME instanced_child_ai COMMAND instanced_child "{fixture}" ai)
add_test(NAME instanced_child_primary_prepared COMMAND instanced_child "{fixture}" primary_prepared)
add_test(NAME instanced_child_ai_prepared COMMAND instanced_child "{fixture}" ai_prepared)
add_test(NAME instanced_child_primary_reference COMMAND instanced_child "{fixture}" primary)
add_test(NAME instanced_child_ai_reference COMMAND instanced_child "{fixture}" ai)
add_test(NAME instanced_child_primary_prepared_reference COMMAND instanced_child "{fixture}" primary_prepared)
add_test(NAME instanced_child_ai_prepared_reference COMMAND instanced_child "{fixture}" ai_prepared)
set_tests_properties(instanced_child_primary_reference instanced_child_ai_reference instanced_child_primary_prepared_reference instanced_child_ai_prepared_reference PROPERTIES ENVIRONMENT "HALO2_REFERENCE_INSTANCED_RAY_CHILD=1")
'''
        if args.sanitizers:cmake+='target_compile_options(instanced_child PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options(instanced_child PRIVATE -fsanitize=address,undefined)\n'
        (project/'CMakeLists.txt').write_text(cmake);results=[]
        for compiler in ('g++','clang++'):
            command=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
                '--scratch-root',str(scratch/'builds'),'--output',str(args.output/compiler),
                '--target','instanced_child','--filter','^instanced_child_','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
            result=subprocess.run(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
            results.append(dict(compiler=compiler,exit_code=result.returncode));assert result.returncode==0,result.stdout[-4000:]
        report.update(passed=True,production_binding=args.require_binding,sanitizers=args.sanitizers,original_cases=len(rows),complete_provider_returns=boundary_count,
            retained_prefixes=len(prefixes),native_comparisons_per_compiler=boundary_count*4+len(prefixes)*2,
            source_and_bridge_routes=['raw','bridge','primary','ai','primary_prepared','ai_prepared','primary_reference','ai_reference','primary_prepared_reference','ai_prepared_reference'],
            total_native_and_reference_comparisons_per_compiler=boundary_count*32+len(prefixes)*16,admission_refusals=22,prepared_owners=prepared_owners,
            disposable_literal_body_counters_after_exact_prefix=True,prepared_reference_cursor_only_is_not_native_execution=True,
            complete_original_pre_io_packets=sum(len(row['calls']) for row in rows),executed_sites=sorted(executed),
            authority=authority,prefix_authority=prefix_authority,compiler_results=results)
        if args.mutations:
            core=(ROOT/'include/halo2/native_tu15_instanced_ray_child_kernel.hpp').read_text();controls=[]
            for label,old,new in(('vtable-owner','0x46625cu','0x466260u'),
                ('key-acquisition-order','d.scenario=frame.scenario();frame.store(Field::scenario,d.scenario,d);\n    d.key=frame.key();','d.scenario=frame.scenario();d.key=frame.key();frame.store(Field::scenario,d.scenario,d);')):
                if core.count(old)!=1:raise ValueError('Missing child DATA mutation witness '+label)
                directory=scratch/('mutant-'+label);(directory/'halo2').mkdir(parents=True)
                (directory/'halo2/native_tu15_instanced_ray_child_kernel.hpp').write_text(core.replace(old,new,1));binary=directory/'probe'
                command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(directory),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_instanced_ray_child_binding_tests.cpp'),'-o',str(binary)]
                subprocess.run(command,check=True);result=subprocess.run([str(binary),str(fixture),'bridge'],capture_output=True,text=True,timeout=60)
                if result.returncode==0:raise ValueError('Incorrect shared child DATA policy escaped original CPU '+label)
                if 'differ'not in result.stderr:raise ValueError('Child DATA mutation failed without a semantic oracle mismatch '+result.stderr[:2000])
                controls.append(dict(mutation=label,compiled=True,rejected=True,reason=result.stderr[:1500]))
            report['compiled_semantic_mutation_rejections']=controls
    except BaseException as error:
        report['error']=str(error) or repr(error);raise
    finally:
        report['source_after']=hashes()
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
